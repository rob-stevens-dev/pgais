#include "aislib/messages/msg4_11.h"
#include "aislib/messages/msg1_3.h"   // kLongitudeUnavailable, kLatitudeUnavailable
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

static constexpr std::size_t kMsg4_11BitLength = 168u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg4_11BaseStationReport::Msg4_11BaseStationReport(
    MessageType type,
    uint32_t    mmsi,
    uint8_t     repeat_indicator,
    uint16_t    utc_year,
    uint8_t     utc_month,
    uint8_t     utc_day,
    uint8_t     utc_hour,
    uint8_t     utc_minute,
    uint8_t     utc_second,
    bool        position_accuracy,
    int32_t     longitude,
    int32_t     latitude,
    uint8_t     epfd,
    bool        raim,
    uint32_t    radio_status
) noexcept
    : Message(mmsi, repeat_indicator)
    , type_(type)
    , utc_year_(utc_year)
    , utc_month_(utc_month)
    , utc_day_(utc_day)
    , utc_hour_(utc_hour)
    , utc_minute_(utc_minute)
    , utc_second_(utc_second)
    , position_accuracy_(position_accuracy)
    , longitude_(longitude)
    , latitude_(latitude)
    , epfd_(epfd)
    , raim_(raim)
    , radio_status_(radio_status)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg4_11BaseStationReport::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg4_11BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    const auto mt = message_type_from_id(static_cast<uint8_t>(type_r.value()));
    if (mt != MessageType::BaseStationReport && mt != MessageType::UTCDateResponse) {
        return Error{ ErrorCode::MessageDecodeFailure, "unexpected message type" };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI (30 bits)
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–51: UTC year (14 bits)
    auto year_r = reader.read_uint(38u, 14u);
    if (!year_r) return Error{ year_r.error() };

    // Bits 52–55: UTC month (4 bits)
    auto month_r = reader.read_uint(52u, 4u);
    if (!month_r) return Error{ month_r.error() };

    // Bits 56–60: UTC day (5 bits)
    auto day_r = reader.read_uint(56u, 5u);
    if (!day_r) return Error{ day_r.error() };

    // Bits 61–65: UTC hour (5 bits)
    auto hour_r = reader.read_uint(61u, 5u);
    if (!hour_r) return Error{ hour_r.error() };

    // Bits 66–71: UTC minute (6 bits)
    auto min_r = reader.read_uint(66u, 6u);
    if (!min_r) return Error{ min_r.error() };

    // Bits 72–77: UTC second (6 bits)
    auto sec_r = reader.read_uint(72u, 6u);
    if (!sec_r) return Error{ sec_r.error() };

    // Bit 78: position accuracy
    auto pa_r = reader.read_uint(78u, 1u);
    if (!pa_r) return Error{ pa_r.error() };

    // Bits 79–106: longitude, signed (28 bits)
    auto lon_r = reader.read_int(79u, 28u);
    if (!lon_r) return Error{ lon_r.error() };

    // Bits 107–133: latitude, signed (27 bits)
    auto lat_r = reader.read_int(107u, 27u);
    if (!lat_r) return Error{ lat_r.error() };

    // Bits 134–137: EPFD type (4 bits)
    auto epfd_r = reader.read_uint(134u, 4u);
    if (!epfd_r) return Error{ epfd_r.error() };

    // Bits 138–147: spare (10 bits, ignored)

    // Bit 148: RAIM flag
    auto raim_r = reader.read_uint(148u, 1u);
    if (!raim_r) return Error{ raim_r.error() };

    // Bits 149–167: SOTDMA radio status (19 bits)
    auto rs_r = reader.read_uint(149u, 19u);
    if (!rs_r) return Error{ rs_r.error() };

    return std::unique_ptr<Message>(new Msg4_11BaseStationReport(
        mt,
        static_cast<uint32_t>(mmsi_r.value()),
        static_cast<uint8_t>(ri_r.value()),
        static_cast<uint16_t>(year_r.value()),
        static_cast<uint8_t>(month_r.value()),
        static_cast<uint8_t>(day_r.value()),
        static_cast<uint8_t>(hour_r.value()),
        static_cast<uint8_t>(min_r.value()),
        static_cast<uint8_t>(sec_r.value()),
        pa_r.value() != 0u,
        static_cast<int32_t>(lon_r.value()),
        static_cast<int32_t>(lat_r.value()),
        static_cast<uint8_t>(epfd_r.value()),
        raim_r.value() != 0u,
        static_cast<uint32_t>(rs_r.value())
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg4_11BaseStationReport::encode() const
{
    BitWriter w(kMsg4_11BitLength);

    auto r = w.write_uint(static_cast<uint64_t>(type_), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(utc_year_, 14u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(utc_month_, 4u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(utc_day_, 5u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(utc_hour_, 5u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(utc_minute_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(utc_second_, 6u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(position_accuracy_);
    if (!r) return Error{ r.error() };

    r = w.write_int(longitude_, 28u);
    if (!r) return Error{ r.error() };

    r = w.write_int(latitude_, 27u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(epfd_, 4u);
    if (!r) return Error{ r.error() };

    // 10 spare bits, written as zero
    r = w.write_uint(0u, 10u);
    if (!r) return Error{ r.error() };

    r = w.write_bool(raim_);
    if (!r) return Error{ r.error() };

    r = w.write_uint(radio_status_, 19u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg4_11BitLength);
}

// ---------------------------------------------------------------------------
// Coordinate helpers
// ---------------------------------------------------------------------------

std::optional<double> Msg4_11BaseStationReport::longitude() const noexcept
{
    if (longitude_ == kLongitudeUnavailable) return std::nullopt;
    return static_cast<double>(longitude_) / 600000.0;
}

std::optional<double> Msg4_11BaseStationReport::latitude() const noexcept
{
    if (latitude_ == kLatitudeUnavailable) return std::nullopt;
    return static_cast<double>(latitude_) / 600000.0;
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg4_11(const BitReader& reader)
{
    return Msg4_11BaseStationReport::decode(reader);
}

} // namespace aislib
