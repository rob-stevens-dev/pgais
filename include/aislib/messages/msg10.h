#pragma once

#include "aislib/message.h"

#include <cstdint>
#include <string>

namespace aislib {

/**
 * @file messages/msg10.h
 * @brief AIS message type 10 — UTC/date inquiry.
 *
 * ITU-R M.1371-5, Section 3.3.10 defines message type 10 as a request
 * from a vessel to a specific destination station asking for the current
 * UTC date and time.  The destination station replies with message type 11
 * (UTC/date response, handled by Msg4_11BaseStationReport).
 *
 * The payload is 72 bits (12 armoured characters).  It is the shortest
 * non-trivial AIS message type in the standard.
 *
 * Bit layout (ITU-R M.1371-5 Table 39):
 *
 *   Bits  0–5  : Message type (6), fixed value 10
 *   Bits  6–7  : Repeat indicator (2)
 *   Bits  8–37 : MMSI of the inquiring station (30)
 *   Bits 38–39 : Spare (2)
 *   Bits 40–69 : MMSI of the destination station (30)
 *   Bits 70–71 : Spare (2)
 *
 * There are no position, time, or status fields.  The message merely
 * identifies the requesting MMSI and the destination MMSI.
 */

// ---------------------------------------------------------------------------
// Msg10UTCDateInquiry
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message type 10.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg10() registered with MessageRegistry.
 */
class Msg10UTCDateInquiry final : public Message {
public:
    /**
     * @brief Decodes a UTC/date inquiry from the given bit reader.
     *
     * The BitReader must be positioned at bit 0 of the fully assembled and
     * decoded payload.  The method reads exactly 72 bits.
     *
     * @param reader  A BitReader over the decoded payload bit stream.
     * @return        A decoded message object, or an Error.
     *
     * Possible errors: ErrorCode::PayloadTooShort, ErrorCode::MessageDecodeFailure.
     */
    [[nodiscard]] static Result<std::unique_ptr<Message>> decode(const BitReader& reader);

    /** @brief Returns MessageType::UTCDateInquiry. */
    [[nodiscard]] MessageType type() const noexcept override
    {
        return MessageType::UTCDateInquiry;
    }

    /**
     * @brief Encodes this message into an NMEA AIS armoured payload string.
     *
     * Produces a 72-bit payload with 0 fill bits.
     *
     * @return  Pair of (armoured payload, fill bits), or an Error.
     */
    [[nodiscard]] Result<std::pair<std::string, uint8_t>> encode() const override;

    // -----------------------------------------------------------------------
    // Field accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the MMSI of the destination station being queried.
     *
     * The destination station is expected to respond with a message type 11
     * (UTC/date response).
     */
    [[nodiscard]] uint32_t destination_mmsi() const noexcept { return destination_mmsi_; }

private:
    /**
     * @brief Private constructor used exclusively by decode().
     *
     * @param mmsi              MMSI of the inquiring station (30 bits).
     * @param repeat_indicator  Repeat indicator [0–3].
     * @param destination_mmsi  MMSI of the destination station (30 bits).
     */
    Msg10UTCDateInquiry(
        uint32_t mmsi,
        uint8_t  repeat_indicator,
        uint32_t destination_mmsi
    ) noexcept;

    uint32_t destination_mmsi_;
};

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

/**
 * @brief Decoder function for AIS message type 10.
 *
 * Satisfies the @c DecoderFn signature required by MessageRegistry.
 * Delegates directly to Msg10UTCDateInquiry::decode().
 *
 * @param reader  A BitReader positioned at bit 0 of the decoded payload.
 * @return        A decoded message object, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg10(const BitReader& reader);

} // namespace aislib