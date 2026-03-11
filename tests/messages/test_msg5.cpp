/**
 * @file test_msg5.cpp
 * @brief Unit tests for AIS message type 5 (static and voyage related data).
 *
 * Tests cover:
 *   - Decoding a constructed type 5 payload and verifying all field values.
 *   - Text field stripping of trailing '@' padding.
 *   - ETA field values.
 *   - Dimensions and EPFD.
 *   - Encode/decode round-trip for all fields.
 *   - Error paths: payload too short, wrong message type.
 *   - Registry dispatch for type ID 5.
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg5.h"
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>
#include <string>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg5Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helper: construct a 426-bit type 5 payload
// ---------------------------------------------------------------------------

/**
 * @brief Builds a minimal type 5 bit buffer with specified field values.
 *
 * Parameters use the same field order as Msg5StaticAndVoyageData::decode().
 * The buffer is exactly 426 bits: 424 content bits plus the 2-bit spare
 * defined in ITU-R M.1371-5 Table 47 (bits 423-424).
 */
static std::vector<uint8_t> build_type5(
    uint32_t           mmsi          = 123456789u,
    uint8_t            ais_ver       = 0u,
    uint32_t           imo           = 9876543u,
    const std::string& call_sign     = "ABCDE",
    const std::string& vessel_name   = "MY VESSEL NAME",
    uint8_t            ship_type     = 70u,
    uint16_t           dim_bow       = 100u,
    uint16_t           dim_stern     = 20u,
    uint8_t            dim_port      = 10u,
    uint8_t            dim_stbd      = 12u,
    uint8_t            epfd          = 1u,
    uint8_t            eta_month     = 7u,
    uint8_t            eta_day       = 4u,
    uint8_t            eta_hour      = 8u,
    uint8_t            eta_minute    = 0u,
    uint8_t            draught       = 55u,
    const std::string& destination   = "PORT OF HOUSTON",
    bool               dte           = false
)
{
    BitWriter w(426u);
    EXPECT_TRUE(w.write_uint(5u,          6u).ok());
    EXPECT_TRUE(w.write_uint(0u,          2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi,       30u).ok());
    EXPECT_TRUE(w.write_uint(ais_ver,     2u).ok());
    EXPECT_TRUE(w.write_uint(imo,        30u).ok());
    EXPECT_TRUE(w.write_text(call_sign,   7u).ok());
    EXPECT_TRUE(w.write_text(vessel_name,20u).ok());
    EXPECT_TRUE(w.write_uint(ship_type,   8u).ok());
    EXPECT_TRUE(w.write_uint(dim_bow,     9u).ok());
    EXPECT_TRUE(w.write_uint(dim_stern,   9u).ok());
    EXPECT_TRUE(w.write_uint(dim_port,    6u).ok());
    EXPECT_TRUE(w.write_uint(dim_stbd,    6u).ok());
    EXPECT_TRUE(w.write_uint(epfd,        4u).ok());
    EXPECT_TRUE(w.write_uint(eta_month,   4u).ok());
    EXPECT_TRUE(w.write_uint(eta_day,     5u).ok());
    EXPECT_TRUE(w.write_uint(eta_hour,    5u).ok());
    EXPECT_TRUE(w.write_uint(eta_minute,  6u).ok());
    EXPECT_TRUE(w.write_uint(draught,     8u).ok());
    EXPECT_TRUE(w.write_text(destination,20u).ok());
    EXPECT_TRUE(w.write_bool(dte).ok());
    // Bits 423-424: spare (2 bits), always zero per ITU-R M.1371-5 Table 47.
    EXPECT_TRUE(w.write_uint(0u,          2u).ok());
    return w.buffer();
}

// ---------------------------------------------------------------------------
// Field value tests
// ---------------------------------------------------------------------------

TEST_F(Msg5Test, DecodeType5_MessageType)
{
    auto buf = build_type5();
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();
    EXPECT_EQ(r.value()->type(), MessageType::StaticAndVoyageData);
}

TEST_F(Msg5Test, DecodeType5_MMSI)
{
    auto buf = build_type5(123456789u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value()->mmsi(), 123456789u);
}

TEST_F(Msg5Test, DecodeType5_AISVersion)
{
    auto buf = build_type5(0u, 2u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->ais_version(), 2u);
}

TEST_F(Msg5Test, DecodeType5_IMONumber)
{
    auto buf = build_type5(0u, 0u, 9876543u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->imo_number(), 9876543u);
}

TEST_F(Msg5Test, DecodeType5_CallSign)
{
    auto buf = build_type5(0u, 0u, 0u, "WX4GLO");
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->call_sign(), "WX4GLO");
}

TEST_F(Msg5Test, DecodeType5_VesselName)
{
    auto buf = build_type5(0u, 0u, 0u, "CALL1", "OCEAN EXPLORER");
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->vessel_name(), "OCEAN EXPLORER");
}

TEST_F(Msg5Test, DecodeType5_ShipType)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 70u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->ship_type(), 70u);
}

TEST_F(Msg5Test, DecodeType5_Dimensions)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 0u,
                            150u, 30u, 12u, 15u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->dim_to_bow(),       150u);
    EXPECT_EQ(msg->dim_to_stern(),      30u);
    EXPECT_EQ(msg->dim_to_port(),       12u);
    EXPECT_EQ(msg->dim_to_starboard(),  15u);
}

TEST_F(Msg5Test, DecodeType5_EPFD)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 0u, 0u, 0u, 0u, 0u, 7u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->epfd(), 7u);
}

TEST_F(Msg5Test, DecodeType5_ETA)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 0u, 0u, 0u, 0u, 0u, 0u,
                            7u, 4u, 8u, 30u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->eta_month(),  7u);
    EXPECT_EQ(msg->eta_day(),    4u);
    EXPECT_EQ(msg->eta_hour(),   8u);
    EXPECT_EQ(msg->eta_minute(), 30u);
}

TEST_F(Msg5Test, DecodeType5_Draught)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 0u, 0u, 0u, 0u, 0u, 0u,
                            0u, 0u, 0u, 0u, 87u);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->draught(), 87u);
}

TEST_F(Msg5Test, DecodeType5_Destination)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 0u, 0u, 0u, 0u, 0u, 0u,
                            0u, 0u, 0u, 0u, 0u, "HAMBURG");
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->destination(), "HAMBURG");
}

TEST_F(Msg5Test, DecodeType5_DTE_NotAvailable)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 0u, 0u, 0u, 0u, 0u, 0u,
                            0u, 0u, 0u, 0u, 0u, "", true);
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->dte());
}

// ---------------------------------------------------------------------------
// Trailing '@' padding is stripped
// ---------------------------------------------------------------------------

TEST_F(Msg5Test, CallSignPaddingStripped)
{
    // "AB" is 2 characters; write_text pads to 7 with '@'.
    // read_text strips the trailing '@' on decode, so we expect "AB" back.
    auto buf = build_type5(0u, 0u, 0u, "AB");
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->call_sign(), "AB");
}

TEST_F(Msg5Test, DestinationPaddingStripped)
{
    auto buf = build_type5(0u, 0u, 0u, "", "", 0u, 0u, 0u, 0u, 0u, 0u,
                            0u, 0u, 0u, 0u, 0u, "OSLO");
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->destination(), "OSLO");
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg5Test, EncodeDecodeRoundtrip_AllFields)
{
    auto buf = build_type5(
        987654321u, 1u, 1234567u,
        "WD5IVD", "ATLANTIC SUNRISE",
        71u, 200u, 40u, 15u, 18u, 3u,
        9u, 20u, 14u, 0u, 65u,
        "ROTTERDAM", false
    );
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg5StaticAndVoyageData*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    auto enc = msg->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();

    auto buf2 = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf2.ok());
    BitReader reader2(buf2.value());
    auto r2 = Msg5StaticAndVoyageData::decode(reader2);
    ASSERT_TRUE(r2.ok());
    const auto* msg2 = dynamic_cast<Msg5StaticAndVoyageData*>(r2.value().get());
    ASSERT_NE(msg2, nullptr);

    EXPECT_EQ(msg2->mmsi(),          987654321u);
    EXPECT_EQ(msg2->ais_version(),   1u);
    EXPECT_EQ(msg2->imo_number(),    1234567u);
    EXPECT_EQ(msg2->call_sign(),     "WD5IVD");
    EXPECT_EQ(msg2->vessel_name(),   "ATLANTIC SUNRISE");
    EXPECT_EQ(msg2->ship_type(),     71u);
    EXPECT_EQ(msg2->dim_to_bow(),    200u);
    EXPECT_EQ(msg2->dim_to_stern(),   40u);
    EXPECT_EQ(msg2->dim_to_port(),    15u);
    EXPECT_EQ(msg2->dim_to_starboard(), 18u);
    EXPECT_EQ(msg2->epfd(),          3u);
    EXPECT_EQ(msg2->eta_month(),     9u);
    EXPECT_EQ(msg2->eta_day(),       20u);
    EXPECT_EQ(msg2->eta_hour(),      14u);
    EXPECT_EQ(msg2->eta_minute(),    0u);
    EXPECT_EQ(msg2->draught(),       65u);
    EXPECT_EQ(msg2->destination(),   "ROTTERDAM");
    EXPECT_FALSE(msg2->dte());
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg5Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    BitWriter w;
    ASSERT_TRUE(w.write_uint(5u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg5Test, DecodeWrongType_ReturnsDecodeFailure)
{
    auto buf = build_type5();
    // Overwrite the first 6 bits with type 1.
    // Type 5 is 0b000101, packed into the high 6 bits of byte 0 as 0b00010100 = 0x14.
    // Type 1 is 0b000001, which becomes 0b00000100 = 0x04 in the high 6 bits.
    buf[0] = static_cast<uint8_t>((1u << 2u) | (buf[0] & 0x03u));
    BitReader reader(buf);
    auto r = Msg5StaticAndVoyageData::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg5Test, Registry_DispatchesType5)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(5u));
}