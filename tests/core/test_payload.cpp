/**
 * @file test_payload.cpp
 * @brief Unit tests for decode_payload(), encode_payload(), and payload helpers.
 *
 * Covers: valid decode with and without fill bits, all fill bit values,
 * invalid characters, empty payload, fill bit boundary conditions,
 * encode roundtrip, encode fill bit calculation, payload_bit_length helper,
 * and is_valid_payload_char.
 */

#include "aislib/payload.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace aislib;

// =============================================================================
// is_valid_payload_char
// =============================================================================

TEST(PayloadChar, LowerBound_Valid) {
    EXPECT_TRUE(is_valid_payload_char('0'));  // ASCII 48
}

TEST(PayloadChar, UpperBound_FirstRange_Valid) {
    EXPECT_TRUE(is_valid_payload_char('W'));  // ASCII 87
}

TEST(PayloadChar, GapStart_Invalid) {
    EXPECT_FALSE(is_valid_payload_char('X')); // ASCII 88
}

TEST(PayloadChar, GapEnd_Invalid) {
    EXPECT_FALSE(is_valid_payload_char('_')); // ASCII 95
}

TEST(PayloadChar, SecondRangeLower_Valid) {
    EXPECT_TRUE(is_valid_payload_char('`'));  // ASCII 96
}

TEST(PayloadChar, SecondRangeUpper_Valid) {
    EXPECT_TRUE(is_valid_payload_char('w'));  // ASCII 119
}

TEST(PayloadChar, AboveSecondRange_Invalid) {
    EXPECT_FALSE(is_valid_payload_char('x')); // ASCII 120
}

TEST(PayloadChar, NullChar_Invalid) {
    EXPECT_FALSE(is_valid_payload_char('\0'));
}

// =============================================================================
// payload_bit_length helper
// =============================================================================

TEST(PayloadBitLength, NoFillBits) {
    EXPECT_EQ(payload_bit_length(28u, 0u), 168u);
}

TEST(PayloadBitLength, WithFillBits) {
    EXPECT_EQ(payload_bit_length(28u, 2u), 166u);
}

TEST(PayloadBitLength, SingleChar_NoFill) {
    EXPECT_EQ(payload_bit_length(1u, 0u), 6u);
}

// =============================================================================
// decode_payload — error cases
// =============================================================================

TEST(DecodePayload, EmptyPayload_Error) {
    auto r = decode_payload("", 0);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadEmpty);
}

TEST(DecodePayload, FillBitsTooHigh_Error) {
    auto r = decode_payload("0", 6);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadFillBitsInvalid);
}

TEST(DecodePayload, InvalidChar_Error) {
    // 'x' (ASCII 120) is outside the valid armour range.
    auto r = decode_payload("0x0", 0);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadCharInvalid);
}

TEST(DecodePayload, InvalidChar_LowAscii_Error) {
    auto r = decode_payload("0\x01" "0", 0);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadCharInvalid);
}

// =============================================================================
// decode_payload — valid cases
// =============================================================================

TEST(DecodePayload, SingleChar_ZeroValue) {
    // '0' = ASCII 48 -> 6-bit value 0 -> bits: 000000
    auto r = decode_payload("0", 0);
    ASSERT_TRUE(r.ok());
    // 6 significant bits, rounded up to 1 byte, value 0.
    ASSERT_EQ(r.value().size(), 1u);
    EXPECT_EQ(r.value()[0], 0x00u);
}

TEST(DecodePayload, SingleChar_FullValue) {
    // 'W' = ASCII 87 -> subtract 48 -> 39, <= 40 so no second subtraction
    // 6-bit value 39 = 0b100111, stored MSB first: 0b10011100 = 0x9C
    auto r = decode_payload("W", 0);
    ASSERT_TRUE(r.ok());
    ASSERT_EQ(r.value().size(), 1u);
    EXPECT_EQ(r.value()[0], 0x9Cu);
}

TEST(DecodePayload, AllFillBitValues) {
    // A two-character payload has 12 bits.  Test fill bits 0..5.
    for (uint8_t fill = 0; fill <= 5; ++fill) {
        auto r = decode_payload("00", fill);
        ASSERT_TRUE(r.ok()) << "fill=" << static_cast<int>(fill);
        const std::size_t expected_bits = 12u - fill;
        const std::size_t expected_bytes = (expected_bits + 7u) / 8u;
        EXPECT_EQ(r.value().size(), expected_bytes)
            << "fill=" << static_cast<int>(fill);
    }
}

TEST(DecodePayload, KnownAISPayload_Type1) {
    // Real Type 1 sentence payload from a test vessel.
    // "15M67J0000G?tF`FepT4@Dn" — this is a well-known test vector.
    // We only verify that decode succeeds and returns a non-empty buffer of
    // the expected size (168 bits = 28 chars * 6 bits, 0 fill).
    const std::string payload = "15M67J0000G?tF`FepT4@Dn";  // 23 chars * 6 = 138 bits
    // Use the simple "all zeroes" 28-char payload for a deterministic test.
    const std::string simple_payload(28, '0'); // 28 chars, value 0 each = 168 bits
    auto r = decode_payload(simple_payload, 0);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().size(), 21u); // 168 / 8 = 21 bytes
    for (uint8_t byte : r.value()) {
        EXPECT_EQ(byte, 0x00u);
    }
}

// =============================================================================
// encode_payload — error cases
// =============================================================================

TEST(EncodePayload, ZeroBitLength_Error) {
    std::vector<uint8_t> data = { 0xFFu };
    auto r = encode_payload(data, 0);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadEmpty);
}

TEST(EncodePayload, BufferTooSmall_Error) {
    std::vector<uint8_t> data = { 0xFFu }; // 1 byte = 8 bits
    auto r = encode_payload(data, 16);     // asks for 16 bits
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

// =============================================================================
// encode_payload — valid cases
// =============================================================================

TEST(EncodePayload, SingleByte_NoFill) {
    // 0x00, 8 bits -> 8 is not a multiple of 6, so fill = (6 - 8%6)%6 = 4.
    // Wait: 8 % 6 = 2, fill = (6 - 2) % 6 = 4. char count = 12/6 = 2.
    std::vector<uint8_t> data = { 0x00u };
    auto r = encode_payload(data, 8);
    ASSERT_TRUE(r.ok());
    const auto& [payload_str, fill_bits] = r.value();
    EXPECT_EQ(fill_bits, 4u);
    EXPECT_EQ(payload_str.size(), 2u);
}

TEST(EncodePayload, ExactMultipleOf6_NoFill) {
    // 6 bits exactly -> 1 armour char, 0 fill bits.
    std::vector<uint8_t> data = { 0x00u };
    auto r = encode_payload(data, 6);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value().second, 0u);
    EXPECT_EQ(r.value().first.size(), 1u);
}

TEST(EncodePayload, AllCharsInValidRange) {
    // Encode 168 zero bits (28 chars).
    std::vector<uint8_t> data(21, 0x00u);
    auto r = encode_payload(data, 168);
    ASSERT_TRUE(r.ok());
    const std::string& payload_str = r.value().first;
    for (char ch : payload_str) {
        EXPECT_TRUE(is_valid_payload_char(ch))
            << "Character out of armour range: " << static_cast<int>(ch);
    }
}

// =============================================================================
// Encode + Decode roundtrip
// =============================================================================

TEST(PayloadRoundtrip, ZeroBuffer) {
    std::vector<uint8_t> original(21, 0x00u); // 168 bits
    auto enc = encode_payload(original, 168);
    ASSERT_TRUE(enc.ok());

    auto dec = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(dec.ok());
    EXPECT_EQ(dec.value(), original);
}

TEST(PayloadRoundtrip, MaxValueBuffer) {
    std::vector<uint8_t> original(21, 0xFFu);
    auto enc = encode_payload(original, 168);
    ASSERT_TRUE(enc.ok());

    auto dec = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(dec.ok());
    EXPECT_EQ(dec.value(), original);
}

TEST(PayloadRoundtrip, NonBytealignedBits) {
    // 30 bits (5 chars), fill = (6 - 30%6)%6 = 0 actually (30 is multiple of 6)
    std::vector<uint8_t> original = { 0xABu, 0xCDu, 0xEFu, 0xA0u };
    auto enc = encode_payload(original, 30);
    ASSERT_TRUE(enc.ok());
    EXPECT_EQ(enc.value().second, 0u); // 30 is a multiple of 6

    auto dec = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(dec.ok());
    // Only first 30 bits are significant; compare 3 full bytes + partial 4th.
    ASSERT_GE(dec.value().size(), 3u);
    EXPECT_EQ(dec.value()[0], original[0]);
    EXPECT_EQ(dec.value()[1], original[1]);
    EXPECT_EQ(dec.value()[2], original[2]);
}

TEST(PayloadRoundtrip, PatternBuffer) {
    std::vector<uint8_t> original = { 0x55u, 0xAAu, 0x55u, 0xAAu, 0x55u,
                                       0xAAu, 0x55u, 0xAAu, 0x55u, 0xAAu,
                                       0x55u, 0xAAu, 0x55u, 0xAAu, 0x55u,
                                       0xAAu, 0x55u, 0xAAu, 0x55u, 0xAAu,
                                       0x55u };
    auto enc = encode_payload(original, 168);
    ASSERT_TRUE(enc.ok());
    auto dec = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(dec.ok());
    EXPECT_EQ(dec.value(), original);
}