/**
 * @file test_msg10.cpp
 * @brief Unit tests for AIS message type 10 (UTC/date inquiry).
 *
 * Tests cover:
 *   - Decoding a constructed type 10 payload and verifying all field values.
 *   - Destination MMSI field: zero value, maximum value, typical value.
 *   - Repeat indicator variants.
 *   - Encode/decode round-trip for all fields.
 *   - Error paths: payload too short, wrong message type.
 *   - Registry dispatch for type ID 10.
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg10.h"
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg10Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helper: build a type 10 payload buffer
// ---------------------------------------------------------------------------

/**
 * @brief Constructs a 72-bit type 10 payload buffer.
 *
 * Spare bits are written as zero by this helper.
 */
static BitWriter build_type10(
    uint32_t mmsi,
    uint8_t  repeat_indicator,
    uint32_t destination_mmsi
)
{
    BitWriter w(72u);
    EXPECT_TRUE(w.write_uint(10u, 6u).ok());
    EXPECT_TRUE(w.write_uint(repeat_indicator, 2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi, 30u).ok());
    EXPECT_TRUE(w.write_uint(0u, 2u).ok());         // spare
    EXPECT_TRUE(w.write_uint(destination_mmsi, 30u).ok());
    EXPECT_TRUE(w.write_uint(0u, 2u).ok());         // spare
    return w;
}

// ---------------------------------------------------------------------------
// Basic decode — field values
// ---------------------------------------------------------------------------

TEST_F(Msg10Test, Decode_TypicalMessage_CorrectValues)
{
    auto w = build_type10(123456789u, 0u, 987654321u);

    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();

    auto* msg = dynamic_cast<Msg10UTCDateInquiry*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    EXPECT_EQ(msg->mmsi(), 123456789u);
    EXPECT_EQ(msg->repeat_indicator(), 0u);
    EXPECT_EQ(msg->destination_mmsi(), 987654321u);
    EXPECT_EQ(msg->type(), MessageType::UTCDateInquiry);
}

TEST_F(Msg10Test, Decode_DestinationMmsiZero)
{
    auto w = build_type10(111111111u, 0u, 0u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg10UTCDateInquiry*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->destination_mmsi(), 0u);
}

TEST_F(Msg10Test, Decode_DestinationMmsiMaxValue)
{
    // Maximum 30-bit MMSI: 2^30 - 1 = 1073741823
    auto w = build_type10(222222222u, 0u, 1073741823u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg10UTCDateInquiry*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->destination_mmsi(), 1073741823u);
}

// ---------------------------------------------------------------------------
// Repeat indicator variants
// ---------------------------------------------------------------------------

TEST_F(Msg10Test, Decode_RepeatIndicator1)
{
    auto w = build_type10(333333333u, 1u, 444444444u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg10UTCDateInquiry*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->repeat_indicator(), 1u);
}

TEST_F(Msg10Test, Decode_RepeatIndicator3)
{
    auto w = build_type10(555555555u, 3u, 666666666u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg10UTCDateInquiry*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->repeat_indicator(), 3u);
    EXPECT_EQ(msg->destination_mmsi(), 666666666u);
}

// ---------------------------------------------------------------------------
// MMSI distinct from destination MMSI
// ---------------------------------------------------------------------------

TEST_F(Msg10Test, Decode_SourceAndDestinationMmsiDistinct)
{
    auto w = build_type10(111000111u, 0u, 222000222u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg10UTCDateInquiry*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_NE(msg->mmsi(), msg->destination_mmsi());
    EXPECT_EQ(msg->mmsi(), 111000111u);
    EXPECT_EQ(msg->destination_mmsi(), 222000222u);
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg10Test, RoundTrip_TypicalValues)
{
    auto w = build_type10(123456789u, 2u, 987654321u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();
    EXPECT_EQ(enc.value().second, 0u);  // 72 bits, no fill

    auto buf = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf.ok());

    BitReader reader2(buf.value());
    auto r2 = Msg10UTCDateInquiry::decode(reader2);
    ASSERT_TRUE(r2.ok());

    auto* m1 = dynamic_cast<Msg10UTCDateInquiry*>(r.value().get());
    auto* m2 = dynamic_cast<Msg10UTCDateInquiry*>(r2.value().get());
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);

    EXPECT_EQ(m2->mmsi(), m1->mmsi());
    EXPECT_EQ(m2->repeat_indicator(), m1->repeat_indicator());
    EXPECT_EQ(m2->destination_mmsi(), m1->destination_mmsi());
}

TEST_F(Msg10Test, RoundTrip_ZeroDestination)
{
    auto w = build_type10(100000001u, 0u, 0u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok());

    auto buf = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf.ok());

    BitReader reader2(buf.value());
    auto r2 = Msg10UTCDateInquiry::decode(reader2);
    ASSERT_TRUE(r2.ok());

    auto* m2 = dynamic_cast<Msg10UTCDateInquiry*>(r2.value().get());
    ASSERT_NE(m2, nullptr);
    EXPECT_EQ(m2->destination_mmsi(), 0u);
}

TEST_F(Msg10Test, RoundTrip_MaxMmsiValues)
{
    auto w = build_type10(1073741823u, 3u, 1073741823u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok());

    auto buf = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf.ok());

    BitReader reader2(buf.value());
    auto r2 = Msg10UTCDateInquiry::decode(reader2);
    ASSERT_TRUE(r2.ok());

    auto* m2 = dynamic_cast<Msg10UTCDateInquiry*>(r2.value().get());
    ASSERT_NE(m2, nullptr);
    EXPECT_EQ(m2->mmsi(), 1073741823u);
    EXPECT_EQ(m2->destination_mmsi(), 1073741823u);
}

// ---------------------------------------------------------------------------
// Encode produces correct payload length
// ---------------------------------------------------------------------------

TEST_F(Msg10Test, Encode_PayloadLength_12Chars_0Fill)
{
    auto w = build_type10(123456789u, 0u, 987654321u);
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok());

    // 72 bits / 6 bits per char = 12 armoured characters, 0 fill bits
    EXPECT_EQ(enc.value().first.size(), 12u);
    EXPECT_EQ(enc.value().second, 0u);
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg10Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    BitWriter w;
    ASSERT_TRUE(w.write_uint(10u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg10Test, DecodeWrongType_ReturnsDecodeFailure)
{
    BitWriter w(72u);
    ASSERT_TRUE(w.write_uint(4u, 6u).ok());  // type 4, not 10
    for (int i = 0; i < 66; ++i) ASSERT_TRUE(w.write_uint(0u, 1u).ok());
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

TEST_F(Msg10Test, DecodeWrongType_Type11_ReturnsDecodeFailure)
{
    // Type 11 and type 10 share the same bit length but different type IDs.
    BitWriter w(72u);
    ASSERT_TRUE(w.write_uint(11u, 6u).ok());
    for (int i = 0; i < 66; ++i) ASSERT_TRUE(w.write_uint(0u, 1u).ok());
    BitReader reader(w.buffer());
    auto r = Msg10UTCDateInquiry::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg10Test, Registry_DispatchesType10)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(10u));
}