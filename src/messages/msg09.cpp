#include "aislib/messages/msg09.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

static constexpr std::size_t kMsg9BitLength = 168u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg9SARPositionReport::Msg9SARPositionReport(
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
) noexcept
    : Message(mmsi, repeat_indicator)
    , altitude_(altitude)
    , sog_(sog)
    , position_accuracy_(position_accuracy)
    , longitude_(longitude)
    , latitude_(latitude)
    , cog_(cog)
    , time_stamp_(time_stamp)
    , regional_reserved_(regional_reserved)
    , dte_(dte)
    , assigned_(assigned)
    , raim_(raim)
    , radio_status_(radio_status)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg9SARPositionReport::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg9BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    if (message_type_from_id(static_cast<uint8_t>(type_r.value())) !=
            MessageType::StandardSARPositionReport) {
        return Error{ ErrorCode::MessageDecodeFailure };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–49: altitude in metres (12 bits)
    auto alt_r = reader.read_uint(38u, 12u);
    if (!alt_r) return Error{ alt_r.error() };

    // Bits 50–59: speed over ground in whole knots (10 bits)
    auto sog_r = reader.read_uint(50u, 10u);
    if (!sog_r) return Error{ sog_r.error() };

    // Bit 60: position accuracy
    auto pa_r = reader.read_uint(60u, 1u);
    if (!pa_r) return Error{ pa_r.error() };

    // Bits 61–88: longitude (28 bits, signed two's complement)
    auto lon_r = reader.read_int(61u, 28u);
    if (!lon_r) return Error{ lon_r.error() };

    // Bits 89–115: latitude (27 bits, signed two's complement)
    auto lat_r = reader.read_int(89u, 27u);
    if (!lat_r) return Error{ lat_r.error() };

    // Bits 116–127: course over ground (12 bits)
    auto cog_r = reader.read_uint(116u, 12u);
    if (!cog_r) return Error{ cog_r.error() };

    // Bits 128–133: time stamp (6 bits)
    auto ts_r = reader.read_uint(128u, 6u);
    if (!ts_r) return Error{ ts_r.error() };

    // Bits 134–139: regional reserved (6 bits)
    auto rr_r = reader.read_uint(134u, 6u);
    if (!rr_r) return Error{ rr_r.error() };

    // Bit 140: DTE flag
    auto dte_r = reader.read_uint(140u, 1u);
    if (!dte_r) return Error{ dte_r.error() };

    // Bits 141–145: spare (5 bits, ignored)

    // Bit 146: assigned mode flag
    auto asgn_r = reader.read_uint(146u, 1u);
    if (!asgn_r) return Error{ asgn_r.error() };

    // Bit 147: RAIM flag
    auto raim_r = reader.read_uint(147u, 1u);
    if (!raim_r) return Error{ raim_r.error() };

    // Bits 148–167: radio status (20 bits)
    auto rs_r = reader.read_uint(148u, 20u);
    if (!rs_r) return Error{ rs_r.error() };

    return std::unique_ptr<Message>(new Msg9SARPositionReport(
        static_cast<uint32_t>(mmsi_r.value()),
        static_cast<uint8_t>(ri_r.value()),
        static_cast<uint16_t>(alt_r.value()),
        static_cast<uint16_t>(sog_r.value()),
        pa_r.value() != 0u,
        static_cast<int32_t>(lon_r.value()),
        static_cast<int32_t>(lat_r.value()),
        static_cast<uint16_t>(cog_r.value()),
        static_cast<uint8_t>(ts_r.value()),
        static_cast<uint8_t>(rr_r.value()),
        dte_r.value() != 0u,
        asgn_r.value() != 0u,
        raim_r.value() != 0u,
        static_cast<uint32_t>(rs_r.value())
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg9SARPositionReport::encode() const
{
    BitWriter w(kMsg9BitLength);

    auto r = w.write_uint(
        static_cast<uint64_t>(MessageType::StandardSARPositionReport), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(altitude_, 12u);
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

    r = w.write_uint(time_stamp_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(regional_reserved_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(dte_);
    if (!r) return Error{ r.error() };

    // 5 spare bits, written as zero
    r = w.write_uint(0u, 5u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(assigned_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(raim_);
    if (!r) return Error{ r.error() };

    r = w.write_uint(radio_status_, 20u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg9BitLength);
}

// ---------------------------------------------------------------------------
// Coordinate and altitude helpers
// ---------------------------------------------------------------------------

std::optional<uint16_t> Msg9SARPositionReport::altitude_m() const noexcept
{
    if (altitude_ == kAltitudeUnavailable) return std::nullopt;
    return altitude_;
}

std::optional<double> Msg9SARPositionReport::longitude() const noexcept
{
    if (longitude_ == kLongitudeUnavailable) return std::nullopt;
    return static_cast<double>(longitude_) / 600000.0;
}

std::optional<double> Msg9SARPositionReport::latitude() const noexcept
{
    if (latitude_ == kLatitudeUnavailable) return std::nullopt;
    return static_cast<double>(latitude_) / 600000.0;
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg9(const BitReader& reader)
{
    return Msg9SARPositionReport::decode(reader);
}

} // namespace aislib