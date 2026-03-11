# pgais

A C++17 library for parsing and generating NMEA 0183 v4.1 AIS (Automatic
Identification System) messages.  The library implements all 27 message types
defined in ITU-R M.1371-5 and is designed to serve as the foundation for a
PostgreSQL extension module targeting versions 11 and later.

## Features

- Full NMEA 0183 sentence framing and checksum validation
- NMEA AIS armoured payload encoding and decoding (6-bit ASCII)
- Bit-level field extraction and insertion with big-endian AIS bit ordering
- Stateful multi-part sentence reassembly
- Typed error handling via `Result<T>` — no exceptions thrown across library boundaries
- Message type registry with a pluggable decoder interface
- Implementations of AIS message types 1-27 (added in subsequent phases)
- Unit test coverage at 90% line coverage or greater

## Requirements

- C++17-compliant compiler (GCC 8+, Clang 7+, MSVC 19.14+)
- CMake 3.16 or later
- Internet access during the first build (Google Test is fetched via CMake
  FetchContent)

## Building

```sh
git clone https://github.com/rob-stevens-dev/pgais.git
cd pgais
cmake --preset release
cmake --build --preset release
```

To build and run the test suite:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

To enable coverage instrumentation (GCC or Clang only):

```sh
cmake --preset coverage
cmake --build --preset coverage
ctest --preset coverage
lcov --capture --directory build/coverage --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

See `CMakePresets.json` for the full list of available presets.

## Installation

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix /usr/local
```

After installation the library is importable from another CMake project via:

```cmake
find_package(pgais REQUIRED)
target_link_libraries(my_target PRIVATE pgais::pgais_core)
```

## Usage

An umbrella header (`aislib/aislib.h`) will be provided once the first
concrete message decoders land in Phase 2.  Until then, include the individual
headers directly.

### Parsing a single sentence

```cpp
#include <aislib/sentence.h>
#include <aislib/message.h>

auto sr = aislib::Sentence::parse("!AIVDM,1,1,,B,15M67J`P00G?Ue`E`FepT4@000S8,0*32");
if (!sr) {
    std::cerr << sr.error().message() << "\n";
    return;
}

aislib::SentenceAssembler assembler;
auto ar = assembler.feed(sr.value());
if (!ar || !ar.value()) {
    // Error or waiting for more parts.
    return;
}

const aislib::AssembledMessage& msg = ar.value().value();
auto dr = aislib::MessageRegistry::instance().decode(msg.payload, msg.fill_bits);
if (!dr) {
    std::cerr << dr.error().message() << "\n";
    return;
}
// dr.value() is a std::unique_ptr<aislib::Message>
```

### Working with the bit stream directly

```cpp
#include <aislib/payload.h>
#include <aislib/bitfield.h>

auto buf = aislib::decode_payload(payload_string, fill_bits);
if (!buf) { /* handle error */ }

aislib::BitReader reader(buf.value());
auto type_r = reader.read_uint(0, 6);   // message type field
auto mmsi_r = reader.read_uint(8, 30);  // MMSI field
```

## Architecture

The library is organised into the following components, each with a
corresponding header and implementation file:

| Component       | Header                    | Purpose                                                              |
|-----------------|---------------------------|----------------------------------------------------------------------|
| Error handling  | `aislib/error.h`          | `ErrorCode` enum, `Error` class, `Result<T>` discriminated union     |
| Bit engine      | `aislib/bitfield.h`       | `BitReader` and `BitWriter` over big-endian packed bit streams       |
| Payload codec   | `aislib/payload.h`        | NMEA 6-bit ASCII armour encode and decode                            |
| Sentence framer | `aislib/sentence.h`       | NMEA sentence parsing, validation, and multi-part reassembly         |
| Message base    | `aislib/message.h`        | `Message` base class, `MessageType` enum, `MessageRegistry`         |

Message type implementations (types 1-27) are added in subsequent development
phases and follow the same pattern: a decoder function is registered with
`MessageRegistry::instance().register_decoder()` at library initialisation.

## Project Roadmap

Phase 1 establishes the core parsing infrastructure described above.
Subsequent phases add concrete message decoders in the following order:

- Phase 2: Message types 1, 2, 3 (Position Report Class A)
- Phase 3: Message types 4, 5, 11 (Base Station Report, Static/Voyage Data)
- Phase 4: Message types 6, 7, 8, 9 (Binary addressed, acknowledge, broadcast, SAR)
- Phase 5: Message types 10, 12 through 17
- Phase 6: Message types 18 through 27 (Class B, long-range, channel management)
- Phase 7: Multi-part binary data completeness and the generation (encode) path
- Phase 8: PostgreSQL extension scaffold (versions 11 and later)
- Phase 9: Integration tests, fuzz testing, and coverage hardening

## Error Handling Model

pgais does not throw exceptions from its public API.  All fallible operations
return `Result<T>`, which holds either a value or an `Error`.  This design is a
hard requirement for embedding the library in a PostgreSQL extension, where the
server uses `longjmp`-based error handling that is incompatible with C++ stack
unwinding.

The caller is expected to check the return value of every fallible call.
The `[[nodiscard]]` attribute is applied to all `Result<T>`-returning functions
to help enforce this at compile time.

## License

MIT License.  See `LICENSE` for details.