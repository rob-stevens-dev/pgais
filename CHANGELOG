# Changelog

All notable changes to pgais are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Version numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added

- **`include/aislib/message.h`** — added a pure virtual `encode()` method to
  the `Message` base class returning
  `Result<std::pair<std::string, uint8_t>>`.  Every concrete message subclass
  introduced in later phases must implement this method.  The return type
  matches the output contract of `encode_payload()` directly.  A subclass that
  has no valid encoding (e.g. a decode-only stub) should return
  `ErrorCode::MessageTypeUnsupported`.  Adding the pure virtual now makes the
  interface contract explicit and prevents any future subclass from being
  instantiated without providing an encode path.

- **`CMakePresets.json`** and **`CMakeLists.txt`** — resolved a version
  conflict.  The presets file declared `"version": 3`, which requires CMake
  3.21, while `cmake_minimum_required` was set to 3.16.  On any CMake in
  [3.16, 3.20] the presets file would fail to load.  `cmake_minimum_required`
  has been raised to 3.20 and the presets version lowered to 2 (requires
  3.20).  CMake 3.20 is the practical floor for `--preset` command-line
  support and is available on all CI-targeted platforms.

- **`include/aislib/error.h`** — added `ErrorCode::MultipartTalkerConflict = 405`.  This

  covers the case where a subsequent fragment of a multi-part sequence carries
  a different sentence type (VDM vs VDO) than the first fragment.  The error
  is distinct from `MultipartCountInvalid` and `MultipartOutOfOrder` because
  it indicates a logical contradiction in the sequence rather than a structural
  framing problem.

- **`CMakePresets.json`** — added a presets file defining `debug`, `release`,
  `coverage`, and `ci` configure/build/test presets.  The presets remove the
  need to remember flag combinations manually and are understood natively by
  CMake 3.19+, VS Code CMake Tools, and CLion 2023+.  The README build
  instructions have been updated to use preset names.

- **`.github/workflows/ci.yml`** — added a GitHub Actions workflow triggered
  on push and pull request to `main`.  The matrix covers GCC 12 and Clang 15
  on Ubuntu 22.04.  Every leg configures via the `ci` preset, builds, and runs
  the full test suite with `--output-on-failure`.  The GCC leg additionally
  runs a coverage build using the `coverage` preset, generates an lcov report,
  and uploads it as a workflow artifact with a 14-day retention window.

### Fixed

- **`src/core/sentence.cpp`** — `SentenceAssembler::feed()` now validates that
  every fragment of a multi-part sequence carries the same sentence type
  (VDM or VDO) as the first fragment.  Previously, a sequence opened by a VDM
  sentence could be completed by a VDO sentence, silently producing an
  `AssembledMessage` with an incorrect `is_own_vessel` flag.  The new check
  returns `ErrorCode::MultipartTalkerConflict` with a detail string identifying
  the seq_id and the conflicting sentence types.

- **`README.md`** — removed a forward reference to `aislib/aislib.h`, which
  does not exist in Phase 1.  The umbrella header will be introduced when the
  first concrete message decoder lands in Phase 2.  The usage examples now
  include the individual headers directly.  Build instructions have been
  updated to use `cmake --preset` rather than bare `-D` flag combinations.

### Fixed

- **`src/core/message.cpp`** — added a debug-mode `assert` at the top of
  `MessageRegistry::register_decoder()` that fires when `type_id` is outside
  [1, 27].  The existing release-mode range guard is retained so that
  out-of-range calls degrade silently rather than writing out of bounds.  The
  assert catches registration mistakes during development without any runtime
  cost in release builds.

- **`tests/core/test_message.cpp`** — the `DecodeUnregisteredTypeReturnsUnsupported`
  test previously looped over type IDs 1-27 while supplying the same fixed
  payload string on every iteration.  The loop variable `t` was unused, so
  every call was dispatching to type 1 rather than to the type the iteration
  claimed to be testing.  The test now builds a minimal payload for each
  iteration by packing `t` into the high 6 bits of a single byte and encoding
  it through `encode_payload()`, so each iteration genuinely exercises a
  distinct type ID in the registry dispatch path.

### Documentation

- **`src/core/sentence.cpp`** — strengthened the comment on `kValidTalkers` to
  make the strict-validation policy explicit.  The accepted set (AB, AI, AN,
  AR, AS, AT, AX, BS, SA) is drawn from ITU-R M.1371-5 Annex 8.  The comment
  now explains that the strict policy is deliberate and advises integrators
  working with non-standard talker IDs to normalise their input before calling
  `Sentence::parse()`.

- **`include/aislib/message.h`** — strengthened the `clear()` Doxygen comment
  to make the production risk explicit.  The method is for test fixtures only;
  calling it in production code will disable all decoding until decoders are
  re-registered.  A `@warning` tag has been added.

### Fixed (carried from previous session)

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