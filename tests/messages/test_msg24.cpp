/**
 * @file test_msg24.cpp
 * @brief Unit tests for AIS message type 24 (Class B CS static data report).
 *
 * Tests cover:
 *   - Decoding Part A and verifying vessel name field.
 *   - Decoding Part B and verifying all Part B fields.
 *   - Trailing '@' padding is stripped from text fields.
 *   - Encode/decode round-trip for both parts.
 *   - Error paths: payload too short, wrong message type, invalid part number.
 *   - Registry dispatch for type ID 24.
 *   - Part A object has empty Part B fields (ship_type, call_sign, etc.).
 *   - Part B object has empty Part A field (vessel_name).
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg24.h"
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>
#include <string>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg24Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helpers: build Part A and Part B payloads
// ---------------------------------------------------------------------------

/**
 * @brief Constructs a 168-bit type 24 Part A payload.
 */
static std::vector<uint8_t> build_part_a(
    uint32_t           mmsi        = 338220391u,
    const std::string& vessel_name = "MY CLASS B VESSEL"
)
{
    BitWriter w(168u);
    EXPECT_TRUE(w.write_uint(24u,       6u).ok());
    EXPECT_TRUE(w.write_uint(0u,        2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi,     30u).ok());
    EXPECT_TRUE(w.write_uint(0u,        2u).ok()); // part number = 0
    EXPECT_TRUE(w.write_text(vessel_name, 20u).ok());
    EXPECT_TRUE(w.write_uint(0u,        8u).ok()); // spare
    return w.buffer();
}

/**
 * @brief Constructs a 168-bit type 24 Part B payload.
 */
static std::vector<uint8_t> build_part_b(
    uint32_t           mmsi          = 338220391u,
    uint8_t            ship_type     = 36u,
    const std::string& vendor_id     = "FURUNO",
    const std::string& call_sign     = "WD5XYZ",
    uint16_t           dim_bow       = 5u,
    uint16_t           dim_stern     = 2u,
    uint8_t            dim_port      = 1u,
    uint8_t            dim_stbd      = 1u,
    uint8_t            epfd          = 1u
)
{
    BitWriter w(168u);
    EXPECT_TRUE(w.write_uint(24u,       6u).ok());
    EXPECT_TRUE(w.write_uint(0u,        2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi,     30u).ok());
    EXPECT_TRUE(w.write_uint(1u,        2u).ok()); // part number = 1
    EXPECT_TRUE(w.write_uint(ship_type, 8u).ok());
    EXPECT_TRUE(w.write_text(vendor_id, 7u).ok());
    EXPECT_TRUE(w.write_text(call_sign, 7u).ok());
    EXPECT_TRUE(w.write_uint(dim_bow,   9u).ok());
    EXPECT_TRUE(w.write_uint(dim_stern, 9u).ok());
    EXPECT_TRUE(w.write_uint(dim_port,  6u).ok());
    EXPECT_TRUE(w.write_uint(dim_stbd,  6u).ok());
    EXPECT_TRUE(w.write_uint(epfd,      4u).ok());
    EXPECT_TRUE(w.write_uint(0u,        2u).ok()); // spare
    return w.buffer();
}

// ---------------------------------------------------------------------------
// Part A tests
// ---------------------------------------------------------------------------

TEST_F(Msg24Test, DecodePartA_MessageType)
{
    auto buf = build_part_a();
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();
    EXPECT_EQ(r.value()->type(), MessageType::ClassBCSStaticDataReport);
}

TEST_F(Msg24Test, DecodePartA_PartNumber)
{
    auto buf = build_part_a();
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->part_number(), 0u);
}

TEST_F(Msg24Test, DecodePartA_MMSI)
{
    auto buf = build_part_a(338220391u);
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    EXPECT_EQ(r.value()->mmsi(), 338220391u);
}

TEST_F(Msg24Test, DecodePartA_VesselName)
{
    auto buf = build_part_a(0u, "OCEAN SPRITE");
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->vessel_name(), "OCEAN SPRITE");
}

TEST_F(Msg24Test, DecodePartA_PartBFieldsAreDefault)
{
    // Part A should have no Part B data populated.
    auto buf = build_part_a(0u, "TEST VESSEL");
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->ship_type(),         0u);
    EXPECT_TRUE(msg->call_sign().empty());
    EXPECT_TRUE(msg->vendor_id().empty());
    EXPECT_EQ(msg->dim_to_bow(),        0u);
    EXPECT_EQ(msg->dim_to_stern(),      0u);
    EXPECT_EQ(msg->dim_to_port(),       0u);
    EXPECT_EQ(msg->dim_to_starboard(),  0u);
    EXPECT_EQ(msg->epfd(),              0u);
    EXPECT_EQ(msg->mothership_mmsi(),   0u);
}

TEST_F(Msg24Test, DecodePartA_VesselNamePaddingStripped)
{
    // "STAR" is 4 chars; write_text pads to 20 with '@'; read_text strips it.
    auto buf = build_part_a(0u, "STAR");
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->vessel_name(), "STAR");
}

// ---------------------------------------------------------------------------
// Part B tests
// ---------------------------------------------------------------------------

TEST_F(Msg24Test, DecodePartB_PartNumber)
{
    auto buf = build_part_b();
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->part_number(), 1u);
}

TEST_F(Msg24Test, DecodePartB_ShipType)
{
    auto buf = build_part_b(0u, 36u);
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->ship_type(), 36u);
}

TEST_F(Msg24Test, DecodePartB_VendorID)
{
    auto buf = build_part_b(0u, 0u, "GARMIN");
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->vendor_id(), "GARMIN");
}

TEST_F(Msg24Test, DecodePartB_CallSign)
{
    auto buf = build_part_b(0u, 0u, "", "KA1ABC");
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->call_sign(), "KA1ABC");
}

TEST_F(Msg24Test, DecodePartB_Dimensions)
{
    auto buf = build_part_b(0u, 0u, "", "", 8u, 3u, 2u, 2u);
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->dim_to_bow(),       8u);
    EXPECT_EQ(msg->dim_to_stern(),     3u);
    EXPECT_EQ(msg->dim_to_port(),      2u);
    EXPECT_EQ(msg->dim_to_starboard(), 2u);
}

TEST_F(Msg24Test, DecodePartB_EPFD)
{
    auto buf = build_part_b(0u, 0u, "", "", 0u, 0u, 0u, 0u, 7u);
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->epfd(), 7u);
}

TEST_F(Msg24Test, DecodePartB_VesselNameIsEmpty)
{
    auto buf = build_part_b();
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->vessel_name().empty());
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg24Test, EncodeDecodeRoundtrip_PartA)
{
    auto buf = build_part_a(338220391u, "SEABIRD III");
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    auto enc = msg->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();

    auto buf2 = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf2.ok());
    BitReader reader2(buf2.value());
    auto r2 = Msg24StaticDataReport::decode(reader2);
    ASSERT_TRUE(r2.ok());
    const auto* msg2 = dynamic_cast<Msg24StaticDataReport*>(r2.value().get());
    ASSERT_NE(msg2, nullptr);

    EXPECT_EQ(msg2->mmsi(),          338220391u);
    EXPECT_EQ(msg2->part_number(),   0u);
    EXPECT_EQ(msg2->vessel_name(),   "SEABIRD III");
}

TEST_F(Msg24Test, EncodeDecodeRoundtrip_PartB)
{
    auto buf = build_part_b(338220391u, 37u, "FURUNO", "WD5XYZ",
                             6u, 2u, 1u, 2u, 1u);
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_TRUE(r.ok());
    const auto* msg = dynamic_cast<Msg24StaticDataReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    auto enc = msg->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();

    auto buf2 = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf2.ok());
    BitReader reader2(buf2.value());
    auto r2 = Msg24StaticDataReport::decode(reader2);
    ASSERT_TRUE(r2.ok());
    const auto* msg2 = dynamic_cast<Msg24StaticDataReport*>(r2.value().get());
    ASSERT_NE(msg2, nullptr);

    EXPECT_EQ(msg2->mmsi(),          338220391u);
    EXPECT_EQ(msg2->part_number(),   1u);
    EXPECT_EQ(msg2->ship_type(),     37u);
    EXPECT_EQ(msg2->vendor_id(),     "FURUNO");
    EXPECT_EQ(msg2->call_sign(),     "WD5XYZ");
    EXPECT_EQ(msg2->dim_to_bow(),    6u);
    EXPECT_EQ(msg2->dim_to_stern(),  2u);
    EXPECT_EQ(msg2->dim_to_port(),   1u);
    EXPECT_EQ(msg2->dim_to_starboard(), 2u);
    EXPECT_EQ(msg2->epfd(),          1u);
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg24Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    BitWriter w;
    ASSERT_TRUE(w.write_uint(24u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg24Test, DecodeWrongMessageType_ReturnsDecodeFailure)
{
    auto buf = build_part_a();
    // Overwrite type field with type 18.
    buf[0] = static_cast<uint8_t>((18u << 2u) | (buf[0] & 0x03u));
    BitReader reader(buf);
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

TEST_F(Msg24Test, DecodeInvalidPartNumber_ReturnsDecodeFailure)
{
    // Build a payload with part_number = 2 (invalid; only 0 and 1 are defined).
    BitWriter w(168u);
    ASSERT_TRUE(w.write_uint(24u, 6u).ok());
    ASSERT_TRUE(w.write_uint(0u,  2u).ok());
    ASSERT_TRUE(w.write_uint(0u, 30u).ok());
    ASSERT_TRUE(w.write_uint(2u,  2u).ok()); // part_number = 2
    for (int i = 0; i < 128; ++i) ASSERT_TRUE(w.write_uint(0u, 1u).ok());

    BitReader reader(w.buffer());
    auto r = Msg24StaticDataReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg24Test, Registry_DispatchesType24)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(24u));
}
