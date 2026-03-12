/**
 * @file test_msg09.cpp
 * @brief Unit tests for AIS message type 9 (Standard SAR aircraft position report).
 *
 * Tests cover:
 *   - Decoding a constructed type 9 payload and verifying all field values.
 *   - Altitude helper method (altitude_m()).
 *   - Coordinate helper methods (longitude(), latitude()).
 *   - Unavailability sentinels for altitude, SOG, COG, and position.
 *   - Boolean flag fields (position accuracy, DTE, assigned, RAIM).
 *   - Radio status field.
 *   - Encode/decode round-trip for all fields.
 *   - Error paths: payload too short, wrong message type.
 *   - Registry dispatch for type ID 9.
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg09.h"
#include "aislib/messages/msg1_3.h"  // sentinel constants
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg9Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helper: build a type 9 payload buffer
// ---------------------------------------------------------------------------

/**
 * @brief Constructs a 168-bit type 9 payload buffer.
 *
 * Parameters follow the field order in the ITU-R M.1371-5 bit layout.
 * The 5 spare bits (141–145) are written as zero by this helper.
 */
static BitWriter build_type9(
    uint32_t mmsi,
    uint8_t  repeat_indicator,
    uint16_t altitude,
    uint16_t sog,
    bool     position_accuracy,
    int32_t  longitude,
    int32_t  latitude,
    uint16_t cog,
    uint8_t  time_stamp,
    uint8_t  regional_reserved,
    bool     dte,
    bool     assigned,
    bool     raim,
    uint32_t radio_status
)
{
    BitWriter w(168u);
    EXPECT_TRUE(w.write_uint(9u, 6u).ok());
    EXPECT_TRUE(w.write_uint(repeat_indicator, 2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi, 30u).ok());
    EXPECT_TRUE(w.write_uint(altitude, 12u).ok());
    EXPECT_TRUE(w.write_uint(sog, 10u).ok());
    EXPECT_TRUE(w.write_bool(position_accuracy).ok());
    EXPECT_TRUE(w.write_int(longitude, 28u).ok());
    EXPECT_TRUE(w.write_int(latitude, 27u).ok());
    EXPECT_TRUE(w.write_uint(cog, 12u).ok());
    EXPECT_TRUE(w.write_uint(time_stamp, 6u).ok());
    EXPECT_TRUE(w.write_uint(regional_reserved, 6u).ok());
    EXPECT_TRUE(w.write_bool(dte).ok());
    EXPECT_TRUE(w.write_uint(0u, 5u).ok());   // spare
    EXPECT_TRUE(w.write_bool(assigned).ok());
    EXPECT_TRUE(w.write_bool(raim).ok());
    EXPECT_TRUE(w.write_uint(radio_status, 20u).ok());
    return w;
}

// ---------------------------------------------------------------------------
// Basic decode — field values
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, Decode_AllFields_CorrectValues)
{
    auto w = build_type9(
        /*mmsi*/            111222333u,
        /*ri*/              0u,
        /*altitude*/        3000u,       // 3000 metres MSL
        /*sog*/             300u,        // 300 knots
        /*pa*/              false,
        /*lon*/             1200000,     // 2.0 degrees
        /*lat*/             600000,      // 1.0 degree
        /*cog*/             900u,        // 90.0 degrees
        /*ts*/              30u,
        /*rr*/              0u,
        /*dte*/             false,
        /*assigned*/        false,
        /*raim*/            true,
        /*radio_status*/    0xABCDEu
    );

    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();

    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    EXPECT_EQ(msg->mmsi(), 111222333u);
    EXPECT_EQ(msg->repeat_indicator(), 0u);
    EXPECT_EQ(msg->altitude(), 3000u);
    EXPECT_EQ(msg->sog(), 300u);
    EXPECT_FALSE(msg->position_accuracy());
    EXPECT_EQ(msg->longitude_raw(), 1200000);
    EXPECT_EQ(msg->latitude_raw(), 600000);
    EXPECT_EQ(msg->cog(), 900u);
    EXPECT_EQ(msg->time_stamp(), 30u);
    EXPECT_EQ(msg->regional_reserved(), 0u);
    EXPECT_FALSE(msg->dte());
    EXPECT_FALSE(msg->assigned());
    EXPECT_TRUE(msg->raim());
    EXPECT_EQ(msg->radio_status(), 0xABCDEu);
    EXPECT_EQ(msg->type(), MessageType::StandardSARPositionReport);
}

// ---------------------------------------------------------------------------
// Altitude helper
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, AltitudeAvailable_ReturnsValue)
{
    auto w = build_type9(100000001u, 0u, 1000u, 200u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto alt = msg->altitude_m();
    ASSERT_TRUE(alt.has_value());
    EXPECT_EQ(alt.value(), 1000u);
}

TEST_F(Msg9Test, AltitudeUnavailable_NullOpt)
{
    auto w = build_type9(100000002u, 0u, kAltitudeUnavailable, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->altitude_m().has_value());
    EXPECT_EQ(msg->altitude(), kAltitudeUnavailable);
}

TEST_F(Msg9Test, AltitudeZero_Available)
{
    auto w = build_type9(100000003u, 0u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto alt = msg->altitude_m();
    ASSERT_TRUE(alt.has_value());
    EXPECT_EQ(alt.value(), 0u);
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, Longitude_Available_CorrectDegrees)
{
    auto w = build_type9(200000001u, 0u, 0u, 0u, false,
        600000, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lon = msg->longitude();
    ASSERT_TRUE(lon.has_value());
    EXPECT_DOUBLE_EQ(lon.value(), 1.0);
}

TEST_F(Msg9Test, Longitude_Unavailable_NullOpt)
{
    auto w = build_type9(200000002u, 0u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->longitude().has_value());
}

TEST_F(Msg9Test, Latitude_Available_CorrectDegrees)
{
    auto w = build_type9(200000003u, 0u, 0u, 0u, false,
        kLongitudeUnavailable, 600000,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lat = msg->latitude();
    ASSERT_TRUE(lat.has_value());
    EXPECT_DOUBLE_EQ(lat.value(), 1.0);
}

TEST_F(Msg9Test, Latitude_Unavailable_NullOpt)
{
    auto w = build_type9(200000004u, 0u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->latitude().has_value());
}

TEST_F(Msg9Test, Longitude_Negative_CorrectDegrees)
{
    auto w = build_type9(200000005u, 0u, 0u, 0u, false,
        -600000, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lon = msg->longitude();
    ASSERT_TRUE(lon.has_value());
    EXPECT_DOUBLE_EQ(lon.value(), -1.0);
}

// ---------------------------------------------------------------------------
// SOG — SAR uses whole knots
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, SogUnavailable_RawValue1023)
{
    auto w = build_type9(300000001u, 0u, 0u, kSarSogUnavailable, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sog(), kSarSogUnavailable);
}

TEST_F(Msg9Test, SogMaxValid_1022)
{
    auto w = build_type9(300000002u, 0u, 0u, 1022u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sog(), 1022u);
}

// ---------------------------------------------------------------------------
// Flag fields
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, Flags_AllTrue)
{
    auto w = build_type9(400000001u, 3u, 0u, 0u, true,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, true, true, true, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->position_accuracy());
    EXPECT_TRUE(msg->dte());
    EXPECT_TRUE(msg->assigned());
    EXPECT_TRUE(msg->raim());
    EXPECT_EQ(msg->repeat_indicator(), 3u);
}

TEST_F(Msg9Test, Flags_AllFalse)
{
    auto w = build_type9(400000002u, 0u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 0u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->position_accuracy());
    EXPECT_FALSE(msg->dte());
    EXPECT_FALSE(msg->assigned());
    EXPECT_FALSE(msg->raim());
}

// ---------------------------------------------------------------------------
// Regional reserved
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, RegionalReserved_MaxValue63)
{
    auto w = build_type9(500000001u, 0u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, 60u, 63u, false, false, false, 0u);
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->regional_reserved(), 63u);
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, RoundTrip_AllFields)
{
    auto w = build_type9(
        777888999u, 2u, 2000u, 450u, true,
        -5400000, 1800000,   // -9.0, 3.0 degrees
        2700u, 45u, 15u,
        true, false, true, 0x12345u
    );

    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();

    auto buf = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf.ok());

    BitReader reader2(buf.value());
    auto r2 = Msg9SARPositionReport::decode(reader2);
    ASSERT_TRUE(r2.ok());

    auto* m1 = dynamic_cast<Msg9SARPositionReport*>(r.value().get());
    auto* m2 = dynamic_cast<Msg9SARPositionReport*>(r2.value().get());
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);

    EXPECT_EQ(m2->mmsi(), m1->mmsi());
    EXPECT_EQ(m2->repeat_indicator(), m1->repeat_indicator());
    EXPECT_EQ(m2->altitude(), m1->altitude());
    EXPECT_EQ(m2->sog(), m1->sog());
    EXPECT_EQ(m2->position_accuracy(), m1->position_accuracy());
    EXPECT_EQ(m2->longitude_raw(), m1->longitude_raw());
    EXPECT_EQ(m2->latitude_raw(), m1->latitude_raw());
    EXPECT_EQ(m2->cog(), m1->cog());
    EXPECT_EQ(m2->time_stamp(), m1->time_stamp());
    EXPECT_EQ(m2->regional_reserved(), m1->regional_reserved());
    EXPECT_EQ(m2->dte(), m1->dte());
    EXPECT_EQ(m2->assigned(), m1->assigned());
    EXPECT_EQ(m2->raim(), m1->raim());
    EXPECT_EQ(m2->radio_status(), m1->radio_status());
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    BitWriter w;
    ASSERT_TRUE(w.write_uint(9u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg9Test, DecodeWrongType_ReturnsDecodeFailure)
{
    BitWriter w(168u);
    ASSERT_TRUE(w.write_uint(1u, 6u).ok());  // type 1, not 9
    for (int i = 0; i < 162; ++i) ASSERT_TRUE(w.write_uint(0u, 1u).ok());
    BitReader reader(w.buffer());
    auto r = Msg9SARPositionReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg9Test, Registry_DispatchesType9)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(9u));
}