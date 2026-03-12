#include "aislib/messages/msg10.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"

namespace aislib {

static constexpr std::size_t kMsg10BitLength = 72u;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------

Msg10UTCDateInquiry::Msg10UTCDateInquiry(
    uint32_t mmsi,
    uint8_t  repeat_indicator,
    uint32_t destination_mmsi
) noexcept
    : Message(mmsi, repeat_indicator)
    , destination_mmsi_(destination_mmsi)
{}

// ---------------------------------------------------------------------------
// decode()
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> Msg10UTCDateInquiry::decode(const BitReader& reader)
{
    if (reader.bit_length() < kMsg10BitLength) {
        return Error{ ErrorCode::PayloadTooShort };
    }

    // Bits 0–5: message type
    auto type_r = reader.read_uint(0u, 6u);
    if (!type_r) return Error{ type_r.error() };
    if (message_type_from_id(static_cast<uint8_t>(type_r.value())) !=
            MessageType::UTCDateInquiry) {
        return Error{ ErrorCode::MessageDecodeFailure };
    }

    // Bits 6–7: repeat indicator
    auto ri_r = reader.read_uint(6u, 2u);
    if (!ri_r) return Error{ ri_r.error() };

    // Bits 8–37: MMSI
    auto mmsi_r = reader.read_uint(8u, 30u);
    if (!mmsi_r) return Error{ mmsi_r.error() };

    // Bits 38–39: spare (2 bits, ignored)

    // Bits 40–69: destination MMSI
    auto dest_r = reader.read_uint(40u, 30u);
    if (!dest_r) return Error{ dest_r.error() };

    // Bits 70–71: spare (2 bits, ignored)

    return std::unique_ptr<Message>(new Msg10UTCDateInquiry(
        static_cast<uint32_t>(mmsi_r.value()),
        static_cast<uint8_t>(ri_r.value()),
        static_cast<uint32_t>(dest_r.value())
    ));
}

// ---------------------------------------------------------------------------
// encode()
// ---------------------------------------------------------------------------

Result<std::pair<std::string, uint8_t>> Msg10UTCDateInquiry::encode() const
{
    BitWriter w(kMsg10BitLength);

    auto r = w.write_uint(static_cast<uint64_t>(MessageType::UTCDateInquiry), 6u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(repeat_indicator(), 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(mmsi(), 30u);
    if (!r) return Error{ r.error() };

    // 2 spare bits
    r = w.write_uint(0u, 2u);
    if (!r) return Error{ r.error() };

    r = w.write_uint(destination_mmsi_, 30u);
    if (!r) return Error{ r.error() };

    // 2 spare bits
    r = w.write_uint(0u, 2u);
    if (!r) return Error{ r.error() };

    return encode_payload(w.buffer(), kMsg10BitLength);
}

// ---------------------------------------------------------------------------
// DecoderFn wrapper
// ---------------------------------------------------------------------------

Result<std::unique_ptr<Message>> decode_msg10(const BitReader& reader)
{
    return Msg10UTCDateInquiry::decode(reader);
}

} // namespace aislib