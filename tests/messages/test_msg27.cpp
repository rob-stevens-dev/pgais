/**
 * @file test_msg27.cpp
 * @brief Unit tests for AIS message type 27 (Long-range AIS broadcast message).
 *
 * Tests cover:
 *   - Decoding a constructed type 27 payload and verifying all field values.
 *   - Coordinate helper methods (longitude(), latitude()).
 *   - Type 27 coordinate resolution (1/10 degree, not 1/10 000 minute).
 *   - Unavailability sentinels for longitude, latitude, SOG, and COG.
 *   - Navigation status field.
 *   - Boolean flag fields (position accuracy, RAIM, GNSS position status).
 *   - SOG and COG range boundary values.
 *   - Encode/decode round-trip for all fields.
 *   - Error paths: payload too short, wrong message type.
 *   - Registry dispatch for type ID 27.
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg27.h"
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg27Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helper: build a type 27 payload buffer
// ---------------------------------------------------------------------------

/**
 * @brief Constructs a 96-bit type 27 payload buffer.
 *
 * Coordinates are in 1/10 degree units (not 1/10 000 minute).
 * Spare bits (93–95) are written as zero by this helper.
 */
static BitWriter build_type27(
    uint32_t mmsi,
    uint8_t  repeat_indicator,
    bool     position_accuracy,
    bool     raim,
    uint8_t  nav_status,
    int32_t  longitude,            // 1/10 degree, 18-bit signed
    int32_t  latitude,             // 1/10 degree, 17-bit signed
    uint8_t  sog,                  // whole knots, 6 bits
    uint16_t cog,                  // whole degrees, 9 bits
    bool     gnss_position_status
)
{
    BitWriter w(96u);
    EXPECT_TRUE(w.write_uint(27u, 6u).ok());
    EXPECT_TRUE(w.write_uint(repeat_indicator, 2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi, 30u).ok());
    EXPECT_TRUE(w.write_bool(position_accuracy).ok());
    EXPECT_TRUE(w.write_bool(raim).ok());
    EXPECT_TRUE(w.write_uint(nav_status, 4u).ok());
    EXPECT_TRUE(w.write_int(longitude, 18u).ok());
    EXPECT_TRUE(w.write_int(latitude, 17u).ok());
    EXPECT_TRUE(w.write_uint(sog, 6u).ok());
    EXPECT_TRUE(w.write_uint(cog, 9u).ok());
    EXPECT_TRUE(w.write_bool(gnss_position_status).ok());
    EXPECT_TRUE(w.write_uint(0u, 1u).ok());   // spare
    return w;
}

// ---------------------------------------------------------------------------
// Basic decode — field values
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, Decode_AllFields_CorrectValues)
{
    // Longitude: 12.5 degrees => 125 raw (1/10 degree)
    // Latitude:  55.3 degrees => 553 raw
    auto w = build_type27(
        /*mmsi*/            219024194u,
        /*ri*/              0u,
        /*pa*/              true,
        /*raim*/            false,
        /*nav_status*/      0u,    // under way using engine
        /*longitude*/       125,   // 12.5 degrees
        /*latitude*/        553,   // 55.3 degrees
        /*sog*/             5u,    // 5 knots
        /*cog*/             270u,  // 270 degrees (west)
        /*gnss*/            false
    );

    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();

    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    EXPECT_EQ(msg->mmsi(), 219024194u);
    EXPECT_EQ(msg->repeat_indicator(), 0u);
    EXPECT_TRUE(msg->position_accuracy());
    EXPECT_FALSE(msg->raim());
    EXPECT_EQ(msg->nav_status(), 0u);
    EXPECT_EQ(msg->longitude_raw(), 125);
    EXPECT_EQ(msg->latitude_raw(), 553);
    EXPECT_EQ(msg->sog(), 5u);
    EXPECT_EQ(msg->cog(), 270u);
    EXPECT_FALSE(msg->gnss_position_status());
    EXPECT_EQ(msg->type(), MessageType::PositionReportLongRange);
}

// ---------------------------------------------------------------------------
// Coordinate helpers — 1/10 degree resolution
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, Longitude_PositiveValue_CorrectDegrees)
{
    // 1 raw unit = 0.1 degrees
    auto w = build_type27(100000001u, 0u, false, false, 0u,
        10, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lon = msg->longitude();
    ASSERT_TRUE(lon.has_value());
    EXPECT_DOUBLE_EQ(lon.value(), 1.0);
}

TEST_F(Msg27Test, Longitude_NegativeValue_CorrectDegrees)
{
    // -10 raw = -1.0 degree
    auto w = build_type27(100000002u, 0u, false, false, 0u,
        -10, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lon = msg->longitude();
    ASSERT_TRUE(lon.has_value());
    EXPECT_DOUBLE_EQ(lon.value(), -1.0);
}

TEST_F(Msg27Test, Longitude_Unavailable_NullOpt)
{
    auto w = build_type27(100000003u, 0u, false, false, 0u,
        kMsg27LongitudeUnavailable, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->longitude().has_value());
    EXPECT_EQ(msg->longitude_raw(), kMsg27LongitudeUnavailable);
}

TEST_F(Msg27Test, Latitude_PositiveValue_CorrectDegrees)
{
    auto w = build_type27(200000001u, 0u, false, false, 0u,
        0, 10, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lat = msg->latitude();
    ASSERT_TRUE(lat.has_value());
    EXPECT_DOUBLE_EQ(lat.value(), 1.0);
}

TEST_F(Msg27Test, Latitude_NegativeValue_CorrectDegrees)
{
    auto w = build_type27(200000002u, 0u, false, false, 0u,
        0, -10, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lat = msg->latitude();
    ASSERT_TRUE(lat.has_value());
    EXPECT_DOUBLE_EQ(lat.value(), -1.0);
}

TEST_F(Msg27Test, Latitude_Unavailable_NullOpt)
{
    auto w = build_type27(200000003u, 0u, false, false, 0u,
        0, kMsg27LatitudeUnavailable, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->latitude().has_value());
    EXPECT_EQ(msg->latitude_raw(), kMsg27LatitudeUnavailable);
}

TEST_F(Msg27Test, Longitude_MaxValid_1799)
{
    // 1799 raw = 179.9 degrees (just under 180)
    auto w = build_type27(300000001u, 0u, false, false, 0u,
        1799, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lon = msg->longitude();
    ASSERT_TRUE(lon.has_value());
    EXPECT_DOUBLE_EQ(lon.value(), 179.9);
}

TEST_F(Msg27Test, Latitude_MaxValid_899)
{
    // 899 raw = 89.9 degrees
    auto w = build_type27(300000002u, 0u, false, false, 0u,
        0, 899, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lat = msg->latitude();
    ASSERT_TRUE(lat.has_value());
    EXPECT_DOUBLE_EQ(lat.value(), 89.9);
}

// ---------------------------------------------------------------------------
// SOG sentinel and boundary values
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, SogUnavailable_Value63)
{
    auto w = build_type27(400000001u, 0u, false, false, 0u,
        0, 0, kMsg27SogUnavailable, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sog(), kMsg27SogUnavailable);
}

TEST_F(Msg27Test, SogMaxValid_62)
{
    auto w = build_type27(400000002u, 0u, false, false, 0u,
        0, 0, 62u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sog(), 62u);
}

TEST_F(Msg27Test, SogZero)
{
    auto w = build_type27(400000003u, 0u, false, false, 0u,
        0, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sog(), 0u);
}

// ---------------------------------------------------------------------------
// COG sentinel and boundary values
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, CogUnavailable_Value511)
{
    auto w = build_type27(500000001u, 0u, false, false, 0u,
        0, 0, 0u, kMsg27CogUnavailable, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->cog(), kMsg27CogUnavailable);
}

TEST_F(Msg27Test, CogMaxValid_359)
{
    auto w = build_type27(500000002u, 0u, false, false, 0u,
        0, 0, 0u, 359u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->cog(), 359u);
}

TEST_F(Msg27Test, CogZero)
{
    auto w = build_type27(500000003u, 0u, false, false, 0u,
        0, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->cog(), 0u);
}

// ---------------------------------------------------------------------------
// Navigation status
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, NavStatus_AtAnchor_Value1)
{
    auto w = build_type27(600000001u, 0u, false, false, 1u,
        0, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->nav_status(), 1u);
}

TEST_F(Msg27Test, NavStatus_NotDefined_Value15)
{
    auto w = build_type27(600000002u, 0u, false, false, 15u,
        0, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->nav_status(), 15u);
}

// ---------------------------------------------------------------------------
// Boolean flags
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, Flags_AllTrue)
{
    auto w = build_type27(700000001u, 3u, true, true, 0u,
        0, 0, 0u, 0u, true);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->position_accuracy());
    EXPECT_TRUE(msg->raim());
    EXPECT_TRUE(msg->gnss_position_status());
    EXPECT_EQ(msg->repeat_indicator(), 3u);
}

TEST_F(Msg27Test, Flags_AllFalse)
{
    auto w = build_type27(700000002u, 0u, false, false, 0u,
        0, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->position_accuracy());
    EXPECT_FALSE(msg->raim());
    EXPECT_FALSE(msg->gnss_position_status());
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, RoundTrip_AllFields)
{
    auto w = build_type27(
        219024194u, 1u, true, true, 3u,
        125,     // 12.5 degrees
        553,     // 55.3 degrees
        14u,     // 14 knots
        355u,    // 355 degrees
        false
    );

    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();
    EXPECT_EQ(enc.value().second, 0u);  // 96 bits, no fill

    auto buf = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf.ok());

    BitReader reader2(buf.value());
    auto r2 = Msg27LongRangeBroadcast::decode(reader2);
    ASSERT_TRUE(r2.ok());

    auto* m1 = dynamic_cast<Msg27LongRangeBroadcast*>(r.value().get());
    auto* m2 = dynamic_cast<Msg27LongRangeBroadcast*>(r2.value().get());
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);

    EXPECT_EQ(m2->mmsi(), m1->mmsi());
    EXPECT_EQ(m2->repeat_indicator(), m1->repeat_indicator());
    EXPECT_EQ(m2->position_accuracy(), m1->position_accuracy());
    EXPECT_EQ(m2->raim(), m1->raim());
    EXPECT_EQ(m2->nav_status(), m1->nav_status());
    EXPECT_EQ(m2->longitude_raw(), m1->longitude_raw());
    EXPECT_EQ(m2->latitude_raw(), m1->latitude_raw());
    EXPECT_EQ(m2->sog(), m1->sog());
    EXPECT_EQ(m2->cog(), m1->cog());
    EXPECT_EQ(m2->gnss_position_status(), m1->gnss_position_status());
}

TEST_F(Msg27Test, RoundTrip_NegativeCoordinates)
{
    auto w = build_type27(111222333u, 0u, false, false, 5u,
        -745,    // -74.5 degrees (approximate New York longitude)
        408,     //  40.8 degrees (approximate New York latitude)
        0u, 0u, true);

    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok());

    auto buf = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf.ok());

    BitReader reader2(buf.value());
    auto r2 = Msg27LongRangeBroadcast::decode(reader2);
    ASSERT_TRUE(r2.ok());

    auto* m2 = dynamic_cast<Msg27LongRangeBroadcast*>(r2.value().get());
    ASSERT_NE(m2, nullptr);
    EXPECT_EQ(m2->longitude_raw(), -745);
    EXPECT_EQ(m2->latitude_raw(), 408);
}

// ---------------------------------------------------------------------------
// Encode produces correct payload length
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, Encode_PayloadLength_16Chars_0Fill)
{
    auto w = build_type27(123456789u, 0u, false, false, 0u,
        0, 0, 0u, 0u, false);
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok());

    // 96 bits / 6 bits per char = 16 armoured characters, 0 fill bits
    EXPECT_EQ(enc.value().first.size(), 16u);
    EXPECT_EQ(enc.value().second, 0u);
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    BitWriter w;
    ASSERT_TRUE(w.write_uint(27u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg27Test, DecodeWrongType_ReturnsDecodeFailure)
{
    BitWriter w(96u);
    ASSERT_TRUE(w.write_uint(1u, 6u).ok());  // type 1, not 27
    for (int i = 0; i < 90; ++i) ASSERT_TRUE(w.write_uint(0u, 1u).ok());
    BitReader reader(w.buffer());
    auto r = Msg27LongRangeBroadcast::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg27Test, Registry_DispatchesType27)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(27u));
}