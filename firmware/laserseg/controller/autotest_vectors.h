// GERADO POR tools/gen_autotest_vectors.py - NAO EDITAR A MAO.
// Fonte: tools/protocol_ref.py (contrato v0.1).
#ifndef LASERSEG_AUTOTEST_VECTORS_H
#define LASERSEG_AUTOTEST_VECTORS_H

#include "parser.h"

struct TestVector {
  const char* name;
  const char* datagram;
  size_t len;
  int64_t rx_ms;
  int64_t now_ms;
  bool has_last_seq;
  int32_t last_seq;
  laserseg::Kind kind;
  laserseg::Reason reason;
  bool accepted;
  int32_t seq;
  int32_t x;
  int32_t y;
  bool valid;
  bool enabled;
  laserseg::Axis axis;
  int8_t direction;
};

const TestVector kTestVectors[] = {
  {"track_on", "TRACK,1", 7, 0, -1, false, 0, laserseg::Kind::kTrack, laserseg::Reason::kNone, true, 0, 0, 0, false, true, laserseg::Axis::kNone, 0},
  {"track_off", "TRACK,0", 7, 0, -1, false, 0, laserseg::Kind::kTrack, laserseg::Reason::kNone, true, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_two", "TRACK,2", 7, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_missing", "TRACK", 5, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_empty", "TRACK,", 6, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_extra", "TRACK,1,0", 9, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_leading_space", "TRACK, 1", 8, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_trailing_space", "TRACK,1 ", 8, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_negative", "TRACK,-1", 8, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"track_negative_garbage", "TRACK,-abc", 10, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"light_on", "LIGHT,1", 7, 0, -1, false, 0, laserseg::Kind::kLight, laserseg::Reason::kNone, true, 0, 0, 0, false, true, laserseg::Axis::kNone, 0},
  {"light_off", "LIGHT,0", 7, 0, -1, false, 0, laserseg::Kind::kLight, laserseg::Reason::kNone, true, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"light_two", "LIGHT,2", 7, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"light_garbage", "LIGHT,on", 8, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"manual_pan_forward", "MANUAL,pan,1", 12, 0, -1, false, 0, laserseg::Kind::kManual, laserseg::Reason::kNone, true, 0, 0, 0, false, false, laserseg::Axis::kPan, 1},
  {"manual_tilt_backward", "MANUAL,tilt,-1", 14, 0, -1, false, 0, laserseg::Kind::kManual, laserseg::Reason::kNone, true, 0, 0, 0, false, false, laserseg::Axis::kTilt, -1},
  {"manual_bad_axis", "MANUAL,roll,1", 13, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"manual_direction_zero", "MANUAL,pan,0", 12, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"manual_direction_two", "MANUAL,pan,2", 12, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"manual_missing_direction", "MANUAL,pan", 10, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"manual_extra", "MANUAL,pan,1,0", 14, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"manual_uppercase_axis", "MANUAL,PAN,1", 12, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_center", "T,1,0,0,1", 9, 0, -1, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 1, 0, 0, true, false, laserseg::Axis::kNone, 0},
  {"sample_extreme", "T,2,1000,-1000,1", 16, 0, -1, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 2, 1000, -1000, true, false, laserseg::Axis::kNone, 0},
  {"sample_invalid_flag", "T,3,1000,0,0", 12, 0, -1, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 3, 1000, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_opposite", "T,4,-1000,1000,1", 16, 0, -1, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 4, -1000, 1000, true, false, laserseg::Axis::kNone, 0},
  {"sample_leading_zeros", "T,007,-01,0001,1", 16, 0, -1, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 7, -1, 1, true, false, laserseg::Axis::kNone, 0},
  {"sample_bad_seq", "T,abc,0,0,1", 11, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_x_above", "T,3,1001,0,1", 12, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_y_below", "T,3,0,-1001,1", 13, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_valid_two", "T,3,0,0,2", 9, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_missing_field", "T,1,0,0", 7, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_extra_field", "T,1,0,0,1,9", 11, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_negative_seq", "T,-1,0,0,1", 10, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kRange, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sample_float", "T,1,0.5,0,1", 11, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sequence_repeated", "T,5,0,0,1", 9, 0, -1, true, 5, laserseg::Kind::kRejected, laserseg::Reason::kSequence, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sequence_regressing", "T,4,0,0,1", 9, 0, -1, true, 5, laserseg::Kind::kRejected, laserseg::Reason::kSequence, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"sequence_advancing", "T,6,0,0,1", 9, 0, -1, true, 5, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 6, 0, 0, true, false, laserseg::Axis::kNone, 0},
  {"age_exact_limit", "T,1,0,0,1", 9, 1000, 2200, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 1, 0, 0, true, false, laserseg::Axis::kNone, 0},
  {"age_past_limit", "T,1,0,0,1", 9, 1000, 2201, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kAge, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"age_track", "TRACK,1", 7, 1000, 2201, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kAge, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"age_light", "LIGHT,1", 7, 1000, 2201, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kAge, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"age_manual", "MANUAL,pan,1", 12, 1000, 2201, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kAge, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"age_light_exact", "LIGHT,1", 7, 1000, 2200, false, 0, laserseg::Kind::kLight, laserseg::Reason::kNone, true, 0, 0, 0, false, true, laserseg::Axis::kNone, 0},
  {"first_sample_after_track_on", "T,1,0,0,1", 9, 10, -1, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 1, 0, 0, true, false, laserseg::Axis::kNone, 0},
  {"missing_now_large_rx", "T,1,0,0,1", 9, 5000000, -1, false, 0, laserseg::Kind::kSample, laserseg::Reason::kNone, true, 1, 0, 0, true, false, laserseg::Axis::kNone, 0},
  {"expired_large_rx", "T,1,0,0,1", 9, 5000000, 5001201, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kAge, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"empty", "", 0, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kLength, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"oversized", "T,11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", 202, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kLength, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"nul_byte", "TRACK,1\000blocked", 15, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kEncoding, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"high_byte", "TRACK,\377", 7, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kEncoding, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"newline", "TRACK,1\012", 8, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"unknown_command", "BOGUS,1", 7, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"lowercase_command", "track,1", 7, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
  {"prefix_without_comma", "TRACKX,1", 8, 0, -1, false, 0, laserseg::Kind::kRejected, laserseg::Reason::kFormat, false, 0, 0, 0, false, false, laserseg::Axis::kNone, 0},
};

constexpr size_t kTestVectorCount =
    sizeof(kTestVectors) / sizeof(kTestVectors[0]);

#endif  // LASERSEG_AUTOTEST_VECTORS_H
