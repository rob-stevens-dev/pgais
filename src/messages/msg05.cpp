#include "aislib/messages/msg05.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

// ITU-R M.1371-5 Table 47: message type 5 payload length is 426 bits.
// 426 / 6 = 71 armour characters exactly; fill bits = 0.
static constexpr std::size_t kMsg5BitLength = 426u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg5StaticAndVoyageData::Msg5StaticAndVoyageData(
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
) noexcept
    : Message(mmsi, repeat_indicator)
    , ais_version_(ais_version)
    , imo_number_(imo_number)
    , call_sign_(std::move(call_sign))
    , vessel_name_(std::move(vessel_name))
    , ship_type_(ship_type)
    , dim_to_bow_(dim_to_bow)
    , dim_to_stern_(dim_to_stern)
    , dim_to_port_(dim_to_port)
    , dim_to_starboard_(dim_to_starboard)
    , epfd_(epfd)
    , eta_month_(eta_month)
    , eta_day_(eta_day)
    , eta_hour_(eta_hour)
    , eta_minute_(eta_minute)
    , draught_(draught)
    , destination_(std::move(destination))
    , dte_(dte)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg5StaticAndVoyageData::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg5BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    if (message_type_from_id(static_cast<uint8_t>(type_r.value()))
            != MessageType::StaticAndVoyageData) {
        return Error{ ErrorCode::MessageDecodeFailure,
                      "bit reader does not contain a type 5 payload" };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–39: AIS version
    auto ver_r = reader.read_uint(38u, 2u);
    if (!ver_r) return Error{ ver_r.error() };

    // Bits 40–69: IMO number
    auto imo_r = reader.read_uint(40u, 30u);
    if (!imo_r) return Error{ imo_r.error() };

    // Bits 70–111: call sign (7 characters × 6 bits)
    auto cs_r = reader.read_text(70u, 7u);
    if (!cs_r) return Error{ cs_r.error() };

    // Bits 112–231: vessel name (20 characters × 6 bits)
    auto name_r = reader.read_text(112u, 20u);
    if (!name_r) return Error{ name_r.error() };

    // Bits 232–239: ship and cargo type
    auto ship_r = reader.read_uint(232u, 8u);
    if (!ship_r) return Error{ ship_r.error() };

    // Bits 240–248: dimension to bow (9 bits)
    auto bow_r = reader.read_uint(240u, 9u);
    if (!bow_r) return Error{ bow_r.error() };

    // Bits 249–257: dimension to stern (9 bits)
    auto stern_r = reader.read_uint(249u, 9u);
    if (!stern_r) return Error{ stern_r.error() };

    // Bits 258–263: dimension to port (6 bits)
    auto port_r = reader.read_uint(258u, 6u);
    if (!port_r) return Error{ port_r.error() };

    // Bits 264–269: dimension to starboard (6 bits)
    auto stbd_r = reader.read_uint(264u, 6u);
    if (!stbd_r) return Error{ stbd_r.error() };

    // Bits 270–273: EPFD type (4 bits)
    auto epfd_r = reader.read_uint(270u, 4u);
    if (!epfd_r) return Error{ epfd_r.error() };

    // Bits 274–277: ETA month (4 bits)
    auto eta_mo_r = reader.read_uint(274u, 4u);
    if (!eta_mo_r) return Error{ eta_mo_r.error() };

    // Bits 278–282: ETA day (5 bits)
    auto eta_dy_r = reader.read_uint(278u, 5u);
    if (!eta_dy_r) return Error{ eta_dy_r.error() };

    // Bits 283–287: ETA hour (5 bits)
    auto eta_hr_r = reader.read_uint(283u, 5u);
    if (!eta_hr_r) return Error{ eta_hr_r.error() };

    // Bits 288–293: ETA minute (6 bits)
    auto eta_mn_r = reader.read_uint(288u, 6u);
    if (!eta_mn_r) return Error{ eta_mn_r.error() };

    // Bits 294–301: draught (8 bits, 1/10 metre units)
    auto drft_r = reader.read_uint(294u, 8u);
    if (!drft_r) return Error{ drft_r.error() };

    // Bits 302–421: destination (20 characters × 6 bits)
    auto dest_r = reader.read_text(302u, 20u);
    if (!dest_r) return Error{ dest_r.error() };

    // Bit 422: DTE flag
    auto dte_r = reader.read_bool(422u);
    if (!dte_r) return Error{ dte_r.error() };

    // Bit 423: spare (ignored on decode)

    return std::unique_ptr<Message>(new Msg5StaticAndVoyageData(
        static_cast<uint32_t>(mmsi_r.value()),
        static_cast<uint8_t>(ri_r.value()),
        static_cast<uint8_t>(ver_r.value()),
        static_cast<uint32_t>(imo_r.value()),
        std::move(cs_r.value()),
        std::move(name_r.value()),
        static_cast<uint8_t>(ship_r.value()),
        static_cast<uint16_t>(bow_r.value()),
        static_cast<uint16_t>(stern_r.value()),
        static_cast<uint8_t>(port_r.value()),
        static_cast<uint8_t>(stbd_r.value()),
        static_cast<uint8_t>(epfd_r.value()),
        static_cast<uint8_t>(eta_mo_r.value()),
        static_cast<uint8_t>(eta_dy_r.value()),
        static_cast<uint8_t>(eta_hr_r.value()),
        static_cast<uint8_t>(eta_mn_r.value()),
        static_cast<uint8_t>(drft_r.value()),
        std::move(dest_r.value()),
        dte_r.value()
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg5StaticAndVoyageData::encode() const
{
    BitWriter w(kMsg5BitLength);

    auto r = w.write_uint(static_cast<uint64_t>(MessageType::StaticAndVoyageData), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(ais_version_, 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(imo_number_, 30u);
    if (!r) return Error{ r.error() };

    r = w.write_text(call_sign_, 7u);
    if (!r) return Error{ r.error() };

    r = w.write_text(vessel_name_, 20u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(ship_type_, 8u);
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

    r = w.write_uint(eta_month_, 4u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(eta_day_, 5u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(eta_hour_, 5u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(eta_minute_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(draught_, 8u);
    if (!r) return Error{ r.error() };

    r = w.write_text(destination_, 20u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(dte_);
    if (!r) return Error{ r.error() };

    // Bits 423–424: spare, written as zero.
    r = w.write_uint(0u, 2u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg5BitLength);
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg5(const BitReader& reader)
{
    return Msg5StaticAndVoyageData::decode(reader);
}

} // namespace aislib