#pragma once

#include "aislib/message.h"

#include <cstdint>
#include <optional>
#include <string>

namespace aislib {

/**
 * @file messages/msg4_11.h
 * @brief AIS message types 4 and 11 — Base station report and UTC/date response.
 *
 * ITU-R M.1371-5 defines two message types with an identical 168-bit payload
 * layout:
 *
 *   Type  4 — Base station report (transmitted by fixed base stations)
 *   Type 11 — UTC/date response (reply to a UTC/date inquiry, type 10)
 *
 * Both types carry a UTC timestamp, the transmitting station's position, and
 * a RAIM/SOTDMA radio status field.  The @c type() accessor distinguishes
 * the two variants.
 *
 * Year, month, day, hour, minute, and second are transmitted as separate
 * unsigned integer fields.  The value 0 in any time component indicates
 * "not available".  A value of 0 for the year specifically means the year
 * is not available (years are transmitted in four digits, e.g. 2024).
 *
 * Coordinate conventions follow ITU-R M.1371-5 Table 44, identical to
 * types 1–3.  See msg1_3.h for sentinel value documentation.
 */

// ---------------------------------------------------------------------------
// Msg4_11BaseStationReport
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message types 4 and 11.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg4_11() registered with
 * MessageRegistry.
 */
class Msg4_11BaseStationReport final : public Message {
public:
    /**
     * @brief Decodes a base station report from the given bit reader.
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

    /** @brief Returns MessageType::BaseStationReport or MessageType::UTCDateResponse. */
    [[nodiscard]] MessageType type() const noexcept override { return type_; }

    /**
     * @brief Encodes this message into an NMEA AIS armoured payload string.
     *
     * Produces a 168-bit payload with 0 fill bits.
     *
     * @return  Pair of (armoured payload, fill bits), or an Error.
     */
    [[nodiscard]] Result<std::pair<std::string, uint8_t>> encode() const override;

    // -----------------------------------------------------------------------
    // Time fields
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the UTC year [1–9999], or 0 when not available.
     */
    [[nodiscard]] uint16_t utc_year() const noexcept { return utc_year_; }

    /**
     * @brief Returns the UTC month [1–12], or 0 when not available.
     */
    [[nodiscard]] uint8_t utc_month() const noexcept { return utc_month_; }

    /**
     * @brief Returns the UTC day [1–31], or 0 when not available.
     */
    [[nodiscard]] uint8_t utc_day() const noexcept { return utc_day_; }

    /**
     * @brief Returns the UTC hour [0–23], or 24 when not available.
     */
    [[nodiscard]] uint8_t utc_hour() const noexcept { return utc_hour_; }

    /**
     * @brief Returns the UTC minute [0–59], or 60 when not available.
     */
    [[nodiscard]] uint8_t utc_minute() const noexcept { return utc_minute_; }

    /**
     * @brief Returns the UTC second [0–59], or 60 when not available.
     */
    [[nodiscard]] uint8_t utc_second() const noexcept { return utc_second_; }

    // -----------------------------------------------------------------------
    // Position fields
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if the position was determined by DGNSS.
     */
    [[nodiscard]] bool position_accuracy() const noexcept { return position_accuracy_; }

    /**
     * @brief Returns the raw longitude in 1/10 000 minute units.
     *
     * Divide by 600 000.0 for decimal degrees.  The sentinel value
     * kLongitudeUnavailable indicates "not available".
     */
    [[nodiscard]] int32_t longitude_raw() const noexcept { return longitude_; }

    /**
     * @brief Returns the raw latitude in 1/10 000 minute units.
     *
     * Divide by 600 000.0 for decimal degrees.  The sentinel value
     * kLatitudeUnavailable indicates "not available".
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
     * @brief Returns the EPFD (Electronic Position Fixing Device) type [0–15].
     *
     * Common values: 0 = undefined; 1 = GPS; 2 = GLONASS; 3 = combined
     * GPS/GLONASS; 4 = Loran-C; 5 = Chayka; 6 = integrated navigation system;
     * 7 = surveyed; 8 = Galileo.
     */
    [[nodiscard]] uint8_t epfd() const noexcept { return epfd_; }

    /**
     * @brief Returns true if the RAIM flag is set.
     */
    [[nodiscard]] bool raim() const noexcept { return raim_; }

    /**
     * @brief Returns the 19-bit SOTDMA radio status field.
     */
    [[nodiscard]] uint32_t radio_status() const noexcept { return radio_status_; }

private:
    /**
     * @brief Private constructor used exclusively by decode().
     */
    explicit Msg4_11BaseStationReport(
        MessageType type,
        uint32_t    mmsi,
        uint8_t     repeat_indicator,
        uint16_t    utc_year,
        uint8_t     utc_month,
        uint8_t     utc_day,
        uint8_t     utc_hour,
        uint8_t     utc_minute,
        uint8_t     utc_second,
        bool        position_accuracy,
        int32_t     longitude,
        int32_t     latitude,
        uint8_t     epfd,
        bool        raim,
        uint32_t    radio_status
    ) noexcept;

    MessageType type_;
    uint16_t    utc_year_;
    uint8_t     utc_month_;
    uint8_t     utc_day_;
    uint8_t     utc_hour_;
    uint8_t     utc_minute_;
    uint8_t     utc_second_;
    bool        position_accuracy_;
    int32_t     longitude_;
    int32_t     latitude_;
    uint8_t     epfd_;
    bool        raim_;
    uint32_t    radio_status_;
};

// ---------------------------------------------------------------------------
// Decoder function — registered with MessageRegistry
// ---------------------------------------------------------------------------

/**
 * @brief Decoder function for AIS message types 4 and 11.
 *
 * Registered with MessageRegistry::register_decoder() for type IDs 4 and 11
 * during library initialisation.
 *
 * @param reader  BitReader positioned at the start of the decoded payload.
 * @return        Decoded message, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg4_11(const BitReader& reader);

} // namespace aislib
