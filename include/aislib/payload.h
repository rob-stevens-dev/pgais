#pragma once

#include "error.h"

#include <cstdint>
#include <string>
#include <vector>

namespace aislib {

/**
 * @file payload.h
 * @brief NMEA 0183 AIS payload encoding and decoding.
 *
 * NMEA 0183 transports AIS binary data as a sequence of printable ASCII
 * characters known as "armoured" payload.  Each character represents exactly
 * 6 bits of binary data via a specific offset mapping defined in ITU-R
 * M.1371-5, section 3.4.
 *
 * The encoding rule is:
 *   - Add 48 to the 6-bit value.
 *   - If the result is >= 88, add a further 8.
 *
 * The decoding rule (inverse):
 *   - Subtract 48 from the ASCII value.
 *   - If the result is > 40, subtract a further 8.
 *   - The result must lie in [0, 63].
 *
 * Fill bits (0-5) are appended to the last payload word to pad the total bit
 * length to a multiple of 6.  They are stripped during decoding.  The fill
 * bit count appears as a separate field in the NMEA sentence.
 *
 * This module provides:
 *   - decode(): armoured string -> packed byte vector (bit stream)
 *   - encode(): packed byte vector -> armoured string + fill bit count
 *   - validate_payload_char(): single-character validation
 */

/**
 * @brief Decodes an NMEA AIS armoured payload string into a packed byte buffer.
 *
 * Each ASCII character in @p payload is converted to its 6-bit value and the
 * resulting bits are packed MSB-first into the output vector.  The trailing
 * @p fill_bits are removed, so the output represents exactly the message bits
 * without padding.
 *
 * The returned byte vector may not be a multiple of 8 bits in length; the
 * caller must use BitReader to access individual fields and must not assume
 * byte alignment.  The bit length of the decoded stream is:
 *   (payload.size() * 6) - fill_bits
 *
 * @param payload    The armoured payload characters (field 5 of a VDM/VDO sentence).
 * @param fill_bits  The fill-bit count (field 6 of a VDM/VDO sentence).  Must be [0, 5].
 * @return           A packed byte buffer holding the decoded bit stream, or an error.
 *
 * Possible errors:
 *   - ErrorCode::PayloadEmpty           — @p payload is empty.
 *   - ErrorCode::PayloadFillBitsInvalid — @p fill_bits is not in [0, 5].
 *   - ErrorCode::PayloadCharInvalid     — a character in @p payload is outside
 *                                         the valid armour range.
 */
[[nodiscard]] Result<std::vector<uint8_t>> decode_payload(
    const std::string& payload,
    uint8_t            fill_bits);

/**
 * @brief Encodes a packed byte buffer into an NMEA AIS armoured payload string.
 *
 * @p bit_length specifies the number of significant bits in @p data.  If
 * @p bit_length is not a multiple of 6, fill bits are added to round up and
 * the count is returned in the second element of the result pair.
 *
 * @param data        The packed byte buffer to encode.
 * @param bit_length  Number of significant bits in @p data.  The caller is
 *                    responsible for ensuring data.size() * 8 >= bit_length.
 * @return            On success: pair<armoured_string, fill_bit_count>.
 *                    On failure: an error (ErrorCode::PayloadTooShort if
 *                    data.size()*8 < bit_length).
 */
[[nodiscard]] Result<std::pair<std::string, uint8_t>> encode_payload(
    const std::vector<uint8_t>& data,
    std::size_t                 bit_length);

/**
 * @brief Validates that a single character is a legal NMEA AIS armour character.
 *
 * Legal values are ASCII 48 ('0') through 87 ('W') and 96 ('`') through
 * 119 ('w'), corresponding to 6-bit values 0-63.  The gap [88, 95] is
 * excluded by the encoding rule.
 *
 * @param ch  The character to test.
 * @return    True if @p ch is a valid armour character.
 */
[[nodiscard]] bool is_valid_payload_char(char ch) noexcept;

/**
 * @brief Returns the number of bits encoded in a payload string after fill removal.
 *
 * This is a pure arithmetic helper: @c (payload.size() * 6) - fill_bits.
 * No validation is performed.
 *
 * @param payload_len  Number of armour characters.
 * @param fill_bits    Number of fill bits [0, 5].
 * @return             Number of significant bits in the decoded stream.
 */
[[nodiscard]] constexpr std::size_t payload_bit_length(
    std::size_t payload_len,
    uint8_t     fill_bits) noexcept
{
    return (payload_len * 6u) - fill_bits;
}

} // namespace aislib