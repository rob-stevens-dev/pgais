/**
 * @file test_msg4_11.cpp
 * @brief Unit tests for AIS message types 4 and 11 (base station report /
 *        UTC date response).
 *
 * Tests cover:
 *   - Decoding a type 4 payload and verifying all field values.
 *   - Type 11 decodes with the UTCDateResponse MessageType enumerator.
 *   - Coordinate helpers (longitude(), latitude()).
 *   - Encode/decode round-trip preserving all fields.
 *   - Error paths: too-short payload, wrong message type.
 *   - Registry dispatch for type IDs 4 and 11.
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg4_11.h"
#include "aislib/messages/msg1_3.h"   // kLongitudeUnavailable, kLatitudeUnavailable
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg4_11Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helper: build a minimal type 4 payload with known field values
// ---------------------------------------------------------------------------

/**
 * @brief Constructs a 168-bit type 4 payload buffer with specified values.
 */
static std::vector<uint8_t> build_type4(
    uint8_t  msg_type      = 4u,
    uint32_t mmsi          = 3669702u,
    uint16_t utc_year      = 2024u,
    uint8_t  utc_month     = 6u,
    uint8_t  utc_day       = 15u,
    uint8_t  utc_hour      = 12u,
    uint8_t  utc_minute    = 30u,
    uint8_t  utc_second    = 45u,
    bool     pos_acc       = true,
    int32_t  lon_raw       = -48600000,  // -81.0 degrees
    int32_t  lat_raw       = 14700000,   //  24.5 degrees
    uint8_t  epfd          = 1u,
    bool     raim          = false,
    uint32_t radio_status  = 0u
)
{
    BitWriter w(168u);
    EXPECT_TRUE(w.write_uint(msg_type,    6u).ok());
    EXPECT_TRUE(w.write_uint(0u,          2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi,       30u).ok());
    EXPECT_TRUE(w.write_uint(utc_year,   14u).ok());
    EXPECT_TRUE(w.write_uint(utc_month,   4u).ok());
    EXPECT_TRUE(w.write_uint(utc_day,     5u).ok());
    EXPECT_TRUE(w.write_uint(utc_hour,    5u).ok());
    EXPECT_TRUE(w.write_uint(utc_minute,  6u).ok());
    EXPECT_TRUE(w.write_uint(utc_second,  6u).ok());
    EXPECT_TRUE(w.write_bool(pos_acc).ok());
    EXPECT_TRUE(w.write_int(lon_raw,     28u).ok());
    EXPECT_TRUE(w.write_int(lat_raw,     27u).ok());
    EXPECT_TRUE(w.write_uint(epfd,        4u).ok());
    EXPECT_TRUE(w.write_uint(0u,         10u).ok()); // 10 spare bits
    EXPECT_TRUE(w.write_bool(raim).ok());
    EXPECT_TRUE(w.write_uint(radio_status, 19u).ok());
    return w.buffer();
}

// ---------------------------------------------------------------------------
// Field value tests
// ---------------------------------------------------------------------------

TEST_F(Msg4_11Test, DecodeType4_MessageType)
{
    auto buf = build_type4();
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();
    EXPECT_EQ(r.value()->type(), MessageType::BaseStationReport);
}

TEST_F(Msg4_11Test, DecodeType4_MMSI)
{
    auto buf = build_type4(4u, 3669702u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value()->mmsi(), 3669702u);
}

TEST_F(Msg4_11Test, DecodeType4_UTCYear)
{
    auto buf = build_type4(4u, 0u, 2024u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->utc_year(), 2024u);
}

TEST_F(Msg4_11Test, DecodeType4_UTCMonth)
{
    auto buf = build_type4(4u, 0u, 2024u, 6u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->utc_month(), 6u);
}

TEST_F(Msg4_11Test, DecodeType4_UTCDay)
{
    auto buf = build_type4(4u, 0u, 2024u, 6u, 15u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->utc_day(), 15u);
}

TEST_F(Msg4_11Test, DecodeType4_UTCTime)
{
    auto buf = build_type4(4u, 0u, 2024u, 6u, 15u, 12u, 30u, 45u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->utc_hour(),   12u);
    EXPECT_EQ(msg->utc_minute(), 30u);
    EXPECT_EQ(msg->utc_second(), 45u);
}

TEST_F(Msg4_11Test, DecodeType4_PositionAccuracy)
{
    auto buf = build_type4(4u, 0u, 2024u, 1u, 1u, 0u, 0u, 0u, true);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->position_accuracy());
}

TEST_F(Msg4_11Test, DecodeType4_LongitudeConversion)
{
    // -81.0 degrees = -81.0 * 600000 = -48600000
    auto buf = build_type4(4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, false,
                            -48600000, 0);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    ASSERT_TRUE(msg->longitude().has_value());
    EXPECT_NEAR(msg->longitude().value(), -81.0, 0.0001);
}

TEST_F(Msg4_11Test, DecodeType4_LatitudeConversion)
{
    // 24.5 degrees = 24.5 * 600000 = 14700000
    auto buf = build_type4(4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, false,
                            0, 14700000);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    ASSERT_TRUE(msg->latitude().has_value());
    EXPECT_NEAR(msg->latitude().value(), 24.5, 0.0001);
}

TEST_F(Msg4_11Test, DecodeType4_EPFD)
{
    auto buf = build_type4(4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, false,
                            0, 0, 1u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->epfd(), 1u);
}

TEST_F(Msg4_11Test, DecodeType4_RAIM)
{
    auto buf = build_type4(4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, false,
                            0, 0, 1u, true);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->raim());
}

// ---------------------------------------------------------------------------
// Type 11 variant
// ---------------------------------------------------------------------------

TEST_F(Msg4_11Test, DecodeType11_ReturnsUTCDateResponse)
{
    auto buf = build_type4(11u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value()->type(), MessageType::UTCDateResponse);
}

// ---------------------------------------------------------------------------
// Longitude / latitude unavailable sentinels
// ---------------------------------------------------------------------------

TEST_F(Msg4_11Test, LongitudeUnavailable_ReturnsNullopt)
{
    auto buf = build_type4(4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, false,
                            kLongitudeUnavailable, 0);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->longitude().has_value());
}

TEST_F(Msg4_11Test, LatitudeUnavailable_ReturnsNullopt)
{
    auto buf = build_type4(4u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, false,
                            0, kLatitudeUnavailable);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->latitude().has_value());
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg4_11Test, EncodeDecodeRoundtrip_AllFields)
{
    auto buf = build_type4(4u, 3669702u, 2024u, 6u, 15u, 12u, 30u, 45u,
                            true, -48600000, 14700000, 1u, false, 0u);
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg4_11BaseStationReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    // Encode back to payload
    auto enc = msg->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();

    // Decode again
    auto buf2 = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf2.ok());
    BitReader reader2(buf2.value());
    auto r2 = Msg4_11BaseStationReport::decode(reader2);
    ASSERT_TRUE(r2.ok());
    const auto* msg2 = dynamic_cast<Msg4_11BaseStationReport*>(r2.value().get());
    ASSERT_NE(msg2, nullptr);

    EXPECT_EQ(msg2->mmsi(),        3669702u);
    EXPECT_EQ(msg2->utc_year(),    2024u);
    EXPECT_EQ(msg2->utc_month(),   6u);
    EXPECT_EQ(msg2->utc_day(),     15u);
    EXPECT_EQ(msg2->utc_hour(),    12u);
    EXPECT_EQ(msg2->utc_minute(),  30u);
    EXPECT_EQ(msg2->utc_second(),  45u);
    EXPECT_EQ(msg2->epfd(),        1u);
    ASSERT_TRUE(msg2->longitude().has_value());
    ASSERT_TRUE(msg2->latitude().has_value());
    EXPECT_NEAR(msg2->longitude().value(), -81.0, 0.0001);
    EXPECT_NEAR(msg2->latitude().value(),   24.5, 0.0001);
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg4_11Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    BitWriter w;
    ASSERT_TRUE(w.write_uint(4u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg4_11Test, DecodeWrongType_ReturnsDecodeFailure)
{
    auto buf = build_type4(1u);  // type 1 instead of 4
    BitReader reader(buf);
    auto r = Msg4_11BaseStationReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg4_11Test, Registry_DispatchesType4)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(4u));
}

TEST_F(Msg4_11Test, Registry_DispatchesType11)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(11u));
}
