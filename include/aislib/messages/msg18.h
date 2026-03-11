#pragma once

#include "aislib/message.h"
#include "aislib/messages/msg1_3.h"  // sentinel constants

#include <cstdint>
#include <optional>
#include <string>

namespace aislib {

/**
 * @file messages/msg18.h
 * @brief AIS message type 18 — Standard Class B CS position report.
 *
 * ITU-R M.1371-5, Section 3.3.18 defines message type 18 as the position
 * report transmitted by Class B SOTDMA (Carrier-Sense TDMA) equipment.
 * The payload is 168 bits.
 *
 * Class B equipment differs from Class A in several respects:
 *
 *   - No navigation status field (always "not available" for Class B).
 *   - No rate of turn field.
 *   - SOG resolution is 1/10 knot, same as Class A.
 *   - COG and heading fields are present.
 *   - A CS (Carrier-Sense) flag indicates Class B CS capability.
 *   - A display flag indicates whether the unit has a display.
 *   - A DSC flag indicates whether the unit has an integrated DSC function.
 *   - A band flag indicates whether the unit can operate on the second channel.
 *   - A message 22 flag indicates whether channel management is applied.
 *   - An assigned mode flag: 0 = autonomous, 1 = assigned.
 *
 * Coordinate conventions, SOG, COG, and heading sentinel values follow
 * ITU-R M.1371-5 Table 44 and are identical to message types 1–3.
 * Refer to msg1_3.h for the sentinel constant definitions.
 */

// ---------------------------------------------------------------------------
// Msg18ClassBPositionReport
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message type 18.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg18() registered with MessageRegistry.
 */
class Msg18ClassBPositionReport final : public Message {
public:
    /**
     * @brief Decodes a Class B position report from the given bit reader.
     *
     * The BitReader must be positioned at bit 0 of the fully assembled and
     * decoded payload.  The method reads exactly 168 bits.
     *
     * @param reader  A BitReader over the decoded payload bit stream.
     * @return        A decoded message object, or an Error.
     *
     * Possible errors: ErrorCode::PayloadTooShort, ErrorCode::MessageDecodeFailure.
     */
    [[nodiscard]] static Result<std::unique_ptr<Message>> decode(const BitReader& reader);

    /** @brief Returns MessageType::StandardClassBPositionReport. */
    [[nodiscard]] MessageType type() const noexcept override
    {
        return MessageType::StandardClassBPositionReport;
    }

    /**
     * @brief Encodes this message into an NMEA AIS armoured payload string.
     *
     * Produces a 168-bit payload with 0 fill bits.
     *
     * @return  Pair of (armoured payload, fill bits), or an Error.
     */
    [[nodiscard]] Result<std::pair<std::string, uint8_t>> encode() const override;

    // -----------------------------------------------------------------------
    // Field accessors — raw values in ITU-R M.1371-5 units
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the Speed Over Ground (SOG) in 1/10 knot units [0–1023].
     *
     * The value 1023 means "not available".
     */
    [[nodiscard]] uint16_t sog() const noexcept { return sog_; }

    /**
     * @brief Returns true if the position was determined by DGNSS.
     */
    [[nodiscard]] bool position_accuracy() const noexcept { return position_accuracy_; }

    /**
     * @brief Returns the raw longitude in 1/10 000 minute units.
     *
     * Divide by 600 000.0 for decimal degrees.
     * The sentinel value kLongitudeUnavailable indicates "not available".
     */
    [[nodiscard]] int32_t longitude_raw() const noexcept { return longitude_; }

    /**
     * @brief Returns the raw latitude in 1/10 000 minute units.
     *
     * Divide by 600 000.0 for decimal degrees.
     * The sentinel value kLatitudeUnavailable indicates "not available".
     */
    [[nodiscard]] int32_t latitude_raw() const noexcept { return latitude_; }

    /**
     * @brief Returns longitude in decimal degrees, or std::nullopt when unavailable.
     */
    [[nodiscard]] std::optional<double> longitude() const noexcept;

    /**
     * @brief Returns latitude in decimal degrees, or std::nullopt when unavailable.
     */
    [[nodiscard]] std::optional<double> latitude() const noexcept;

    /**
     * @brief Returns the Course Over Ground (COG) in 1/10 degree units [0–3600].
     *
     * The value 3600 means "not available".
     */
    [[nodiscard]] uint16_t cog() const noexcept { return cog_; }

    /**
     * @brief Returns the True Heading in degrees [0–359, or 511 when not available].
     */
    [[nodiscard]] uint16_t true_heading() const noexcept { return true_heading_; }

    /**
     * @brief Returns the UTC second time stamp [0–63].
     *
     * Values 0–59: UTC second at which the position was obtained.
     * 60 = not available; 61 = manual input; 62 = dead reckoning; 63 = inoperative.
     */
    [[nodiscard]] uint8_t time_stamp() const noexcept { return time_stamp_; }

    /**
     * @brief Returns the regional reserved bits [0–3].
     *
     * These two bits are reserved for regional use and their interpretation
     * is not defined by the base standard.
     */
    [[nodiscard]] uint8_t regional_reserved() const noexcept { return regional_reserved_; }

    /**
     * @brief Returns the CS (Carrier-Sense) mode flag.
     *
     * true = Class B SOTDMA unit.  false = Class B CS unit in non-SOTDMA mode.
     */
    [[nodiscard]] bool cs_flag() const noexcept { return cs_flag_; }

    /**
     * @brief Returns the display flag.
     *
     * true = the equipment has a display capable of displaying an AIS target
     * list; false = no such display.
     */
    [[nodiscard]] bool display_flag() const noexcept { return display_flag_; }

    /**
     * @brief Returns the DSC (Digital Selective Calling) flag.
     *
     * true = the unit has a DSC function; false = no DSC function.
     */
    [[nodiscard]] bool dsc_flag() const noexcept { return dsc_flag_; }

    /**
     * @brief Returns the band flag.
     *
     * true = capable of operating over the whole marine band;
     * false = capable of operating on the AIS channels only.
     */
    [[nodiscard]] bool band_flag() const noexcept { return band_flag_; }

    /**
     * @brief Returns the message 22 flag.
     *
     * true = frequency management via message 22 (channel management) is applied;
     * false = no frequency management applied.
     */
    [[nodiscard]] bool msg22_flag() const noexcept { return msg22_flag_; }

    /**
     * @brief Returns the assigned mode flag.
     *
     * false = autonomous and continuous mode; true = assigned mode.
     */
    [[nodiscard]] bool assigned() const noexcept { return assigned_; }

    /**
     * @brief Returns true if the RAIM flag is set.
     */
    [[nodiscard]] bool raim() const noexcept { return raim_; }

    /**
     * @brief Returns the 20-bit radio status field.
     *
     * Contains the SOTDMA communication state as defined in
     * ITU-R M.1371-5 Annex 2.
     */
    [[nodiscard]] uint32_t radio_status() const noexcept { return radio_status_; }

private:
    /**
     * @brief Private constructor used exclusively by decode().
     */
    explicit Msg18ClassBPositionReport(
        uint32_t mmsi,
        uint8_t  repeat_indicator,
        uint16_t sog,
        bool     position_accuracy,
        int32_t  longitude,
        int32_t  latitude,
        uint16_t cog,
        uint16_t true_heading,
        uint8_t  time_stamp,
        uint8_t  regional_reserved,
        bool     cs_flag,
        bool     display_flag,
        bool     dsc_flag,
        bool     band_flag,
        bool     msg22_flag,
        bool     assigned,
        bool     raim,
        uint32_t radio_status
    ) noexcept;

    uint16_t sog_;
    bool     position_accuracy_;
    int32_t  longitude_;
    int32_t  latitude_;
    uint16_t cog_;
    uint16_t true_heading_;
    uint8_t  time_stamp_;
    uint8_t  regional_reserved_;
    bool     cs_flag_;
    bool     display_flag_;
    bool     dsc_flag_;
    bool     band_flag_;
    bool     msg22_flag_;
    bool     assigned_;
    bool     raim_;
    uint32_t radio_status_;
};

// ---------------------------------------------------------------------------
// Decoder function — registered with MessageRegistry
// ---------------------------------------------------------------------------

/**
 * @brief Decoder function for AIS message type 18.
 *
 * Registered with MessageRegistry::register_decoder() for type ID 18 during
 * library initialisation.
 *
 * @param reader  BitReader positioned at the start of the decoded payload.
 * @return        Decoded message, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg18(const BitReader& reader);

} // namespace aislib
