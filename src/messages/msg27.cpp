#include "aislib/messages/msg27.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

static constexpr std::size_t kMsg27BitLength = 96u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg27LongRangeBroadcast::Msg27LongRangeBroadcast(
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
) noexcept
    : Message(mmsi, repeat_indicator)
    , position_accuracy_(position_accuracy)
    , raim_(raim)
    , nav_status_(nav_status)
    , longitude_(longitude)
    , latitude_(latitude)
    , sog_(sog)
    , cog_(cog)
    , gnss_position_status_(gnss_position_status)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg27LongRangeBroadcast::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg27BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    if (message_type_from_id(static_cast<uint8_t>(type_r.value())) !=
            MessageType::PositionReportLongRange) {
        return Error{ ErrorCode::MessageDecodeFailure };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bit 38: position accuracy
    auto pa_r = reader.read_uint(38u, 1u);
    if (!pa_r) return Error{ pa_r.error() };

    // Bit 39: RAIM flag
    auto raim_r = reader.read_uint(39u, 1u);
    if (!raim_r) return Error{ raim_r.error() };

    // Bits 40–43: navigation status (4 bits)
    auto nav_r = reader.read_uint(40u, 4u);
    if (!nav_r) return Error{ nav_r.error() };

    // Bits 44–61: longitude (18 bits, signed two's complement, 1/10 degree)
    auto lon_r = reader.read_int(44u, 18u);
    if (!lon_r) return Error{ lon_r.error() };

    // Bits 62–78: latitude (17 bits, signed two's complement, 1/10 degree)
    auto lat_r = reader.read_int(62u, 17u);
    if (!lat_r) return Error{ lat_r.error() };

    // Bits 79–84: speed over ground (6 bits, whole knots)
    auto sog_r = reader.read_uint(79u, 6u);
    if (!sog_r) return Error{ sog_r.error() };

    // Bits 85–93: course over ground (9 bits, whole degrees)
    auto cog_r = reader.read_uint(85u, 9u);
    if (!cog_r) return Error{ cog_r.error() };

    // Bit 94: GNSS position status
    auto gnss_r = reader.read_uint(94u, 1u);
    if (!gnss_r) return Error{ gnss_r.error() };

    // Bit 95: spare (1 bit, ignored)

    return std::unique_ptr<Message>(new Msg27LongRangeBroadcast(
        static_cast<uint32_t>(mmsi_r.value()),
        static_cast<uint8_t>(ri_r.value()),
        pa_r.value() != 0u,
        raim_r.value() != 0u,
        static_cast<uint8_t>(nav_r.value()),
        static_cast<int32_t>(lon_r.value()),
        static_cast<int32_t>(lat_r.value()),
        static_cast<uint8_t>(sog_r.value()),
        static_cast<uint16_t>(cog_r.value()),
        gnss_r.value() != 0u
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg27LongRangeBroadcast::encode() const
{
    BitWriter w(kMsg27BitLength);

    auto r = w.write_uint(
        static_cast<uint64_t>(MessageType::PositionReportLongRange), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(position_accuracy_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(raim_);
    if (!r) return Error{ r.error() };

    r = w.write_uint(nav_status_, 4u);
    if (!r) return Error{ r.error() };

    r = w.write_int(longitude_, 18u);
    if (!r) return Error{ r.error() };

    r = w.write_int(latitude_, 17u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(sog_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(cog_, 9u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(gnss_position_status_);
    if (!r) return Error{ r.error() };

    // 1 spare bit, written as zero
    r = w.write_uint(0u, 1u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg27BitLength);
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

std::optional<double> Msg27LongRangeBroadcast::longitude() const noexcept
{
    if (longitude_ == kMsg27LongitudeUnavailable) return std::nullopt;
    return static_cast<double>(longitude_) / 10.0;
}

std::optional<double> Msg27LongRangeBroadcast::latitude() const noexcept
{
    if (latitude_ == kMsg27LatitudeUnavailable) return std::nullopt;
    return static_cast<double>(latitude_) / 10.0;
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg27(const BitReader& reader)
{
    return Msg27LongRangeBroadcast::decode(reader);
}

} // namespace aislib