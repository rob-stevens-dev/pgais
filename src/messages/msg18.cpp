#include "aislib/messages/msg18.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

static constexpr std::size_t kMsg18BitLength = 168u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg18ClassBPositionReport::Msg18ClassBPositionReport(
    uint32_t mmsi,
    uint8_t  repeat_indicator,
    uint16_t sog,
    bool     position_accuracy,
    int32_t  longitude,
    int32_t  latitude,
    uint16_t cog,
    uint16_t true_heading,
    uint8_t  time_stamp,
    uint8_t  regional_reserved,
    bool     cs_flag,
    bool     display_flag,
    bool     dsc_flag,
    bool     band_flag,
    bool     msg22_flag,
    bool     assigned,
    bool     raim,
    uint32_t radio_status
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
    , cs_flag_(cs_flag)
    , display_flag_(display_flag)
    , dsc_flag_(dsc_flag)
    , band_flag_(band_flag)
    , msg22_flag_(msg22_flag)
    , assigned_(assigned)
    , raim_(raim)
    , radio_status_(radio_status)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg18ClassBPositionReport::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg18BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    if (message_type_from_id(static_cast<uint8_t>(type_r.value())) !=
            MessageType::StandardClassBPositionReport) {
        return Error{ ErrorCode::MessageDecodeFailure, "unexpected message type" };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI (30 bits)
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–45: reserved (8 bits, ignored)

    // Bits 46–55: speed over ground (10 bits)
    auto sog_r = reader.read_uint(46u, 10u);
    if (!sog_r) return Error{ sog_r.error() };

    // Bit 56: position accuracy
    auto pa_r = reader.read_uint(56u, 1u);
    if (!pa_r) return Error{ pa_r.error() };

    // Bits 57–84: longitude, signed (28 bits)
    auto lon_r = reader.read_int(57u, 28u);
    if (!lon_r) return Error{ lon_r.error() };

    // Bits 85–111: latitude, signed (27 bits)
    auto lat_r = reader.read_int(85u, 27u);
    if (!lat_r) return Error{ lat_r.error() };

    // Bits 112–123: course over ground (12 bits)
    auto cog_r = reader.read_uint(112u, 12u);
    if (!cog_r) return Error{ cog_r.error() };

    // Bits 124–132: true heading (9 bits)
    auto hdg_r = reader.read_uint(124u, 9u);
    if (!hdg_r) return Error{ hdg_r.error() };

    // Bits 133–138: time stamp (6 bits)
    auto ts_r = reader.read_uint(133u, 6u);
    if (!ts_r) return Error{ ts_r.error() };

    // Bits 139–140: regional reserved (2 bits)
    auto rr_r = reader.read_uint(139u, 2u);
    if (!rr_r) return Error{ rr_r.error() };

    // Bit 141: CS flag
    auto cs_r = reader.read_uint(141u, 1u);
    if (!cs_r) return Error{ cs_r.error() };

    // Bit 142: display flag
    auto disp_r = reader.read_uint(142u, 1u);
    if (!disp_r) return Error{ disp_r.error() };

    // Bit 143: DSC flag
    auto dsc_r = reader.read_uint(143u, 1u);
    if (!dsc_r) return Error{ dsc_r.error() };

    // Bit 144: band flag
    auto band_r = reader.read_uint(144u, 1u);
    if (!band_r) return Error{ band_r.error() };

    // Bit 145: message 22 flag
    auto m22_r = reader.read_uint(145u, 1u);
    if (!m22_r) return Error{ m22_r.error() };

    // Bit 146: assigned mode flag
    auto asgn_r = reader.read_uint(146u, 1u);
    if (!asgn_r) return Error{ asgn_r.error() };

    // Bit 147: RAIM flag
    auto raim_r = reader.read_uint(147u, 1u);
    if (!raim_r) return Error{ raim_r.error() };

    // Bits 148–167: radio status (20 bits)
    auto rs_r = reader.read_uint(148u, 20u);
    if (!rs_r) return Error{ rs_r.error() };

    return std::unique_ptr<Message>(new Msg18ClassBPositionReport(
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
        cs_r.value() != 0u,
        disp_r.value() != 0u,
        dsc_r.value() != 0u,
        band_r.value() != 0u,
        m22_r.value() != 0u,
        asgn_r.value() != 0u,
        raim_r.value() != 0u,
        static_cast<uint32_t>(rs_r.value())
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg18ClassBPositionReport::encode() const
{
    BitWriter w(kMsg18BitLength);

    auto r = w.write_uint(static_cast<uint64_t>(MessageType::StandardClassBPositionReport), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    // 8 reserved bits, written as zero
    r = w.write_uint(0u, 8u);
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

    r = w.write_uint(regional_reserved_, 2u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(cs_flag_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(display_flag_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(dsc_flag_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(band_flag_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(msg22_flag_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(assigned_);
    if (!r) return Error{ r.error() };

    r = w.write_bool(raim_);
    if (!r) return Error{ r.error() };

    r = w.write_uint(radio_status_, 20u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg18BitLength);
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

std::optional<double> Msg18ClassBPositionReport::longitude() const noexcept
{
    if (longitude_ == kLongitudeUnavailable) return std::nullopt;
    return static_cast<double>(longitude_) / 600000.0;
}

std::optional<double> Msg18ClassBPositionReport::latitude() const noexcept
{
    if (latitude_ == kLatitudeUnavailable) return std::nullopt;
    return static_cast<double>(latitude_) / 600000.0;
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg18(const BitReader& reader)
{
    return Msg18ClassBPositionReport::decode(reader);
}

} // namespace aislib
