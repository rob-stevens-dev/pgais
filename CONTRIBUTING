# Contributing to aislib

This document describes the conventions, standards, and workflow expected for
contributions to aislib.  Read it in full before submitting a pull request.

## Branch naming

Branches follow the pattern `feature/<phase>-<short-description>` or
`fix/<short-description>`.  Examples:

    feature/phase2-message-type-1
    fix/multipart-seq-id-boundary
    refactor/bitwriter-reserve-hint

## Code style

The library targets C++17.  The style rules below are enforced by the CI
pipeline via clang-format.

- Indentation: 4 spaces.  No tabs.
- Brace style: Allman (opening brace on its own line for function definitions
  and class declarations; same line for control flow when the body is a single
  statement).
- Line length: 100 characters maximum.
- Include order: project headers, then standard library headers, in
  alphabetical order within each group.  Separate the two groups with a blank
  line.
- Trailing whitespace is not permitted.
- Files end with a single newline character.

## Documentation requirements

Every public function, class, method, and non-obvious constant must carry a
Doxygen comment block.  The minimum required tags are `@brief` for all
declarations, plus `@param`, `@return`, and `@pre` where applicable.  Refer to
the existing headers for the expected style.

Documentation prose must be written in plain technical English.  Avoid
colloquialisms, filler phrases, and excessive hedging.  Sentence-level grammar
is required; incomplete sentences are acceptable only inside parameter and
return descriptions that follow an established shorthand convention.

## Test requirements

All contributions must include unit tests.  The coverage threshold for any
modified or new file is 90% line coverage; the target for the overall project
is 95%.  Coverage is measured by the CI pipeline using gcov and lcov.

Tests are written with Google Test.  Fixtures must be used wherever shared
setup or teardown logic would otherwise be duplicated across test cases.  Each
`TEST` or `TEST_F` case must test exactly one behaviour; avoid asserting
unrelated conditions in the same test case.

Negative tests (error paths) are required for every error code that a function
can produce.  Roundtrip tests are required for every codec and serialisation
function.

## Commit messages

Commit messages must have a subject line of 72 characters or fewer, followed
by a blank line and a body that describes the motivation for the change.  The
subject line must be in the imperative mood ("Add support for type 5 decoding"
not "Added support for type 5 decoding").

Reference the relevant GitHub issue number in the body where one exists.

## Pull request process

1. Fork the repository and create a branch from `main` following the naming
   convention above.
2. Implement your changes, ensuring all tests pass and coverage thresholds are
   met.
3. Update `CHANGELOG.md` with a summary of the change under the `Unreleased`
   section.
4. Open a pull request against `main`.  The description must include a summary
   of the change, the motivation, and any testing notes.
5. All CI checks must pass before a pull request is eligible for review.
6. At least one review approval is required before merging.

## Error handling

aislib does not throw exceptions.  All new fallible functions must return
`Result<T>`.  Adding `throw` anywhere in the library source is not permitted.
This requirement exists because the library is designed to be embedded in
PostgreSQL extensions, which use `longjmp`-based error handling incompatible
with C++ stack unwinding.

## Adding a new message type decoder

1. Add a header `include/aislib/messages/msgN.h` and implementation
   `src/messages/msgN.cpp` where N is the message type number.
2. Derive from `aislib::Message` and implement the `type()` override.
3. Implement a free decoder function with the signature required by
   `aislib::DecoderFn`.
4. Register the decoder in the library initialisation translation unit.
5. Add comprehensive unit tests in `tests/test_message_N.cpp`.

## Versioning

aislib follows Semantic Versioning.  The version is defined in the root
`CMakeLists.txt` `project()` call.  The major version is incremented when a
change breaks the public API.  The minor version is incremented when new
functionality is added in a backward-compatible manner.  The patch version is
incremented for backward-compatible bug fixes.