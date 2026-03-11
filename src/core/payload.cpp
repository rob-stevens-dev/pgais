#include "aislib/payload.h"

namespace aislib {

// =============================================================================
// Armour character validation
// =============================================================================

bool is_valid_payload_char(char ch) noexcept {
    // The NMEA AIS armour encoding adds 48 to the 6-bit value, then adds 8
    // more if the result exceeds 87.  This produces values in two ranges:
    //   [48, 87]  -> 6-bit values [0, 39]   (ASCII '0' to 'W')
    //   [96, 119] -> 6-bit values [40, 63]  (ASCII '`' to 'w')
    // The gap [88, 95] (ASCII 'X' to '_') is not produced by any valid value.
    const auto u = static_cast<unsigned char>(ch);
    return (u >= 48u && u <= 87u) || (u >= 96u && u <= 119u);
}

// =============================================================================
// Armour decode: single character -> 6-bit value
// =============================================================================

/**
 * @brief Converts a single NMEA armour character to its 6-bit payload value.
 *
 * The decoding rule from ITU-R M.1371-5, section 3.4:
 *   1. Subtract 48 from the ASCII value.
 *   2. If the result is greater than 40, subtract a further 8.
 *
 * @param ch  A character that has already been validated by is_valid_payload_char().
 * @return    The 6-bit value in [0, 63].
 */
static uint8_t armour_decode(char ch) noexcept {
    int val = static_cast<unsigned char>(ch) - 48;
    if (val > 40) {
        val -= 8;
    }
    return static_cast<uint8_t>(val & 0x3F);
}

// =============================================================================
// Armour encode: 6-bit value -> single character
// =============================================================================

/**
 * @brief Converts a 6-bit payload value to its NMEA armour character.
 *
 * The encoding rule from ITU-R M.1371-5, section 3.4:
 *   1. Add 48 to the 6-bit value.
 *   2. If the result is greater than or equal to 88, add a further 8.
 *
 * @param val  A 6-bit value in [0, 63].
 * @return     The corresponding armour character.
 */
static char armour_encode(uint8_t val) noexcept {
    int result = static_cast<int>(val & 0x3Fu) + 48;
    if (result >= 88) {
        result += 8;
    }
    return static_cast<char>(result);
}

// =============================================================================
// unpack_bits (internal helper)
// =============================================================================

/**
 * @brief Unpacks armoured payload characters into a packed big-endian byte
 *        buffer, stopping once @p significant_bits bits have been written.
 *
 * This helper exists to avoid a goto for early exit from the nested loop in
 * decode_payload().  It is called after all validation has passed, so every
 * character in @p payload is guaranteed to be a valid armour character.
 *
 * @param payload          The validated armoured payload string.
 * @param significant_bits Total number of bits to write (fill bits excluded).
 * @param out              Pre-allocated output buffer, zero-initialised,
 *                         sized to hold exactly @p significant_bits bits.
 */
static void unpack_bits(const std::string&    payload,
                        std::size_t           significant_bits,
                        std::vector<uint8_t>& out) noexcept
{
    for (std::size_t ci = 0; ci < payload.size(); ++ci) {
        const uint8_t sixbit = armour_decode(payload[ci]);
        for (std::size_t b = 0; b < 6u; ++b) {
            const std::size_t bit_idx = ci * 6u + b;
            if (bit_idx >= significant_bits) {
                return; // all significant bits written; fill bits discarded
            }
            const uint8_t     bit      = (sixbit >> (5u - b)) & 0x01u;
            const std::size_t byte_idx = bit_idx / 8u;
            const std::size_t bit_pos  = 7u - (bit_idx % 8u);
            out[byte_idx] |= static_cast<uint8_t>(bit << bit_pos);
        }
    }
}

// =============================================================================
// decode_payload
// =============================================================================

Result<std::vector<uint8_t>> decode_payload(
    const std::string& payload,
    uint8_t            fill_bits)
{
    if (payload.empty()) {
        return Error{ ErrorCode::PayloadEmpty };
    }

    if (fill_bits > 5u) {
        return Error{ ErrorCode::PayloadFillBitsInvalid,
                      std::to_string(static_cast<int>(fill_bits)) };
    }

    // Validate all characters before allocating the output buffer.
    for (std::size_t i = 0; i < payload.size(); ++i) {
        if (!is_valid_payload_char(payload[i])) {
            return Error{ ErrorCode::PayloadCharInvalid,
                          std::string(1, payload[i]) };
        }
    }

    // Each character contributes 6 bits; we pack those bits MSB-first into
    // output bytes.  The final fill_bits are discarded.
    const std::size_t total_bits = payload.size() * 6u;
    if (total_bits < static_cast<std::size_t>(fill_bits)) {
        // Degenerate case: fill bits exceed the total bit count.
        return Error{ ErrorCode::PayloadFillBitsInvalid };
    }
    const std::size_t significant_bits = total_bits - fill_bits;
    const std::size_t byte_count       = (significant_bits + 7u) / 8u;

    std::vector<uint8_t> out(byte_count, 0u);

    // Unpack 6 bits per character into the output, stopping at significant_bits.
    // Each iteration deposits at most one bit; returning early once bit_idx
    // reaches significant_bits avoids processing fill bits without requiring a
    // goto to break out of the nested loop.
    unpack_bits(payload, significant_bits, out);

    return out;
}

// =============================================================================
// encode_payload
// =============================================================================

Result<std::pair<std::string, uint8_t>> encode_payload(
    const std::vector<uint8_t>& data,
    std::size_t                 bit_length)
{
    if (bit_length == 0) {
        return Error{ ErrorCode::PayloadEmpty };
    }

    // Verify the caller supplied enough bytes.
    const std::size_t required_bytes = (bit_length + 7u) / 8u;
    if (data.size() < required_bytes) {
        return Error{ ErrorCode::PayloadTooShort,
                      "data buffer smaller than bit_length requires" };
    }

    // Determine fill bits: the number of zero bits appended to round up to a
    // multiple of 6.
    const uint8_t fill_bits = static_cast<uint8_t>(
        (6u - (bit_length % 6u)) % 6u);

    const std::size_t total_bits = bit_length + fill_bits;
    const std::size_t char_count = total_bits / 6u;

    std::string result;
    result.reserve(char_count);

    for (std::size_t ci = 0; ci < char_count; ++ci) {
        uint8_t sixbit = 0u;
        for (std::size_t b = 0; b < 6u; ++b) {
            const std::size_t bit_idx = ci * 6u + b;
            uint8_t bit = 0u;
            if (bit_idx < bit_length) {
                const std::size_t byte_idx = bit_idx / 8u;
                const std::size_t bit_pos  = 7u - (bit_idx % 8u);
                bit = (data[byte_idx] >> bit_pos) & 0x01u;
            }
            sixbit = static_cast<uint8_t>((sixbit << 1u) | bit);
        }
        result.push_back(armour_encode(sixbit));
    }

    return std::make_pair(std::move(result), fill_bits);
}

} // namespace aislib