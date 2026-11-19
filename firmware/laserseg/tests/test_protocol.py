"""Testes do contrato do parser UDP do laserseg.

Estes testes sao escritos ANTES da implementacao (TDD). Eles definem o que a
controladora deve aceitar, rejeitar e calcular. A implementacao C++ de
producao deve reproduzir exatamente esta tabela.

Execute com o Python LTS:

    powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1
"""

from __future__ import annotations

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))

import protocol_ref as p  # noqa: E402


def parse(text: str, **kwargs) -> p.Result:
    return p.parse(text.encode("ascii"), **kwargs)


class TrackMessage(unittest.TestCase):
    def test_track_on_is_accepted_and_enables_following(self) -> None:
        result = parse("TRACK,1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "track")
        self.assertTrue(result.enabled)

    def test_track_off_is_accepted_and_disables_following(self) -> None:
        result = parse("TRACK,0", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "track")
        self.assertFalse(result.enabled)

    def test_track_with_value_two_is_rejected(self) -> None:
        result = parse("TRACK,2", rx_ms=0)
        self.assertFalse(result.accepted)
        self.assertEqual(result.kind, "rejected")
        self.assertEqual(result.reason, "range")

    def test_track_without_value_is_rejected(self) -> None:
        result = parse("TRACK", rx_ms=0)
        self.assertFalse(result.accepted)
        self.assertEqual(result.reason, "format")

    def test_track_with_empty_value_is_rejected(self) -> None:
        self.assertEqual(parse("TRACK,", rx_ms=0).reason, "format")

    def test_track_with_extra_field_is_rejected(self) -> None:
        self.assertEqual(parse("TRACK,1,0", rx_ms=0).reason, "format")

    def test_track_with_leading_space_in_value_is_rejected(self) -> None:
        self.assertEqual(parse("TRACK, 1", rx_ms=0).reason, "format")

    def test_track_with_trailing_space_in_value_is_rejected(self) -> None:
        self.assertEqual(parse("TRACK,1 ", rx_ms=0).reason, "format")

    def test_track_with_negative_value_is_rejected(self) -> None:
        self.assertEqual(parse("TRACK,-1", rx_ms=0).reason, "range")

    def test_track_with_negative_garbage_is_format_error(self) -> None:
        self.assertEqual(parse("TRACK,-abc", rx_ms=0).reason, "format")


class LightMessage(unittest.TestCase):
    def test_light_on_is_accepted(self) -> None:
        result = parse("LIGHT,1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "light")
        self.assertTrue(result.enabled)

    def test_light_off_is_accepted(self) -> None:
        result = parse("LIGHT,0", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertFalse(result.enabled)

    def test_light_with_value_two_is_rejected(self) -> None:
        self.assertEqual(parse("LIGHT,2", rx_ms=0).reason, "range")

    def test_light_with_garbage_value_is_rejected(self) -> None:
        self.assertEqual(parse("LIGHT,on", rx_ms=0).reason, "format")


class ManualMessage(unittest.TestCase):
    def test_manual_pan_forward_is_accepted(self) -> None:
        result = parse("MANUAL,pan,1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "manual")
        self.assertEqual(result.axis, "pan")
        self.assertEqual(result.direction, 1)

    def test_manual_tilt_backward_is_accepted(self) -> None:
        result = parse("MANUAL,tilt,-1", rx_ms=0)
        self.assertTrue(result.accepted)
        self.assertEqual(result.axis, "tilt")
        self.assertEqual(result.direction, -1)

    def test_manual_with_unknown_axis_is_rejected(self) -> None:
        result = parse("MANUAL,roll,1", rx_ms=0)
        self.assertFalse(result.accepted)
        self.assertEqual(result.reason, "range")

    def test_manual_with_direction_zero_is_rejected(self) -> None:
        self.assertEqual(parse("MANUAL,pan,0", rx_ms=0).reason, "range")

    def test_manual_with_direction_two_is_rejected(self) -> None:
        self.assertEqual(parse("MANUAL,pan,2", rx_ms=0).reason, "range")

    def test_manual_without_direction_is_rejected(self) -> None:
        self.assertEqual(parse("MANUAL,pan", rx_ms=0).reason, "format")

    def test_manual_with_extra_field_is_rejected(self) -> None:
        self.assertEqual(parse("MANUAL,pan,1,0", rx_ms=0).reason, "format")

    def test_manual_with_uppercase_axis_is_rejected(self) -> None:
        self.assertEqual(parse("MANUAL,PAN,1", rx_ms=0).reason, "range")


class SampleMessage(unittest.TestCase):
    def test_centered_sample_is_accepted(self) -> None:
        result = parse("T,1,0,0,1", rx_ms=0, last_seq=None)
        self.assertTrue(result.accepted)
        self.assertEqual(result.kind, "sample")
        self.assertEqual((result.seq, result.x, result.y), (1, 0, 0))
        self.assertTrue(result.valid)

    def test_extreme_sample_is_accepted(self) -> None:
        result = parse("T,2,1000,-1000,1", rx_ms=0, last_seq=None)
        self.assertTrue(result.accepted)
        self.assertEqual((result.x, result.y), (1000, -1000))

    def test_invalid_sample_is_accepted_but_flagged(self) -> None:
        result = parse("T,3,1000,0,0", rx_ms=0, last_seq=None)
        self.assertTrue(result.accepted)
        self.assertFalse(result.valid)

    def test_opposite_extremes_are_accepted(self) -> None:
        result = parse("T,4,-1000,1000,1", rx_ms=0, last_seq=None)
        self.assertTrue(result.accepted)
        self.assertEqual((result.x, result.y), (-1000, 1000))

    def test_leading_zeros_are_accepted(self) -> None:
        result = parse("T,007,-01,0001,1", rx_ms=0, last_seq=None)
        self.assertTrue(result.accepted)
        self.assertEqual((result.seq, result.x, result.y), (7, -1, 1))

    def test_non_numeric_sequence_is_rejected(self) -> None:
        result = parse("T,abc,0,0,1", rx_ms=0, last_seq=None)
        self.assertFalse(result.accepted)
        self.assertEqual(result.reason, "format")

    def test_x_above_range_is_rejected(self) -> None:
        self.assertEqual(parse("T,3,1001,0,1", rx_ms=0).reason, "range")

    def test_y_below_range_is_rejected(self) -> None:
        self.assertEqual(parse("T,3,0,-1001,1", rx_ms=0).reason, "range")

    def test_valid_flag_two_is_rejected(self) -> None:
        self.assertEqual(parse("T,3,0,0,2", rx_ms=0).reason, "range")

    def test_missing_field_is_rejected(self) -> None:
        self.assertEqual(parse("T,1,0,0", rx_ms=0).reason, "format")

    def test_extra_field_is_rejected(self) -> None:
        self.assertEqual(parse("T,1,0,0,1,9", rx_ms=0).reason, "format")

    def test_negative_sequence_is_rejected(self) -> None:
        self.assertEqual(parse("T,-1,0,0,1", rx_ms=0).reason, "range")

    def test_float_coordinate_is_rejected(self) -> None:
        self.assertEqual(parse("T,1,0.5,0,1", rx_ms=0).reason, "format")


class SequenceRules(unittest.TestCase):
    def test_repeated_sequence_is_rejected(self) -> None:
        result = parse("T,5,0,0,1", rx_ms=0, last_seq=5)
        self.assertFalse(result.accepted)
        self.assertEqual(result.reason, "sequence")

    def test_regressing_sequence_is_rejected(self) -> None:
        result = parse("T,4,0,0,1", rx_ms=0, last_seq=5)
        self.assertFalse(result.accepted)
        self.assertEqual(result.reason, "sequence")

    def test_advancing_sequence_is_accepted(self) -> None:
        self.assertTrue(parse("T,6,0,0,1", rx_ms=0, last_seq=5).accepted)

    def test_first_sample_after_track_on_is_accepted(self) -> None:
        self.assertTrue(parse("T,1,0,0,1", rx_ms=0, last_seq=None).accepted)


class AgeRules(unittest.TestCase):
    def test_sample_exactly_at_timeout_is_accepted(self) -> None:
        result = parse("T,1,0,0,1", rx_ms=1000, now_ms=2200)
        self.assertTrue(result.accepted)

    def test_sample_past_timeout_is_rejected(self) -> None:
        result = parse("T,1,0,0,1", rx_ms=1000, now_ms=2201)
        self.assertFalse(result.accepted)
        self.assertEqual(result.reason, "age")

    def test_missing_now_means_no_queueing_delay(self) -> None:
        self.assertTrue(parse("T,1,0,0,1", rx_ms=999999).accepted)

    def test_expired_track_message_is_rejected(self) -> None:
        self.assertEqual(parse("TRACK,1", rx_ms=1000, now_ms=2201).reason, "age")

    def test_expired_light_message_is_rejected(self) -> None:
        self.assertEqual(parse("LIGHT,1", rx_ms=1000, now_ms=2201).reason, "age")

    def test_expired_manual_message_is_rejected(self) -> None:
        self.assertEqual(
            parse("MANUAL,pan,1", rx_ms=1000, now_ms=2201).reason, "age"
        )

    def test_light_exactly_at_timeout_is_accepted(self) -> None:
        self.assertTrue(parse("LIGHT,1", rx_ms=1000, now_ms=2200).accepted)


class DatagramRules(unittest.TestCase):
    def test_empty_datagram_is_rejected(self) -> None:
        self.assertEqual(p.parse(b"", rx_ms=0).reason, "length")

    def test_oversized_datagram_is_rejected(self) -> None:
        self.assertEqual(p.parse(b"T," + b"1" * 200, rx_ms=0).reason, "length")

    def test_datagram_with_nul_is_rejected(self) -> None:
        self.assertEqual(p.parse(b"TRACK,1\x00blocked", rx_ms=0).reason, "encoding")

    def test_datagram_with_high_byte_is_rejected(self) -> None:
        self.assertEqual(p.parse(b"TRACK,\xff", rx_ms=0).reason, "encoding")

    def test_datagram_with_newline_is_rejected(self) -> None:
        self.assertEqual(p.parse(b"TRACK,1\n", rx_ms=0).reason, "format")

    def test_unknown_command_is_rejected(self) -> None:
        self.assertEqual(parse("BOGUS,1", rx_ms=0).reason, "format")

    def test_lowercase_command_is_rejected(self) -> None:
        self.assertEqual(parse("track,1", rx_ms=0).reason, "format")

    def test_command_prefix_without_comma_is_rejected(self) -> None:
        self.assertEqual(parse("TRACKX,1", rx_ms=0).reason, "format")


class TargetFromError(unittest.TestCase):
    """Converte erro normalizado (-1000..1000) em alvo de passos."""

    MAX_TRAVEL = 533  # STEPS_PER_REV 3200 / 6, truncado

    def test_error_inside_dead_band_does_not_move(self) -> None:
        self.assertEqual(p.target_from_error(0, max_travel=self.MAX_TRAVEL, gain=0.75), 0)

    def test_error_exactly_on_dead_band_does_not_move(self) -> None:
        self.assertEqual(p.target_from_error(55, max_travel=self.MAX_TRAVEL, gain=0.75), 0)

    def test_error_just_above_dead_band_moves(self) -> None:
        self.assertGreater(abs(p.target_from_error(56, max_travel=self.MAX_TRAVEL, gain=0.75)), 0)

    def test_full_scale_error_reaches_three_quarters_of_travel(self) -> None:
        self.assertEqual(p.target_from_error(1000, max_travel=self.MAX_TRAVEL, gain=0.75), 399)

    def test_gain_one_and_full_scale_reaches_the_limit(self) -> None:
        self.assertEqual(p.target_from_error(-1000, max_travel=self.MAX_TRAVEL, gain=1.0), -533)

    def test_result_never_exceeds_travel_limit(self) -> None:
        self.assertEqual(p.target_from_error(1000, max_travel=self.MAX_TRAVEL, gain=2.0), self.MAX_TRAVEL)


class ManualTarget(unittest.TestCase):
    MAX_TRAVEL = 533

    def test_step_forward_from_center(self) -> None:
        self.assertEqual(p.manual_target(0, 1, max_travel=self.MAX_TRAVEL), 80)

    def test_step_backward_from_center(self) -> None:
        self.assertEqual(p.manual_target(0, -1, max_travel=self.MAX_TRAVEL), -80)

    def test_forward_step_is_clamped_to_limit(self) -> None:
        self.assertEqual(p.manual_target(500, 1, max_travel=self.MAX_TRAVEL), self.MAX_TRAVEL)

    def test_backward_step_is_clamped_to_limit(self) -> None:
        self.assertEqual(p.manual_target(-500, -1, max_travel=self.MAX_TRAVEL), -self.MAX_TRAVEL)


class LinkWatchdog(unittest.TestCase):
    def test_link_is_alive_before_timeout(self) -> None:
        self.assertFalse(p.link_expired(last_packet_ms=1000, now_ms=2199))

    def test_link_expires_past_timeout(self) -> None:
        self.assertTrue(p.link_expired(last_packet_ms=1000, now_ms=2201))

    def test_link_expires_before_any_packet(self) -> None:
        self.assertTrue(p.link_expired(last_packet_ms=None, now_ms=0))


class SessionState(unittest.TestCase):
    def test_fresh_session_starts_with_axes_stopped_and_light_off(self) -> None:
        session = p.Session()
        self.assertTrue(session.axes_stopped)
        self.assertFalse(session.light)
        self.assertFalse(session.tracking)

    def test_accepted_datagram_updates_last_packet(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        self.assertEqual(session.last_packet_ms, 1000)

    def test_accepted_sample_updates_last_packet(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,1,0,0,1", rx_ms=1001)
        self.assertEqual(session.last_packet_ms, 1001)

    def test_accepted_manual_updates_last_packet(self) -> None:
        session = p.Session()
        session.feed(b"MANUAL,pan,1", rx_ms=1234)
        self.assertEqual(session.last_packet_ms, 1234)

    def test_expired_command_does_not_update_last_packet_or_light(self) -> None:
        session = p.Session()
        result = session.feed(b"LIGHT,1", rx_ms=1000, now_ms=2201)
        self.assertFalse(result.accepted)
        self.assertIsNone(session.last_packet_ms)
        self.assertFalse(session.light)

    def test_rejected_command_does_not_change_light(self) -> None:
        session = p.Session()
        session.feed(b"LIGHT,1", rx_ms=1000)
        session.feed(b"LIGHT,2", rx_ms=1001)
        self.assertTrue(session.light)

    def test_rejected_datagram_does_not_update_last_packet(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,2", rx_ms=1000)
        self.assertIsNone(session.last_packet_ms)

    def test_sequence_is_recorded_on_accepted_sample(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,7,0,0,1", rx_ms=1001)
        self.assertEqual(session.last_seq, 7)

    def test_rejected_sample_does_not_change_last_sequence(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,7,0,0,1", rx_ms=1001)
        result = session.feed(b"T,8,9999,0,1", rx_ms=1002)
        self.assertFalse(result.accepted)
        self.assertEqual(result.reason, "range")
        self.assertEqual(session.last_seq, 7)

    def test_track_on_restarts_the_sequence_window(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,1,0,0,1", rx_ms=1001)
        session.feed(b"TRACK,1", rx_ms=1002)
        self.assertTrue(session.feed(b"T,1,0,0,1", rx_ms=1003).accepted)

    def test_track_off_disables_tracking(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"TRACK,0", rx_ms=1001)
        self.assertFalse(session.tracking)

    def test_light_message_updates_output_state(self) -> None:
        session = p.Session()
        session.feed(b"LIGHT,1", rx_ms=1000)
        self.assertTrue(session.light)
        session.feed(b"LIGHT,0", rx_ms=1001)
        self.assertFalse(session.light)

    def test_watchdog_forces_light_off_tracking_off_and_stops_axes(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,1,100,0,1", rx_ms=1001)
        session.feed(b"LIGHT,1", rx_ms=1002)
        self.assertFalse(session.axes_stopped)

        self.assertTrue(session.poll(now_ms=2203))
        self.assertFalse(session.light)
        self.assertFalse(session.tracking)
        self.assertTrue(session.axes_stopped)

    def test_watchdog_does_not_trigger_inside_window(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        self.assertFalse(session.poll(now_ms=2200))

    def test_track_off_stops_axes(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,1,100,0,1", rx_ms=1001)
        self.assertFalse(session.axes_stopped)
        session.feed(b"TRACK,0", rx_ms=1002)
        self.assertTrue(session.axes_stopped)

    def test_sample_without_valid_detection_stops_axes(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,1,100,0,1", rx_ms=1001)
        session.feed(b"T,2,100,0,0", rx_ms=1002)
        self.assertTrue(session.axes_stopped)

    def test_valid_sample_releases_the_stop_state(self) -> None:
        session = p.Session()
        session.feed(b"TRACK,1", rx_ms=1000)
        session.feed(b"T,1,100,0,1", rx_ms=1001)
        self.assertFalse(session.axes_stopped)


if __name__ == "__main__":
    unittest.main(verbosity=2)
