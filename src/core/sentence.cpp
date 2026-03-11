#include "aislib/sentence.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstring>
#include <sstream>

namespace aislib {

// =============================================================================
// Internal helpers
// =============================================================================

/**
 * @brief Set of recognised AIS talker IDs per ITU-R M.1371-5 Annex 8.
 *
 * The talker ID is the two-character prefix of the NMEA sentence type field,
 * e.g. "AI" in "!AIVDM".
 */
static constexpr std::array<const char*, 9> kValidTalkers = {
    "AB", "AI", "AN", "AR", "AS", "AT", "AX", "BS", "SA"
};

/**
 * @brief Returns true if the given two-character string is a valid AIS talker ID.
 * @param talker  Pointer to exactly two characters (not null-terminated required).
 */
static bool is_valid_talker(const std::string& talker) noexcept {
    if (talker.size() != 2u) return false;
    for (const char* t : kValidTalkers) {
        if (talker[0] == t[0] && talker[1] == t[1]) return true;
    }
    return false;
}

/**
 * @brief Splits a string on a single delimiter character, returning a vector
 *        of substrings.  Empty fields (adjacent delimiters) produce empty
 *        strings in the output.
 *
 * @param str    The string to split.
 * @param delim  The delimiter character.
 * @return       Vector of field strings.
 */
static std::vector<std::string> split_fields(const std::string& str, char delim) {
    std::vector<std::string> fields;
    fields.reserve(8u);
    std::size_t start = 0u;
    for (std::size_t i = 0u; i <= str.size(); ++i) {
        if (i == str.size() || str[i] == delim) {
            fields.push_back(str.substr(start, i - start));
            start = i + 1u;
        }
    }
    return fields;
}

/**
 * @brief Parses an unsigned decimal integer from a string.
 *
 * @param s    The source string.
 * @param out  Output parameter for the parsed value.
 * @return     True on success, false if the string is not a valid decimal integer
 *             or if it is empty.
 */
static bool parse_uint8(const std::string& s, uint8_t& out) noexcept {
    if (s.empty()) return false;
    unsigned int v = 0;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        v = v * 10u + static_cast<unsigned int>(c - '0');
        if (v > 255u) return false;
    }
    out = static_cast<uint8_t>(v);
    return true;
}

/**
 * @brief Parses a two-digit uppercase hexadecimal string into a uint8_t.
 *
 * @param s    A string of exactly two hexadecimal characters.
 * @param out  Output parameter for the parsed value.
 * @return     True on success.
 */
static bool parse_hex8(const std::string& s, uint8_t& out) noexcept {
    if (s.size() != 2u) return false;
    uint8_t result = 0u;
    for (char c : s) {
        result <<= 4u;
        if (c >= '0' && c <= '9')      result |= static_cast<uint8_t>(c - '0');
        else if (c >= 'A' && c <= 'F') result |= static_cast<uint8_t>(c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') result |= static_cast<uint8_t>(c - 'a' + 10);
        else return false;
    }
    out = result;
    return true;
}

// =============================================================================
// Sentence::compute_checksum
// =============================================================================

std::string Sentence::compute_checksum(const std::string& data) {
    uint8_t xor_val = 0u;
    for (char c : data) {
        xor_val ^= static_cast<uint8_t>(c);
    }
    // Format as two uppercase hex digits.
    static const char hex_digits[] = "0123456789ABCDEF";
    std::string result(2, '0');
    result[0] = hex_digits[(xor_val >> 4u) & 0x0Fu];
    result[1] = hex_digits[xor_val & 0x0Fu];
    return result;
}

// =============================================================================
// Sentence::parse
// =============================================================================

Result<Sentence> Sentence::parse(const std::string& raw) {
    if (raw.empty()) {
        return Error{ ErrorCode::SentenceEmpty };
    }

    // Strip trailing CR and/or LF.
    std::size_t end = raw.size();
    while (end > 0 && (raw[end - 1] == '\r' || raw[end - 1] == '\n')) {
        --end;
    }

    const std::string s = raw.substr(0, end);

    if (s.size() < 14u) {
        // Minimum realistic sentence: "!AIVDM,1,1,,A,0,0*XX"
        return Error{ ErrorCode::SentenceTooShort };
    }

    // Must begin with '!' (AIS) or '$' (NMEA standard).
    if (s[0] != '!' && s[0] != '$') {
        return Error{ ErrorCode::SentenceInvalidStart };
    }

    // Locate the checksum delimiter '*'.
    const std::size_t star_pos = s.rfind('*');
    if (star_pos == std::string::npos) {
        return Error{ ErrorCode::SentenceMissingChecksum };
    }

    // The checksum field is the two characters after '*'.
    if (star_pos + 3u > s.size()) {
        return Error{ ErrorCode::SentenceMissingChecksum };
    }

    const std::string checksum_str = s.substr(star_pos + 1u, 2u);
    uint8_t transmitted_checksum = 0u;
    if (!parse_hex8(checksum_str, transmitted_checksum)) {
        return Error{ ErrorCode::SentenceChecksumBad, checksum_str };
    }

    // The checksum covers everything between '!' (exclusive) and '*' (exclusive).
    const std::string checksum_region = s.substr(1u, star_pos - 1u);
    const std::string computed_str    = Sentence::compute_checksum(checksum_region);
    uint8_t computed_checksum = 0u;
    parse_hex8(computed_str, computed_checksum); // always succeeds for our own output

    if (computed_checksum != transmitted_checksum) {
        return Error{ ErrorCode::SentenceChecksumBad,
                      "expected " + computed_str + " got " + checksum_str };
    }

    // The sentence body is everything from byte 1 up to (not including) '*'.
    const std::string body = checksum_region;

    // Split on commas.  Expected fields:
    //   [0] AIVDM / AIVDO   (talker + type, 5 chars)
    //   [1] total part count
    //   [2] part index
    //   [3] sequential message id (may be empty)
    //   [4] channel
    //   [5] payload
    //   [6] fill bits
    const std::vector<std::string> fields = split_fields(body, ',');

    if (fields.size() < 7u) {
        return Error{ ErrorCode::SentenceFieldMissing,
                      "expected 7 comma-separated fields, got " +
                      std::to_string(fields.size()) };
    }

    // Parse the talker + type field, e.g. "AIVDM".
    const std::string& type_field = fields[0];
    if (type_field.size() != 5u) {
        return Error{ ErrorCode::SentenceFieldInvalid, type_field };
    }

    const std::string talker        = type_field.substr(0u, 2u);
    const std::string sentence_type = type_field.substr(2u, 3u);

    if (!is_valid_talker(talker)) {
        return Error{ ErrorCode::SentenceTalkerInvalid, talker };
    }

    if (sentence_type != "VDM" && sentence_type != "VDO") {
        return Error{ ErrorCode::SentenceTypeInvalid, sentence_type };
    }

    // Part count.
    uint8_t part_count = 0u;
    if (!parse_uint8(fields[1], part_count) || part_count == 0u || part_count > kMaxSentenceParts) {
        return Error{ ErrorCode::MultipartCountInvalid, fields[1] };
    }

    // Part index (1-based).
    uint8_t part_index = 0u;
    if (!parse_uint8(fields[2], part_index) || part_index == 0u || part_index > part_count) {
        return Error{ ErrorCode::SentenceFieldInvalid, "part_index=" + fields[2] };
    }

    // Sequential message ID: empty for single-part, digit for multi-part.
    std::optional<uint8_t> seq_id;
    if (!fields[3].empty()) {
        uint8_t sid = 0u;
        if (!parse_uint8(fields[3], sid) || sid > kMaxSeqId) {
            return Error{ ErrorCode::MultipartSeqInvalid, fields[3] };
        }
        seq_id = sid;
    } else if (part_count > 1u) {
        // Multi-part messages must carry a seq_id.
        return Error{ ErrorCode::MultipartSeqInvalid, "empty seq_id for multi-part sentence" };
    }

    // Channel: 'A' or 'B' (or '1'/'2' in some implementations; accept any single char).
    char channel = 'A';
    if (!fields[4].empty()) {
        channel = fields[4][0];
    }

    // Payload.
    const std::string& payload = fields[5];
    if (payload.empty() && part_count == 1u) {
        // Single-part messages must have a payload.
        return Error{ ErrorCode::PayloadEmpty };
    }

    // Fill bits.
    uint8_t fill_bits = 0u;
    if (!parse_uint8(fields[6], fill_bits) || fill_bits > 5u) {
        return Error{ ErrorCode::PayloadFillBitsInvalid, fields[6] };
    }

    // Assemble the Sentence.
    Sentence sentence;
    sentence.talker_        = talker;
    sentence.sentence_type_ = sentence_type;
    sentence.part_count_    = part_count;
    sentence.part_index_    = part_index;
    sentence.seq_id_        = seq_id;
    sentence.channel_       = channel;
    sentence.payload_       = payload;
    sentence.fill_bits_     = fill_bits;

    return sentence;
}

// =============================================================================
// SentenceAssembler
// =============================================================================

Result<std::optional<AssembledMessage>> SentenceAssembler::feed(const Sentence& sentence) {
    // Single-part messages are assembled immediately without buffering.
    if (sentence.is_single_part()) {
        AssembledMessage msg;
        msg.payload        = sentence.payload();
        msg.fill_bits      = sentence.fill_bits();
        msg.is_own_vessel  = (sentence.sentence_type() == "VDO");
        msg.talker         = sentence.talker();
        return std::optional<AssembledMessage>{ std::move(msg) };
    }

    // Multi-part path.
    const uint8_t part_count = sentence.part_count();
    const uint8_t part_index = sentence.part_index();

    if (part_count == 0u || part_count > kMaxSentenceParts) {
        return Error{ ErrorCode::MultipartCountInvalid };
    }

    if (!sentence.seq_id().has_value()) {
        return Error{ ErrorCode::MultipartSeqInvalid };
    }

    const uint8_t seq_id = *sentence.seq_id();

    // TODO(phase-2): The reassembly key is seq_id alone, which is the value
    // carried in the NMEA sentence.  In a live AIS feed two independent
    // transmitters can legally use the same seq_id concurrently on the same
    // channel, making the key ambiguous.  The correct disambiguation key is
    // (channel, seq_id) or (channel, seq_id, talker).  Addressing this
    // requires changing sequences_ to an unordered_map keyed on a small
    // struct with a custom hash; deferred to keep Phase 1 focused on the
    // core parsing infrastructure.

    auto it = sequences_.find(seq_id);

    if (it == sequences_.end()) {
        // First part of a new sequence.
        if (part_index != 1u) {
            return Error{ ErrorCode::MultipartOutOfOrder,
                          "first received part has index " + std::to_string(part_index) };
        }
        Sequence seq;
        seq.part_count    = part_count;
        seq.received      = 0u;
        seq.parts.resize(part_count);
        seq.talker        = sentence.talker();
        seq.is_own_vessel = (sentence.sentence_type() == "VDO");
        it = sequences_.emplace(seq_id, std::move(seq)).first;
    } else {
        Sequence& existing = it->second;

        // Validate consistency with what we already know.
        if (existing.part_count != part_count) {
            return Error{ ErrorCode::MultipartCountInvalid,
                          "part count mismatch in seq_id " + std::to_string(seq_id) };
        }

        const uint8_t expected_next = existing.received + 1u;
        if (part_index != expected_next) {
            if (part_index <= existing.received) {
                return Error{ ErrorCode::MultipartDuplicate,
                              "seq_id=" + std::to_string(seq_id) +
                              " part=" + std::to_string(part_index) };
            }
            return Error{ ErrorCode::MultipartOutOfOrder,
                          "expected part " + std::to_string(expected_next) +
                          " got " + std::to_string(part_index) };
        }
    }

    Sequence& seq = it->second;
    seq.parts[part_index - 1u] = sentence.payload();
    ++seq.received;

    if (seq.received < seq.part_count) {
        // More parts expected.
        return std::optional<AssembledMessage>{};
    }

    // All parts received.  Concatenate and return.
    AssembledMessage msg;
    msg.fill_bits     = sentence.fill_bits(); // only the final part's fill bits matter
    msg.is_own_vessel = seq.is_own_vessel;
    msg.talker        = seq.talker;

    for (const std::string& part : seq.parts) {
        msg.payload += part;
    }

    sequences_.erase(it);
    return std::optional<AssembledMessage>{ std::move(msg) };
}

void SentenceAssembler::reset() noexcept {
    sequences_.clear();
}

std::size_t SentenceAssembler::pending_count() const noexcept {
    return sequences_.size();
}

} // namespace aislib