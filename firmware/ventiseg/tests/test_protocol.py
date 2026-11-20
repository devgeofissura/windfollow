"""Testes TDD do nucleo do ventiseg (contrato v0.1, serial-only).

Escritos antes da implementacao: definem exatamente o que o parser e a
sessao devem aceitar, rejeitar e calcular. O firmware C++ deve reproduzir
esta mesma tabela (via vetores gerados em ``controller/autotest_vectors.h``).

    powershell -ExecutionPolicy Bypass -File firmware/ventiseg/tests/run.ps1
"""

from __future__ import annotations

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))

import protocol_ref as p  # noqa: E402


def parse(text: str, **kwargs) -> p.Result:
    return p.parse(text.encode("ascii"), **kwargs)


class Constants(unittest.TestCase):
    def test_contract_numbers(self) -> None:
        self.assertEqual(p.MAX_DATAGRAM_BYTES, 96)
        self.assertEqual(p.LINK_TIMEOUT_MS, 1200)
        self.assertEqual(p.DEAD_BAND, 55)
        self.assertEqual(p.MANUAL_STEP, 80)
        self.assertEqual(p.STEPS_PER_REV, 3200)
        self.assertEqual(p.MAX_TRAVEL, 533)
        self.assertEqual(p.TRACK_GAIN, 0.75)
        self.assertEqual(p.X_LIMIT, 1000)

    def test_state_and_fault_vocabulary(self) -> None:
        self.assertEqual(
            (p.STATE_SAFE, p.STATE_TRACK, p.STATE_MANUAL),
            ("SAFE", "TRACK", "MANUAL"),
        )
        self.assertEqual(
            (p.FAULT_NONE, p.FAULT_LNK, p.FAULT_RANGE),
            ("None", "Lnk", "Range"),
        )

    def test_no_fields_from_other_projects(self) -> None:
        result = parse("T,1,0,1", rx_ms=0)
        for absent in ("y", "axis", "light", "tilt", "relay", "limit"):
            self.assertFalse(hasattr(result, absent))


class LengthAndEncoding(unittest.TestCase):
    def test_empty_datagram_is_rejected(self) -> None:
        result = p.parse(b"", rx_ms=0)
        self.assertEqual(result.reason, "length")
        self.assertFalse(result.accepted)

    def test_datagram_at_limit_is_parsed_not_rejected_by_length(self) -> None:
        datagram = b"T,1,1" + b"0" * 89 + b",1"
        self.assertEqual(len(datagram), p.MAX_DATAGRAM_BYTES)
        # x com muitos digitos e um inteiro valido, mas fora da faixa.
        self.assertEqual(p.parse(datagram, rx_ms=0).reason, "range")

    def test_datagram_over_limit_is_rejected(self) -> None:
        datagram = b"TRACK,1" + b" " * 90
        self.assertGreater(len(datagram), p.MAX_DATAGRAM_BYTES)
        self.assertEqual(p.parse(datagram, rx_ms=0).reason, "length")

    def test_non_ascii_byte_is_rejected(self) -> None:
        self.assertEqual(p.parse(b"TRACK,\xff", rx_ms=0).reason, "encoding")

    def test_nul_byte_is_rejected(self) -> None:
        self.assertEqual(
            p.parse(b"TRACK,1\x00x", rx_ms=0).reason, "encoding"
        )

    def test_newline_is_not_part_of_command(self) -> None:
        self.assertEqual(
            p.parse(b"TRACK,1\n", rx_ms=0).reason, "format"
        )

    def test_unknown_command_is_rejected(self) -> None:
        self.assertEqual(parse("BOGUS,1", rx_ms=0).reason, "format")

    def test_lowercase_command_is_rejected(self) -> None:
        self.assertEqual(parse("track,1", rx_ms=0).reason, "format")

    def test_command_prefix_without_comma_is_rejected(self) -> None:
        self.assertEqual(parse("TRACKX,1", rx_ms=0).reason, "format")


class TrackCommand(unittest.TestCase):
    def test_track_one_is_accepted(self) -> None:
        result = parse("TRACK,1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "track")
        self.assertTrue(result.enabled)

    def test_track_zero_is_accepted(self) -> None:
        result = parse("TRACK,0", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertFalse(result.enabled)

    def test_track_two_is_range_error(self) -> None:
        self.assertEqual(parse("TRACK,2", rx_ms=0).reason, "range")

    def test_track_negative_one_is_range_error(self) -> None:
        self.assertEqual(parse("TRACK,-1", rx_ms=0).reason, "range")

    def test_track_without_value_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK", rx_ms=0).reason, "format")

    def test_track_empty_value_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,", rx_ms=0).reason, "format")

    def test_track_extra_field_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,1,0", rx_ms=0).reason, "format")

    def test_track_leading_space_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK, 1", rx_ms=0).reason, "format")

    def test_track_trailing_space_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,1 ", rx_ms=0).reason, "format")

    def test_track_garbage_value_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,on", rx_ms=0).reason, "format")

    def test_track_negative_garbage_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,-abc", rx_ms=0).reason, "format")

    def test_track_float_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,1.0", rx_ms=0).reason, "format")

    def test_track_plus_sign_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,+1", rx_ms=0).reason, "format")


class ManualCommand(unittest.TestCase):
    def test_manual_forward_is_accepted(self) -> None:
        result = parse("MANUAL,1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "manual")
        self.assertEqual(result.direction, 1)

    def test_manual_backward_is_accepted(self) -> None:
        result = parse("MANUAL,-1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.direction, -1)

    def test_manual_zero_is_accepted_to_stop(self) -> None:
        result = parse("MANUAL,0", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.direction, 0)

    def test_manual_two_is_range_error(self) -> None:
        self.assertEqual(parse("MANUAL,2", rx_ms=0).reason, "range")

    def test_manual_minus_two_is_range_error(self) -> None:
        self.assertEqual(parse("MANUAL,-2", rx_ms=0).reason, "range")

    def test_manual_without_value_is_format_error(self) -> None:
        self.assertEqual(parse("MANUAL", rx_ms=0).reason, "format")

    def test_manual_empty_value_is_format_error(self) -> None:
        self.assertEqual(parse("MANUAL,", rx_ms=0).reason, "format")

    def test_manual_extra_field_is_format_error(self) -> None:
        self.assertEqual(parse("MANUAL,1,0", rx_ms=0).reason, "format")

    def test_manual_garbage_value_is_format_error(self) -> None:
        self.assertEqual(parse("MANUAL,front", rx_ms=0).reason, "format")

    def test_manual_float_is_format_error(self) -> None:
        self.assertEqual(parse("MANUAL,1.0", rx_ms=0).reason, "format")

    def test_manual_axis_field_is_not_part_of_contract(self) -> None:
        self.assertEqual(parse("MANUAL,pan,1", rx_ms=0).reason, "format")

    def test_manual_leading_zero_is_accepted(self) -> None:
        self.assertEqual(parse("MANUAL,01", rx_ms=0).direction, 1)


class SampleCommand(unittest.TestCase):
    def test_center_sample_is_accepted(self) -> None:
        result = parse("T,1,0,1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "sample")
        self.assertEqual((result.seq, result.x, result.valid), (1, 0, True))

    def test_extreme_samples_are_accepted(self) -> None:
        self.assertTrue(parse("T,1,1000,1", rx_ms=0).accepted)
        self.assertTrue(parse("T,1,-1000,0", rx_ms=0).accepted)

    def test_sample_with_valid_zero_is_accepted(self) -> None:
        result = parse("T,7,123,0", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertFalse(result.valid)

    def test_leading_zeros_are_accepted(self) -> None:
        result = parse("T,007,-001,01", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual((result.seq, result.x, result.valid), (7, -1, True))

    def test_negative_zero_x_is_accepted(self) -> None:
        self.assertEqual(parse("T,1,-0,1", rx_ms=0).x, 0)

    def test_large_seq_beyond_int32_is_range_error(self) -> None:
        self.assertEqual(
            parse("T,2147483648,0,1", rx_ms=0).reason, "range"
        )

    def test_negative_seq_is_range_error(self) -> None:
        self.assertEqual(parse("T,-1,0,1", rx_ms=0).reason, "range")

    def test_x_above_limit_is_range_error(self) -> None:
        self.assertEqual(parse("T,1,1001,1", rx_ms=0).reason, "range")

    def test_x_below_limit_is_range_error(self) -> None:
        self.assertEqual(parse("T,1,-1001,1", rx_ms=0).reason, "range")

    def test_valid_two_is_range_error(self) -> None:
        self.assertEqual(parse("T,1,0,2", rx_ms=0).reason, "range")

    def test_valid_minus_one_is_range_error(self) -> None:
        self.assertEqual(parse("T,1,0,-1", rx_ms=0).reason, "range")

    def test_bad_seq_is_format_error(self) -> None:
        self.assertEqual(parse("T,abc,0,1", rx_ms=0).reason, "format")

    def test_bad_x_is_format_error(self) -> None:
        self.assertEqual(parse("T,1,abc,1", rx_ms=0).reason, "format")

    def test_bad_valid_is_format_error(self) -> None:
        self.assertEqual(parse("T,1,0,abc", rx_ms=0).reason, "format")

    def test_float_x_is_format_error(self) -> None:
        self.assertEqual(parse("T,1,0.5,1", rx_ms=0).reason, "format")

    def test_missing_field_is_format_error(self) -> None:
        self.assertEqual(parse("T,1,0", rx_ms=0).reason, "format")

    def test_extra_field_is_format_error(self) -> None:
        self.assertEqual(parse("T,1,0,1,9", rx_ms=0).reason, "format")

    def test_four_coordinate_fields_are_not_part_of_contract(self) -> None:
        self.assertEqual(parse("T,1,0,0,1", rx_ms=0).reason, "format")

    def test_repeated_seq_is_sequence_error(self) -> None:
        self.assertEqual(
            parse("T,5,0,1", rx_ms=0, last_seq=5).reason, "sequence"
        )

    def test_regressing_seq_is_sequence_error(self) -> None:
        self.assertEqual(
            parse("T,4,0,1", rx_ms=0, last_seq=5).reason, "sequence"
        )

    def test_advancing_seq_is_accepted(self) -> None:
        result = parse("T,6,0,1", rx_ms=0, last_seq=5)
        self.assertTrue(result.accepted)
        self.assertEqual(result.seq, 6)

    def test_first_zero_seq_is_accepted(self) -> None:
        self.assertTrue(parse("T,0,0,1", rx_ms=0).accepted)

    def test_range_checked_before_sequence(self) -> None:
        self.assertEqual(
            parse("T,1999,1001,1", rx_ms=0, last_seq=2000).reason, "range"
        )


class MessageAge(unittest.TestCase):
    def test_age_at_limit_is_accepted(self) -> None:
        self.assertTrue(parse("T,1,0,1", rx_ms=1000, now_ms=2200).accepted)

    def test_age_past_limit_is_rejected(self) -> None:
        self.assertEqual(
            parse("T,1,0,1", rx_ms=1000, now_ms=2201).reason, "age"
        )

    def test_negative_age_is_rejected(self) -> None:
        self.assertEqual(
            parse("T,1,0,1", rx_ms=1000, now_ms=999).reason, "age"
        )

    def test_age_applies_to_track(self) -> None:
        self.assertEqual(
            parse("TRACK,1", rx_ms=1000, now_ms=2201).reason, "age"
        )

    def test_age_applies_to_manual(self) -> None:
        self.assertEqual(
            parse("MANUAL,1", rx_ms=1000, now_ms=2201).reason, "age"
        )

    def test_age_age_checked_after_sequence(self) -> None:
        self.assertEqual(
            parse("T,1,0,1", rx_ms=1000, now_ms=2201, last_seq=1).reason,
            "sequence",
        )

    def test_omitted_now_uses_rx_time(self) -> None:
        self.assertTrue(parse("T,1,0,1", rx_ms=5_000_000).accepted)


class Helpers(unittest.TestCase):
    def test_clamp(self) -> None:
        self.assertEqual(p.clamp(600, 533), 533)
        self.assertEqual(p.clamp(-600, 533), -533)
        self.assertEqual(p.clamp(-1, 533), -1)

    def test_track_steps_dead_band_is_inclusive(self) -> None:
        self.assertEqual(p.track_steps(0), 0)
        self.assertEqual(p.track_steps(55), 0)
        self.assertEqual(p.track_steps(-55), 0)
        self.assertNotEqual(p.track_steps(56), 0)

    def test_track_steps_gain_and_clamp(self) -> None:
        self.assertEqual(p.track_steps(56), 22)
        self.assertEqual(p.track_steps(1000), 399)
        self.assertEqual(p.track_steps(-1000), -399)
        self.assertEqual(p.track_steps(1000, gain=1.0), 533)
        self.assertEqual(p.track_steps(1000, gain=2.0), 533)

    def test_manual_target_steps_and_limits(self) -> None:
        self.assertEqual(p.manual_target(0, 1), 80)
        self.assertEqual(p.manual_target(0, -1), -80)
        self.assertEqual(p.manual_target(500, 1), 533)
        self.assertEqual(p.manual_target(-500, -1), -533)

    def test_manual_target_rejects_zero(self) -> None:
        with self.assertRaises(ValueError):
            p.manual_target(0, 0)

    def test_link_expired_boundaries(self) -> None:
        self.assertTrue(p.link_expired(last_sample_ms=None, now_ms=0))
        self.assertFalse(p.link_expired(last_sample_ms=1000, now_ms=2200))
        self.assertTrue(p.link_expired(last_sample_ms=1000, now_ms=2201))


class SessionBoot(unittest.TestCase):
    def test_boot_is_safe(self) -> None:
        session = p.Session()
        self.assertEqual(session.mode, "SAFE")
        self.assertEqual(session.fault, "None")
        self.assertEqual(session.seq, -1)
        self.assertEqual(session.axis, 0)
        self.assertFalse(session.moving)
        self.assertIsNone(session.last_sample_ms)
        self.assertFalse(session.link_lost)

    def test_boot_telemetry(self) -> None:
        self.assertEqual(p.format_telemetry(p.Session()), "ACK,-1,SAFE,None")

    def test_telemetry_has_exactly_four_fields(self) -> None:
        fields = p.format_telemetry(p.Session()).split(",")
        self.assertEqual(len(fields), 4)
        self.assertEqual(fields[0], "ACK")

    def test_boot_does_not_trigger_watchdog(self) -> None:
        session = p.Session()
        self.assertFalse(session.poll(now_ms=100_000))
        self.assertEqual(session.fault, "None")


class SessionCommands(unittest.TestCase):
    def test_track_on_enters_track_mode(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        self.assertEqual(session.mode, "TRACK")
        self.assertTrue(session.moving)
        self.assertEqual(p.format_telemetry(session), "ACK,-1,TRACK,None")

    def test_track_off_returns_to_safe_and_stops(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,1,100,1", rx_ms=11)
        session.feed(b"TRACK,0", rx_ms=12)
        self.assertEqual(session.mode, "SAFE")
        self.assertFalse(session.moving)
        self.assertEqual(session.seq, 1)

    def test_manual_forward_moves_one_step(self) -> None:
        session = p.Session()
        session.feed(b"MANUAL,1", rx_ms=10)
        self.assertEqual(session.mode, "MANUAL")
        self.assertTrue(session.moving)
        self.assertEqual(session.axis, p.MANUAL_STEP)
        self.assertEqual(p.format_telemetry(session), "ACK,-1,MANUAL,None")

    def test_manual_backward_moves_one_step(self) -> None:
        session = p.Session()
        session.feed(b"MANUAL,-1", rx_ms=10)
        self.assertEqual(session.axis, -p.MANUAL_STEP)

    def test_manual_zero_stops_without_changing_axis(self) -> None:
        session = p.Session()
        session.feed(b"MANUAL,1", rx_ms=10)
        session.feed(b"MANUAL,0", rx_ms=11)
        self.assertEqual(session.mode, "SAFE")
        self.assertFalse(session.moving)
        self.assertEqual(session.axis, 80)

    def test_manual_steps_accumulate_and_clamp(self) -> None:
        session = p.Session()
        for _ in range(20):
            session.feed(b"MANUAL,1", rx_ms=10)
        self.assertEqual(session.axis, p.MAX_TRAVEL)
        for _ in range(30):
            session.feed(b"MANUAL,-1", rx_ms=11)
        self.assertEqual(session.axis, -p.MAX_TRAVEL)

    def test_valid_sample_sets_axis_from_dead_band(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,1,55,1", rx_ms=11)
        self.assertEqual(session.axis, 0)
        self.assertTrue(session.moving)

    def test_valid_sample_tracks_target(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,1,1000,1", rx_ms=11)
        self.assertEqual(session.axis, 399)
        self.assertEqual(session.seq, 1)
        self.assertTrue(session.moving)

    def test_invalid_sample_zeroes_axis_and_stops(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,1,1000,1", rx_ms=11)
        session.feed(b"T,2,900,0", rx_ms=12)
        self.assertEqual(session.axis, 0)
        self.assertFalse(session.moving)
        self.assertEqual(session.seq, 2)
        self.assertEqual(session.mode, "TRACK")

    def test_sample_without_track_does_not_move_axis(self) -> None:
        session = p.Session()
        session.feed(b"T,1,1000,1", rx_ms=10)
        self.assertEqual(session.axis, 399)
        self.assertFalse(session.moving)
        self.assertEqual(session.mode, "SAFE")

    def test_telemetry_reflects_sample_sequence(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,7,-200,1", rx_ms=11)
        self.assertEqual(p.format_telemetry(session), "ACK,7,TRACK,None")


class RejectionDoesNotChangeState(unittest.TestCase):
    def _moving_session(self) -> p.Session:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=100)
        session.feed(b"T,1,500,1", rx_ms=101)
        return session

    def test_bad_track_value_keeps_state(self) -> None:
        session = self._moving_session()
        result = session.feed(b"TRACK,2", rx_ms=102)
        self.assertEqual(result.reason, "range")
        self.assertEqual(session.mode, "TRACK")
        self.assertTrue(session.moving)
        self.assertEqual(session.seq, 1)
        self.assertEqual(session.last_sample_ms, 101)

    def test_bad_manual_value_keeps_state(self) -> None:
        session = self._moving_session()
        session.feed(b"MANUAL,5", rx_ms=102)
        self.assertEqual(session.mode, "TRACK")
        self.assertEqual(session.axis, p.track_steps(500))
        self.assertEqual(session.seq, 1)

    def test_bad_range_sample_keeps_axis_and_sequence(self) -> None:
        session = self._moving_session()
        result = session.feed(b"T,2,1001,1", rx_ms=102)
        self.assertEqual(result.reason, "range")
        self.assertEqual(session.axis, p.track_steps(500))
        self.assertEqual(session.seq, 1)
        self.assertEqual(session.last_sample_ms, 101)
        self.assertTrue(session.moving)

    def test_bad_format_keeps_fault_unchanged(self) -> None:
        session = self._moving_session()
        session.feed(b"T,2,1001,1", rx_ms=102)
        self.assertEqual(session.fault, "Range")
        session.feed(b"BOGUS,1", rx_ms=103)
        self.assertEqual(session.fault, "Range")

    def test_repeated_sequence_keeps_state(self) -> None:
        session = self._moving_session()
        result = session.feed(b"T,1,100,1", rx_ms=102)
        self.assertEqual(result.reason, "sequence")
        self.assertEqual(session.seq, 1)
        self.assertEqual(session.axis, p.track_steps(500))

    def test_aged_command_keeps_state(self) -> None:
        session = self._moving_session()
        result = session.feed(b"T,2,100,1", rx_ms=102, now_ms=1400)
        self.assertEqual(result.reason, "age")
        self.assertEqual(session.seq, 1)
        self.assertEqual(session.last_sample_ms, 101)

    def test_range_fault_is_reported_in_telemetry(self) -> None:
        session = self._moving_session()
        session.feed(b"T,2,1001,1", rx_ms=102)
        self.assertEqual(
            p.format_telemetry(session), "ACK,1,TRACK,Range"
        )

    def test_accepted_command_clears_range_fault(self) -> None:
        session = self._moving_session()
        session.feed(b"T,2,1001,1", rx_ms=102)
        session.feed(b"TRACK,1", rx_ms=103)
        self.assertEqual(session.fault, "None")

    def test_range_fault_does_not_change_mode(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,0", rx_ms=10)
        session.feed(b"MANUAL,2", rx_ms=11)
        self.assertEqual(session.mode, "SAFE")
        self.assertEqual(session.fault, "Range")
        self.assertEqual(session.axis, 0)


class SessionWatchdog(unittest.TestCase):
    def test_watchdog_boundaries(self) -> None:
        session = p.Session()
        session.feed(b"T,1,100,1", rx_ms=1000)
        self.assertFalse(session.poll(now_ms=2200))
        self.assertTrue(session.poll(now_ms=2201))
        self.assertEqual(session.mode, "SAFE")
        self.assertFalse(session.moving)
        self.assertEqual(session.fault, "Lnk")

    def test_watchdog_telemetry(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,3,200,1", rx_ms=11)
        session.poll(now_ms=1212)
        self.assertEqual(p.format_telemetry(session), "ACK,3,SAFE,Lnk")

    def test_watchdog_only_fires_once(self) -> None:
        session = p.Session()
        session.feed(b"T,1,0,1", rx_ms=1000)
        self.assertTrue(session.poll(now_ms=3000))
        self.assertFalse(session.poll(now_ms=9000))

    def test_watchdog_preserves_axis_and_seq(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,5,500,1", rx_ms=11)
        axis = session.axis
        session.poll(now_ms=1212)
        self.assertEqual(session.axis, axis)
        self.assertEqual(session.seq, 5)

    def test_only_accepted_sample_refreshes_watchdog(self) -> None:
        session = p.Session()
        session.feed(b"T,1,100,1", rx_ms=1000)
        session.feed(b"TRACK,1", rx_ms=2000)
        session.feed(b"MANUAL,1", rx_ms=2100)
        self.assertTrue(session.poll(now_ms=2300))

    def test_rejected_sample_does_not_refresh_watchdog(self) -> None:
        session = p.Session()
        session.feed(b"T,1,100,1", rx_ms=1000)
        session.feed(b"T,1,200,1", rx_ms=2000)
        self.assertTrue(session.poll(now_ms=2300))

    def test_new_sample_clears_link_fault(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=10)
        session.feed(b"T,1,100,1", rx_ms=11)
        session.poll(now_ms=1300)
        self.assertEqual(session.fault, "Lnk")
        session.feed(b"T,2,100,1", rx_ms=1400)
        self.assertEqual(session.fault, "None")
        self.assertFalse(session.link_lost)
        self.assertEqual(session.mode, "SAFE")

    def test_track_command_does_not_clear_link_fault(self) -> None:
        session = p.Session()
        session.feed(b"T,1,100,1", rx_ms=10)
        session.poll(now_ms=1300)
        session.feed(b"TRACK,1", rx_ms=1400)
        self.assertEqual(session.mode, "TRACK")
        self.assertEqual(session.fault, "Lnk")

    def test_manual_command_does_not_clear_link_fault(self) -> None:
        session = p.Session()
        session.feed(b"T,1,100,1", rx_ms=10)
        session.poll(now_ms=1300)
        session.feed(b"MANUAL,1", rx_ms=1400)
        self.assertEqual(session.mode, "MANUAL")
        self.assertEqual(session.fault, "Lnk")

    def test_watchdog_does_not_accept_stale_sample(self) -> None:
        session = p.Session()
        session.feed(b"T,1,100,1", rx_ms=10)
        result = session.feed(b"T,2,100,1", rx_ms=20, now_ms=1400)
        self.assertEqual(result.reason, "age")
        self.assertEqual(session.seq, 1)

    def test_sample_at_exact_timeout_keeps_link(self) -> None:
        session = p.Session()
        session.feed(b"T,1,0,1", rx_ms=100)
        session.feed(b"MANUAL,-1", rx_ms=1300)
        self.assertFalse(session.poll(now_ms=1300))


class SequenceAcrossSession(unittest.TestCase):
    def test_sequence_is_monotonic_without_track_restart(self) -> None:
        session = p.Session()
        session.feed(b"T,1,0,1", rx_ms=10)
        session.feed(b"TRACK,1", rx_ms=11)
        result = session.feed(b"T,1,0,1", rx_ms=12)
        self.assertEqual(result.reason, "sequence")
        self.assertEqual(session.seq, 1)

    def test_sequence_can_advance_after_track_restart(self) -> None:
        session = p.Session()
        session.feed(b"T,1,0,1", rx_ms=10)
        session.feed(b"TRACK,1", rx_ms=11)
        self.assertTrue(session.feed(b"T,2,0,1", rx_ms=12).accepted)

    def test_failed_sequence_does_not_update_clock(self) -> None:
        session = p.Session()
        session.feed(b"T,1,0,1", rx_ms=10)
        session.feed(b"T,1,0,1", rx_ms=1000)
        self.assertEqual(session.last_sample_ms, 10)


if __name__ == "__main__":
    unittest.main()
