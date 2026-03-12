#pragma once

#include "aislib/message.h"
#include "aislib/messages/msg1_3.h"  // sentinel constants

#include <cstdint>
#include <optional>
#include <string>

namespace aislib {

/**
 * @file messages/msg09.h
 * @brief AIS message type 9 — Standard SAR aircraft position report.
 *
 * ITU-R M.1371-5, Section 3.3.9 defines message type 9 as the position
 * report transmitted by Search and Rescue (SAR) aircraft transponders.
 * The payload is 168 bits, matching the Class A position report length,
 * but the field layout differs significantly.
 *
 * Bit layout (ITU-R M.1371-5 Table 38):
 *
 *   Bits   0–5   : Message type (6), fixed value 9
 *   Bits   6–7   : Repeat indicator (2)
 *   Bits   8–37  : MMSI (30)
 *   Bits  38–49  : Altitude (12, metres, MSL; 4095 = not available)
 *   Bits  50–59  : Speed over ground (10, knots; 1023 = not available)
 *   Bit   60     : Position accuracy (1)
 *   Bits  61–88  : Longitude (28, signed, 1/10 000 min)
 *   Bits  89–115 : Latitude (27, signed, 1/10 000 min)
 *   Bits 116–127 : Course over ground (12, 1/10 degree)
 *   Bits 128–133 : Time stamp (6)
 *   Bits 134–139 : Regional reserved (6)
 *   Bit  140     : DTE flag (1)
 *   Bits 141–145 : Spare (5)
 *   Bit  146     : Assigned mode flag (1)
 *   Bit  147     : RAIM flag (1)
 *   Bits 148–167 : Radio status (20)
 *
 * Key differences from Class A (types 1–3):
 *
 *   - No navigation status field.
 *   - No rate of turn (ROT) field.
 *   - Altitude replaces the ROT and navigation status bytes.
 *   - SOG is in knots (not 1/10 knot) and maximum meaningful value is 1022.
 *   - Longitude occupies bits 61–88 (not 57–84 as in type 1).
 *   - A DTE flag is present.
 *   - No manoeuvre indicator field.
 *
 * Altitude is in metres above MSL.  The value 4095 means "not available".
 * Values 0–4094 are valid.
 *
 * Speed over ground uses whole-knot steps for aircraft.  The value 1023
 * means "not available".
 *
 * Coordinate conventions follow ITU-R M.1371-5 Table 44.  Refer to
 * msg1_3.h for the sentinel constant definitions.
 */

// ---------------------------------------------------------------------------
// Sentinel values specific to type 9
// ---------------------------------------------------------------------------

/** Altitude "not available" raw value (metres MSL). */
inline constexpr uint16_t kAltitudeUnavailable = 4095u;

/** SAR SOG "not available" raw value (whole knots). */
inline constexpr uint16_t kSarSogUnavailable = 1023u;

// ---------------------------------------------------------------------------
// Msg9SARPositionReport
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message type 9.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg9() registered with MessageRegistry.
 */
class Msg9SARPositionReport final : public Message {
public:
    /**
     * @brief Decodes a SAR aircraft position report from the given bit reader.
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

    /** @brief Returns MessageType::StandardSARPositionReport. */
    [[nodiscard]] MessageType type() const noexcept override
    {
        return MessageType::StandardSARPositionReport;
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
     * @brief Returns the altitude in metres above MSL [0–4094].
     *
     * The value 4095 means "not available".  Use altitude_m() for a
     * decoded optional value.
     */
    [[nodiscard]] uint16_t altitude() const noexcept { return altitude_; }

    /**
     * @brief Returns the altitude in metres, or std::nullopt when unavailable.
     */
    [[nodiscard]] std::optional<uint16_t> altitude_m() const noexcept;

    /**
     * @brief Returns the Speed Over Ground (SOG) in whole knots [0–1022].
     *
     * The value 1023 means "not available".  Note that SAR SOG uses
     * whole-knot resolution, unlike the 1/10-knot resolution of Class A.
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
     * @brief Returns the UTC second time stamp [0–63].
     *
     * Values 0–59: UTC second at which the position was obtained.
     * 60 = not available; 61 = manual input; 62 = dead reckoning; 63 = inoperative.
     */
    [[nodiscard]] uint8_t time_stamp() const noexcept { return time_stamp_; }

    /**
     * @brief Returns the regional reserved bits [0–63].
     *
     * These six bits are reserved for regional use.
     */
    [[nodiscard]] uint8_t regional_reserved() const noexcept { return regional_reserved_; }

    /**
     * @brief Returns true if Data Terminal Equipment is not ready (DTE = 1).
     *
     * false = DTE available; true = DTE not available.
     */
    [[nodiscard]] bool dte() const noexcept { return dte_; }

    /**
     * @brief Returns the assigned mode flag.
     *
     * false = autonomous and continuous mode; true = assigned mode.
     */
    [[nodiscard]] bool assigned() const noexcept { return assigned_; }

    /**
     * @brief Returns true if the RAIM (Receiver Autonomous Integrity Monitoring) flag is set.
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
     *
     * @param mmsi              Maritime Mobile Service Identity (30 bits).
     * @param repeat_indicator  Repeat indicator [0–3].
     * @param altitude          Altitude in metres MSL [0–4095].
     * @param sog               Speed over ground in whole knots [0–1023].
     * @param position_accuracy True if position is DGNSS-derived.
     * @param longitude         Raw longitude in 1/10 000 minute units.
     * @param latitude          Raw latitude in 1/10 000 minute units.
     * @param cog               Course over ground in 1/10 degree units.
     * @param time_stamp        UTC second time stamp.
     * @param regional_reserved Regional reserved bits (6 bits).
     * @param dte               DTE flag.
     * @param assigned          Assigned mode flag.
     * @param raim              RAIM flag.
     * @param radio_status      20-bit radio status field.
     */
    Msg9SARPositionReport(
        uint32_t mmsi,
        uint8_t  repeat_indicator,
        uint16_t altitude,
        uint16_t sog,
        bool     position_accuracy,
        int32_t  longitude,
        int32_t  latitude,
        uint16_t cog,
        uint8_t  time_stamp,
        uint8_t  regional_reserved,
        bool     dte,
        bool     assigned,
        bool     raim,
        uint32_t radio_status
    ) noexcept;

    uint16_t altitude_;
    uint16_t sog_;
    bool     position_accuracy_;
    int32_t  longitude_;
    int32_t  latitude_;
    uint16_t cog_;
    uint8_t  time_stamp_;
    uint8_t  regional_reserved_;
    bool     dte_;
    bool     assigned_;
    bool     raim_;
    uint32_t radio_status_;
};

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

/**
 * @brief Decoder function for AIS message type 9.
 *
 * Satisfies the @c DecoderFn signature required by MessageRegistry.
 * Delegates directly to Msg9SARPositionReport::decode().
 *
 * @param reader  A BitReader positioned at bit 0 of the decoded payload.
 * @return        A decoded message object, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg9(const BitReader& reader);

} // namespace aislib