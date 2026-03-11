/**
 * @file test_msg18.cpp
 * @brief Unit tests for AIS message type 18 (Standard Class B CS position report).
 *
 * Tests cover:
 *   - Decoding a constructed type 18 payload and verifying all field values.
 *   - Coordinate helper methods (longitude(), latitude()).
 *   - Unavailability sentinels for SOG, COG, heading, and position.
 *   - Boolean flag fields (CS, display, DSC, band, msg22, assigned, RAIM).
 *   - Encode/decode round-trip for all fields.
 *   - Error paths: payload too short, wrong message type.
 *   - Registry dispatch for type ID 18.
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg18.h"
#include "aislib/messages/msg1_3.h"  // sentinel constants
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg18Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helper: build a type 18 payload buffer
// ---------------------------------------------------------------------------

/**
 * @brief Constructs a 168-bit type 18 payload buffer.
 *
 * Fields are provided in bit-layout order.  The 8 reserved bits after the MMSI
 * are written as zero.
 */
static std::vector<uint8_t> build_type18(
    uint32_t mmsi             = 338220390u,
    uint16_t sog              = 10u,
    bool     pos_acc          = false,
    int32_t  lon_raw          = -119820000,   // ~-119.70 degrees
    int32_t  lat_raw          = 34290000,     //  ~34.15 degrees
    uint16_t cog              = 450u,
    uint16_t true_heading     = 135u,
    uint8_t  time_stamp       = 34u,
    uint8_t  regional_res     = 0u,
    bool     cs_flag          = true,
    bool     display_flag     = false,
    bool     dsc_flag         = true,
    bool     band_flag        = true,
    bool     msg22_flag       = false,
    bool     assigned         = false,
    bool     raim             = false,
    uint32_t radio_status     = 917510u
)
{
    BitWriter w(168u);
    EXPECT_TRUE(w.write_uint(18u,     6u).ok());
    EXPECT_TRUE(w.write_uint(0u,      2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi,   30u).ok());
    EXPECT_TRUE(w.write_uint(0u,      8u).ok()); // reserved
    EXPECT_TRUE(w.write_uint(sog,    10u).ok());
    EXPECT_TRUE(w.write_bool(pos_acc).ok());
    EXPECT_TRUE(w.write_int(lon_raw, 28u).ok());
    EXPECT_TRUE(w.write_int(lat_raw, 27u).ok());
    EXPECT_TRUE(w.write_uint(cog,    12u).ok());
    EXPECT_TRUE(w.write_uint(true_heading, 9u).ok());
    EXPECT_TRUE(w.write_uint(time_stamp,   6u).ok());
    EXPECT_TRUE(w.write_uint(regional_res, 2u).ok());
    EXPECT_TRUE(w.write_bool(cs_flag).ok());
    EXPECT_TRUE(w.write_bool(display_flag).ok());
    EXPECT_TRUE(w.write_bool(dsc_flag).ok());
    EXPECT_TRUE(w.write_bool(band_flag).ok());
    EXPECT_TRUE(w.write_bool(msg22_flag).ok());
    EXPECT_TRUE(w.write_bool(assigned).ok());
    EXPECT_TRUE(w.write_bool(raim).ok());
    EXPECT_TRUE(w.write_uint(radio_status, 20u).ok());
    return w.buffer();
}

// ---------------------------------------------------------------------------
// Field value tests
// ---------------------------------------------------------------------------

TEST_F(Msg18Test, DecodeType18_MessageType)
{
    auto buf = build_type18();
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();
    EXPECT_EQ(r.value()->type(), MessageType::StandardClassBPositionReport);
}

TEST_F(Msg18Test, DecodeType18_MMSI)
{
    auto buf = build_type18(338220390u);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value()->mmsi(), 338220390u);
}

TEST_F(Msg18Test, DecodeType18_SOG)
{
    auto buf = build_type18(0u, 50u);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sog(), 50u);
}

TEST_F(Msg18Test, DecodeType18_PositionAccuracy)
{
    auto buf = build_type18(0u, 0u, true);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->position_accuracy());
}

TEST_F(Msg18Test, DecodeType18_LongitudeConversion)
{
    // -119.7 degrees = -119.7 * 600000 = -71820000
    auto buf = build_type18(0u, 0u, false, -71820000, 0);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    ASSERT_TRUE(msg->longitude().has_value());
    EXPECT_NEAR(msg->longitude().value(), -119.7, 0.0001);
}

TEST_F(Msg18Test, DecodeType18_LatitudeConversion)
{
    // 34.15 degrees = 34.15 * 600000 = 20490000
    auto buf = build_type18(0u, 0u, false, 0, 20490000);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    ASSERT_TRUE(msg->latitude().has_value());
    EXPECT_NEAR(msg->latitude().value(), 34.15, 0.0001);
}

TEST_F(Msg18Test, DecodeType18_COG)
{
    auto buf = build_type18(0u, 0u, false, 0, 0, 2700u); // 270.0 degrees
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->cog(), 2700u);
}

TEST_F(Msg18Test, DecodeType18_TrueHeading)
{
    auto buf = build_type18(0u, 0u, false, 0, 0, 0u, 270u);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->true_heading(), 270u);
}

TEST_F(Msg18Test, DecodeType18_TimeStamp)
{
    auto buf = build_type18(0u, 0u, false, 0, 0, 0u, 0u, 55u);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->time_stamp(), 55u);
}

TEST_F(Msg18Test, DecodeType18_CSFlag)
{
    auto buf = build_type18(0u, 0u, false, 0, 0, 0u, 0u, 0u, 0u, true);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->cs_flag());
}

TEST_F(Msg18Test, DecodeType18_DSCFlag)
{
    auto buf = build_type18(0u, 0u, false, 0, 0, 0u, 0u, 0u, 0u,
                             false, false, true);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->dsc_flag());
}

TEST_F(Msg18Test, DecodeType18_RAIM)
{
    auto buf = build_type18(0u, 0u, false, 0, 0, 0u, 0u, 0u, 0u,
                             false, false, false, false, false, false, true);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->raim());
}

// ---------------------------------------------------------------------------
// Sentinel values
// ---------------------------------------------------------------------------

TEST_F(Msg18Test, LongitudeUnavailable_ReturnsNullopt)
{
    auto buf = build_type18(0u, 0u, false, kLongitudeUnavailable, 0);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->longitude().has_value());
}

TEST_F(Msg18Test, LatitudeUnavailable_ReturnsNullopt)
{
    auto buf = build_type18(0u, 0u, false, 0, kLatitudeUnavailable);
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->latitude().has_value());
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg18Test, EncodeDecodeRoundtrip_AllFields)
{
    auto buf = build_type18(
        338220390u, 25u, true,
        -71820000, 20490000,
        1800u, 180u, 42u, 0u,
        true, false, true, true, false, false, false, 917510u
    );
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg18ClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    auto enc = msg->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();

    auto buf2 = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf2.ok());
    BitReader reader2(buf2.value());
    auto r2 = Msg18ClassBPositionReport::decode(reader2);
    ASSERT_TRUE(r2.ok());
    const auto* msg2 = dynamic_cast<Msg18ClassBPositionReport*>(r2.value().get());
    ASSERT_NE(msg2, nullptr);

    EXPECT_EQ(msg2->mmsi(),         338220390u);
    EXPECT_EQ(msg2->sog(),          25u);
    EXPECT_TRUE(msg2->position_accuracy());
    EXPECT_EQ(msg2->cog(),          1800u);
    EXPECT_EQ(msg2->true_heading(), 180u);
    EXPECT_EQ(msg2->time_stamp(),   42u);
    EXPECT_TRUE(msg2->cs_flag());
    EXPECT_TRUE(msg2->dsc_flag());
    EXPECT_TRUE(msg2->band_flag());
    EXPECT_FALSE(msg2->raim());
    ASSERT_TRUE(msg2->longitude().has_value());
    ASSERT_TRUE(msg2->latitude().has_value());
    EXPECT_NEAR(msg2->longitude().value(), -119.7, 0.0001);
    EXPECT_NEAR(msg2->latitude().value(),   34.15, 0.0001);
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg18Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    BitWriter w;
    ASSERT_TRUE(w.write_uint(18u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg18Test, DecodeWrongType_ReturnsDecodeFailure)
{
    auto buf = build_type18();
    // Overwrite type field with type 1.
    buf[0] = static_cast<uint8_t>((1u << 2u) | (buf[0] & 0x03u));
    BitReader reader(buf);
    auto r = Msg18ClassBPositionReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg18Test, Registry_DispatchesType18)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(18u));
}
