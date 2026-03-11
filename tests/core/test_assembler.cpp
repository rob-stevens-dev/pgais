/**
 * @file test_assembler.cpp
 * @brief Unit tests for aislib::SentenceAssembler.
 *
 * Covers single-part fast-path, multi-part in-order reassembly, concurrent
 * independent sequences, all documented error conditions, and reset behaviour.
 */

#include <gtest/gtest.h>
#include "aislib/sentence.h"

#include <cstdio>
#include <string>

using namespace aislib;

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

/**
 * @brief Builds a minimal but valid NMEA sentence string with a correct
 *        checksum.
 *
 * Constructs the sentence body from the provided fields, computes the NMEA
 * XOR checksum, and returns the complete sentence string including the '!'
 * start delimiter and '*XY' suffix.
 *
 * @param talker     Two-character talker identifier (e.g. "AI").
 * @param part_count Total fragment count.
 * @param part_index This fragment's 1-based index.
 * @param seq_id     Sequential message identifier string; empty for none.
 * @param channel    Radio channel character (e.g. 'A'), or '\0' for none.
 * @param payload    ASCII-armored payload string.
 * @param fill       Fill bits (0-5).
 * @return           A complete NMEA sentence string with correct checksum.
 */
static std::string build_sentence(const std::string& talker,
                                  int  part_count,
                                  int  part_index,
                                  const std::string& seq_id,
                                  char channel,
                                  const std::string& payload,
                                  int  fill) {
    std::string body = talker + "VDM," +
                       std::to_string(part_count) + "," +
                       std::to_string(part_index) + "," +
                       seq_id + "," +
                       (channel ? std::string(1, channel) : "") + "," +
                       payload + "," +
                       std::to_string(fill);
    uint8_t cs = 0;
    for (char c : body) cs ^= static_cast<uint8_t>(c);
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", cs);
    return "!" + body + "*" + hex;
}

// ---------------------------------------------------------------------------
// SentenceAssembler – single-part messages
// ---------------------------------------------------------------------------

TEST(SentenceAssembler, SinglePartMessageCompletes) {
    SentenceAssembler assembler;
    const std::string raw = build_sentence("AI", 1, 1, "", 'A',
                                           "15M67J0000000000000000000000", 0);
    auto sr = Sentence::parse(raw);
    ASSERT_TRUE(static_cast<bool>(sr));

    auto ar = assembler.feed(sr.value());
    ASSERT_TRUE(static_cast<bool>(ar));
    ASSERT_TRUE(ar.value().has_value());

    const AssembledMessage& msg = ar.value().value();
    EXPECT_EQ(msg.payload,   "15M67J0000000000000000000000");
    EXPECT_EQ(msg.fill_bits, 0u);
    EXPECT_EQ(msg.talker,    "AI");
    EXPECT_FALSE(msg.is_own_vessel);
}

TEST(SentenceAssembler, SinglePartDoesNotModifyPendingCount) {
    SentenceAssembler assembler;
    EXPECT_EQ(assembler.pending_count(), 0u);

    const std::string raw = build_sentence("AI", 1, 1, "", 'A',
                                           "15M67J0000000000000000000000", 0);
    auto sr = Sentence::parse(raw);
    ASSERT_TRUE(static_cast<bool>(sr));
    ASSERT_TRUE(static_cast<bool>(assembler.feed(sr.value())));

    EXPECT_EQ(assembler.pending_count(), 0u);
}

TEST(SentenceAssembler, SinglePartFillBitsPreserved) {
    SentenceAssembler assembler;
    const std::string raw = build_sentence("AI", 1, 1, "", 'A',
                                           "15M67J0000000000000000000000", 2);
    auto sr = Sentence::parse(raw);
    ASSERT_TRUE(static_cast<bool>(sr));

    auto ar = assembler.feed(sr.value());
    ASSERT_TRUE(static_cast<bool>(ar));
    ASSERT_TRUE(ar.value().has_value());
    EXPECT_EQ(ar.value().value().fill_bits, 2u);
}

TEST(SentenceAssembler, SinglePartVDOSetsIsOwnVessel) {
    SentenceAssembler assembler;
    // Build a VDO sentence (own-vessel traffic).
    std::string body = "AIVDO,1,1,,A,15M67J0000000000000000000000,0";
    uint8_t cs = 0;
    for (char c : body) cs ^= static_cast<uint8_t>(c);
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", cs);
    const std::string raw = "!" + body + "*" + hex;

    auto sr = Sentence::parse(raw);
    ASSERT_TRUE(static_cast<bool>(sr));

    auto ar = assembler.feed(sr.value());
    ASSERT_TRUE(static_cast<bool>(ar));
    ASSERT_TRUE(ar.value().has_value());
    EXPECT_TRUE(ar.value().value().is_own_vessel);
}

// ---------------------------------------------------------------------------
// SentenceAssembler – two-part messages
// ---------------------------------------------------------------------------

TEST(SentenceAssembler, TwoPartMessageInOrderCompletes) {
    SentenceAssembler assembler;
    const std::string p1 = build_sentence("AI", 2, 1, "0", 'A', "PART1", 0);
    const std::string p2 = build_sentence("AI", 2, 2, "0", 'A', "PART2", 2);

    auto sr1 = Sentence::parse(p1); ASSERT_TRUE(static_cast<bool>(sr1));
    auto sr2 = Sentence::parse(p2); ASSERT_TRUE(static_cast<bool>(sr2));

    auto ar1 = assembler.feed(sr1.value());
    ASSERT_TRUE(static_cast<bool>(ar1));
    EXPECT_FALSE(ar1.value().has_value());  // incomplete: still waiting for part 2
    EXPECT_EQ(assembler.pending_count(), 1u);

    auto ar2 = assembler.feed(sr2.value());
    ASSERT_TRUE(static_cast<bool>(ar2));
    ASSERT_TRUE(ar2.value().has_value());   // complete

    EXPECT_EQ(ar2.value().value().payload,   "PART1PART2");
    EXPECT_EQ(ar2.value().value().fill_bits, 2u);
    EXPECT_EQ(ar2.value().value().talker,    "AI");
    EXPECT_EQ(assembler.pending_count(), 0u);
}

TEST(SentenceAssembler, TwoPartFillBitsTakenFromLastPart) {
    SentenceAssembler assembler;
    // First part has fill=1, last part has fill=3.  Result must carry fill=3.
    const std::string p1 = build_sentence("AI", 2, 1, "1", 'A', "AA", 1);
    const std::string p2 = build_sentence("AI", 2, 2, "1", 'A', "BB", 3);

    auto sr1 = Sentence::parse(p1); ASSERT_TRUE(static_cast<bool>(sr1));
    auto sr2 = Sentence::parse(p2); ASSERT_TRUE(static_cast<bool>(sr2));

    ASSERT_TRUE(static_cast<bool>(assembler.feed(sr1.value())));
    auto ar2 = assembler.feed(sr2.value());
    ASSERT_TRUE(static_cast<bool>(ar2));
    ASSERT_TRUE(ar2.value().has_value());
    EXPECT_EQ(ar2.value().value().fill_bits, 3u);
}

// ---------------------------------------------------------------------------
// SentenceAssembler – three-part messages
// ---------------------------------------------------------------------------

TEST(SentenceAssembler, ThreePartMessageInOrderCompletes) {
    SentenceAssembler assembler;
    for (int i = 1; i <= 2; ++i) {
        const std::string raw = build_sentence("AI", 3, i, "1", 'B',
                                               "SEG" + std::to_string(i), 0);
        auto sr = Sentence::parse(raw);
        ASSERT_TRUE(static_cast<bool>(sr));
        auto ar = assembler.feed(sr.value());
        ASSERT_TRUE(static_cast<bool>(ar));
        EXPECT_FALSE(ar.value().has_value());
    }
    const std::string raw3 = build_sentence("AI", 3, 3, "1", 'B', "SEG3", 0);
    auto sr3 = Sentence::parse(raw3);
    ASSERT_TRUE(static_cast<bool>(sr3));
    auto ar3 = assembler.feed(sr3.value());
    ASSERT_TRUE(static_cast<bool>(ar3));
    ASSERT_TRUE(ar3.value().has_value());
    EXPECT_EQ(ar3.value().value().payload, "SEG1SEG2SEG3");
}

TEST(SentenceAssembler, ThreePartPendingCountDecrementsOnCompletion) {
    SentenceAssembler assembler;
    for (int i = 1; i <= 2; ++i) {
        const std::string raw = build_sentence("AI", 3, i, "2", 'A', "X", 0);
        auto sr = Sentence::parse(raw);
        ASSERT_TRUE(static_cast<bool>(sr));
        ASSERT_TRUE(static_cast<bool>(assembler.feed(sr.value())));
    }
    EXPECT_EQ(assembler.pending_count(), 1u);

    const std::string raw3 = build_sentence("AI", 3, 3, "2", 'A', "X", 0);
    auto sr3 = Sentence::parse(raw3);
    ASSERT_TRUE(static_cast<bool>(sr3));
    ASSERT_TRUE(static_cast<bool>(assembler.feed(sr3.value())));
    EXPECT_EQ(assembler.pending_count(), 0u);
}

// ---------------------------------------------------------------------------
// SentenceAssembler – concurrent independent sequences
// ---------------------------------------------------------------------------

TEST(SentenceAssembler, TwoConcurrentSequencesIndependent) {
    SentenceAssembler assembler;
    const std::string s1p1 = build_sentence("AI", 2, 1, "3", 'A', "S1F1", 0);
    const std::string s2p1 = build_sentence("AI", 2, 1, "4", 'A', "S2F1", 0);
    const std::string s1p2 = build_sentence("AI", 2, 2, "3", 'A', "S1F2", 0);
    const std::string s2p2 = build_sentence("AI", 2, 2, "4", 'A', "S2F2", 0);

    auto r1 = Sentence::parse(s1p1); ASSERT_TRUE(static_cast<bool>(r1));
    auto r2 = Sentence::parse(s2p1); ASSERT_TRUE(static_cast<bool>(r2));
    auto r3 = Sentence::parse(s1p2); ASSERT_TRUE(static_cast<bool>(r3));
    auto r4 = Sentence::parse(s2p2); ASSERT_TRUE(static_cast<bool>(r4));

    auto a1 = assembler.feed(r1.value()); ASSERT_TRUE(static_cast<bool>(a1)); EXPECT_FALSE(a1.value().has_value());
    auto a2 = assembler.feed(r2.value()); ASSERT_TRUE(static_cast<bool>(a2)); EXPECT_FALSE(a2.value().has_value());
    EXPECT_EQ(assembler.pending_count(), 2u);

    auto a3 = assembler.feed(r3.value());
    ASSERT_TRUE(static_cast<bool>(a3));
    ASSERT_TRUE(a3.value().has_value());
    EXPECT_EQ(a3.value().value().payload, "S1F1S1F2");

    auto a4 = assembler.feed(r4.value());
    ASSERT_TRUE(static_cast<bool>(a4));
    ASSERT_TRUE(a4.value().has_value());
    EXPECT_EQ(a4.value().value().payload, "S2F1S2F2");

    EXPECT_EQ(assembler.pending_count(), 0u);
}

// ---------------------------------------------------------------------------
// SentenceAssembler – error cases
// ---------------------------------------------------------------------------

TEST(SentenceAssembler, DuplicatePartReturnsError) {
    SentenceAssembler assembler;
    const std::string raw1 = build_sentence("AI", 2, 1, "2", 'A', "P1", 0);
    auto sr1 = Sentence::parse(raw1);
    ASSERT_TRUE(static_cast<bool>(sr1));
    ASSERT_TRUE(static_cast<bool>(assembler.feed(sr1.value())));

    // Submit the same part again.
    auto sr1b = Sentence::parse(raw1);
    ASSERT_TRUE(static_cast<bool>(sr1b));
    auto dup = assembler.feed(sr1b.value());
    EXPECT_FALSE(static_cast<bool>(dup));
    EXPECT_EQ(dup.error().code(), ErrorCode::MultipartDuplicate);
}

TEST(SentenceAssembler, PartCountMismatchReturnsError) {
    SentenceAssembler assembler;
    // Part 1 of 2 accepted first.
    const std::string raw1 = build_sentence("AI", 2, 1, "3", 'A', "P1", 0);
    auto sr1 = Sentence::parse(raw1);
    ASSERT_TRUE(static_cast<bool>(sr1));
    ASSERT_TRUE(static_cast<bool>(assembler.feed(sr1.value())));

    // Part 2 claims a total count of 3 – inconsistency with the first part.
    const std::string raw2 = build_sentence("AI", 3, 2, "3", 'A', "P2", 0);
    auto sr2 = Sentence::parse(raw2);
    ASSERT_TRUE(static_cast<bool>(sr2));
    auto ar2 = assembler.feed(sr2.value());
    EXPECT_FALSE(static_cast<bool>(ar2));
    EXPECT_EQ(ar2.error().code(), ErrorCode::MultipartCountInvalid);
}

TEST(SentenceAssembler, MultipartWithoutSeqIdRejectedAtParse) {
    // Your Sentence::parse() rejects multi-part sentences that lack a
    // sequential message identifier, returning MultipartSeqInvalid.
    // The error therefore surfaces from parse(), not from feed().
    const std::string raw = build_sentence("AI", 2, 1, "", 'A', "P1", 0);
    auto sr = Sentence::parse(raw);
    EXPECT_FALSE(static_cast<bool>(sr));
    EXPECT_EQ(sr.error().code(), ErrorCode::MultipartSeqInvalid);
}

// ---------------------------------------------------------------------------
// SentenceAssembler::reset
// ---------------------------------------------------------------------------

TEST(SentenceAssembler, ResetClearsPendingSequences) {
    SentenceAssembler assembler;
    const std::string raw = build_sentence("AI", 2, 1, "5", 'A', "P1", 0);
    auto sr = Sentence::parse(raw);
    ASSERT_TRUE(static_cast<bool>(sr));
    ASSERT_TRUE(static_cast<bool>(assembler.feed(sr.value())));
    EXPECT_EQ(assembler.pending_count(), 1u);

    assembler.reset();
    EXPECT_EQ(assembler.pending_count(), 0u);
}

TEST(SentenceAssembler, ResetAllowsImmediateReuse) {
    SentenceAssembler assembler;
    const std::string raw1 = build_sentence("AI", 2, 1, "9", 'A', "PA", 0);
    auto sr1 = Sentence::parse(raw1);
    ASSERT_TRUE(static_cast<bool>(sr1));
    ASSERT_TRUE(static_cast<bool>(assembler.feed(sr1.value())));

    assembler.reset();

    // A fresh single-part message must complete without error after reset.
    const std::string raw_sp = build_sentence("AI", 1, 1, "", 'A',
                                              "15M67J0000000000000000000000", 0);
    auto sr_sp = Sentence::parse(raw_sp);
    ASSERT_TRUE(static_cast<bool>(sr_sp));
    auto ar = assembler.feed(sr_sp.value());
    EXPECT_TRUE(static_cast<bool>(ar));
    EXPECT_TRUE(ar.value().has_value());
}

TEST(SentenceAssembler, ResetOnEmptyAssemblerIsNoOp) {
    SentenceAssembler assembler;
    EXPECT_EQ(assembler.pending_count(), 0u);
    assembler.reset();
    EXPECT_EQ(assembler.pending_count(), 0u);
}