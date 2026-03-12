#pragma once

#include "aislib/message.h"

#include <cstdint>
#include <string>

namespace aislib {

/**
 * @file messages/msg05.h
 * @brief AIS message type 5 — Static and voyage related data.
 *
 * ITU-R M.1371-5, Section 3.3.5 defines message type 5 as a two-part message
 * carrying static and voyage-related information about a vessel.  The combined
 * payload is 426 bits spread across two NMEA sentences.  426 bits / 6 = 71
 * armour characters exactly, so the encoded form carries 0 fill bits.  The
 * SentenceAssembler reassembles the two parts before the decoder is called.
 *
 * Fields of interest:
 *
 *   IMO number: 1–999 999 999.  The value 0 indicates "not available".
 *
 *   Call sign: 7 characters, 6-bit ASCII padded with '@'.
 *
 *   Vessel name: 20 characters, 6-bit ASCII padded with '@'.
 *
 *   Ship type: 8-bit unsigned integer.  Values 0–99 per ITU-R M.1371-5
 *   Table 54.  The value 0 indicates "not available or no ship".
 *
 *   Dimensions: bow, stern, port, starboard offsets in metres from the
 *   AIS antenna reference point to each edge of the vessel.  Each field
 *   is 6 or 9 bits; a value of 0 in any dimension field means "not available".
 *
 *   EPFD type: 4-bit field, same coding as message type 4.
 *
 *   ETA: month (4 bits), day (5 bits), hour (5 bits), minute (6 bits).
 *   Values of 0 in any ETA component indicate "not available".
 *
 *   Draught: 8-bit unsigned integer in 1/10 metre units.  A value of 0
 *   means "not available".
 *
 *   Destination: 20 characters, 6-bit ASCII.
 *
 *   DTE: 1-bit Data Terminal Equipment flag (0 = available, 1 = not available).
 *
 *   Spare: 2 bits (bits 423–424), always zero.  The 2 spare bits bring the
 *   total to 426 bits (71 armour characters, 0 fill bits).
 */

// ---------------------------------------------------------------------------
// Msg5StaticAndVoyageData
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message type 5.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg5() registered with MessageRegistry.
 */
class Msg5StaticAndVoyageData final : public Message {
public:
    /**
     * @brief Decodes a static and voyage data message from the given bit reader.
     *
     * The BitReader must be positioned at bit 0 of the fully assembled and
     * decoded two-part payload.  The method reads exactly 426 bits.
     *
     * @param reader  A BitReader over the decoded payload bit stream.
     * @return        A decoded message object, or an Error.
     *
     * Possible errors: ErrorCode::PayloadTooShort, ErrorCode::MessageDecodeFailure.
     */
    [[nodiscard]] static Result<std::unique_ptr<Message>> decode(const BitReader& reader);

    /** @brief Returns MessageType::StaticAndVoyageData. */
    [[nodiscard]] MessageType type() const noexcept override
    {
        return MessageType::StaticAndVoyageData;
    }

    /**
     * @brief Encodes this message into an NMEA AIS armoured payload string.
     *
     * Produces a 426-bit payload encoded as 71 armour characters with 0 fill
     * bits (424 content bits + 2 spare bits = 426 = 71 * 6 exactly).
     *
     * Note: multi-part framing (splitting the payload across two NMEA
     * sentences) is the caller's responsibility using SentenceAssembler or
     * equivalent logic.
     *
     * @return  Pair of (armoured payload string, fill bits), or an Error.
     *          On success, fill bits will always be 0.
     */
    [[nodiscard]] Result<std::pair<std::string, uint8_t>> encode() const override;

    // -----------------------------------------------------------------------
    // Field accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the AIS version indicator [0–3].
     *
     * 0 = ITU-R M.1371-1; 1 = ITU-R M.1371-3; 2 = ITU-R M.1371-5;
     * 3 = reserved.
     */
    [[nodiscard]] uint8_t ais_version() const noexcept { return ais_version_; }

    /**
     * @brief Returns the IMO ship identification number [0–999999999].
     *
     * The value 0 means "not available".
     */
    [[nodiscard]] uint32_t imo_number() const noexcept { return imo_number_; }

    /**
     * @brief Returns the call sign string (up to 7 characters).
     *
     * Trailing '@' padding characters are stripped on decode.
     */
    [[nodiscard]] const std::string& call_sign() const noexcept { return call_sign_; }

    /**
     * @brief Returns the vessel name string (up to 20 characters).
     *
     * Trailing '@' padding characters and trailing spaces are stripped on
     * decode.
     */
    [[nodiscard]] const std::string& vessel_name() const noexcept { return vessel_name_; }

    /**
     * @brief Returns the ship and cargo type code [0–255].
     *
     * Refer to ITU-R M.1371-5 Table 54 for the full enumeration.  The value
     * 0 indicates "not available or no ship".
     */
    [[nodiscard]] uint8_t ship_type() const noexcept { return ship_type_; }

    /**
     * @brief Returns the bow dimension in metres [0–511].
     *
     * Distance from the AIS antenna reference point to the bow.  0 = unknown.
     */
    [[nodiscard]] uint16_t dim_to_bow() const noexcept { return dim_to_bow_; }

    /**
     * @brief Returns the stern dimension in metres [0–511].
     *
     * Distance from the AIS antenna reference point to the stern.  0 = unknown.
     */
    [[nodiscard]] uint16_t dim_to_stern() const noexcept { return dim_to_stern_; }

    /**
     * @brief Returns the port dimension in metres [0–63].
     *
     * Distance from the AIS antenna reference point to the port side.
     * 0 = unknown.
     */
    [[nodiscard]] uint8_t dim_to_port() const noexcept { return dim_to_port_; }

    /**
     * @brief Returns the starboard dimension in metres [0–63].
     *
     * Distance from the AIS antenna reference point to the starboard side.
     * 0 = unknown.
     */
    [[nodiscard]] uint8_t dim_to_starboard() const noexcept { return dim_to_starboard_; }

    /**
     * @brief Returns the EPFD (Electronic Position Fixing Device) type [0–15].
     *
     * Common values: 0 = undefined; 1 = GPS; 2 = GLONASS; 3 = combined
     * GPS/GLONASS; 4 = Loran-C; 5 = Chayka; 6 = integrated navigation system;
     * 7 = surveyed; 8 = Galileo.
     */
    [[nodiscard]] uint8_t epfd() const noexcept { return epfd_; }

    /**
     * @brief Returns the ETA month [1–12], or 0 when not available.
     */
    [[nodiscard]] uint8_t eta_month() const noexcept { return eta_month_; }

    /**
     * @brief Returns the ETA day [1–31], or 0 when not available.
     */
    [[nodiscard]] uint8_t eta_day() const noexcept { return eta_day_; }

    /**
     * @brief Returns the ETA hour [0–23], or 24 when not available.
     */
    [[nodiscard]] uint8_t eta_hour() const noexcept { return eta_hour_; }

    /**
     * @brief Returns the ETA minute [0–59], or 60 when not available.
     */
    [[nodiscard]] uint8_t eta_minute() const noexcept { return eta_minute_; }

    /**
     * @brief Returns the draught in 1/10 metre units [0–255].
     *
     * Divide by 10.0 for metres.  The value 0 means "not available".
     * Maximum draught is 25.5 metres (value 255).
     */
    [[nodiscard]] uint8_t draught() const noexcept { return draught_; }

    /**
     * @brief Returns the destination string (up to 20 characters).
     *
     * Trailing '@' padding characters and trailing spaces are stripped on
     * decode.
     */
    [[nodiscard]] const std::string& destination() const noexcept { return destination_; }

    /**
     * @brief Returns the Data Terminal Equipment (DTE) flag.
     *
     * false = DTE available; true = DTE not available.
     */
    [[nodiscard]] bool dte() const noexcept { return dte_; }

private:
    /**
     * @brief Constructs a fully decoded type 5 message.
     *
     * All string arguments must already have trailing padding stripped.
     * Only callable from the decode() factory.
     */
    Msg5StaticAndVoyageData(
        uint32_t    mmsi,
        uint8_t     repeat_indicator,
        uint8_t     ais_version,
        uint32_t    imo_number,
        std::string call_sign,
        std::string vessel_name,
        uint8_t     ship_type,
        uint16_t    dim_to_bow,
        uint16_t    dim_to_stern,
        uint8_t     dim_to_port,
        uint8_t     dim_to_starboard,
        uint8_t     epfd,
        uint8_t     eta_month,
        uint8_t     eta_day,
        uint8_t     eta_hour,
        uint8_t     eta_minute,
        uint8_t     draught,
        std::string destination,
        bool        dte
    ) noexcept;

    uint8_t     ais_version_{ 0 };
    uint32_t    imo_number_{ 0 };
    std::string call_sign_;
    std::string vessel_name_;
    uint8_t     ship_type_{ 0 };
    uint16_t    dim_to_bow_{ 0 };
    uint16_t    dim_to_stern_{ 0 };
    uint8_t     dim_to_port_{ 0 };
    uint8_t     dim_to_starboard_{ 0 };
    uint8_t     epfd_{ 0 };
    uint8_t     eta_month_{ 0 };
    uint8_t     eta_day_{ 0 };
    uint8_t     eta_hour_{ 0 };
    uint8_t     eta_minute_{ 0 };
    uint8_t     draught_{ 0 };
    std::string destination_;
    bool        dte_{ false };
};

/**
 * @brief Free decoder function for AIS message type 5.
 *
 * Conforms to the DecoderFn signature required by MessageRegistry.
 * Delegates to Msg5StaticAndVoyageData::decode().
 *
 * @param reader  A BitReader over the complete assembled payload bit stream.
 * @return        A decoded Msg5StaticAndVoyageData, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg5(const BitReader& reader);

} // namespace aislib