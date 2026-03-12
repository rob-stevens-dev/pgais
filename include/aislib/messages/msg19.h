#pragma once

#include "aislib/message.h"
#include "aislib/messages/msg1_3.h"  // sentinel constants

#include <cstdint>
#include <optional>
#include <string>

namespace aislib {

/**
 * @file messages/msg19.h
 * @brief AIS message type 19 — Extended Class B CS position report.
 *
 * ITU-R M.1371-5, Section 3.3.19 defines message type 19 as the extended
 * position report transmitted by Class B CS (Carrier-Sense) equipment when
 * the vessel name, ship type, and reference point dimensions are required in
 * addition to the standard Class B position data.
 *
 * The payload is 312 bits (52 armoured characters).  It is always a single
 * NMEA sentence; multi-part assembly is not required.
 *
 * Bit layout (ITU-R M.1371-5 Table 46):
 *
 *   Bits   0–5   : Message type (6), fixed value 19
 *   Bits   6–7   : Repeat indicator (2)
 *   Bits   8–37  : MMSI (30)
 *   Bits  38–47  : Reserved (10)
 *   Bits  48–57  : Speed over ground (10, 1/10 knot)
 *   Bit   58     : Position accuracy (1)
 *   Bits  59–86  : Longitude (28, signed, 1/10 000 min)
 *   Bits  87–113 : Latitude (27, signed, 1/10 000 min)
 *   Bits 114–125 : Course over ground (12, 1/10 degree)
 *   Bits 126–134 : True heading (9, degrees)
 *   Bits 135–140 : Time stamp (6)
 *   Bits 141–144 : Regional reserved (4)
 *   Bits 145–264 : Name (120, 20 × 6-bit ASCII)
 *   Bits 265–272 : Ship and cargo type (8)
 *   Bits 273–281 : Dimension to bow (9)
 *   Bits 282–290 : Dimension to stern (9)
 *   Bits 291–296 : Dimension to port (6)
 *   Bits 297–302 : Dimension to starboard (6)
 *   Bits 303–306 : EPFD type (4)
 *   Bit  307     : RAIM flag (1)
 *   Bit  308     : DTE flag (1)
 *   Bit  309     : Assigned mode flag (1)
 *   Bits 310–311 : Spare (2)
 *
 * Coordinate conventions, SOG, COG, and heading sentinel values follow
 * ITU-R M.1371-5 Table 44 and are identical to message types 1–3 and 18.
 * Refer to msg1_3.h for the sentinel constant definitions.
 *
 * The Name field uses 6-bit ASCII encoding.  Trailing '@' padding characters
 * and trailing spaces are stripped on decode.  On encode, the name is padded
 * to 20 characters with '@'.
 */

// ---------------------------------------------------------------------------
// Msg19ExtendedClassBPositionReport
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message type 19.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg19() registered with MessageRegistry.
 */
class Msg19ExtendedClassBPositionReport final : public Message {
public:
    /**
     * @brief Decodes an extended Class B position report from the given bit reader.
     *
     * The BitReader must be positioned at bit 0 of the fully assembled and
     * decoded payload.  The method reads exactly 312 bits.
     *
     * @param reader  A BitReader over the decoded payload bit stream.
     * @return        A decoded message object, or an Error.
     *
     * Possible errors: ErrorCode::PayloadTooShort, ErrorCode::MessageDecodeFailure.
     */
    [[nodiscard]] static Result<std::unique_ptr<Message>> decode(const BitReader& reader);

    /** @brief Returns MessageType::ExtendedClassBPositionReport. */
    [[nodiscard]] MessageType type() const noexcept override
    {
        return MessageType::ExtendedClassBPositionReport;
    }

    /**
     * @brief Encodes this message into an NMEA AIS armoured payload string.
     *
     * Produces a 312-bit payload with 0 fill bits.
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
     * @brief Returns the regional reserved bits [0–15].
     *
     * These four bits are reserved for regional use.
     */
    [[nodiscard]] uint8_t regional_reserved() const noexcept { return regional_reserved_; }

    /**
     * @brief Returns the vessel name with trailing padding stripped.
     *
     * Up to 20 characters, stripped of trailing '@' and trailing spaces.
     * An empty string indicates the name was not set or was all padding.
     */
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /**
     * @brief Returns the ship and cargo type code [0–255].
     *
     * Refer to ITU-R M.1371-5 Table 52 for the complete enumeration.
     * The value 0 means "not available or no ship".
     */
    [[nodiscard]] uint8_t ship_type() const noexcept { return ship_type_; }

    /**
     * @brief Returns the distance from the reference point to the bow in metres [0–511].
     *
     * The value 0 means "not available" or the default.
     */
    [[nodiscard]] uint16_t dim_bow() const noexcept { return dim_bow_; }

    /**
     * @brief Returns the distance from the reference point to the stern in metres [0–511].
     *
     * The value 0 means "not available" or the default.
     */
    [[nodiscard]] uint16_t dim_stern() const noexcept { return dim_stern_; }

    /**
     * @brief Returns the distance from the reference point to port in metres [0–63].
     *
     * The value 0 means "not available" or the default.
     */
    [[nodiscard]] uint8_t dim_port() const noexcept { return dim_port_; }

    /**
     * @brief Returns the distance from the reference point to starboard in metres [0–63].
     *
     * The value 0 means "not available" or the default.
     */
    [[nodiscard]] uint8_t dim_starboard() const noexcept { return dim_starboard_; }

    /**
     * @brief Returns the EPFD (Electronic Position Fixing Device) type [0–15].
     *
     * Common values: 0 = undefined; 1 = GPS; 2 = GLONASS; 3 = combined
     * GPS/GLONASS; 4 = Loran-C; 5 = Chayka; 6 = integrated navigation system;
     * 7 = surveyed; 8 = Galileo.
     */
    [[nodiscard]] uint8_t epfd_type() const noexcept { return epfd_type_; }

    /**
     * @brief Returns true if the RAIM (Receiver Autonomous Integrity Monitoring) flag is set.
     */
    [[nodiscard]] bool raim() const noexcept { return raim_; }

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

private:
    /**
     * @brief Private constructor used exclusively by decode().
     *
     * @param mmsi              Maritime Mobile Service Identity (30 bits).
     * @param repeat_indicator  Repeat indicator [0–3].
     * @param sog               Speed over ground in 1/10 knot units.
     * @param position_accuracy True if position is DGNSS-derived.
     * @param longitude         Raw longitude in 1/10 000 minute units.
     * @param latitude          Raw latitude in 1/10 000 minute units.
     * @param cog               Course over ground in 1/10 degree units.
     * @param true_heading      True heading in degrees.
     * @param time_stamp        UTC second time stamp.
     * @param regional_reserved Regional reserved bits.
     * @param name              Vessel name (stripped of trailing padding).
     * @param ship_type         Ship and cargo type code.
     * @param dim_bow           Dimension to bow in metres.
     * @param dim_stern         Dimension to stern in metres.
     * @param dim_port          Dimension to port in metres.
     * @param dim_starboard     Dimension to starboard in metres.
     * @param epfd_type         EPFD type code.
     * @param raim              RAIM flag.
     * @param dte               DTE flag.
     * @param assigned          Assigned mode flag.
     */
    Msg19ExtendedClassBPositionReport(
        uint32_t    mmsi,
        uint8_t     repeat_indicator,
        uint16_t    sog,
        bool        position_accuracy,
        int32_t     longitude,
        int32_t     latitude,
        uint16_t    cog,
        uint16_t    true_heading,
        uint8_t     time_stamp,
        uint8_t     regional_reserved,
        std::string name,
        uint8_t     ship_type,
        uint16_t    dim_bow,
        uint16_t    dim_stern,
        uint8_t     dim_port,
        uint8_t     dim_starboard,
        uint8_t     epfd_type,
        bool        raim,
        bool        dte,
        bool        assigned
    ) noexcept;

    uint16_t    sog_;
    bool        position_accuracy_;
    int32_t     longitude_;
    int32_t     latitude_;
    uint16_t    cog_;
    uint16_t    true_heading_;
    uint8_t     time_stamp_;
    uint8_t     regional_reserved_;
    std::string name_;
    uint8_t     ship_type_;
    uint16_t    dim_bow_;
    uint16_t    dim_stern_;
    uint8_t     dim_port_;
    uint8_t     dim_starboard_;
    uint8_t     epfd_type_;
    bool        raim_;
    bool        dte_;
    bool        assigned_;
};

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

/**
 * @brief Decoder function for AIS message type 19.
 *
 * Satisfies the @c DecoderFn signature required by MessageRegistry.
 * Delegates directly to Msg19ExtendedClassBPositionReport::decode().
 *
 * @param reader  A BitReader positioned at bit 0 of the decoded payload.
 * @return        A decoded message object, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg19(const BitReader& reader);

} // namespace aislib