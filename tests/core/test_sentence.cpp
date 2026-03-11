/**
 * @file test_sentence.cpp
 * @brief Unit tests for Sentence::parse() and Sentence::compute_checksum().
 *
 * Covers: valid single-part and multi-part sentences, all valid AIS talker IDs,
 * VDM and VDO sentence types, checksum computation and validation, every
 * error path in the parser, accessor correctness, and edge cases such as
 * trailing CR/LF stripping and maximum part counts.
 */

#include "aislib/sentence.h"

#include <gtest/gtest.h>
#include <string>

using namespace aislib;

// =============================================================================
// Helpers — known-good sentences
// =============================================================================

// A real single-part Type 1 VDM sentence with verified checksum.
// Checksum region: AIVDM,1,1,,B,15M67J`P00G?Ue`E`FepT4@000S8,0 -> XOR -> 32
static constexpr const char* kType1Single =
    "!AIVDM,1,1,,B,15M67J`P00G?Ue`E`FepT4@000S8,0*32";

// A real two-part Type 5 sentence (first part) with verified checksum.
static constexpr const char* kType5Part1 =
    "!AIVDM,2,1,3,B,55?Te`01h;JtpE62<0@T4@Dn2222220l1@E634Dn>5L0Ac`1lH,0*33";

// The matching second part of the above, verified checksum.
static constexpr const char* kType5Part2 =
    "!AIVDM,2,2,3,B,888888888888880,2*24";

// =============================================================================
// compute_checksum
// =============================================================================

TEST(ComputeChecksum, EmptyString) {
    // XOR of nothing = 0 -> "00"
    EXPECT_EQ(Sentence::compute_checksum(""), "00");
}

TEST(ComputeChecksum, SingleByte) {
    // XOR of 'A' (65 = 0x41) -> "41"
    EXPECT_EQ(Sentence::compute_checksum("A"), "41");
}

TEST(ComputeChecksum, KnownSentenceRegion) {
    // Checksum covers everything between '!' and '*' in the kType1Single sentence.
    const std::string region = "AIVDM,1,1,,B,15M67J`P00G?Ue`E`FepT4@000S8,0";
    EXPECT_EQ(Sentence::compute_checksum(region), "32");
}

TEST(ComputeChecksum, AllSameBytes) {
    // XOR of 4x 0xFF = 0x00 -> "00"
    EXPECT_EQ(Sentence::compute_checksum("\xFF\xFF\xFF\xFF"), "00");
}

// =============================================================================
// parse — valid single-part sentences
// =============================================================================

TEST(SentenceParse, ValidType1_Basic) {
    auto r = Sentence::parse(kType1Single);
    ASSERT_TRUE(r.ok()) << r.error().message();

    const Sentence& s = r.value();
    EXPECT_EQ(s.talker(),        "AI");
    EXPECT_EQ(s.sentence_type(), "VDM");
    EXPECT_EQ(s.part_count(),    1u);
    EXPECT_EQ(s.part_index(),    1u);
    EXPECT_FALSE(s.seq_id().has_value());
    EXPECT_EQ(s.fill_bits(),     0u);
    EXPECT_TRUE(s.is_single_part());
    EXPECT_TRUE(s.is_last_part());
    EXPECT_FALSE(s.payload().empty());
}

TEST(SentenceParse, VDO_SentenceType) {
    // Construct a minimal VDO sentence.
    // Region: "AIVDO,1,1,,A,0,0", checksum = XOR.
    const std::string region = "AIVDO,1,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_TRUE(r.ok()) << r.error().message();
    EXPECT_EQ(r.value().sentence_type(), "VDO");
}

TEST(SentenceParse, AllValidTalkers) {
    const std::vector<std::string> talkers = {
        "AB", "AI", "AN", "AR", "AS", "AT", "AX", "BS", "SA"
    };
    for (const std::string& talker : talkers) {
        const std::string region = talker + "VDM,1,1,,A,0,0";
        const std::string cs     = Sentence::compute_checksum(region);
        const std::string raw    = "!" + region + "*" + cs;

        auto r = Sentence::parse(raw);
        ASSERT_TRUE(r.ok()) << "Talker " << talker << ": " << r.error().message();
        EXPECT_EQ(r.value().talker(), talker);
    }
}

TEST(SentenceParse, DollarPrefix_Accepted) {
    // Some devices prefix sentences with '$' instead of '!'.
    const std::string region = "AIVDM,1,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "$" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_TRUE(r.ok()) << r.error().message();
}

TEST(SentenceParse, TrailingCRLF_Stripped) {
    const std::string region = "AIVDM,1,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs + "\r\n";

    auto r = Sentence::parse(raw);
    ASSERT_TRUE(r.ok()) << r.error().message();
}

TEST(SentenceParse, TrailingLF_Stripped) {
    const std::string region = "AIVDM,1,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs + "\n";

    auto r = Sentence::parse(raw);
    ASSERT_TRUE(r.ok()) << r.error().message();
}

// =============================================================================
// parse — valid multi-part sentences
// =============================================================================

TEST(SentenceParse, MultiPart_FirstPart) {
    auto r = Sentence::parse(kType5Part1);
    ASSERT_TRUE(r.ok()) << r.error().message();

    const Sentence& s = r.value();
    EXPECT_EQ(s.part_count(), 2u);
    EXPECT_EQ(s.part_index(), 1u);
    ASSERT_TRUE(s.seq_id().has_value());
    EXPECT_EQ(*s.seq_id(), 3u);
    EXPECT_FALSE(s.is_last_part());
}

TEST(SentenceParse, MultiPart_LastPart) {
    auto r = Sentence::parse(kType5Part2);
    ASSERT_TRUE(r.ok()) << r.error().message();

    const Sentence& s = r.value();
    EXPECT_EQ(s.part_count(),  2u);
    EXPECT_EQ(s.part_index(),  2u);
    EXPECT_EQ(s.fill_bits(),   2u);
    EXPECT_TRUE(s.is_last_part());
}

TEST(SentenceParse, MaxPartCount_5) {
    const std::string region = "AIVDM,5,1,1,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_TRUE(r.ok()) << r.error().message();
    EXPECT_EQ(r.value().part_count(), 5u);
}

TEST(SentenceParse, FillBits_1_Through_5) {
    for (uint8_t fill = 1; fill <= 5; ++fill) {
        const std::string region = "AIVDM,1,1,,A,0," + std::to_string(fill);
        const std::string cs     = Sentence::compute_checksum(region);
        const std::string raw    = "!" + region + "*" + cs;

        auto r = Sentence::parse(raw);
        ASSERT_TRUE(r.ok()) << "fill=" << static_cast<int>(fill)
                             << ": " << r.error().message();
        EXPECT_EQ(r.value().fill_bits(), fill);
    }
}

// =============================================================================
// parse — error paths
// =============================================================================

TEST(SentenceParse, EmptyString_Error) {
    auto r = Sentence::parse("");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceEmpty);
}

TEST(SentenceParse, TooShort_Error) {
    auto r = Sentence::parse("!AI*00");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceTooShort);
}

TEST(SentenceParse, InvalidStart_Error) {
    // Begins with '#' instead of '!' or '$'.
    auto r = Sentence::parse("#AIVDM,1,1,,A,0,0*00");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceInvalidStart);
}

TEST(SentenceParse, MissingChecksum_Error) {
    // No '*' delimiter.
    auto r = Sentence::parse("!AIVDM,1,1,,A,0,0");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceMissingChecksum);
}

TEST(SentenceParse, ChecksumBad_Error) {
    // Correct sentence with wrong checksum.
    auto r = Sentence::parse("!AIVDM,1,1,,A,0,0*FF");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceChecksumBad);
}

TEST(SentenceParse, InvalidTalker_Error) {
    const std::string region = "XXVDM,1,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceTalkerInvalid);
}

TEST(SentenceParse, InvalidSentenceType_Error) {
    const std::string region = "AIINV,1,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceTypeInvalid);
}

TEST(SentenceParse, InvalidPartCount_Zero_Error) {
    const std::string region = "AIVDM,0,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MultipartCountInvalid);
}

TEST(SentenceParse, InvalidPartCount_TooHigh_Error) {
    const std::string region = "AIVDM,6,1,1,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MultipartCountInvalid);
}

TEST(SentenceParse, PartIndexExceedsCount_Error) {
    const std::string region = "AIVDM,1,2,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceFieldInvalid);
}

TEST(SentenceParse, MultiPartMissingSeqId_Error) {
    // Part count > 1 but seq_id field is empty.
    const std::string region = "AIVDM,2,1,,A,0,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::MultipartSeqInvalid);
}

TEST(SentenceParse, FillBitsTooHigh_Error) {
    const std::string region = "AIVDM,1,1,,A,0,6";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadFillBitsInvalid);
}

TEST(SentenceParse, TooFewFields_Error) {
    // Only 6 comma-separated fields (missing fill bits), not 7.
    const std::string region = "AIVDM,1,1,,A,0";
    const std::string cs     = Sentence::compute_checksum(region);
    const std::string raw    = "!" + region + "*" + cs;

    auto r = Sentence::parse(raw);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceFieldMissing);
}

TEST(SentenceParse, ChecksumFieldTooShort_Error) {
    // Truncate checksum to one digit.
    auto r = Sentence::parse("!AIVDM,1,1,,A,0,0*1");
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceMissingChecksum);
}