# Changelog

All notable changes to pgais are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Version numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Fixed

- **`payload.cpp`** — replaced a `goto`-based early exit from a nested loop in
  `decode_payload()` with a named static helper function `unpack_bits()`.  The
  logic is identical; the `goto` was the only use of that construct in the
  codebase and is inconsistent with idiomatic C++17.

- **`sentence.cpp`** — corrected the field-index comment in `Sentence::parse()`
  that mapped `[4]` to "payload".  The correct mapping is `[4]` channel,
  `[5]` payload, `[6]` fill bits, which matches the actual `fields[]` access
  pattern immediately below the comment.

- **`bitfield.h` / `bitfield.cpp`** — resolved an inconsistency between the
  Doxygen comment on `ascii_to_ais6bit()` and its implementation.  The header
  stated the invalid-input sentinel was 63; the implementation returned 15.
  The sentinel is now consistently 63 (maps back to `'?'` via
  `ais6bit_to_ascii()`), chosen because it is printable, unambiguously signals
  an encoding failure, and is distinct from the padding character `'@'`
  (value 0).  The test `InvalidChar_Returns15` has been renamed to
  `InvalidChar_Returns63` and updated to assert the correct value.  The test
  `Value63_IsUnderscore_Again` has been renamed to `Value63_IsQuestionMark`
  with an accurate description of the mapping.

### Documentation

- **`sentence.h`** / **`sentence.cpp`** — added `TODO(phase-2)` comments on
  the `SentenceAssembler::sequences_` member (header) and at the point of
  `sequences_.find()` (implementation) documenting the known limitation that
  the reassembly map is keyed on `seq_id` alone.  In a live AIS feed, two
  independent transmitters may legally use the same `seq_id` concurrently on
  the same channel, making the key ambiguous.  The correct key is
  `(channel, seq_id)` or `(channel, seq_id, talker)`.  This is an intentional
  deferral; the fix requires a composite map key with a custom hash and is
  scoped to Phase 2.

---

## [0.1.0] — 2026-03-11

Initial release.  Implements the complete Phase 1 core parsing infrastructure.

### Added

- `include/aislib/error.h` — `ErrorCode` enum, `Error` class, and `Result<T>`
  discriminated union.  All fallible public APIs return `Result<T>`; no
  exceptions are thrown across library boundaries.

- `include/aislib/bitfield.h` / `src/core/bitfield.cpp` — `BitReader` and
  `BitWriter` for packed MSB-first bit-field access over a
  `std::vector<uint8_t>` buffer.  Supports unsigned, signed (two's-complement),
  boolean, 6-bit ASCII text, and raw bit reads and writes.  All operations are
  bounds-checked.

- `include/aislib/payload.h` / `src/core/payload.cpp` — NMEA 6-bit ASCII
  armour encode and decode (`decode_payload`, `encode_payload`).  Fill-bit
  handling is explicit in both directions.

- `include/aislib/sentence.h` / `src/core/sentence.cpp` — `Sentence::parse()`
  for full NMEA 0183 sentence validation including XOR checksum verification.
  `SentenceAssembler` provides stateful multi-part message reassembly with
  support for concurrent independent sequences keyed by `seq_id`.

- `include/aislib/message.h` / `src/core/message.cpp` — `Message` abstract
  base class, `MessageType` enumeration covering all 27 ITU-R M.1371-5 type
  IDs, `message_type_from_id()` helper, and `MessageRegistry` singleton
  providing a decoder dispatch table.  No concrete decoders are registered in
  Phase 1.

- `CMakeLists.txt` — C++17 build with strict per-target warning flags and
  `-Werror`.  GoogleTest 1.14.0 fetched via `FetchContent` with warnings
  suppressed during its build and its headers marked `SYSTEM`.  Coverage
  instrumentation available via `-DPGAIS_ENABLE_COVERAGE=ON`.

- `tests/core/` — 152 unit tests across six files covering all public APIs,
  all documented error conditions, and encode/decode round-trips.