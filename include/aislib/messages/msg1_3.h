#pragma once

#include "aislib/message.h"

#include <cstdint>
#include <optional>
#include <string>

namespace aislib {

/**
 * @file messages/msg1_3.h
 * @brief AIS message types 1, 2, and 3 — Class A position report.
 *
 * ITU-R M.1371-5, Section 3.3.1 defines three closely related Class A
 * position report message types that share an identical 168-bit payload
 * layout:
 *
 *   Type 1 — Scheduled position report (autonomous mode)
 *   Type 2 — Assigned scheduled position report
 *   Type 3 — Special position report, response to interrogation
 *
 * All three types are decoded by the same concrete class,
 * Msg1_3PositionReportClassA.  The @c type() accessor returns the specific
 * MessageType enumerator that was present in the payload, allowing callers
 * to distinguish the three variants when necessary.
 *
 * Coordinate conventions follow ITU-R M.1371-5 Table 44:
 *
 *   Longitude: 1/10 000 min, range [-180, +180] degrees.
 *              The integer field stores the value multiplied by 10 000 * 60.
 *              The sentinel value 0x6791AC0 (181 * 600000) indicates
 *              "not available" and is exposed as longitude_unavailable().
 *
 *   Latitude:  1/10 000 min, range [-90, +90] degrees.
 *              The integer field stores the value multiplied by 10 000 * 60.
 *              The sentinel value 0x3412140 (91 * 600000) indicates
 *              "not available" and is exposed as latitude_unavailable().
 *
 * Speed over ground (SOG) is in 1/10 knot steps, 0–102.2 knots.
 * The value 1023 (0x3FF) indicates "not available".
 *
 * Course over ground (COG) is in 1/10 degree steps, 0–359.9 degrees.
 * The value 3600 indicates "not available".
 *
 * True heading is in 1 degree steps, 0–359 degrees.
 * The value 511 indicates "not available".
 */

// ---------------------------------------------------------------------------
// Sentinel values (ITU-R M.1371-5 Table 44)
// ---------------------------------------------------------------------------

/** Longitude "not available" raw integer value. */
inline constexpr int32_t kLongitudeUnavailable = 0x6791AC0;

/** Latitude "not available" raw integer value. */
inline constexpr int32_t kLatitudeUnavailable  = 0x3412140;

/** Speed over ground "not available" raw value (1/10 knot units). */
inline constexpr uint16_t kSogUnavailable       = 1023u;

/** Course over ground "not available" raw value (1/10 degree units). */
inline constexpr uint16_t kCogUnavailable       = 3600u;

/** True heading "not available" raw value. */
inline constexpr uint16_t kHeadingUnavailable   = 511u;

/** Rate of turn "not available" raw value. */
inline constexpr int8_t   kRotUnavailable       = -128;

// ---------------------------------------------------------------------------
// Msg1_3PositionReportClassA
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message types 1, 2, and 3.
 *
 * All fields are stored in the raw units defined by ITU-R M.1371-5 to avoid
 * precision loss in intermediate representations.  Helper methods are provided
 * to convert to SI or conventional floating-point units where the conversion
 * is unambiguous.
 *
 * Construction is only possible through the static @c decode() factory or via
 * the decoder function @c decode_msg1_3() registered with MessageRegistry.
 */
class Msg1_3PositionReportClassA final : public Message {
public:
    /**
     * @brief Decodes a Class A position report from the given bit reader.
     *
     * The BitReader must be positioned at bit 0 of the fully assembled and
     * decoded payload.  The method reads the first 168 bits, validates field
     * values against the specification, and returns a heap-allocated message
     * object on success.
     *
     * @param reader  A BitReader over the decoded payload bit stream.
     * @return        A decoded message object, or an Error.
     *
     * Possible errors: ErrorCode::PayloadTooShort, ErrorCode::MessageDecodeFailure.
     */
    [[nodiscard]] static Result<std::unique_ptr<Message>> decode(const BitReader& reader);

    /** @brief Returns MessageType::PositionReportClassA, _Assigned, or _Response. */
    [[nodiscard]] MessageType type() const noexcept override { return type_; }

    /**
     * @brief Encodes this message into an NMEA AIS armoured payload string.
     *
     * Produces a 168-bit payload encoded as 28 armour characters with 0 fill
     * bits.  The returned pair is suitable for wrapping in a Sentence.
     *
     * @return  Pair of (armoured payload, fill bits), or an Error.
     */
    [[nodiscard]] Result<std::pair<std::string, uint8_t>> encode() const override;

    // -----------------------------------------------------------------------
    // Field accessors — raw values in ITU-R M.1371-5 units
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the Navigation Status field [0–15].
     *
     * Common values: 0 = under way using engine, 1 = at anchor,
     * 5 = moored, 15 = not defined/default.
     */
    [[nodiscard]] uint8_t nav_status() const noexcept { return nav_status_; }

    /**
     * @brief Returns the Rate of Turn (ROT) field [-128, +127].
     *
     * Value -128 (0x80) indicates "not available".  Values in [-126, +126]
     * represent the ROT index: ROTIND = 4.733 * sqrt(|ROT_sensor|), where
     * ROTIND is in degrees per minute.  Values +127 and -127 indicate
     * "turning right/left at more than 5 degrees per 30 seconds".
     */
    [[nodiscard]] int8_t rot() const noexcept { return rot_; }

    /**
     * @brief Returns the Speed Over Ground (SOG) in 1/10 knot units [0–1023].
     *
     * Divide by 10.0 to obtain knots.  The value 1023 means "not available".
     * Values 1022 (102.2 knots) indicate "102.2 knots or higher".
     */
    [[nodiscard]] uint16_t sog() const noexcept { return sog_; }

    /**
     * @brief Returns true if the position was determined by DGNSS.
     *
     * When false, the position was determined by the GNSS receiver in
     * autonomous mode or by another method.
     */
    [[nodiscard]] bool position_accuracy() const noexcept { return position_accuracy_; }

    /**
     * @brief Returns the raw longitude in 1/10 000 minute units.
     *
     * Range: [-108 000 000, +108 000 000].  Divide by 600 000.0 to obtain
     * decimal degrees.  The sentinel value kLongitudeUnavailable indicates
     * "not available".
     */
    [[nodiscard]] int32_t longitude_raw() const noexcept { return longitude_; }

    /**
     * @brief Returns the raw latitude in 1/10 000 minute units.
     *
     * Range: [-54 000 000, +54 000 000].  Divide by 600 000.0 to obtain
     * decimal degrees.  The sentinel value kLatitudeUnavailable indicates
     * "not available".
     */
    [[nodiscard]] int32_t latitude_raw() const noexcept { return latitude_; }

    /**
     * @brief Returns longitude in decimal degrees, or std::nullopt when unavailable.
     *
     * Convenience wrapper around longitude_raw() that performs the unit
     * conversion and sentinel check.
     */
    [[nodiscard]] std::optional<double> longitude() const noexcept;

    /**
     * @brief Returns latitude in decimal degrees, or std::nullopt when unavailable.
     *
     * Convenience wrapper around latitude_raw() that performs the unit
     * conversion and sentinel check.
     */
    [[nodiscard]] std::optional<double> latitude() const noexcept;

    /**
     * @brief Returns the Course Over Ground (COG) in 1/10 degree units [0–3600].
     *
     * Divide by 10.0 to obtain degrees.  The value 3600 means "not available".
     */
    [[nodiscard]] uint16_t cog() const noexcept { return cog_; }

    /**
     * @brief Returns the True Heading in degrees [0–359, or 511].
     *
     * The value 511 means "not available".
     */
    [[nodiscard]] uint16_t true_heading() const noexcept { return true_heading_; }

    /**
     * @brief Returns the Time Stamp field [0–63].
     *
     * Values 0–59 represent the UTC second at which the position was obtained.
     * 60 = not available (default); 61 = manual input; 62 = dead reckoning;
     * 63 = inoperative.
     */
    [[nodiscard]] uint8_t time_stamp() const noexcept { return time_stamp_; }

    /**
     * @brief Returns the Manoeuvre Indicator field [0–2].
     *
     * 0 = not available; 1 = no special manoeuvre; 2 = special manoeuvre.
     */
    [[nodiscard]] uint8_t manoeuvre_indicator() const noexcept { return manoeuvre_indicator_; }

    /**
     * @brief Returns true if the RAIM flag is set.
     *
     * RAIM (Receiver Autonomous Integrity Monitoring) indicates that the
     * position sensor is operating in a self-consistency check mode.
     */
    [[nodiscard]] bool raim() const noexcept { return raim_; }

    /**
     * @brief Returns the 19-bit radio status field.
     *
     * The interpretation depends on whether the message carries SOTDMA or
     * ITDMA communication state.  Bit 18 of the field is the selector bit:
     * 0 = SOTDMA, 1 = ITDMA.  The remaining 18 bits encode the communication
     * state as defined in ITU-R M.1371-5 Annex 2.
     */
    [[nodiscard]] uint32_t radio_status() const noexcept { return radio_status_; }

private:
    /**
     * @brief Private constructor used exclusively by decode().
     *
     * @param type               The specific MessageType (1, 2, or 3).
     * @param mmsi               MMSI from bits [8, 37].
     * @param repeat_indicator   Repeat indicator from bits [6, 7].
     * @param nav_status         Navigation status from bits [38, 41].
     * @param rot                Rate of turn from bits [42, 49].
     * @param sog                Speed over ground from bits [50, 59].
     * @param position_accuracy  Position accuracy bit at bit [60].
     * @param longitude          Longitude from bits [61, 88].
     * @param latitude           Latitude from bits [89, 115].
     * @param cog                Course over ground from bits [116, 127].
     * @param true_heading       True heading from bits [128, 136].
     * @param time_stamp         Time stamp from bits [137, 142].
     * @param manoeuvre_indicator Manoeuvre indicator from bits [143, 144].
     * @param raim               RAIM flag at bit [148].
     * @param radio_status       Radio status from bits [149, 167].
     */
    explicit Msg1_3PositionReportClassA(
        MessageType type,
        uint32_t    mmsi,
        uint8_t     repeat_indicator,
        uint8_t     nav_status,
        int8_t      rot,
        uint16_t    sog,
        bool        position_accuracy,
        int32_t     longitude,
        int32_t     latitude,
        uint16_t    cog,
        uint16_t    true_heading,
        uint8_t     time_stamp,
        uint8_t     manoeuvre_indicator,
        bool        raim,
        uint32_t    radio_status
    ) noexcept;

    MessageType type_;
    uint8_t     nav_status_;
    int8_t      rot_;
    uint16_t    sog_;
    bool        position_accuracy_;
    int32_t     longitude_;
    int32_t     latitude_;
    uint16_t    cog_;
    uint16_t    true_heading_;
    uint8_t     time_stamp_;
    uint8_t     manoeuvre_indicator_;
    bool        raim_;
    uint32_t    radio_status_;
};

// ---------------------------------------------------------------------------
// Decoder function — registered with MessageRegistry
// ---------------------------------------------------------------------------

/**
 * @brief Decoder function for AIS message types 1, 2, and 3.
 *
 * This function matches the DecoderFn signature and is registered with
 * MessageRegistry::register_decoder() for type IDs 1, 2, and 3 during
 * library initialisation.
 *
 * @param reader  BitReader positioned at the start of the decoded payload.
 * @return        Decoded message, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg1_3(const BitReader& reader);

} // namespace aislib
