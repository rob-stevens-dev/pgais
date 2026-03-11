/**
 * @file test_message.cpp
 * @brief Unit tests for MessageType, message_type_from_id, and MessageRegistry.
 *
 * The MessageRegistry is a process-lifetime singleton.  Phase 2 and later test
 * fixtures call register_all_decoders() in their SetUp() methods, which means
 * that by the time this translation unit's tests run the registry may already
 * be populated.  Any test that requires the registry to be in a specific state
 * must use the RegistryIsolation fixture, which clears the registry in SetUp()
 * and restores it to a fully-registered state in TearDown().
 *
 * Tests that do not interact with the registry state are plain TEST() cases
 * and carry no fixture overhead.
 */

#include <gtest/gtest.h>

#include "aislib/error.h"
#include "aislib/message.h"
#include "aislib/payload.h"
#include "aislib/registry_init.h"

using namespace aislib;

// =============================================================================
// Fixture: RegistryIsolation
//
// Clears the registry before each test and restores it after.  Use for any
// test that makes assertions about which decoders are or are not registered.
// =============================================================================

/**
 * @brief Test fixture that provides an isolated MessageRegistry state.
 *
 * SetUp() clears all registered decoders so that the test starts with an
 * empty registry.  TearDown() calls register_all_decoders() to restore the
 * singleton to the fully-populated state expected by other test suites that
 * share this process.
 *
 * Do not use this fixture for tests that merely decode a payload; use it only
 * when the test specifically asserts on registration state.
 */
class RegistryIsolation : public ::testing::Test {
protected:
    void SetUp() override
    {
        MessageRegistry::instance().clear();
    }

    void TearDown() override
    {
        // Restore decoders so that subsequent test suites in this process see
        // a fully initialised registry, matching what register_all_decoders()
        // would have installed.
        register_all_decoders();
    }
};

// =============================================================================
// MessageType enum values
// =============================================================================

TEST(MessageType, NumericValuesMatchSpec) {
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

// =============================================================================
// message_type_from_id
// =============================================================================

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

// =============================================================================
// MessageRegistry — singleton identity
// =============================================================================

TEST(MessageRegistry, InstanceIsSingleton) {
    EXPECT_EQ(&MessageRegistry::instance(), &MessageRegistry::instance());
}

// =============================================================================
// MessageRegistry — registration state (uses RegistryIsolation fixture)
// =============================================================================

TEST_F(RegistryIsolation, NoDecoders_HasDecoderReturnsFalseForAllTypes) {
    // After clear(), no type ID should have a decoder registered.
    MessageRegistry& reg = MessageRegistry::instance();
    for (uint8_t i = 0; i <= 27; ++i) {
        EXPECT_FALSE(reg.has_decoder(i))
            << "Unexpected decoder present for type " << static_cast<int>(i);
    }
}

TEST_F(RegistryIsolation, NoDecoders_DecodeReturnsUnsupported) {
    // With an empty registry, every valid type ID must return
    // ErrorCode::MessageTypeUnsupported.
    //
    // Each iteration constructs a minimal payload whose first 6 bits encode
    // the type ID under test, ensuring that decode() dispatches to a distinct
    // type ID rather than re-decoding the same payload 27 times.
    MessageRegistry& reg = MessageRegistry::instance();

    for (uint8_t t = 1; t <= 27; ++t) {
        // Pack type ID into the high 6 bits of a 1-byte buffer.
        const uint8_t byte0 = static_cast<uint8_t>(t << 2u);
        const std::vector<uint8_t> raw = { byte0 };

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

TEST_F(RegistryIsolation, RegisterDecoder_HasDecoderReturnsTrue) {
    // Verify that registering a stub decoder makes has_decoder() return true
    // for that type ID and only that type ID.
    MessageRegistry& reg = MessageRegistry::instance();

    auto stub = [](const BitReader&) -> Result<std::unique_ptr<Message>> {
        return Error{ ErrorCode::MessageTypeUnsupported };
    };

    reg.register_decoder(1u, stub);
    EXPECT_TRUE(reg.has_decoder(1u));
    EXPECT_FALSE(reg.has_decoder(2u));
}

// =============================================================================
// MessageRegistry — payload error paths (no fixture needed — these fail before
// dispatch and are independent of whether decoders are registered)
// =============================================================================

TEST(MessageRegistry, DecodeEmptyPayloadReturnsPayloadError) {
    MessageRegistry& reg = MessageRegistry::instance();
    auto r = reg.decode("", 0);
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadEmpty);
}

TEST(MessageRegistry, DecodeInvalidFillBitsReturnsPayloadError) {
    MessageRegistry& reg = MessageRegistry::instance();
    // fill_bits > 5 is always invalid regardless of registration state.
    auto r = reg.decode("15M67J0000000000000000000000", 6);
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadFillBitsInvalid);
}