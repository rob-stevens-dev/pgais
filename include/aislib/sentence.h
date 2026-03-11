#pragma once

#include "error.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace aislib {

/**
 * @file sentence.h
 * @brief NMEA 0183 sentence parsing, validation, and multi-part reassembly.
 *
 * A well-formed NMEA 0183 AIS sentence has the following structure:
 *
 *   !<talker><type>,<count>,<index>,<seq_id>,<payload>,<fill>*<checksum><CR><LF>
 *
 * Where:
 *   talker   — Two-character talker ID.  AIS talkers are: AB, AI, AN, AR, AS,
 *               AT, AX, BS, SA (ITU-R M.1371-5 Annex 8).
 *   type     — Sentence type identifier.  Must be "VDM" (received from others)
 *               or "VDO" (own-vessel).
 *   count    — Total number of parts in a multi-sentence message [1, 5].
 *   index    — This sentence's part number, 1-based [1, count].
 *   seq_id   — Sequential message identifier [0-9], empty for single-part.
 *   payload  — 6-bit ASCII armoured payload characters.
 *   fill     — Fill bit count [0-5].
 *   checksum — XOR of all bytes between '!' and '*', formatted as two hex digits.
 *
 * This module provides:
 *   - Sentence: a parsed, validated single NMEA sentence.
 *   - SentenceAssembler: stateful reassembly of multi-part message sequences.
 *   - AssembledMessage: a complete reassembled payload ready for decoding.
 */

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

/** Maximum number of parts allowed in a multi-sentence AIS message. */
inline constexpr uint8_t kMaxSentenceParts = 5;

/** Maximum sequential message identifier value (inclusive). */
inline constexpr uint8_t kMaxSeqId = 9;

// -----------------------------------------------------------------------------
// Sentence
// -----------------------------------------------------------------------------

/**
 * @brief Represents a single, fully parsed and validated NMEA AIS sentence.
 *
 * A Sentence object is immutable after construction.  All fields are
 * accessible via const accessors.  Construction is only possible through the
 * static @c parse() factory, which performs full validation including checksum
 * verification.
 */
class Sentence {
public:
    /**
     * @brief Parses and validates a raw NMEA sentence string.
     *
     * The input string may contain an optional leading '!' or '$' character.
     * Trailing CR and LF characters are stripped before parsing.  All fields
     * are validated; the XOR checksum is computed and compared against the
     * transmitted value.
     *
     * @param raw  The raw NMEA sentence string.
     * @return     A validated Sentence on success, or an Error on failure.
     *
     * Possible errors: SentenceEmpty, SentenceTooShort, SentenceInvalidStart,
     * SentenceMissingChecksum, SentenceChecksumBad, SentenceFieldMissing,
     * SentenceFieldInvalid, SentenceTalkerInvalid, SentenceTypeInvalid.
     */
    [[nodiscard]] static Result<Sentence> parse(const std::string& raw);

    /** @brief Returns the two-character talker ID, e.g. "AI" or "AB". */
    [[nodiscard]] const std::string& talker() const noexcept { return talker_; }

    /**
     * @brief Returns the sentence type: "VDM" for traffic from other vessels,
     *        "VDO" for own-vessel traffic.
     */
    [[nodiscard]] const std::string& sentence_type() const noexcept { return sentence_type_; }

    /** @brief Returns the total part count for this message sequence [1, 5]. */
    [[nodiscard]] uint8_t part_count() const noexcept { return part_count_; }

    /** @brief Returns the 1-based index of this sentence within the sequence. */
    [[nodiscard]] uint8_t part_index() const noexcept { return part_index_; }

    /**
     * @brief Returns the sequential message identifier [0-9], or nullopt for
     *        single-part messages.
     */
    [[nodiscard]] std::optional<uint8_t> seq_id() const noexcept { return seq_id_; }

    /** @brief Returns the radio channel: 'A' or 'B'. */
    [[nodiscard]] char channel() const noexcept { return channel_; }

    /** @brief Returns the raw armoured payload string. */
    [[nodiscard]] const std::string& payload() const noexcept { return payload_; }

    /**
     * @brief Returns the fill bit count [0-5].
     *
     * Fill bits are only meaningful in the final part of a message.  For
     * intermediate parts the value is always 0 per the specification.
     */
    [[nodiscard]] uint8_t fill_bits() const noexcept { return fill_bits_; }

    /**
     * @brief Returns true if this sentence is the only part of its message.
     */
    [[nodiscard]] bool is_single_part() const noexcept { return part_count_ == 1; }

    /**
     * @brief Returns true if this sentence is the final part of a multi-part
     *        message or the only part of a single-part message.
     */
    [[nodiscard]] bool is_last_part() const noexcept { return part_index_ == part_count_; }

    /**
     * @brief Computes and returns the NMEA XOR checksum of the given data
     *        region (everything between '!' and '*').
     *
     * This static helper is provided for use by encoding routines that need
     * to append a valid checksum to a generated sentence string.
     *
     * @param data  The byte range over which to compute the XOR.
     * @return      Two-character uppercase hex string.
     */
    [[nodiscard]] static std::string compute_checksum(const std::string& data);

private:
    Sentence() = default;

    std::string          talker_;
    std::string          sentence_type_;
    uint8_t              part_count_{ 1 };
    uint8_t              part_index_{ 1 };
    std::optional<uint8_t> seq_id_;
    char                 channel_{ 'A' };
    std::string          payload_;
    uint8_t              fill_bits_{ 0 };
};

// -----------------------------------------------------------------------------
// AssembledMessage
// -----------------------------------------------------------------------------

/**
 * @brief A complete AIS message payload assembled from one or more sentence parts.
 *
 * An AssembledMessage is produced by SentenceAssembler once all fragments of
 * a message sequence have been received.  The concatenated payload and fill
 * bits are ready to be passed to decode_payload().
 */
struct AssembledMessage {
    /**
     * @brief The concatenated armoured payload from all parts, in order.
     *
     * For a single-part message this is identical to Sentence::payload().
     * For multi-part messages it is the concatenation of all parts'
     * payload strings in part-index order.
     */
    std::string payload;

    /**
     * @brief Fill bit count from the final sentence part.
     *
     * Per the AIS specification, intermediate parts always carry a fill bit
     * count of 0; only the last part's fill bits are significant.
     */
    uint8_t fill_bits{ 0 };

    /**
     * @brief True if the message was transmitted as own-vessel traffic (VDO).
     * False for traffic from other vessels (VDM).
     */
    bool is_own_vessel{ false };

    /** @brief Talker ID from the first sentence of the sequence. */
    std::string talker;
};

// -----------------------------------------------------------------------------
// SentenceAssembler
// -----------------------------------------------------------------------------

/**
 * @brief Stateful assembler for multi-part NMEA AIS message sequences.
 *
 * AIS messages longer than ~56 characters of payload must be split across
 * multiple NMEA sentences.  Each sentence carries a sequential message
 * identifier (seq_id) and a part index.  The assembler buffers parts keyed
 * by seq_id and returns a complete AssembledMessage once the final part of
 * a sequence has been received.
 *
 * Single-part messages (part_count == 1) bypass buffering and are returned
 * immediately.
 *
 * Thread safety: SentenceAssembler is not thread-safe.  External
 * synchronisation is required for concurrent use.
 *
 * Usage:
 * @code
 *   SentenceAssembler assembler;
 *   for (const std::string& raw : incoming_sentences) {
 *       auto sr = Sentence::parse(raw);
 *       if (!sr) { handle_error(sr.error()); continue; }
 *
 *       auto ar = assembler.feed(sr.value());
 *       if (!ar) { handle_error(ar.error()); continue; }
 *
 *       if (ar.value()) {
 *           // Complete message ready
 *           process(*ar.value());
 *       }
 *       // nullopt means the part was buffered; more parts expected
 *   }
 * @endcode
 */
class SentenceAssembler {
public:
    /**
     * @brief Default constructor.  Creates an empty assembler with no
     *        in-progress sequences.
     */
    SentenceAssembler() = default;

    /**
     * @brief Feeds a parsed sentence into the assembler.
     *
     * For single-part sentences the method returns an AssembledMessage
     * immediately.  For multi-part sentences it buffers the part and returns
     * nullopt until the sequence is complete, at which point it returns the
     * assembled message and clears the buffer for that seq_id.
     *
     * @param sentence  A fully validated Sentence.
     * @return          On success: optional<AssembledMessage>.  The optional
     *                  has a value when the message is complete, and is empty
     *                  when more parts are expected.
     *                  On failure: an Error.
     *
     * Possible errors:
     *   - MultipartOutOfOrder  — part index does not follow the previous part.
     *   - MultipartDuplicate   — a part with this seq_id and index was already
     *                            received.
     *   - MultipartCountInvalid — part count is 0 or > kMaxSentenceParts.
     *   - MultipartSeqInvalid  — seq_id is absent for a multi-part sentence.
     */
    [[nodiscard]] Result<std::optional<AssembledMessage>> feed(const Sentence& sentence);

    /**
     * @brief Discards all buffered in-progress sequences.
     *
     * This should be called when a receiver reconnects or when a timeout
     * policy evicts stale partial sequences.
     */
    void reset() noexcept;

    /**
     * @brief Returns the number of in-progress (incomplete) sequences
     *        currently buffered.
     */
    [[nodiscard]] std::size_t pending_count() const noexcept;

private:
    /**
     * @brief Internal state for a single in-progress multi-part sequence.
     */
    struct Sequence {
        std::vector<std::string> parts;           ///< Payload fragments, indexed by part_index-1.
        uint8_t                  part_count{ 0 }; ///< Expected total number of parts.
        uint8_t                  received{ 0 };   ///< Number of parts received so far.
        std::string              talker;           ///< Talker from the first part.
        std::string              sentence_type;    ///< "VDM" or "VDO" from the first part; all subsequent parts must match.
        bool                     is_own_vessel{ false }; ///< True when sentence_type is "VDO".
    };

    /**
     * @brief Buffer of in-progress sequences, keyed by seq_id.
     *
     * @note TODO(phase-2): Keying on seq_id alone is insufficient for a live
     *       AIS feed where two transmitters may simultaneously use the same
     *       seq_id on the same channel.  The correct key is (channel, seq_id)
     *       or (channel, seq_id, talker).  This limitation is intentional for
     *       Phase 1; it will be resolved by changing this map to use a
     *       composite key with a suitable hash.
     */
    std::unordered_map<uint8_t, Sequence> sequences_;
};

} // namespace aislib