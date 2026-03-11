/**
 * @file test_message.cpp
 * @brief Unit tests for aislib::MessageType, aislib::MessageRegistry, and
 *        the message_type_from_id() helper.
 *
 * Phase 1 has no registered decoders, so every decode() call is expected to
 * return ErrorCode::MessageTypeUnsupported for valid type IDs in [1, 27] and
 * ErrorCode::MessageTypeUnknown for values outside that range.
 */

#include <gtest/gtest.h>
#include "aislib/message.h"
#include "aislib/payload.h"

#include <cstdint>
#include <vector>

using namespace aislib;

// ---------------------------------------------------------------------------
// MessageType enum – value correctness
// ---------------------------------------------------------------------------

TEST(MessageType, ValuesMatchITU_R_M1371) {
    // Numeric values are normative (ITU-R M.1371-5) and must not drift.
    EXPECT_EQ(static_cast<uint8_t>(MessageType::PositionReportClassA),           1u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::PositionReportClassA_Assigned),  2u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::PositionReportClassA_Response),  3u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::BaseStationReport),              4u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::StaticAndVoyageData),            5u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::BinaryAddressedMessage),         6u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::BinaryAcknowledge),              7u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::BinaryBroadcastMessage),         8u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::StandardSARPositionReport),      9u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::UTCDateInquiry),                10u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::UTCDateResponse),               11u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::AddressedSafetyMessage),        12u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::SafetyAcknowledge),             13u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::SafetyBroadcastMessage),        14u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::Interrogation),                 15u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::AssignmentModeCommand),         16u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::GNSSDGPSBroadcast),             17u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::StandardClassBPositionReport),  18u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::ExtendedClassBPositionReport),  19u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::DataLinkManagement),            20u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::AidToNavigationReport),         21u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::ChannelManagement),             22u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::GroupAssignmentCommand),        23u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::ClassBCSStaticDataReport),      24u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::SingleSlotBinaryMessage),       25u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::MultipleSlotBinaryMessage),     26u);
    EXPECT_EQ(static_cast<uint8_t>(MessageType::PositionReportLongRange),       27u);
}

TEST(MessageType, UnknownSentinelIsZero) {
    EXPECT_EQ(static_cast<uint8_t>(MessageType::Unknown), 0u);
}

// ---------------------------------------------------------------------------
// message_type_from_id
// ---------------------------------------------------------------------------

TEST(MessageTypeFromId, ValidRangeRoundTrips) {
    for (uint8_t i = 1; i <= 27; ++i) {
        MessageType mt = message_type_from_id(i);
        EXPECT_EQ(static_cast<uint8_t>(mt), i)
            << "message_type_from_id(" << static_cast<int>(i) << ") mismatch";
        EXPECT_NE(mt, MessageType::Unknown)
            << "message_type_from_id(" << static_cast<int>(i) << ") should not be Unknown";
    }
}

TEST(MessageTypeFromId, ZeroMapsToUnknown) {
    EXPECT_EQ(message_type_from_id(0), MessageType::Unknown);
}

TEST(MessageTypeFromId, TwentyEightMapsToUnknown) {
    EXPECT_EQ(message_type_from_id(28), MessageType::Unknown);
}

TEST(MessageTypeFromId, SixtyThreeMapsToUnknown) {
    EXPECT_EQ(message_type_from_id(63), MessageType::Unknown);
}

// ---------------------------------------------------------------------------
// MessageRegistry – Phase 1 stub behaviour
// ---------------------------------------------------------------------------

TEST(MessageRegistry, InstanceIsSingleton) {
    // Two calls to instance() must return the same object.
    EXPECT_EQ(&MessageRegistry::instance(), &MessageRegistry::instance());
}

TEST(MessageRegistry, NoDecodersRegisteredInPhase1) {
    // Phase 1 registers no decoders.  has_decoder() must return false for all
    // type IDs.
    MessageRegistry& reg = MessageRegistry::instance();
    for (uint8_t i = 0; i <= 27; ++i) {
        EXPECT_FALSE(reg.has_decoder(i))
            << "Unexpected decoder registered for type " << static_cast<int>(i);
    }
}

TEST(MessageRegistry, DecodeUnregisteredTypeReturnsUnsupported) {
    // With no decoders registered, every valid type ID must return
    // ErrorCode::MessageTypeUnsupported.
    //
    // Each iteration constructs a minimal payload whose first 6 bits encode
    // the specific type ID under test.  This ensures that each call to
    // decode() actually dispatches to a different type ID rather than
    // re-decoding the same payload 27 times.
    //
    // A minimal AIS payload for type `t` requires at least 6 bits.  We build
    // a single byte with the type ID packed into its high 6 bits, leaving the
    // low 2 bits as zero, then encode it via encode_payload.
    MessageRegistry& reg = MessageRegistry::instance();

    for (uint8_t t = 1; t <= 27; ++t) {
        // Pack type ID into the high 6 bits of a 1-byte buffer.
        const uint8_t byte0 = static_cast<uint8_t>(t << 2u);
        const std::vector<uint8_t> raw = { byte0 };

        // encode_payload with bit_length=6 produces exactly one armour
        // character representing the 6 bits of the type ID field.
        auto enc = encode_payload(raw, 6u);
        ASSERT_TRUE(static_cast<bool>(enc))
            << "encode_payload failed for type " << static_cast<int>(t);

        const std::string& armored = enc.value().first;
        const uint8_t      fill    = enc.value().second;

        auto r = reg.decode(armored, fill);
        EXPECT_FALSE(static_cast<bool>(r))
            << "Expected error for type " << static_cast<int>(t);
        if (!r) {
            EXPECT_EQ(r.error().code(), ErrorCode::MessageTypeUnsupported)
                << "Wrong error code for type " << static_cast<int>(t);
        }
    }
}

TEST(MessageRegistry, DecodeEmptyPayloadReturnsPayloadError) {
    MessageRegistry& reg = MessageRegistry::instance();
    auto r = reg.decode("", 0);
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadEmpty);
}

TEST(MessageRegistry, DecodeInvalidFillBitsReturnsPayloadError) {
    MessageRegistry& reg = MessageRegistry::instance();
    auto r = reg.decode("15M67J0000000000000000000000", 6); // fill_bits > 5
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadFillBitsInvalid);
}