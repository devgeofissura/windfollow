// GERADO POR tools/gen_autotest_vectors.py - NAO EDITAR A MAO.
// Fonte: tools/protocol_ref.py (contrato v0.1).
#ifndef VENTISEG_AUTOTEST_VECTORS_H
#define VENTISEG_AUTOTEST_VECTORS_H

#include "parser.h"

struct TestVector {
  const char* name;
  const char* datagram;
  size_t len;
  int64_t rx_ms;
  int64_t now_ms;
  bool has_last_seq;
  int32_t last_seq;
  ventiseg::Kind kind;
  ventiseg::Reason reason;
  bool accepted;
  int32_t seq;
  int32_t x;
  bool valid;
  bool enabled;
  int8_t direction;
};

const TestVector kTestVectors[] = {
  {"track_on", "TRACK,1", 7, 0, -1, false, 0, ventiseg::Kind::kTrack, ventiseg::Reason::kNone, true, 0, 0, false, true, 0},
  {"track_off", "TRACK,0", 7, 0, -1, false, 0, ventiseg::Kind::kTrack, ventiseg::Reason::kNone, true, 0, 0, false, false, 0},
  {"track_two", "TRACK,2", 7, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"track_negative", "TRACK,-1", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"track_missing", "TRACK", 5, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"track_empty", "TRACK,", 6, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"track_extra", "TRACK,1,0", 9, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"track_leading_space", "TRACK, 1", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"track_trailing_space", "TRACK,1 ", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"track_garbage", "TRACK,on", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"track_float", "TRACK,1.0", 9, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"manual_forward", "MANUAL,1", 8, 0, -1, false, 0, ventiseg::Kind::kManual, ventiseg::Reason::kNone, true, 0, 0, false, false, 1},
  {"manual_backward", "MANUAL,-1", 9, 0, -1, false, 0, ventiseg::Kind::kManual, ventiseg::Reason::kNone, true, 0, 0, false, false, -1},
  {"manual_stop", "MANUAL,0", 8, 0, -1, false, 0, ventiseg::Kind::kManual, ventiseg::Reason::kNone, true, 0, 0, false, false, 0},
  {"manual_two", "MANUAL,2", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"manual_minus_two", "MANUAL,-2", 9, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"manual_missing", "MANUAL", 6, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"manual_extra", "MANUAL,1,0", 10, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"manual_garbage", "MANUAL,front", 12, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"manual_float", "MANUAL,1.0", 10, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"manual_axis_field", "MANUAL,pan,1", 12, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"sample_center", "T,1,0,1", 7, 0, -1, false, 0, ventiseg::Kind::kSample, ventiseg::Reason::kNone, true, 1, 0, true, false, 0},
  {"sample_extreme", "T,2,1000,1", 10, 0, -1, false, 0, ventiseg::Kind::kSample, ventiseg::Reason::kNone, true, 2, 1000, true, false, 0},
  {"sample_invalid_flag", "T,3,1000,0", 10, 0, -1, false, 0, ventiseg::Kind::kSample, ventiseg::Reason::kNone, true, 3, 1000, false, false, 0},
  {"sample_leading_zeros", "T,007,-01,01", 12, 0, -1, false, 0, ventiseg::Kind::kSample, ventiseg::Reason::kNone, true, 7, -1, true, false, 0},
  {"sample_bad_seq", "T,abc,0,1", 9, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"sample_bad_x", "T,1,abc,1", 9, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"sample_x_above", "T,3,1001,1", 10, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"sample_x_below", "T,3,-1001,1", 11, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"sample_valid_two", "T,3,0,2", 7, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"sample_missing_field", "T,1,0", 5, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"sample_extra_field", "T,1,0,1,9", 9, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"sample_negative_seq", "T,-1,0,1", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kRange, false, 0, 0, false, false, 0},
  {"sample_float", "T,1,0.5,1", 9, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"sequence_repeated", "T,5,0,1", 7, 0, -1, true, 5, ventiseg::Kind::kRejected, ventiseg::Reason::kSequence, false, 0, 0, false, false, 0},
  {"sequence_regressing", "T,4,0,1", 7, 0, -1, true, 5, ventiseg::Kind::kRejected, ventiseg::Reason::kSequence, false, 0, 0, false, false, 0},
  {"sequence_advancing", "T,6,0,1", 7, 0, -1, true, 5, ventiseg::Kind::kSample, ventiseg::Reason::kNone, true, 6, 0, true, false, 0},
  {"age_exact_limit", "T,1,0,1", 7, 1000, 2200, false, 0, ventiseg::Kind::kSample, ventiseg::Reason::kNone, true, 1, 0, true, false, 0},
  {"age_past_limit", "T,1,0,1", 7, 1000, 2201, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kAge, false, 0, 0, false, false, 0},
  {"age_track", "TRACK,1", 7, 1000, 2201, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kAge, false, 0, 0, false, false, 0},
  {"age_manual", "MANUAL,1", 8, 1000, 2201, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kAge, false, 0, 0, false, false, 0},
  {"age_negative", "T,1,0,1", 7, 1000, 999, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kAge, false, 0, 0, false, false, 0},
  {"missing_now_large_rx", "T,1,0,1", 7, 5000000, -1, false, 0, ventiseg::Kind::kSample, ventiseg::Reason::kNone, true, 1, 0, true, false, 0},
  {"expired_large_rx", "T,1,0,1", 7, 5000000, 5001201, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kAge, false, 0, 0, false, false, 0},
  {"empty", "", 0, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kLength, false, 0, 0, false, false, 0},
  {"oversized", "T,11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", 202, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kLength, false, 0, 0, false, false, 0},
  {"nul_byte", "TRACK,1\000blocked", 15, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kEncoding, false, 0, 0, false, false, 0},
  {"high_byte", "TRACK,\377", 7, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kEncoding, false, 0, 0, false, false, 0},
  {"newline", "TRACK,1\012", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"unknown_command", "BOGUS,1", 7, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"lowercase_command", "track,1", 7, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
  {"prefix_without_comma", "TRACKX,1", 8, 0, -1, false, 0, ventiseg::Kind::kRejected, ventiseg::Reason::kFormat, false, 0, 0, false, false, 0},
};

constexpr size_t kTestVectorCount =
    sizeof(kTestVectors) / sizeof(kTestVectors[0]);

#endif  // VENTISEG_AUTOTEST_VECTORS_H
