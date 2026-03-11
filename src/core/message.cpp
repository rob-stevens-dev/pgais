#include "aislib/message.h"
#include "aislib/payload.h"

#include <cassert>

namespace aislib {

// =============================================================================
// message_type_from_id
// =============================================================================

MessageType message_type_from_id(uint8_t id) noexcept {
    if (id >= 1u && id <= 27u) {
        return static_cast<MessageType>(id);
    }
    return MessageType::Unknown;
}

// =============================================================================
// message_type_name
// =============================================================================

const char* message_type_name(MessageType type) noexcept {
    switch (type) {
        case MessageType::Unknown:                        return "Unknown";
        case MessageType::PositionReportClassA:           return "PositionReportClassA";
        case MessageType::PositionReportClassA_Assigned:  return "PositionReportClassA_Assigned";
        case MessageType::PositionReportClassA_Response:  return "PositionReportClassA_Response";
        case MessageType::BaseStationReport:              return "BaseStationReport";
        case MessageType::StaticAndVoyageData:            return "StaticAndVoyageData";
        case MessageType::BinaryAddressedMessage:         return "BinaryAddressedMessage";
        case MessageType::BinaryAcknowledge:              return "BinaryAcknowledge";
        case MessageType::BinaryBroadcastMessage:         return "BinaryBroadcastMessage";
        case MessageType::StandardSARPositionReport:      return "StandardSARPositionReport";
        case MessageType::UTCDateInquiry:                 return "UTCDateInquiry";
        case MessageType::UTCDateResponse:                return "UTCDateResponse";
        case MessageType::AddressedSafetyMessage:         return "AddressedSafetyMessage";
        case MessageType::SafetyAcknowledge:              return "SafetyAcknowledge";
        case MessageType::SafetyBroadcastMessage:         return "SafetyBroadcastMessage";
        case MessageType::Interrogation:                  return "Interrogation";
        case MessageType::AssignmentModeCommand:          return "AssignmentModeCommand";
        case MessageType::GNSSDGPSBroadcast:              return "GNSSDGPSBroadcast";
        case MessageType::StandardClassBPositionReport:   return "StandardClassBPositionReport";
        case MessageType::ExtendedClassBPositionReport:   return "ExtendedClassBPositionReport";
        case MessageType::DataLinkManagement:             return "DataLinkManagement";
        case MessageType::AidToNavigationReport:          return "AidToNavigationReport";
        case MessageType::ChannelManagement:              return "ChannelManagement";
        case MessageType::GroupAssignmentCommand:         return "GroupAssignmentCommand";
        case MessageType::ClassBCSStaticDataReport:       return "ClassBCSStaticDataReport";
        case MessageType::SingleSlotBinaryMessage:        return "SingleSlotBinaryMessage";
        case MessageType::MultipleSlotBinaryMessage:      return "MultipleSlotBinaryMessage";
        case MessageType::PositionReportLongRange:        return "PositionReportLongRange";
    }
    return "Unknown";
}

// =============================================================================
// MessageRegistry
// =============================================================================

MessageRegistry& MessageRegistry::instance() noexcept {
    // Construct on first use.  The registry lives for the lifetime of the
    // process.  No destruction ordering issues arise because decoders are
    // plain function pointers that do not own resources.
    static MessageRegistry registry;
    return registry;
}

void MessageRegistry::register_decoder(uint8_t type_id, DecoderFn fn) {
    // Fire in debug builds to catch registration mistakes at development time.
    // The range guard below is retained so release builds degrade gracefully
    // rather than writing out of bounds.
    assert(type_id >= 1u && type_id <= 27u &&
           "register_decoder: type_id must be in [1, 27]");
    if (type_id == 0u || type_id > 27u) {
        return; // Silently ignore out-of-range registrations in release builds.
    }
    decoders_[type_id] = fn;
}

bool MessageRegistry::has_decoder(uint8_t type_id) const noexcept {
    if (type_id == 0u || type_id > 27u) return false;
    return decoders_[type_id] != nullptr;
}

void MessageRegistry::clear() noexcept {
    for (auto& d : decoders_) {
        d = nullptr;
    }
}

Result<std::unique_ptr<Message>> MessageRegistry::decode(
    const std::string& payload,
    uint8_t            fill_bits) const
{
    // Step 1: unpack the armoured payload.
    auto payload_result = decode_payload(payload, fill_bits);
    if (!payload_result) {
        return Error{ payload_result.error() };
    }

    const std::vector<uint8_t>& bits = payload_result.value();

    // Step 2: read the 6-bit message type field from bits [0, 5].
    if (bits.empty() || bits.size() * 8u < 6u) {
        return Error{ ErrorCode::PayloadTooShort, "cannot read message type field" };
    }

    BitReader reader(bits);
    auto type_result = reader.read_uint(0u, 6u);
    if (!type_result) {
        return Error{ type_result.error() };
    }

    const uint8_t type_id = static_cast<uint8_t>(type_result.value());

    // Step 3: validate the type ID.
    if (type_id == 0u || type_id > 27u) {
        return Error{ ErrorCode::MessageTypeUnknown,
                      "type_id=" + std::to_string(type_id) };
    }

    // Step 4: look up and invoke the registered decoder.
    if (decoders_[type_id] == nullptr) {
        return Error{ ErrorCode::MessageTypeUnsupported,
                      message_type_name(message_type_from_id(type_id)) };
    }

    return decoders_[type_id](reader);
}

} // namespace aislib