/**
 * @file test_bitfield.cpp
 * @brief Unit tests for BitReader, BitWriter, and the 6-bit ASCII codec.
 *
 * Covers: unsigned and signed reads, boolean reads, text reads, raw bit reads,
 * out-of-bounds and invalid-width errors, BitWriter field writes, roundtrip
 * encode/decode via BitWriter + BitReader, and the ais6bit/ASCII codec.
 */

#include "aislib/bitfield.h"

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstdint>

using namespace aislib;

// =============================================================================
// Helpers
// =============================================================================

/**
 * @brief Builds a byte buffer from an explicit list of uint8_t values.
 */
static std::vector<uint8_t> make_buf(std::initializer_list<uint8_t> bytes) {
    return std::vector<uint8_t>(bytes);
}

// =============================================================================
// BitReader — basic reads
// =============================================================================

TEST(BitReader, BitLength) {
    auto buf = make_buf({ 0xFF, 0x00 });
    BitReader r(buf);
    EXPECT_EQ(r.bit_length(), 16u);
}

TEST(BitReader, ReadUint_SingleByte_FullWidth) {
    // 0xA5 = 1010 0101
    auto buf = make_buf({ 0xA5u });
    BitReader r(buf);
    auto result = r.read_uint(0, 8);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), 0xA5u);
}

TEST(BitReader, ReadUint_LowerNibble) {
    // 0xA5 = 1010 0101, lower 4 bits = 0101 = 5
    auto buf = make_buf({ 0xA5u });
    BitReader r(buf);
    auto result = r.read_uint(4, 4);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), 0x05u);
}

TEST(BitReader, ReadUint_CrossByteBoundary) {
    // buf = 0b00001111 0b11110000
    // bits 4..11 should read as 0b11111111 = 255
    auto buf = make_buf({ 0x0Fu, 0xF0u });
    BitReader r(buf);
    auto result = r.read_uint(4, 8);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), 0xFFu);
}

TEST(BitReader, ReadUint_64Bits) {
    // Eight bytes, all 0xFF.
    std::vector<uint8_t> buf(8, 0xFFu);
    BitReader r(buf);
    auto result = r.read_uint(0, 64);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), UINT64_MAX);
}

TEST(BitReader, ReadUint_OutOfRange) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_uint(4, 8); // bits 4..11, but only 8 bits available
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code(), ErrorCode::BitfieldOutOfRange);
}

TEST(BitReader, ReadUint_WidthZero) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_uint(0, 0);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code(), ErrorCode::BitfieldWidthInvalid);
}

TEST(BitReader, ReadUint_WidthOver64) {
    std::vector<uint8_t> buf(9, 0xFFu);
    BitReader r(buf);
    auto result = r.read_uint(0, 65);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code(), ErrorCode::BitfieldWidthInvalid);
}

// =============================================================================
// BitReader — signed reads
// =============================================================================

TEST(BitReader, ReadInt_Positive) {
    // 0x0C = 0000 1100, bits [0,7] as signed = 12
    auto buf = make_buf({ 0x0Cu });
    BitReader r(buf);
    auto result = r.read_int(0, 8);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), 12);
}

TEST(BitReader, ReadInt_Negative) {
    // 0xFF as 8-bit signed = -1
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_int(0, 8);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), -1);
}

TEST(BitReader, ReadInt_Negative4Bit) {
    // 0xF0: upper nibble = 0b1111 = -1 as 4-bit signed
    auto buf = make_buf({ 0xF0u });
    BitReader r(buf);
    auto result = r.read_int(0, 4);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), -1);
}

TEST(BitReader, ReadInt_SmallNegative) {
    // 0x80 upper nibble: 0b1000 = -8 as 4-bit signed
    auto buf = make_buf({ 0x80u });
    BitReader r(buf);
    auto result = r.read_int(0, 4);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), -8);
}

TEST(BitReader, ReadInt_OutOfRange) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_int(4, 8);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code(), ErrorCode::BitfieldOutOfRange);
}

// =============================================================================
// BitReader — boolean reads
// =============================================================================

TEST(BitReader, ReadBool_One) {
    auto buf = make_buf({ 0x80u }); // MSB = 1
    BitReader r(buf);
    auto result = r.read_bool(0);
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value());
}

TEST(BitReader, ReadBool_Zero) {
    auto buf = make_buf({ 0x40u }); // bit 0 = 0, bit 1 = 1
    BitReader r(buf);
    auto result = r.read_bool(0);
    ASSERT_TRUE(result.ok());
    EXPECT_FALSE(result.value());
}

TEST(BitReader, ReadBool_OutOfRange) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_bool(8);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code(), ErrorCode::BitfieldOutOfRange);
}

// =============================================================================
// BitReader — text reads
// =============================================================================

TEST(BitReader, ReadText_AtSign_IsPadding) {
    // A single 6-bit value of 0 encodes '@', which is treated as padding and stripped.
    // Build a buffer with 6-bit value 0 (byte = 0x00, padded to 8 bits).
    auto buf = make_buf({ 0x00u });
    BitReader r(buf);
    auto result = r.read_text(0, 1);
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().empty()) << "Trailing '@' should be stripped";
}

TEST(BitReader, ReadText_SingleChar_A) {
    // 'A' in 6-bit AIS is value 1 (ASCII 65 - 64 = 1).
    // Value 1 = 0b000001, padded to byte: 0b00000100 = 0x04.
    auto buf = make_buf({ 0x04u });
    BitReader r(buf);
    auto result = r.read_text(0, 1);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), "A");
}

TEST(BitReader, ReadText_EmptyCharCount) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_text(0, 0);
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().empty());
}

TEST(BitReader, ReadText_OutOfRange) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    // 3 characters = 18 bits, but buffer only has 8.
    auto result = r.read_text(0, 3);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code(), ErrorCode::BitfieldOutOfRange);
}

// =============================================================================
// BitReader — raw bit reads
// =============================================================================

TEST(BitReader, ReadBits_FullByte) {
    auto buf = make_buf({ 0xA5u });
    BitReader r(buf);
    auto result = r.read_bits(0, 8);
    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value().size(), 1u);
    EXPECT_EQ(result.value()[0], 0xA5u);
}

TEST(BitReader, ReadBits_SubByte_Padded) {
    // buf = 0xFF; read 4 bits starting at 0 -> 0b11110000 = 0xF0
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_bits(0, 4);
    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value().size(), 1u);
    EXPECT_EQ(result.value()[0], 0xF0u);
}

TEST(BitReader, ReadBits_ZeroWidth) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_bits(0, 0);
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().empty());
}

TEST(BitReader, ReadBits_OutOfRange) {
    auto buf = make_buf({ 0xFFu });
    BitReader r(buf);
    auto result = r.read_bits(4, 8);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code(), ErrorCode::BitfieldOutOfRange);
}

// =============================================================================
// BitWriter — basic writes
// =============================================================================

TEST(BitWriter, WriteUint_SingleByte) {
    BitWriter w;
    ASSERT_TRUE(w.write_uint(0xA5u, 8).ok());
    EXPECT_EQ(w.bit_length(), 8u);
    ASSERT_EQ(w.buffer().size(), 1u);
    EXPECT_EQ(w.buffer()[0], 0xA5u);
}

TEST(BitWriter, WriteUint_Nibble) {
    BitWriter w;
    ASSERT_TRUE(w.write_uint(0x0Fu, 4).ok());
    EXPECT_EQ(w.bit_length(), 4u);
    // 4 bits = 0b11110000 in the first byte (MSB first, zero padded).
    EXPECT_EQ(w.buffer()[0], 0xF0u);
}

TEST(BitWriter, WriteUint_WidthZero_Error) {
    BitWriter w;
    auto r = w.write_uint(1u, 0);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::BitfieldWidthInvalid);
}

TEST(BitWriter, WriteUint_WidthOver64_Error) {
    BitWriter w;
    auto r = w.write_uint(1u, 65);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::BitfieldWidthInvalid);
}

TEST(BitWriter, WriteInt_Negative) {
    BitWriter w;
    ASSERT_TRUE(w.write_int(-1, 8).ok());
    EXPECT_EQ(w.buffer()[0], 0xFFu);
}

TEST(BitWriter, WriteInt_NegativeSmall) {
    BitWriter w;
    // -1 in 4 bits = 0b1111, stored in upper nibble: 0xF0
    ASSERT_TRUE(w.write_int(-1, 4).ok());
    EXPECT_EQ(w.buffer()[0], 0xF0u);
}

TEST(BitWriter, WriteBool_True) {
    BitWriter w;
    ASSERT_TRUE(w.write_bool(true).ok());
    EXPECT_EQ(w.bit_length(), 1u);
    EXPECT_EQ(w.buffer()[0], 0x80u);
}

TEST(BitWriter, WriteBool_False) {
    BitWriter w;
    ASSERT_TRUE(w.write_bool(false).ok());
    EXPECT_EQ(w.buffer()[0], 0x00u);
}

TEST(BitWriter, WriteText_SingleChar) {
    BitWriter w;
    ASSERT_TRUE(w.write_text("A", 1).ok());
    EXPECT_EQ(w.bit_length(), 6u);
    // 'A' -> 6-bit value 1 -> 0b000001, stored MSB-first: 0b00000100 = 0x04
    EXPECT_EQ(w.buffer()[0], 0x04u);
}

TEST(BitWriter, WriteText_Padded) {
    BitWriter w;
    // Write "A" into a 2-char field: 'A' (value 1) + '@' (value 0)
    ASSERT_TRUE(w.write_text("A", 2).ok());
    EXPECT_EQ(w.bit_length(), 12u);
    // 0b000001 000000 = 0b00000100 0000xxxx -> bytes: 0x04, 0x00
    EXPECT_EQ(w.buffer()[0], 0x04u);
    EXPECT_EQ((w.buffer()[1] & 0xF0u), 0x00u);
}

TEST(BitWriter, WriteBits_FullByte) {
    BitWriter w;
    std::vector<uint8_t> src = { 0xA5u };
    ASSERT_TRUE(w.write_bits(src, 8).ok());
    EXPECT_EQ(w.buffer()[0], 0xA5u);
}

TEST(BitWriter, WriteBits_Partial) {
    BitWriter w;
    std::vector<uint8_t> src = { 0xF0u }; // 0b11110000
    ASSERT_TRUE(w.write_bits(src, 4).ok()); // write 4 MSBs: 0b1111
    EXPECT_EQ(w.bit_length(), 4u);
    EXPECT_EQ(w.buffer()[0], 0xF0u); // stored as 0b11110000 (MSB first, zero padded)
}

TEST(BitWriter, WriteBits_BufferTooSmall) {
    BitWriter w;
    std::vector<uint8_t> src = { 0xFFu }; // only 8 bits
    auto r = w.write_bits(src, 16);       // requests 16 bits
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::BitfieldOutOfRange);
}

// =============================================================================
// BitWriter + BitReader roundtrip
// =============================================================================

TEST(BitfieldRoundtrip, UnsignedValue) {
    BitWriter w;
    ASSERT_TRUE(w.write_uint(0x1234u, 13).ok()); // 13-bit value
    BitReader r(w.buffer());
    auto result = r.read_uint(0, 13);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), 0x1234u & 0x1FFFu); // masked to 13 bits
}

TEST(BitfieldRoundtrip, SignedNegative) {
    BitWriter w;
    ASSERT_TRUE(w.write_int(-42, 12).ok());
    BitReader r(w.buffer());
    auto result = r.read_int(0, 12);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), -42);
}

TEST(BitfieldRoundtrip, MultipleFields) {
    BitWriter w;
    ASSERT_TRUE(w.write_uint(5u, 6).ok());   // bits  0- 5
    ASSERT_TRUE(w.write_uint(127u, 7).ok()); // bits  6-12
    ASSERT_TRUE(w.write_int(-3, 5).ok());    // bits 13-17

    BitReader r(w.buffer());
    EXPECT_EQ(r.read_uint(0, 6).value(),  5u);
    EXPECT_EQ(r.read_uint(6, 7).value(),  127u);
    EXPECT_EQ(r.read_int(13, 5).value(),  -3);
}

TEST(BitfieldRoundtrip, Text) {
    const std::string original = "HELLO";
    BitWriter w;
    ASSERT_TRUE(w.write_text(original, 5).ok());
    BitReader r(w.buffer());
    auto result = r.read_text(0, 5);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value(), original);
}

// =============================================================================
// 6-bit ASCII codec
// =============================================================================

TEST(AIS6BitCodec, ValueZero_IsAtSign) {
    EXPECT_EQ(ais6bit_to_ascii(0u), '@');
}

TEST(AIS6BitCodec, ValueOne_IsA) {
    EXPECT_EQ(ais6bit_to_ascii(1u), 'A');
}

TEST(AIS6BitCodec, Value31_IsUnderscore) {
    EXPECT_EQ(ais6bit_to_ascii(31u), '_');
}

TEST(AIS6BitCodec, Value32_IsSpace) {
    EXPECT_EQ(ais6bit_to_ascii(32u), ' ');
}

TEST(AIS6BitCodec, Value63_IsQuestionMark) {
    // Value 63 is >= 32, so the mapping rule returns char(63) directly.
    // ASCII 63 is '?', which is also the sentinel returned by ascii_to_ais6bit
    // for characters outside the valid encoding range.
    EXPECT_EQ(ais6bit_to_ascii(63u), '?');
}

TEST(AIS6BitCodec, RoundtripAllValidValues) {
    for (uint8_t v = 0u; v < 64u; ++v) {
        const char ch      = ais6bit_to_ascii(v);
        const uint8_t back = ascii_to_ais6bit(ch);
        EXPECT_EQ(back, v) << "Roundtrip failed for value " << static_cast<int>(v);
    }
}

TEST(AIS6BitCodec, AtSign_ToValue_Zero) {
    EXPECT_EQ(ascii_to_ais6bit('@'), 0u);
}

TEST(AIS6BitCodec, Space_ToValue_32) {
    EXPECT_EQ(ascii_to_ais6bit(' '), 32u);
}

TEST(AIS6BitCodec, InvalidChar_Returns63) {
    // Characters outside the valid range return the sentinel value 63, which
    // maps back to '?' (ASCII 63) through ais6bit_to_ascii().
    EXPECT_EQ(ascii_to_ais6bit('\x01'), 63u);
    EXPECT_EQ(ascii_to_ais6bit('\xFF'), 63u);
}

TEST(AIS6BitCodec, HighBitsMasked) {
    // Values > 63 should be masked to 6 bits by ais6bit_to_ascii.
    // Value 64 = 0b01000000; masked to 0 -> '@'.
    EXPECT_EQ(ais6bit_to_ascii(64u), '@');
}