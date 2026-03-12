#pragma once

#include "aislib/message.h"

#include <cstdint>
#include <optional>
#include <string>

namespace aislib {

/**
 * @file messages/msg27.h
 * @brief AIS message type 27 — Long-range AIS broadcast message.
 *
 * ITU-R M.1371-5, Section 3.3.27 defines message type 27 as a compact
 * position report intended for transmission over long-range (satellite)
 * channels where bandwidth is at a premium.  The payload is 96 bits
 * (16 armoured characters).
 *
 * Bit layout (ITU-R M.1371-5 Table 56):
 *
 *   Bits  0–5  : Message type (6), fixed value 27
 *   Bits  6–7  : Repeat indicator (2)
 *   Bits  8–37 : MMSI (30)
 *   Bit   38   : Position accuracy (1)
 *   Bit   39   : RAIM flag (1)
 *   Bits 40–43 : Navigation status (4)
 *   Bits 44–61 : Longitude (18, signed, 1/10 degree; sentinel 1810)
 *   Bits 62–78 : Latitude (17, signed, 1/10 degree; sentinel 910)
 *   Bits 79–84 : Speed over ground (6, knots; sentinel 63)
 *   Bits 85–93 : Course over ground (9, degrees; sentinel 511)
 *   Bit   94   : GNSS position status (1)
 *   Bit   95   : Spare (1)
 *
 * Coordinate resolution for type 27 is 1/10 degree, much coarser than the
 * 1/10 000 minute (approximately 1/600 000 degree) used by Class A and B.
 * The sentinels are correspondingly different:
 *
 *   Longitude sentinel: 1810  = 181 * 10  (181 degrees in 1/10 degree units)
 *   Latitude sentinel:   910  =  91 * 10  ( 91 degrees in 1/10 degree units)
 *
 * Speed over ground is in whole knots, maximum valid value 62.  The value 63
 * means "not available or over 62 knots".
 *
 * Course over ground is in whole degrees, range 0–359.  The value 511 means
 * "not available".
 *
 * Navigation status uses the same 4-bit enumeration as message types 1–3:
 *   0 = under way using engine; 1 = at anchor; 2 = not under command; etc.
 *   The value 15 = "not defined".
 *
 * The GNSS position status bit indicates whether the position was obtained
 * from a current GNSS fix (0) or from dead reckoning or manual input (1).
 */

// ---------------------------------------------------------------------------
// Sentinel values specific to type 27
// ---------------------------------------------------------------------------

/**
 * Longitude "not available" raw integer value for type 27 (1/10 degree units).
 * Derived from 181 * 10, representing one degree beyond the valid range of
 * [-180, +180] in 1/10 degree resolution.
 */
inline constexpr int32_t kMsg27LongitudeUnavailable = 1810;

/**
 * Latitude "not available" raw integer value for type 27 (1/10 degree units).
 * Derived from 91 * 10, representing one degree beyond the valid range of
 * [-90, +90] in 1/10 degree resolution.
 */
inline constexpr int32_t kMsg27LatitudeUnavailable = 910;

/** SOG "not available" raw value for type 27 (whole knots). */
inline constexpr uint8_t kMsg27SogUnavailable = 63u;

/** COG "not available" raw value for type 27 (whole degrees). */
inline constexpr uint16_t kMsg27CogUnavailable = 511u;

// ---------------------------------------------------------------------------
// Msg27LongRangeBroadcast
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message type 27.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg27() registered with MessageRegistry.
 */
class Msg27LongRangeBroadcast final : public Message {
public:
    /**
     * @brief Decodes a long-range AIS broadcast from the given bit reader.
     *
     * The BitReader must be positioned at bit 0 of the fully assembled and
     * decoded payload.  The method reads exactly 96 bits.
     *
     * @param reader  A BitReader over the decoded payload bit stream.
     * @return        A decoded message object, or an Error.
     *
     * Possible errors: ErrorCode::PayloadTooShort, ErrorCode::MessageDecodeFailure.
     */
    [[nodiscard]] static Result<std::unique_ptr<Message>> decode(const BitReader& reader);

    /** @brief Returns MessageType::PositionReportLongRange. */
    [[nodiscard]] MessageType type() const noexcept override
    {
        return MessageType::PositionReportLongRange;
    }

    /**
     * @brief Encodes this message into an NMEA AIS armoured payload string.
     *
     * Produces a 96-bit payload with 0 fill bits.
     *
     * @return  Pair of (armoured payload, fill bits), or an Error.
     */
    [[nodiscard]] Result<std::pair<std::string, uint8_t>> encode() const override;

    // -----------------------------------------------------------------------
    // Field accessors — raw values in ITU-R M.1371-5 units
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if the position was determined by DGNSS.
     */
    [[nodiscard]] bool position_accuracy() const noexcept { return position_accuracy_; }

    /**
     * @brief Returns true if the RAIM (Receiver Autonomous Integrity Monitoring) flag is set.
     */
    [[nodiscard]] bool raim() const noexcept { return raim_; }

    /**
     * @brief Returns the navigation status code [0–15].
     *
     * Uses the same enumeration as message types 1–3.  The value 15 means
     * "not defined" (default for type 27 when not set).
     */
    [[nodiscard]] uint8_t nav_status() const noexcept { return nav_status_; }

    /**
     * @brief Returns the raw longitude in 1/10 degree units.
     *
     * Divide by 10.0 for decimal degrees.
     * The sentinel value kMsg27LongitudeUnavailable indicates "not available".
     */
    [[nodiscard]] int32_t longitude_raw() const noexcept { return longitude_; }

    /**
     * @brief Returns the raw latitude in 1/10 degree units.
     *
     * Divide by 10.0 for decimal degrees.
     * The sentinel value kMsg27LatitudeUnavailable indicates "not available".
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
     * @brief Returns the Speed Over Ground (SOG) in whole knots [0–62].
     *
     * The value 63 means "not available or over 62 knots".
     */
    [[nodiscard]] uint8_t sog() const noexcept { return sog_; }

    /**
     * @brief Returns the Course Over Ground (COG) in whole degrees [0–359].
     *
     * The value 511 means "not available".
     */
    [[nodiscard]] uint16_t cog() const noexcept { return cog_; }

    /**
     * @brief Returns the GNSS position status bit.
     *
     * false (0) = current GNSS position fix.
     * true  (1) = not a current GNSS fix (dead reckoning or manual).
     */
    [[nodiscard]] bool gnss_position_status() const noexcept { return gnss_position_status_; }

private:
    /**
     * @brief Private constructor used exclusively by decode().
     *
     * @param mmsi                  Maritime Mobile Service Identity (30 bits).
     * @param repeat_indicator      Repeat indicator [0–3].
     * @param position_accuracy     True if position is DGNSS-derived.
     * @param raim                  RAIM flag.
     * @param nav_status            Navigation status code [0–15].
     * @param longitude             Raw longitude in 1/10 degree units.
     * @param latitude              Raw latitude in 1/10 degree units.
     * @param sog                   Speed over ground in whole knots [0–63].
     * @param cog                   Course over ground in whole degrees [0–511].
     * @param gnss_position_status  GNSS position status flag.
     */
    Msg27LongRangeBroadcast(
        uint32_t mmsi,
        uint8_t  repeat_indicator,
        bool     position_accuracy,
        bool     raim,
        uint8_t  nav_status,
        int32_t  longitude,
        int32_t  latitude,
        uint8_t  sog,
        uint16_t cog,
        bool     gnss_position_status
    ) noexcept;

    bool     position_accuracy_;
    bool     raim_;
    uint8_t  nav_status_;
    int32_t  longitude_;
    int32_t  latitude_;
    uint8_t  sog_;
    uint16_t cog_;
    bool     gnss_position_status_;
};

/**
 * @brief Free decoder function for type 27, compatible with DecoderFn.
 *
 * Delegates to Msg27LongRangeBroadcast::decode().  This function is
 * registered with MessageRegistry by register_all_decoders().
 *
 * @param reader  A BitReader over the decoded payload bit stream.
 * @return        A decoded message object, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg27(const BitReader& reader);

} // namespace aislib