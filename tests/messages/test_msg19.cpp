/**
 * @file test_msg19.cpp
 * @brief Unit tests for AIS message type 19 (Extended Class B CS position report).
 *
 * Tests cover:
 *   - Decoding a constructed type 19 payload and verifying all field values.
 *   - Coordinate helper methods (longitude(), latitude()).
 *   - Unavailability sentinels for SOG, COG, heading, and position.
 *   - Name field: padding strip, all-padding name, maximum-length name.
 *   - Dimension fields (bow, stern, port, starboard).
 *   - Boolean flag fields (position accuracy, RAIM, DTE, assigned).
 *   - Encode/decode round-trip for all fields.
 *   - Error paths: payload too short, wrong message type.
 *   - Registry dispatch for type ID 19.
 */

#include <gtest/gtest.h>

#include "aislib/message.h"
#include "aislib/messages/msg19.h"
#include "aislib/messages/msg1_3.h"  // sentinel constants
#include "aislib/payload.h"
#include "aislib/registry_init.h"

#include <memory>
#include <string>

using namespace aislib;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Msg19Test : public ::testing::Test {
protected:
    void SetUp() override
    {
        register_all_decoders();
    }
};

// ---------------------------------------------------------------------------
// Helper: build a type 19 payload buffer
// ---------------------------------------------------------------------------

/**
 * @brief Constructs a 312-bit type 19 payload buffer.
 *
 * Parameters follow the field order in the ITU-R M.1371-5 bit layout.
 * The 10 reserved bits (38–47) are written as zero by this helper.
 */
static BitWriter build_type19(
    uint32_t    mmsi,
    uint8_t     repeat_indicator,
    uint16_t    sog,
    bool        position_accuracy,
    int32_t     longitude,
    int32_t     latitude,
    uint16_t    cog,
    uint16_t    true_heading,
    uint8_t     time_stamp,
    uint8_t     regional_reserved,
    const char* name,           // exactly 20 characters (padded with '@' if shorter)
    uint8_t     ship_type,
    uint16_t    dim_bow,
    uint16_t    dim_stern,
    uint8_t     dim_port,
    uint8_t     dim_starboard,
    uint8_t     epfd_type,
    bool        raim,
    bool        dte,
    bool        assigned
)
{
    BitWriter w(312u);
    EXPECT_TRUE(w.write_uint(19u, 6u).ok());                           // message type
    EXPECT_TRUE(w.write_uint(repeat_indicator, 2u).ok());
    EXPECT_TRUE(w.write_uint(mmsi, 30u).ok());
    EXPECT_TRUE(w.write_uint(0u, 10u).ok());                           // reserved
    EXPECT_TRUE(w.write_uint(sog, 10u).ok());
    EXPECT_TRUE(w.write_bool(position_accuracy).ok());
    EXPECT_TRUE(w.write_int(longitude, 28u).ok());
    EXPECT_TRUE(w.write_int(latitude, 27u).ok());
    EXPECT_TRUE(w.write_uint(cog, 12u).ok());
    EXPECT_TRUE(w.write_uint(true_heading, 9u).ok());
    EXPECT_TRUE(w.write_uint(time_stamp, 6u).ok());
    EXPECT_TRUE(w.write_uint(regional_reserved, 4u).ok());
    // Name: pad to 20 characters with '@'
    std::string padded(name);
    padded.resize(20u, '@');
    EXPECT_TRUE(w.write_text(padded, 20u).ok());
    EXPECT_TRUE(w.write_uint(ship_type, 8u).ok());
    EXPECT_TRUE(w.write_uint(dim_bow, 9u).ok());
    EXPECT_TRUE(w.write_uint(dim_stern, 9u).ok());
    EXPECT_TRUE(w.write_uint(dim_port, 6u).ok());
    EXPECT_TRUE(w.write_uint(dim_starboard, 6u).ok());
    EXPECT_TRUE(w.write_uint(epfd_type, 4u).ok());
    EXPECT_TRUE(w.write_bool(raim).ok());
    EXPECT_TRUE(w.write_bool(dte).ok());
    EXPECT_TRUE(w.write_bool(assigned).ok());
    EXPECT_TRUE(w.write_uint(0u, 2u).ok());                            // spare
    return w;
}

// ---------------------------------------------------------------------------
// Basic decode — field values
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, Decode_AllFields_CorrectValues)
{
    auto w = build_type19(
        /*mmsi*/            338234631u,
        /*ri*/              0u,
        /*sog*/             85u,          // 8.5 knots
        /*pa*/              true,
        /*lon*/             -13521480,    // -22.536 degrees
        /*lat*/              4167900,    // 6.946 degrees
        /*cog*/             1800u,        // 180.0 degrees
        /*hdg*/             180u,
        /*ts*/              45u,
        /*rr*/              0u,
        /*name*/            "VESSEL NAME   ",
        /*ship_type*/       70u,
        /*bow*/             25u,
        /*stern*/           15u,
        /*port*/            5u,
        /*stbd*/            3u,
        /*epfd*/            1u,           // GPS
        /*raim*/            true,
        /*dte*/             false,
        /*assigned*/        false
    );

    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok()) << r.error().message();

    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);

    EXPECT_EQ(msg->mmsi(), 338234631u);
    EXPECT_EQ(msg->repeat_indicator(), 0u);
    EXPECT_EQ(msg->sog(), 85u);
    EXPECT_TRUE(msg->position_accuracy());
    EXPECT_EQ(msg->longitude_raw(), -13521480);
    EXPECT_EQ(msg->latitude_raw(), 4167900);
    EXPECT_EQ(msg->cog(), 1800u);
    EXPECT_EQ(msg->true_heading(), 180u);
    EXPECT_EQ(msg->time_stamp(), 45u);
    EXPECT_EQ(msg->regional_reserved(), 0u);
    EXPECT_EQ(msg->ship_type(), 70u);
    EXPECT_EQ(msg->dim_bow(), 25u);
    EXPECT_EQ(msg->dim_stern(), 15u);
    EXPECT_EQ(msg->dim_port(), 5u);
    EXPECT_EQ(msg->dim_starboard(), 3u);
    EXPECT_EQ(msg->epfd_type(), 1u);
    EXPECT_TRUE(msg->raim());
    EXPECT_FALSE(msg->dte());
    EXPECT_FALSE(msg->assigned());
    EXPECT_EQ(msg->type(), MessageType::ExtendedClassBPositionReport);
}

// ---------------------------------------------------------------------------
// Name field handling
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, Decode_Name_TrailingPaddingStripped)
{
    auto w = build_type19(338000001u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "AURORA@@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->name(), "AURORA");
}

TEST_F(Msg19Test, Decode_Name_AllPadding_EmptyString)
{
    auto w = build_type19(338000002u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "@@@@@@@@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->name(), "");
}

TEST_F(Msg19Test, Decode_Name_MaxLength_NoStrip)
{
    // 20 non-padding characters: nothing stripped
    auto w = build_type19(338000003u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "ABCDEFGHIJKLMNOPQRST",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->name(), "ABCDEFGHIJKLMNOPQRST");
}

TEST_F(Msg19Test, Decode_Name_TrailingSpaceStripped)
{
    // Build a name that ends in spaces (space = '@' + 32 in 6-bit ASCII context
    // is actually encoded as the value for space char).  Use write_text which
    // handles the encoding.  We just pass a string with trailing spaces.
    auto w = build_type19(338000004u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "VESSEL NAME         ",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    // The name has trailing spaces stripped.
    EXPECT_EQ(msg->name(), "VESSEL NAME");
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, Longitude_Available_CorrectDegrees)
{
    // 600000 raw units = 1.0 degree
    auto w = build_type19(123456789u, 0u, 0u, false,
        600000, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "TEST@@@@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lon = msg->longitude();
    ASSERT_TRUE(lon.has_value());
    EXPECT_DOUBLE_EQ(lon.value(), 1.0);
}

TEST_F(Msg19Test, Longitude_Unavailable_NullOpt)
{
    auto w = build_type19(123456789u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "TEST@@@@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->longitude().has_value());
}

TEST_F(Msg19Test, Latitude_Available_CorrectDegrees)
{
    auto w = build_type19(123456789u, 0u, 0u, false,
        kLongitudeUnavailable, 600000,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "TEST@@@@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lat = msg->latitude();
    ASSERT_TRUE(lat.has_value());
    EXPECT_DOUBLE_EQ(lat.value(), 1.0);
}

TEST_F(Msg19Test, Latitude_Unavailable_NullOpt)
{
    auto w = build_type19(123456789u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "TEST@@@@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->latitude().has_value());
}

TEST_F(Msg19Test, Longitude_NegativeValue_CorrectDegrees)
{
    // -600000 raw = -1.0 degree
    auto w = build_type19(123456789u, 0u, 0u, false,
        -600000, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "TEST@@@@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    auto lon = msg->longitude();
    ASSERT_TRUE(lon.has_value());
    EXPECT_DOUBLE_EQ(lon.value(), -1.0);
}

// ---------------------------------------------------------------------------
// Flag fields
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, Flags_RaimDteAssigned_AllTrue)
{
    auto w = build_type19(111111111u, 1u, 0u, true,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "FLAGTEST@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, true, true, true);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_TRUE(msg->raim());
    EXPECT_TRUE(msg->dte());
    EXPECT_TRUE(msg->assigned());
    EXPECT_EQ(msg->repeat_indicator(), 1u);
}

TEST_F(Msg19Test, Flags_AllFalse)
{
    auto w = build_type19(222222222u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "FLAGTEST@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_FALSE(msg->raim());
    EXPECT_FALSE(msg->dte());
    EXPECT_FALSE(msg->assigned());
    EXPECT_FALSE(msg->position_accuracy());
}

// ---------------------------------------------------------------------------
// Dimension fields
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, Dimensions_MaxValues)
{
    auto w = build_type19(333333333u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "BIGSHIP@@@@@@@@@@@@@",
        0u,
        511u,   // max bow (9 bits)
        511u,   // max stern
        63u,    // max port (6 bits)
        63u,    // max starboard
        0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->dim_bow(), 511u);
    EXPECT_EQ(msg->dim_stern(), 511u);
    EXPECT_EQ(msg->dim_port(), 63u);
    EXPECT_EQ(msg->dim_starboard(), 63u);
}

TEST_F(Msg19Test, Dimensions_ZeroValues)
{
    auto w = build_type19(444444444u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "SMALLBOAT@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->dim_bow(), 0u);
    EXPECT_EQ(msg->dim_stern(), 0u);
    EXPECT_EQ(msg->dim_port(), 0u);
    EXPECT_EQ(msg->dim_starboard(), 0u);
}

// ---------------------------------------------------------------------------
// EPFD type
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, EpfdType_Gps_Value1)
{
    auto w = build_type19(555555555u, 0u, 0u, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "EPFDTEST@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 1u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->epfd_type(), 1u);
}

// ---------------------------------------------------------------------------
// SOG — unavailable sentinel
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, SogUnavailable_RawValue1023)
{
    auto w = build_type19(666666666u, 0u, kSogUnavailable, false,
        kLongitudeUnavailable, kLatitudeUnavailable,
        kCogUnavailable, kHeadingUnavailable, 60u, 0u,
        "SOGTEST@@@@@@@@@@@@@",
        0u, 0u, 0u, 0u, 0u, 0u, false, false, false);
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());
    auto* msg = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->sog(), kSogUnavailable);
}

// ---------------------------------------------------------------------------
// Encode / decode round-trip
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, RoundTrip_AllFields)
{
    auto w = build_type19(
        987654321u, 1u, 120u, true,
        -7200000, 3600000,   // -12.0, 6.0 degrees
        1234u, 270u, 30u, 5u,
        "ROUNDTRIP TEST     ",
        36u,    // passenger vessel
        120u, 80u, 12u, 10u,
        1u,     // GPS
        true, false, true
    );

    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_TRUE(r.ok());

    auto enc = r.value()->encode();
    ASSERT_TRUE(enc.ok()) << enc.error().message();

    auto buf = decode_payload(enc.value().first, enc.value().second);
    ASSERT_TRUE(buf.ok());

    BitReader reader2(buf.value());
    auto r2 = Msg19ExtendedClassBPositionReport::decode(reader2);
    ASSERT_TRUE(r2.ok());

    auto* m1 = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r.value().get());
    auto* m2 = dynamic_cast<Msg19ExtendedClassBPositionReport*>(r2.value().get());
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);

    EXPECT_EQ(m2->mmsi(), m1->mmsi());
    EXPECT_EQ(m2->repeat_indicator(), m1->repeat_indicator());
    EXPECT_EQ(m2->sog(), m1->sog());
    EXPECT_EQ(m2->position_accuracy(), m1->position_accuracy());
    EXPECT_EQ(m2->longitude_raw(), m1->longitude_raw());
    EXPECT_EQ(m2->latitude_raw(), m1->latitude_raw());
    EXPECT_EQ(m2->cog(), m1->cog());
    EXPECT_EQ(m2->true_heading(), m1->true_heading());
    EXPECT_EQ(m2->time_stamp(), m1->time_stamp());
    EXPECT_EQ(m2->regional_reserved(), m1->regional_reserved());
    EXPECT_EQ(m2->name(), m1->name());
    EXPECT_EQ(m2->ship_type(), m1->ship_type());
    EXPECT_EQ(m2->dim_bow(), m1->dim_bow());
    EXPECT_EQ(m2->dim_stern(), m1->dim_stern());
    EXPECT_EQ(m2->dim_port(), m1->dim_port());
    EXPECT_EQ(m2->dim_starboard(), m1->dim_starboard());
    EXPECT_EQ(m2->epfd_type(), m1->epfd_type());
    EXPECT_EQ(m2->raim(), m1->raim());
    EXPECT_EQ(m2->dte(), m1->dte());
    EXPECT_EQ(m2->assigned(), m1->assigned());
}

// ---------------------------------------------------------------------------
// Error paths
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, DecodeTooShort_ReturnsPayloadTooShort)
{
    // Only 6 bits — enough for the type field but not the full message.
    BitWriter w;
    ASSERT_TRUE(w.write_uint(19u, 6u).ok());
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadTooShort);
}

TEST_F(Msg19Test, DecodeWrongType_ReturnsDecodeFailure)
{
    // Build a 312-bit buffer with type ID 18 in the type field.
    BitWriter w(312u);
    ASSERT_TRUE(w.write_uint(18u, 6u).ok());
    for (int i = 0; i < 306; ++i) ASSERT_TRUE(w.write_uint(0u, 1u).ok());
    BitReader reader(w.buffer());
    auto r = Msg19ExtendedClassBPositionReport::decode(reader);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MessageDecodeFailure);
}

// ---------------------------------------------------------------------------
// Registry dispatch
// ---------------------------------------------------------------------------

TEST_F(Msg19Test, Registry_DispatchesType19)
{
    EXPECT_TRUE(MessageRegistry::instance().has_decoder(19u));
}