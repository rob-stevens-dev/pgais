#pragma once

#include "aislib/message.h"

#include <cstdint>
#include <optional>
#include <string>

namespace aislib {

/**
 * @file messages/msg24.h
 * @brief AIS message type 24 — Class B CS static data report.
 *
 * ITU-R M.1371-5, Section 3.3.24 defines message type 24 as carrying static
 * information about a Class B unit.  The message is sent in two independent
 * parts, each of which forms a complete 168-bit NMEA sentence payload:
 *
 *   Part A (part_number == 0): carries only the vessel name.
 *   Part B (part_number == 1): carries ship type, vendor ID, call sign,
 *                               dimensions, EPFD type, and MMSI of mothership.
 *
 * Unlike message type 5, the two parts of type 24 are transmitted as
 * independent single-part sentences.  They are not assembled by
 * SentenceAssembler; instead, each part arrives as a standalone decoded
 * message.  The caller is responsible for correlating Part A and Part B
 * messages by MMSI if a combined view of the vessel is desired.
 *
 * The part_number field at bits [38, 39] of the payload distinguishes the
 * two parts.  This decoder returns a Msg24StaticDataReport object for both
 * parts, with fields not present in the received part left in their default
 * "not available" state (empty strings, zero integers).
 */

// ---------------------------------------------------------------------------
// Msg24StaticDataReport
// ---------------------------------------------------------------------------

/**
 * @brief Decoded representation of AIS message type 24, Part A or Part B.
 *
 * Part A fields: vessel_name() (non-empty when Part A).
 * Part B fields: ship_type(), vendor_id(), call_sign(), dim_to_bow(),
 *                dim_to_stern(), dim_to_port(), dim_to_starboard(), epfd(),
 *                mothership_mmsi() (non-empty / non-zero when Part B).
 *
 * The part_number() accessor returns 0 for Part A and 1 for Part B.
 *
 * Construction is only possible through the static @c decode() factory or
 * via the decoder function @c decode_msg24() registered with MessageRegistry.
 */
class Msg24StaticDataReport final : public Message {
public:
    /**
     * @brief Decodes a Class B static data report from the given bit reader.
     *
     * The BitReader must be positioned at bit 0 of a single decoded payload.
     * The method reads exactly 168 bits, inspects the part number field, and
     * populates the appropriate subset of fields.
     *
     * @param reader  A BitReader over the decoded payload bit stream.
     * @return        A decoded message object, or an Error.
     *
     * Possible errors: ErrorCode::PayloadTooShort, ErrorCode::MessageDecodeFailure.
     */
    [[nodiscard]] static Result<std::unique_ptr<Message>> decode(const BitReader& reader);

    /** @brief Returns MessageType::ClassBCSStaticDataReport. */
    [[nodiscard]] MessageType type() const noexcept override
    {
        return MessageType::ClassBCSStaticDataReport;
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
    // Common fields
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the part number: 0 for Part A, 1 for Part B.
     */
    [[nodiscard]] uint8_t part_number() const noexcept { return part_number_; }

    // -----------------------------------------------------------------------
    // Part A fields
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the vessel name string (Part A only, up to 20 characters).
     *
     * Returns an empty string when this object represents Part B.
     * Trailing '@' padding is stripped before returning.
     */
    [[nodiscard]] const std::string& vessel_name() const noexcept { return vessel_name_; }

    // -----------------------------------------------------------------------
    // Part B fields
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the ship and cargo type code [0–255] (Part B only).
     *
     * Returns 0 when this object represents Part A.
     * Refer to ITU-R M.1371-5 Table 54 for the full enumeration.
     */
    [[nodiscard]] uint8_t ship_type() const noexcept { return ship_type_; }

    /**
     * @brief Returns the vendor ID string (Part B only, up to 7 characters).
     *
     * Trailing '@' padding is stripped.  Returns an empty string for Part A.
     */
    [[nodiscard]] const std::string& vendor_id() const noexcept { return vendor_id_; }

    /**
     * @brief Returns the call sign string (Part B only, up to 7 characters).
     *
     * Trailing '@' padding is stripped.  Returns an empty string for Part A.
     */
    [[nodiscard]] const std::string& call_sign() const noexcept { return call_sign_; }

    /**
     * @brief Returns the bow dimension in metres [0–511] (Part B only).
     */
    [[nodiscard]] uint16_t dim_to_bow() const noexcept { return dim_to_bow_; }

    /**
     * @brief Returns the stern dimension in metres [0–511] (Part B only).
     */
    [[nodiscard]] uint16_t dim_to_stern() const noexcept { return dim_to_stern_; }

    /**
     * @brief Returns the port dimension in metres [0–63] (Part B only).
     */
    [[nodiscard]] uint8_t dim_to_port() const noexcept { return dim_to_port_; }

    /**
     * @brief Returns the starboard dimension in metres [0–63] (Part B only).
     */
    [[nodiscard]] uint8_t dim_to_starboard() const noexcept { return dim_to_starboard_; }

    /**
     * @brief Returns the EPFD type [0–15] (Part B only).
     */
    [[nodiscard]] uint8_t epfd() const noexcept { return epfd_; }

    /**
     * @brief Returns the MMSI of the mothership [0–999999999] (Part B only).
     *
     * Non-zero only for auxiliary craft.  Returns 0 for Part A or when the
     * vessel is not an auxiliary craft.
     */
    [[nodiscard]] uint32_t mothership_mmsi() const noexcept { return mothership_mmsi_; }

private:
    /**
     * @brief Private constructor for Part A messages.
     */
    static Msg24StaticDataReport make_part_a(
        uint32_t    mmsi,
        uint8_t     repeat_indicator,
        std::string vessel_name
    ) noexcept;

    /**
     * @brief Private constructor for Part B messages.
     */
    static Msg24StaticDataReport make_part_b(
        uint32_t    mmsi,
        uint8_t     repeat_indicator,
        uint8_t     ship_type,
        std::string vendor_id,
        std::string call_sign,
        uint16_t    dim_to_bow,
        uint16_t    dim_to_stern,
        uint8_t     dim_to_port,
        uint8_t     dim_to_starboard,
        uint8_t     epfd,
        uint32_t    mothership_mmsi
    ) noexcept;

    explicit Msg24StaticDataReport(
        uint32_t    mmsi,
        uint8_t     repeat_indicator,
        uint8_t     part_number,
        std::string vessel_name,
        uint8_t     ship_type,
        std::string vendor_id,
        std::string call_sign,
        uint16_t    dim_to_bow,
        uint16_t    dim_to_stern,
        uint8_t     dim_to_port,
        uint8_t     dim_to_starboard,
        uint8_t     epfd,
        uint32_t    mothership_mmsi
    ) noexcept;

    uint8_t     part_number_;
    std::string vessel_name_;
    uint8_t     ship_type_;
    std::string vendor_id_;
    std::string call_sign_;
    uint16_t    dim_to_bow_;
    uint16_t    dim_to_stern_;
    uint8_t     dim_to_port_;
    uint8_t     dim_to_starboard_;
    uint8_t     epfd_;
    uint32_t    mothership_mmsi_;
};

// ---------------------------------------------------------------------------
// Decoder function — registered with MessageRegistry
// ---------------------------------------------------------------------------

/**
 * @brief Decoder function for AIS message type 24.
 *
 * Registered with MessageRegistry::register_decoder() for type ID 24 during
 * library initialisation.
 *
 * @param reader  BitReader positioned at the start of the decoded payload.
 * @return        Decoded message, or an Error.
 */
[[nodiscard]] Result<std::unique_ptr<Message>> decode_msg24(const BitReader& reader);

} // namespace aislib
