#include "aislib/messages/msg19.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

static constexpr std::size_t kMsg19BitLength = 312u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg19ExtendedClassBPositionReport::Msg19ExtendedClassBPositionReport(
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
) noexcept
    : Message(mmsi, repeat_indicator)
    , sog_(sog)
    , position_accuracy_(position_accuracy)
    , longitude_(longitude)
    , latitude_(latitude)
    , cog_(cog)
    , true_heading_(true_heading)
    , time_stamp_(time_stamp)
    , regional_reserved_(regional_reserved)
    , name_(std::move(name))
    , ship_type_(ship_type)
    , dim_bow_(dim_bow)
    , dim_stern_(dim_stern)
    , dim_port_(dim_port)
    , dim_starboard_(dim_starboard)
    , epfd_type_(epfd_type)
    , raim_(raim)
    , dte_(dte)
    , assigned_(assigned)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg19ExtendedClassBPositionReport::decode(
    const BitReader& reader)
{
    if (reader.bit_length() < kMsg19BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    if (message_type_from_id(static_cast<uint8_t>(type_r.value())) !=
            MessageType::ExtendedClassBPositionReport) {
        return Error{ ErrorCode::MessageDecodeFailure };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–47: reserved (10 bits, ignored)

    // Bits 48–57: speed over ground (10 bits)
    auto sog_r = reader.read_uint(48u, 10u);
    if (!sog_r) return Error{ sog_r.error() };

    // Bit 58: position accuracy
    auto pa_r = reader.read_uint(58u, 1u);
    if (!pa_r) return Error{ pa_r.error() };

    // Bits 59–86: longitude (28 bits, signed two's complement)
    auto lon_r = reader.read_int(59u, 28u);
    if (!lon_r) return Error{ lon_r.error() };

    // Bits 87–113: latitude (27 bits, signed two's complement)
    auto lat_r = reader.read_int(87u, 27u);
    if (!lat_r) return Error{ lat_r.error() };

    // Bits 114–125: course over ground (12 bits)
    auto cog_r = reader.read_uint(114u, 12u);
    if (!cog_r) return Error{ cog_r.error() };

    // Bits 126–134: true heading (9 bits)
    auto hdg_r = reader.read_uint(126u, 9u);
    if (!hdg_r) return Error{ hdg_r.error() };

    // Bits 135–140: time stamp (6 bits)
    auto ts_r = reader.read_uint(135u, 6u);
    if (!ts_r) return Error{ ts_r.error() };

    // Bits 141–144: regional reserved (4 bits)
    auto rr_r = reader.read_uint(141u, 4u);
    if (!rr_r) return Error{ rr_r.error() };

    // Bits 145–264: vessel name (120 bits = 20 × 6-bit ASCII)
    auto name_r = reader.read_text(145u, 20u);
    if (!name_r) return Error{ name_r.error() };

    // Bits 265–272: ship and cargo type (8 bits)
    auto stype_r = reader.read_uint(265u, 8u);
    if (!stype_r) return Error{ stype_r.error() };

    // Bits 273–281: dimension to bow (9 bits)
    auto bow_r = reader.read_uint(273u, 9u);
    if (!bow_r) return Error{ bow_r.error() };

    // Bits 282–290: dimension to stern (9 bits)
    auto stern_r = reader.read_uint(282u, 9u);
    if (!stern_r) return Error{ stern_r.error() };

    // Bits 291–296: dimension to port (6 bits)
    auto port_r = reader.read_uint(291u, 6u);
    if (!port_r) return Error{ port_r.error() };

    // Bits 297–302: dimension to starboard (6 bits)
    auto stbd_r = reader.read_uint(297u, 6u);
    if (!stbd_r) return Error{ stbd_r.error() };

    // Bits 303–306: EPFD type (4 bits)
    auto epfd_r = reader.read_uint(303u, 4u);
    if (!epfd_r) return Error{ epfd_r.error() };

    // Bit 307: RAIM flag
    auto raim_r = reader.read_uint(307u, 1u);
    if (!raim_r) return Error{ raim_r.error() };

    // Bit 308: DTE flag
    auto dte_r = reader.read_uint(308u, 1u);
    if (!dte_r) return Error{ dte_r.error() };

    // Bit 309: assigned mode flag
    auto asgn_r = reader.read_uint(309u, 1u);
    if (!asgn_r) return Error{ asgn_r.error() };

    // Bits 310–311: spare (2 bits, ignored)

    return std::unique_ptr<Message>(new Msg19ExtendedClassBPositionReport(
        static_cast<uint32_t>(mmsi_r.value()),
        static_cast<uint8_t>(ri_r.value()),
        static_cast<uint16_t>(sog_r.value()),
        pa_r.value() != 0u,
        static_cast<int32_t>(lon_r.value()),
        static_cast<int32_t>(lat_r.value()),
        static_cast<uint16_t>(cog_r.value()),
        static_cast<uint16_t>(hdg_r.value()),
        static_cast<uint8_t>(ts_r.value()),
        static_cast<uint8_t>(rr_r.value()),
        name_r.value(),
        static_cast<uint8_t>(stype_r.value()),
        static_cast<uint16_t>(bow_r.value()),
        static_cast<uint16_t>(stern_r.value()),
        static_cast<uint8_t>(port_r.value()),
        static_cast<uint8_t>(stbd_r.value()),
        static_cast<uint8_t>(epfd_r.value()),
        raim_r.value() != 0u,
        dte_r.value() != 0u,
        asgn_r.value() != 0u
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg19ExtendedClassBPositionReport::encode() const
{
    BitWriter w(kMsg19BitLength);

    auto r = w.write_uint(
        static_cast<uint64_t>(MessageType::ExtendedClassBPositionReport), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    // 10 reserved bits, written as zero
    r = w.write_uint(0u, 10u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(sog_, 10u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(position_accuracy_);
    if (!r) return Error{ r.error() };

    r = w.write_int(longitude_, 28u);
    if (!r) return Error{ r.error() };

    r = w.write_int(latitude_, 27u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(cog_, 12u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(true_heading_, 9u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(time_stamp_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(regional_reserved_, 4u);
    if (!r) return Error{ r.error() };

    r = w.write_text(name_, 20u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(ship_type_, 8u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(dim_bow_, 9u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(dim_stern_, 9u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(dim_port_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(dim_starboard_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(epfd_type_, 4u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(raim_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(dte_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(assigned_);
    if (!r) return Error{ r.error() };

    // 2 spare bits, written as zero
    r = w.write_uint(0u, 2u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg19BitLength);
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

std::optional<double> Msg19ExtendedClassBPositionReport::longitude() const noexcept
{
    if (longitude_ == kLongitudeUnavailable) return std::nullopt;
    return static_cast<double>(longitude_) / 600000.0;
}

std::optional<double> Msg19ExtendedClassBPositionReport::latitude() const noexcept
{
    if (latitude_ == kLatitudeUnavailable) return std::nullopt;
    return static_cast<double>(latitude_) / 600000.0;
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg19(const BitReader& reader)
{
    return Msg19ExtendedClassBPositionReport::decode(reader);
}

} // namespace aislib