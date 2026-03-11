#include "aislib/messages/msg1_3.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

#include <optional>

namespace aislib {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/** Payload bit length for message types 1, 2, and 3. */
static constexpr std::size_t kMsg1_3BitLength = 168u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg1_3PositionReportClassA::Msg1_3PositionReportClassA(
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
) noexcept
    : Message(mmsi, repeat_indicator)
    , type_(type)
    , nav_status_(nav_status)
    , rot_(rot)
    , sog_(sog)
    , position_accuracy_(position_accuracy)
    , longitude_(longitude)
    , latitude_(latitude)
    , cog_(cog)
    , true_heading_(true_heading)
    , time_stamp_(time_stamp)
    , manoeuvre_indicator_(manoeuvre_indicator)
    , raim_(raim)
    , radio_status_(radio_status)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg1_3PositionReportClassA::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg1_3BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bit 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    const auto mt = message_type_from_id(static_cast<uint8_t>(type_r.value()));
    if (mt != MessageType::PositionReportClassA         &&
        mt != MessageType::PositionReportClassA_Assigned &&
        mt != MessageType::PositionReportClassA_Response) {
        return Error{ ErrorCode::MessageDecodeFailure, "unexpected message type" };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI (30 bits)
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–41: navigation status (4 bits)
    auto ns_r = reader.read_uint(38u, 4u);
    if (!ns_r) return Error{ ns_r.error() };

    // Bits 42–49: rate of turn, signed (8 bits)
    auto rot_r = reader.read_int(42u, 8u);
    if (!rot_r) return Error{ rot_r.error() };

    // Bits 50–59: speed over ground (10 bits)
    auto sog_r = reader.read_uint(50u, 10u);
    if (!sog_r) return Error{ sog_r.error() };

    // Bit 60: position accuracy
    auto pa_r = reader.read_uint(60u, 1u);
    if (!pa_r) return Error{ pa_r.error() };

    // Bits 61–88: longitude, signed (28 bits)
    auto lon_r = reader.read_int(61u, 28u);
    if (!lon_r) return Error{ lon_r.error() };

    // Bits 89–115: latitude, signed (27 bits)
    auto lat_r = reader.read_int(89u, 27u);
    if (!lat_r) return Error{ lat_r.error() };

    // Bits 116–127: course over ground (12 bits)
    auto cog_r = reader.read_uint(116u, 12u);
    if (!cog_r) return Error{ cog_r.error() };

    // Bits 128–136: true heading (9 bits)
    auto hdg_r = reader.read_uint(128u, 9u);
    if (!hdg_r) return Error{ hdg_r.error() };

    // Bits 137–142: time stamp (6 bits)
    auto ts_r = reader.read_uint(137u, 6u);
    if (!ts_r) return Error{ ts_r.error() };

    // Bits 143–144: manoeuvre indicator (2 bits)
    auto mi_r = reader.read_uint(143u, 2u);
    if (!mi_r) return Error{ mi_r.error() };

    // Bits 145–147: spare (3 bits, ignored)

    // Bit 148: RAIM flag
    auto raim_r = reader.read_uint(148u, 1u);
    if (!raim_r) return Error{ raim_r.error() };

    // Bits 149–167: radio status (19 bits)
    auto rs_r = reader.read_uint(149u, 19u);
    if (!rs_r) return Error{ rs_r.error() };

    return std::unique_ptr<Message>(new Msg1_3PositionReportClassA(
        mt,
        static_cast<uint32_t>(mmsi_r.value()),
        static_cast<uint8_t>(ri_r.value()),
        static_cast<uint8_t>(ns_r.value()),
        static_cast<int8_t>(rot_r.value()),
        static_cast<uint16_t>(sog_r.value()),
        pa_r.value() != 0u,
        static_cast<int32_t>(lon_r.value()),
        static_cast<int32_t>(lat_r.value()),
        static_cast<uint16_t>(cog_r.value()),
        static_cast<uint16_t>(hdg_r.value()),
        static_cast<uint8_t>(ts_r.value()),
        static_cast<uint8_t>(mi_r.value()),
        raim_r.value() != 0u,
        static_cast<uint32_t>(rs_r.value())
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg1_3PositionReportClassA::encode() const
{
    BitWriter w(kMsg1_3BitLength);

    auto r = w.write_uint(static_cast<uint64_t>(type_), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(nav_status_, 4u);
    if (!r) return Error{ r.error() };

    r = w.write_int(rot_, 8u);
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

    r = w.write_uint(manoeuvre_indicator_, 2u);
    if (!r) return Error{ r.error() };

    // 3 spare bits, written as zero
    r = w.write_uint(0u, 3u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(raim_);
    if (!r) return Error{ r.error() };

    r = w.write_uint(radio_status_, 19u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg1_3BitLength);
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

std::optional<double> Msg1_3PositionReportClassA::longitude() const noexcept
{
    if (longitude_ == kLongitudeUnavailable) return std::nullopt;
    return static_cast<double>(longitude_) / 600000.0;
}

std::optional<double> Msg1_3PositionReportClassA::latitude() const noexcept
{
    if (latitude_ == kLatitudeUnavailable) return std::nullopt;
    return static_cast<double>(latitude_) / 600000.0;
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg1_3(const BitReader& reader)
{
    return Msg1_3PositionReportClassA::decode(reader);
}

} // namespace aislib
