#include "aislib/bitfield.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace aislib {

// =============================================================================
// Internal helpers
// =============================================================================

/**
 * @brief Reads a single bit from a big-endian packed byte buffer.
 *
 * Bit index 0 is the most-significant bit of byte 0.  This helper is the
 * fundamental primitive underlying all multi-bit reads.
 *
 * @param buf  The byte buffer.
 * @param idx  Zero-based bit index.
 * @return     The bit value: 0 or 1.
 */
static inline uint8_t read_bit(const std::vector<uint8_t>& buf, std::size_t idx) noexcept {
    const std::size_t byte_idx = idx / 8u;
    const std::size_t bit_pos  = 7u - (idx % 8u); // MSB first
    return static_cast<uint8_t>((buf[byte_idx] >> bit_pos) & 0x01u);
}

// =============================================================================
// BitReader
// =============================================================================

BitReader::BitReader(const std::vector<uint8_t>& buf) noexcept
    : buf_(buf) {}

std::size_t BitReader::bit_length() const noexcept {
    return buf_.size() * 8u;
}

ErrorCode BitReader::check_bounds(std::size_t offset, std::size_t width) const noexcept {
    if (width == 0 || width > 64u) {
        return ErrorCode::BitfieldWidthInvalid;
    }
    if (offset + width > bit_length()) {
        return ErrorCode::BitfieldOutOfRange;
    }
    return ErrorCode::Ok;
}

Result<uint64_t> BitReader::read_uint(std::size_t offset, std::size_t width) const noexcept {
    ErrorCode ec = check_bounds(offset, width);
    if (ec != ErrorCode::Ok) {
        return Error{ ec };
    }

    uint64_t value = 0;
    for (std::size_t i = 0; i < width; ++i) {
        value = (value << 1u) | read_bit(buf_, offset + i);
    }
    return value;
}

Result<int64_t> BitReader::read_int(std::size_t offset, std::size_t width) const noexcept {
    ErrorCode ec = check_bounds(offset, width);
    if (ec != ErrorCode::Ok) {
        return Error{ ec };
    }

    uint64_t raw = 0;
    for (std::size_t i = 0; i < width; ++i) {
        raw = (raw << 1u) | read_bit(buf_, offset + i);
    }

    // Sign-extend: if the MSB of the field is set, fill upper bits with 1s.
    if (width < 64u && (raw >> (width - 1u)) & 0x01u) {
        raw |= (~static_cast<uint64_t>(0)) << width;
    }
    return static_cast<int64_t>(raw);
}

Result<bool> BitReader::read_bool(std::size_t offset) const noexcept {
    ErrorCode ec = check_bounds(offset, 1u);
    if (ec != ErrorCode::Ok) {
        return Error{ ec };
    }
    return read_bit(buf_, offset) != 0u;
}

Result<std::string> BitReader::read_text(std::size_t offset, std::size_t char_count) const {
    if (char_count == 0) {
        return std::string{};
    }

    const std::size_t total_bits = char_count * 6u;
    ErrorCode ec = check_bounds(offset, total_bits);
    if (ec != ErrorCode::Ok) {
        return Error{ ec };
    }

    std::string result;
    result.reserve(char_count);

    for (std::size_t i = 0; i < char_count; ++i) {
        uint64_t raw = 0;
        for (std::size_t b = 0; b < 6u; ++b) {
            raw = (raw << 1u) | read_bit(buf_, offset + i * 6u + b);
        }
        result.push_back(ais6bit_to_ascii(static_cast<uint8_t>(raw)));
    }

    // Strip trailing '@' characters (6-bit value 0, the AIS padding character)
    // and trailing spaces per ITU-R M.1371-5 Table 44 convention.
    while (!result.empty() && (result.back() == '@' || result.back() == ' ')) {
        result.pop_back();
    }

    return result;
}

Result<std::vector<uint8_t>> BitReader::read_bits(std::size_t offset, std::size_t width) const {
    if (width == 0) {
        return std::vector<uint8_t>{};
    }

    ErrorCode ec = check_bounds(offset, width);
    if (ec != ErrorCode::Ok) {
        return Error{ ec };
    }

    const std::size_t byte_count = (width + 7u) / 8u;
    std::vector<uint8_t> result(byte_count, 0u);

    for (std::size_t i = 0; i < width; ++i) {
        const uint8_t bit = read_bit(buf_, offset + i);
        const std::size_t out_byte = i / 8u;
        const std::size_t out_bit  = 7u - (i % 8u);
        result[out_byte] |= static_cast<uint8_t>(bit << out_bit);
    }
    return result;
}

// =============================================================================
// BitWriter
// =============================================================================

BitWriter::BitWriter(std::size_t reserve_bits) {
    if (reserve_bits > 0) {
        buf_.reserve((reserve_bits + 7u) / 8u);
    }
}

std::size_t BitWriter::bit_length() const noexcept {
    return bit_length_;
}

void BitWriter::push_bit(uint8_t bit) {
    const std::size_t byte_idx = bit_length_ / 8u;
    const std::size_t bit_pos  = 7u - (bit_length_ % 8u);

    if (byte_idx >= buf_.size()) {
        buf_.push_back(0u);
    }
    buf_[byte_idx] |= static_cast<uint8_t>((bit & 0x01u) << bit_pos);
    ++bit_length_;
}

Result<void> BitWriter::write_uint(uint64_t value, std::size_t width) {
    if (width == 0 || width > 64u) {
        return Error{ ErrorCode::BitfieldWidthInvalid };
    }
    for (std::size_t i = width; i > 0; --i) {
        push_bit(static_cast<uint8_t>((value >> (i - 1u)) & 0x01u));
    }
    return Result<void>::success();
}

Result<void> BitWriter::write_int(int64_t value, std::size_t width) {
    if (width == 0 || width > 64u) {
        return Error{ ErrorCode::BitfieldWidthInvalid };
    }
    // Reinterpret as unsigned; two's complement is the same bit pattern.
    return write_uint(static_cast<uint64_t>(value), width);
}

Result<void> BitWriter::write_bool(bool value) {
    push_bit(value ? 1u : 0u);
    return Result<void>::success();
}

Result<void> BitWriter::write_text(const std::string& text, std::size_t char_count) {
    for (std::size_t i = 0; i < char_count; ++i) {
        const char ch     = (i < text.size()) ? text[i] : '@';
        const uint8_t val = ascii_to_ais6bit(ch);
        auto r = write_uint(val, 6u);
        if (!r) return r;
    }
    return Result<void>::success();
}

Result<void> BitWriter::write_bits(const std::vector<uint8_t>& src, std::size_t width) {
    if (width == 0) {
        return Result<void>::success();
    }
    const std::size_t required_bytes = (width + 7u) / 8u;
    if (src.size() < required_bytes) {
        return Error{ ErrorCode::BitfieldOutOfRange };
    }
    for (std::size_t i = 0; i < width; ++i) {
        const std::size_t byte_idx = i / 8u;
        const std::size_t bit_pos  = 7u - (i % 8u);
        const uint8_t bit = static_cast<uint8_t>((src[byte_idx] >> bit_pos) & 0x01u);
        push_bit(bit);
    }
    return Result<void>::success();
}

const std::vector<uint8_t>& BitWriter::buffer() const noexcept {
    return buf_;
}

// =============================================================================
// 6-bit ASCII codec
// =============================================================================

char ais6bit_to_ascii(uint8_t val) noexcept {
    // ITU-R M.1371-5, Table 44:
    //   6-bit value  0 -> '@'  (ASCII 64)
    //   6-bit value  1 -> 'A'  (ASCII 65)
    //   ...
    //   6-bit value 31 -> '_'  (ASCII 95)
    //   6-bit value 32 -> ' '  (ASCII 32)
    //   6-bit value 33 -> '!'  (ASCII 33)
    //   ...
    //   6-bit value 63 -> '_'  (ASCII 95) -- note: same as value 31 in display,
    //                                          but the encoding is different
    //
    // The mapping rule: values [0..31] map to ASCII [64..95], values [32..63]
    // map to ASCII [32..63].
    val &= 0x3Fu; // guard against values > 63
    if (val < 32u) {
        return static_cast<char>(val + 64u);
    }
    return static_cast<char>(val);
}

uint8_t ascii_to_ais6bit(char ch) noexcept {
    const auto uch = static_cast<unsigned char>(ch);
    if (uch >= 64u && uch <= 95u) {
        return static_cast<uint8_t>(uch - 64u);
    }
    if (uch >= 32u && uch <= 63u) {
        return static_cast<uint8_t>(uch);
    }
    // Characters outside the 6-bit printable range map to '?' (6-bit value 63,
    // ASCII 63).  Value 63 is the conventional replacement sentinel: it is a
    // printable character, it round-trips through ais6bit_to_ascii as '?', and
    // it is distinct from the padding value 0 ('@').
    return 63u;
}

} // namespace aislib