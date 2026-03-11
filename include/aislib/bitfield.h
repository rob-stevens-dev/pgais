#pragma once

#include "error.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace aislib {

/**
 * @file bitfield.h
 * @brief Bit-level read and write operations over a packed byte buffer.
 *
 * NMEA 0183 AIS message payloads are sequences of 6-bit values that, once
 * decoded from ASCII armour, form a contiguous bit stream.  Every field in
 * every message type is addressed by a bit offset and a bit width rather than
 * by a byte offset.
 *
 * BitReader and BitWriter operate on a std::vector<uint8_t> that stores the
 * decoded bit stream with bit 0 of the stream at the most-significant bit of
 * byte 0 (big-endian bit ordering, as required by ITU-R M.1371-5).
 *
 * Example — reading the 6-bit message type from the start of a buffer:
 * @code
 *   BitReader reader(buf);
 *   auto r = reader.read_uint(0, 6);   // bits [0, 5]
 *   if (r) { uint8_t msg_type = static_cast<uint8_t>(r.value()); }
 * @endcode
 */

// -----------------------------------------------------------------------------
// Bit ordering note
// -----------------------------------------------------------------------------
// AIS uses network (big-endian) bit order.  Bit 0 of the message is the
// most-significant bit of the first byte.  For a buffer of bytes b[0..N]:
//
//   bit index k  ->  byte b[k/8], bit (7 - k%8)
//
// All functions below follow this convention.
// -----------------------------------------------------------------------------

/**
 * @brief Read-only view over a packed AIS bit stream.
 *
 * BitReader does not own the underlying buffer; the caller must ensure the
 * buffer outlives the reader.  All reads are bounds-checked and return
 * Result<T> rather than throwing.
 */
class BitReader {
public:
    /**
     * @brief Constructs a BitReader over the given byte buffer.
     * @param buf  The decoded payload bytes.  The first bit of the AIS message
     *             is at the MSB of buf[0].
     */
    explicit BitReader(const std::vector<uint8_t>& buf) noexcept;

    /**
     * @brief Returns the total number of bits available in the buffer.
     */
    [[nodiscard]] std::size_t bit_length() const noexcept;

    /**
     * @brief Reads an unsigned integer field from the bit stream.
     *
     * @param offset  Zero-based bit offset of the field's most-significant bit.
     * @param width   Number of bits to read.  Must be in [1, 64].
     * @return        The field value zero-extended to uint64_t, or an error.
     *
     * ErrorCode::BitfieldOutOfRange is returned when offset+width exceeds the
     * buffer length.  ErrorCode::BitfieldWidthInvalid is returned for width
     * values outside [1, 64].
     */
    [[nodiscard]] Result<uint64_t> read_uint(std::size_t offset, std::size_t width) const noexcept;

    /**
     * @brief Reads a signed integer field using two's-complement representation.
     *
     * AIS uses two's-complement for signed fields such as latitude, longitude,
     * rate of turn, and speed over ground deltas.
     *
     * @param offset  Zero-based bit offset of the field's most-significant bit.
     * @param width   Number of bits to read.  Must be in [1, 64].
     * @return        The sign-extended value as int64_t, or an error.
     */
    [[nodiscard]] Result<int64_t> read_int(std::size_t offset, std::size_t width) const noexcept;

    /**
     * @brief Reads a boolean (1-bit) field.
     *
     * @param offset  Zero-based bit offset of the single bit.
     * @return        True if the bit is 1, false if 0, or an error.
     */
    [[nodiscard]] Result<bool> read_bool(std::size_t offset) const noexcept;

    /**
     * @brief Reads a 6-bit ASCII text field and returns it as a std::string.
     *
     * AIS encodes text in 6-bit ASCII (ITU-R M.1371 Table 44).  Each 6-bit
     * value maps to a printable character; trailing '@' (0x00) characters are
     * treated as padding and stripped from the result.
     *
     * @param offset      Zero-based bit offset of the first character's MSB.
     * @param char_count  Number of 6-bit characters to decode.
     * @return            The decoded string, with trailing spaces stripped, or
     *                    an error.
     */
    [[nodiscard]] Result<std::string> read_text(std::size_t offset, std::size_t char_count) const;

    /**
     * @brief Reads an arbitrary run of raw bits as a byte vector.
     *
     * The returned bytes are left-aligned: the first extracted bit becomes the
     * MSB of result[0].  If @p width is not a multiple of 8, the final byte is
     * zero-padded on the right.
     *
     * @param offset  Zero-based bit offset of the first bit.
     * @param width   Number of bits to read.
     * @return        The extracted bytes, or an error.
     */
    [[nodiscard]] Result<std::vector<uint8_t>> read_bits(std::size_t offset, std::size_t width) const;

private:
    const std::vector<uint8_t>& buf_;

    /**
     * @brief Validates that [offset, offset+width) lies within the buffer.
     * @return ErrorCode::Ok on success, or the appropriate error code.
     */
    [[nodiscard]] ErrorCode check_bounds(std::size_t offset, std::size_t width) const noexcept;
};

// -----------------------------------------------------------------------------

/**
 * @brief Write-oriented builder over a packed AIS bit stream.
 *
 * BitWriter owns its internal buffer and grows it as fields are written.
 * Bits are packed in big-endian order consistent with the AIS specification.
 * The completed buffer is retrieved via @c buffer().
 */
class BitWriter {
public:
    /**
     * @brief Constructs an empty BitWriter.
     * @param reserve_bits  Optional hint for the expected total bit length;
     *                      used only to pre-allocate the internal buffer.
     */
    explicit BitWriter(std::size_t reserve_bits = 0);

    /**
     * @brief Returns the number of bits written so far.
     */
    [[nodiscard]] std::size_t bit_length() const noexcept;

    /**
     * @brief Appends an unsigned integer field of the given width.
     *
     * @param value  The value to write.  Bits above @p width are ignored.
     * @param width  Number of bits to write.  Must be in [1, 64].
     * @return       Result<void> indicating success or a width error.
     */
    [[nodiscard]] Result<void> write_uint(uint64_t value, std::size_t width);

    /**
     * @brief Appends a signed integer field in two's-complement form.
     *
     * @param value  The value to encode.
     * @param width  Number of bits to write.  Must be in [1, 64].
     * @return       Result<void> indicating success or a width error.
     */
    [[nodiscard]] Result<void> write_int(int64_t value, std::size_t width);

    /**
     * @brief Appends a single boolean bit.
     *
     * @param value  true writes 1; false writes 0.
     * @return       Result<void> (always succeeds for a single bit write).
     */
    [[nodiscard]] Result<void> write_bool(bool value);

    /**
     * @brief Encodes and appends a 6-bit ASCII text field.
     *
     * The string is padded to @p char_count characters with '@' (0x00 in
     * 6-bit space) if it is shorter than @p char_count.  Characters that
     * cannot be represented in 6-bit ASCII are replaced with '?'.
     *
     * @param text        The string to encode.
     * @param char_count  The fixed field width in characters.
     * @return            Result<void> on success or a character encoding error.
     */
    [[nodiscard]] Result<void> write_text(const std::string& text, std::size_t char_count);

    /**
     * @brief Appends raw bits from a byte vector.
     *
     * Bits are taken from the MSB of src[0] first.  Only the leading @p width
     * bits of @p src are consumed.
     *
     * @param src    Source bytes in left-aligned, MSB-first order.
     * @param width  Number of bits to write.
     * @return       Result<void> on success or a bounds error.
     */
    [[nodiscard]] Result<void> write_bits(const std::vector<uint8_t>& src, std::size_t width);

    /**
     * @brief Returns a read-only reference to the underlying byte buffer.
     *
     * If the total bit length is not a multiple of 8 the final byte is
     * zero-padded on the right.
     */
    [[nodiscard]] const std::vector<uint8_t>& buffer() const noexcept;

private:
    std::vector<uint8_t> buf_;
    std::size_t          bit_length_{ 0 };

    /**
     * @brief Appends a single bit to the stream.
     * @param bit  The bit value (0 or 1).
     */
    void push_bit(uint8_t bit);
};

// -----------------------------------------------------------------------------
// Utility: 6-bit ASCII codec
// -----------------------------------------------------------------------------

/**
 * @brief Maps a 6-bit AIS character value to its printable ASCII equivalent.
 *
 * The mapping follows ITU-R M.1371-5, Table 44.  Values in [0, 31] map to
 * '@' through '_' (offset +64), and values in [32, 63] map directly to ' '
 * through '_'.
 *
 * @param val  A value in [0, 63].
 * @return     The corresponding ASCII character.
 */
[[nodiscard]] char ais6bit_to_ascii(uint8_t val) noexcept;

/**
 * @brief Maps a printable ASCII character to its 6-bit AIS value.
 *
 * Characters that have no valid 6-bit AIS representation return the sentinel
 * value 63, which maps back to '?' (ASCII 63) through ais6bit_to_ascii().
 * Value 63 is chosen because it is printable, is distinct from the padding
 * character '@' (value 0), and unambiguously signals an encoding failure to
 * a caller that inspects decoded text.
 *
 * @param ch  The ASCII character to encode.
 * @return    A value in [0, 63].  Returns 63 for characters outside the
 *            valid 6-bit AIS range.
 */
[[nodiscard]] uint8_t ascii_to_ais6bit(char ch) noexcept;

} // namespace aislib