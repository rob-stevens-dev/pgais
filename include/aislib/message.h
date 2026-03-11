#pragma once

#include "error.h"
#include "payload.h"
#include "bitfield.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace aislib {

/**
 * @file message.h
 * @brief Base AIS message type, message type enumeration, and factory interface.
 *
 * All decoded AIS messages derive from the Message base class.  Message
 * subclasses are introduced in later phases (one phase per message type group).
 * This header establishes the base interface and the factory lookup mechanism
 * so that the remainder of Phase 1 can be assembled and tested without any
 * concrete message implementations.
 *
 * The factory is a two-step operation:
 *   1. Inspect the first 6 bits of the decoded payload to determine the message
 *      type ID.
 *   2. Dispatch to the registered decoder for that type, which returns a
 *      heap-allocated Message subclass via std::unique_ptr.
 *
 * Decoder functions are registered per message type ID using
 * MessageRegistry::register_decoder().  An unregistered type ID causes the
 * factory to return ErrorCode::MessageTypeUnsupported rather than
 * ErrorCode::MessageTypeUnknown, so callers can distinguish "unknown protocol"
 * from "known but not yet implemented".
 */

// -----------------------------------------------------------------------------
// MessageType
// -----------------------------------------------------------------------------

/**
 * @brief Enumeration of all 27 AIS message types defined in ITU-R M.1371-5.
 *
 * The numeric values correspond directly to the 6-bit message type ID field
 * at the start of every AIS payload.  Unknown or reserved values are
 * represented by the @c Unknown enumerator.
 */
enum class MessageType : uint8_t {
    Unknown                        =  0, ///< Unrecognised or reserved type.

    PositionReportClassA           =  1, ///< Scheduled position report.
    PositionReportClassA_Assigned  =  2, ///< Assigned scheduled position report.
    PositionReportClassA_Response  =  3, ///< Special position report (response to interrogation).
    BaseStationReport              =  4, ///< Base station report.
    StaticAndVoyageData            =  5, ///< Static and voyage related data.
    BinaryAddressedMessage         =  6, ///< Binary addressed message.
    BinaryAcknowledge              =  7, ///< Binary acknowledge.
    BinaryBroadcastMessage         =  8, ///< Binary broadcast message.
    StandardSARPositionReport      =  9, ///< Standard SAR aircraft position report.
    UTCDateInquiry                 = 10, ///< UTC/date inquiry.
    UTCDateResponse                = 11, ///< UTC/date response.
    AddressedSafetyMessage         = 12, ///< Addressed safety related message.
    SafetyAcknowledge              = 13, ///< Safety related acknowledge.
    SafetyBroadcastMessage         = 14, ///< Safety related broadcast message.
    Interrogation                  = 15, ///< Interrogation.
    AssignmentModeCommand          = 16, ///< Assignment mode command.
    GNSSDGPSBroadcast              = 17, ///< GNSS binary broadcast message.
    StandardClassBPositionReport   = 18, ///< Standard Class B CS position report.
    ExtendedClassBPositionReport   = 19, ///< Extended Class B CS position report.
    DataLinkManagement             = 20, ///< Data link management message.
    AidToNavigationReport          = 21, ///< Aid-to-navigation report.
    ChannelManagement              = 22, ///< Channel management.
    GroupAssignmentCommand         = 23, ///< Group assignment command.
    ClassBCSStaticDataReport       = 24, ///< Class B CS static data report.
    SingleSlotBinaryMessage        = 25, ///< Single slot binary message.
    MultipleSlotBinaryMessage      = 26, ///< Multiple slot binary message with communications state.
    PositionReportLongRange        = 27, ///< Long-range AIS broadcast message.
};

/**
 * @brief Converts a raw uint8_t message type ID to the MessageType enumeration.
 *
 * Values outside [1, 27] are mapped to MessageType::Unknown.
 *
 * @param id  The raw 6-bit type field value from the AIS payload.
 * @return    The corresponding MessageType.
 */
[[nodiscard]] MessageType message_type_from_id(uint8_t id) noexcept;

/**
 * @brief Returns a short human-readable name for a MessageType.
 *
 * The returned string is suitable for logging and diagnostics.  For example,
 * MessageType::PositionReportClassA returns "PositionReportClassA".
 *
 * @param type  The MessageType to name.
 * @return      A null-terminated string literal.
 */
[[nodiscard]] const char* message_type_name(MessageType type) noexcept;

// -----------------------------------------------------------------------------
// Message (base)
// -----------------------------------------------------------------------------

/**
 * @brief Abstract base class for all decoded AIS messages.
 *
 * Every concrete message type (derived in later phases) must override
 * @c type() to return its MessageType enumerator.  The base class stores the
 * MMSI and a reference to the raw bit stream for subclass use during decoding.
 *
 * Ownership model: decoded messages are returned as std::unique_ptr<Message>
 * from the factory.  Subclasses should not expose raw pointer constructors.
 */
class Message {
public:
    virtual ~Message() = default;

    // Non-copyable, movable
    Message(const Message&)            = delete;
    Message& operator=(const Message&) = delete;
    Message(Message&&)                 = default;
    Message& operator=(Message&&)      = default;

    /**
     * @brief Returns the MessageType of this message.
     *
     * Each concrete subclass overrides this to return its specific type.
     */
    [[nodiscard]] virtual MessageType type() const noexcept = 0;

    /**
     * @brief Returns the Maritime Mobile Service Identity (MMSI).
     *
     * The MMSI is a 30-bit unsigned integer present in all message types at
     * bits [8, 37] of the decoded payload.  It uniquely identifies the
     * transmitting station.
     */
    [[nodiscard]] uint32_t mmsi() const noexcept { return mmsi_; }

    /**
     * @brief Returns the repeat indicator [0-3].
     *
     * The repeat indicator specifies the number of times this message has been
     * repeated.  A value of 3 indicates "do not repeat further".
     */
    [[nodiscard]] uint8_t repeat_indicator() const noexcept { return repeat_indicator_; }

protected:
    /**
     * @brief Constructs the base message fields.
     * @param mmsi              The MMSI from bits [8, 37].
     * @param repeat_indicator  The repeat indicator from bits [6, 7].
     */
    explicit Message(uint32_t mmsi, uint8_t repeat_indicator) noexcept
        : mmsi_(mmsi), repeat_indicator_(repeat_indicator) {}

private:
    uint32_t mmsi_{ 0 };
    uint8_t  repeat_indicator_{ 0 };
};

// -----------------------------------------------------------------------------
// Decoder function signature
// -----------------------------------------------------------------------------

/**
 * @brief Function signature for a message type decoder.
 *
 * A decoder receives a BitReader positioned over the complete decoded payload
 * and must return either a heap-allocated Message subclass or an Error.
 *
 * Decoders are registered by type ID in MessageRegistry.
 */
using DecoderFn = Result<std::unique_ptr<Message>>(*)(const BitReader&);

// -----------------------------------------------------------------------------
// MessageRegistry
// -----------------------------------------------------------------------------

/**
 * @brief Central registry mapping message type IDs to their decoder functions.
 *
 * The registry is a process-lifetime singleton.  Decoders are registered once
 * at program start (or library load) and remain registered for the lifetime of
 * the process.
 *
 * MessageRegistry is not itself thread-safe during the registration phase.
 * All register_decoder() calls must complete before any decode() calls are
 * made from multiple threads.  decode() is safe for concurrent read access
 * once registration is complete.
 */
class MessageRegistry {
public:
    /**
     * @brief Returns the process-lifetime singleton instance.
     */
    [[nodiscard]] static MessageRegistry& instance() noexcept;

    /**
     * @brief Registers a decoder function for a specific message type ID.
     *
     * Registering a decoder for an already-registered type replaces the
     * previous decoder.  This allows later phases to replace stub decoders
     * with full implementations.
     *
     * @param type_id  The message type ID in [1, 27].
     * @param fn       The decoder function to register.
     */
    void register_decoder(uint8_t type_id, DecoderFn fn);

    /**
     * @brief Decodes a complete assembled message payload.
     *
     * The method:
     *   1. Calls decode_payload() to unpack the armoured string.
     *   2. Reads the 6-bit message type field from bits [0, 5].
     *   3. Looks up the registered decoder for that type.
     *   4. Invokes the decoder and returns its result.
     *
     * @param assembled  The assembled message carrying the concatenated payload
     *                   and fill bit count.
     * @return           A unique_ptr<Message> on success, or an Error.
     *
     * Possible errors: all PayloadXxx errors from decode_payload(), plus
     * ErrorCode::MessageTypeUnknown and ErrorCode::MessageTypeUnsupported.
     */
    [[nodiscard]] Result<std::unique_ptr<Message>> decode(
        const std::string& payload,
        uint8_t            fill_bits) const;

    /**
     * @brief Returns true if a decoder is registered for the given type ID.
     * @param type_id  The message type ID to query.
     */
    [[nodiscard]] bool has_decoder(uint8_t type_id) const noexcept;

    /**
     * @brief Removes all registered decoders.
     *
     * Intended for use in test fixtures that need a clean registry state.
     * Not safe to call while other threads may be calling decode().
     */
    void clear() noexcept;

private:
    MessageRegistry() = default;

    /**
     * @brief Sparse array of decoder function pointers indexed by type ID.
     *
     * Index 0 is unused.  Null entries indicate no decoder is registered.
     */
    DecoderFn decoders_[28]{ nullptr };
};

} // namespace aislib