#include "aislib/messages/msg24.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

static constexpr std::size_t kMsg24BitLength = 168u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg24StaticDataReport::Msg24StaticDataReport(
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
) noexcept
    : Message(mmsi, repeat_indicator)
    , part_number_(part_number)
    , vessel_name_(std::move(vessel_name))
    , ship_type_(ship_type)
    , vendor_id_(std::move(vendor_id))
    , call_sign_(std::move(call_sign))
    , dim_to_bow_(dim_to_bow)
    , dim_to_stern_(dim_to_stern)
    , dim_to_port_(dim_to_port)
    , dim_to_starboard_(dim_to_starboard)
    , epfd_(epfd)
    , mothership_mmsi_(mothership_mmsi)
{}

Msg24StaticDataReport Msg24StaticDataReport::make_part_a(
    uint32_t    mmsi,
    uint8_t     repeat_indicator,
    std::string vessel_name
) noexcept
{
    return Msg24StaticDataReport(
        mmsi, repeat_indicator, 0u,
        std::move(vessel_name),
        0u, {}, {}, 0u, 0u, 0u, 0u, 0u, 0u
    );
}

Msg24StaticDataReport Msg24StaticDataReport::make_part_b(
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
) noexcept
{
    return Msg24StaticDataReport(
        mmsi, repeat_indicator, 1u,
        {},
        ship_type, std::move(vendor_id), std::move(call_sign),
        dim_to_bow, dim_to_stern, dim_to_port, dim_to_starboard,
        epfd, mothership_mmsi
    );
}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg24StaticDataReport::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg24BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    if (message_type_from_id(static_cast<uint8_t>(type_r.value())) !=
            MessageType::ClassBCSStaticDataReport) {
        return Error{ ErrorCode::MessageDecodeFailure, "unexpected message type" };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI (30 bits)
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–39: part number (2 bits)
    auto pn_r = reader.read_uint(38u, 2u);
    if (!pn_r) return Error{ pn_r.error() };
    const uint8_t part_number = static_cast<uint8_t>(pn_r.value());

    if (part_number == 0u) {
        // Part A: bits 40–159 — vessel name (120 bits = 20 * 6)
        auto name_r = reader.read_text(40u, 20u);
        if (!name_r) return Error{ name_r.error() };
        // Bits 160–167: spare (ignored)

        auto msg = make_part_a(
            static_cast<uint32_t>(mmsi_r.value()),
            static_cast<uint8_t>(ri_r.value()),
            name_r.value()
        );
        return std::unique_ptr<Message>(new Msg24StaticDataReport(std::move(msg)));
    }

    if (part_number == 1u) {
        // Bits 40–47: ship and cargo type (8 bits)
        auto stype_r = reader.read_uint(40u, 8u);
        if (!stype_r) return Error{ stype_r.error() };

        // Bits 48–89: vendor ID (42 bits = 7 * 6)
        auto vendor_r = reader.read_text(48u, 7u);
        if (!vendor_r) return Error{ vendor_r.error() };

        // Bits 90–131: call sign (42 bits = 7 * 6)
        auto cs_r = reader.read_text(90u, 7u);
        if (!cs_r) return Error{ cs_r.error() };

        // Bits 132–140: dimension to bow (9 bits)
        auto bow_r = reader.read_uint(132u, 9u);
        if (!bow_r) return Error{ bow_r.error() };

        // Bits 141–149: dimension to stern (9 bits)
        auto stern_r = reader.read_uint(141u, 9u);
        if (!stern_r) return Error{ stern_r.error() };

        // Bits 150–155: dimension to port (6 bits)
        auto port_r = reader.read_uint(150u, 6u);
        if (!port_r) return Error{ port_r.error() };

        // Bits 156–161: dimension to starboard (6 bits)
        auto stbd_r = reader.read_uint(156u, 6u);
        if (!stbd_r) return Error{ stbd_r.error() };

        // Bits 162–165: EPFD type (4 bits)
        auto epfd_r = reader.read_uint(162u, 4u);
        if (!epfd_r) return Error{ epfd_r.error() };

        // Bits 166–167: spare (2 bits, ignored)
        // Note: for auxiliary craft the mothership MMSI occupies bits 132–161
        // and EPFD is not present.  The standard uses the same bit range for
        // both interpretations.  We decode as the non-auxiliary layout; callers
        // that need the mothership MMSI should re-interpret dim and EPFD fields.
        // A future extension may add a separate auxiliary-craft flag.

        auto msg = make_part_b(
            static_cast<uint32_t>(mmsi_r.value()),
            static_cast<uint8_t>(ri_r.value()),
            static_cast<uint8_t>(stype_r.value()),
            vendor_r.value(),
            cs_r.value(),
            static_cast<uint16_t>(bow_r.value()),
            static_cast<uint16_t>(stern_r.value()),
            static_cast<uint8_t>(port_r.value()),
            static_cast<uint8_t>(stbd_r.value()),
            static_cast<uint8_t>(epfd_r.value()),
            0u
        );
        return std::unique_ptr<Message>(new Msg24StaticDataReport(std::move(msg)));
    }

    return Error{ ErrorCode::MessageDecodeFailure, "invalid part number in type 24 message" };
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg24StaticDataReport::encode() const
{
    BitWriter w(kMsg24BitLength);

    auto r = w.write_uint(static_cast<uint64_t>(MessageType::ClassBCSStaticDataReport), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(part_number_, 2u);
    if (!r) return Error{ r.error() };

    if (part_number_ == 0u) {
        r = w.write_text(vessel_name_, 20u);
        if (!r) return Error{ r.error() };
        // 8 spare bits
        r = w.write_uint(0u, 8u);
        if (!r) return Error{ r.error() };
    } else {
        r = w.write_uint(ship_type_, 8u);
        if (!r) return Error{ r.error() };

        r = w.write_text(vendor_id_, 7u);
        if (!r) return Error{ r.error() };

        r = w.write_text(call_sign_, 7u);
        if (!r) return Error{ r.error() };

        r = w.write_uint(dim_to_bow_, 9u);
        if (!r) return Error{ r.error() };

        r = w.write_uint(dim_to_stern_, 9u);
        if (!r) return Error{ r.error() };

        r = w.write_uint(dim_to_port_, 6u);
        if (!r) return Error{ r.error() };

        r = w.write_uint(dim_to_starboard_, 6u);
        if (!r) return Error{ r.error() };

        r = w.write_uint(epfd_, 4u);
        if (!r) return Error{ r.error() };

        // 2 spare bits
        r = w.write_uint(0u, 2u);
        if (!r) return Error{ r.error() };
    }

    return encode_payload(w.buffer(), kMsg24BitLength);
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg24(const BitReader& reader)
{
    return Msg24StaticDataReport::decode(reader);
}

} // namespace aislib
