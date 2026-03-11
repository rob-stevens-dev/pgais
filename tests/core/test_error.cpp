/**
 * @file test_error.cpp
 * @brief Unit tests for the Error class and Result<T> template.
 *
 * Covers: error code description coverage, Error::message() with and without
 * detail strings, Result<T> construction (success and failure paths), observer
 * methods, value_or(), Result<void> specialisation, and make_error().
 */

#include "aislib/error.h"

#include <gtest/gtest.h>
#include <string>
#include <type_traits>

using namespace aislib;

// =============================================================================
// Error
// =============================================================================

TEST(Error, CodeRoundtrip) {
    Error e{ ErrorCode::SentenceChecksumBad };
    EXPECT_EQ(e.code(), ErrorCode::SentenceChecksumBad);
}

TEST(Error, MessageNoDetail) {
    Error e{ ErrorCode::SentenceEmpty };
    const std::string msg = e.message();
    EXPECT_FALSE(msg.empty());
    // The message should not contain ": " when there is no detail.
    EXPECT_EQ(msg.find(": "), std::string::npos);
}

TEST(Error, MessageWithDetail) {
    Error e{ ErrorCode::SentenceChecksumBad, "expected AA got BB" };
    const std::string msg = e.message();
    EXPECT_NE(msg.find("expected AA got BB"), std::string::npos);
    EXPECT_NE(msg.find(": "), std::string::npos);
}

TEST(Error, DetailAccessor) {
    Error e{ ErrorCode::PayloadCharInvalid, "bad_char" };
    EXPECT_EQ(e.detail(), "bad_char");
}

TEST(Error, DetailEmptyByDefault) {
    Error e{ ErrorCode::Ok };
    EXPECT_TRUE(e.detail().empty());
}

TEST(Error, EqualityByCode) {
    Error a{ ErrorCode::BitfieldOutOfRange, "detail A" };
    Error b{ ErrorCode::BitfieldOutOfRange, "detail B" };
    Error c{ ErrorCode::BitfieldWidthInvalid };
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(Error, AllErrorCodesHaveDescriptions) {
    // Ensure every declared ErrorCode produces a non-empty description.
    const std::vector<ErrorCode> all_codes = {
        ErrorCode::Ok,
        ErrorCode::Unknown,
        ErrorCode::SentenceEmpty,
        ErrorCode::SentenceTooShort,
        ErrorCode::SentenceInvalidStart,
        ErrorCode::SentenceMissingChecksum,
        ErrorCode::SentenceChecksumBad,
        ErrorCode::SentenceFieldMissing,
        ErrorCode::SentenceFieldInvalid,
        ErrorCode::SentenceTalkerInvalid,
        ErrorCode::SentenceTypeInvalid,
        ErrorCode::PayloadEmpty,
        ErrorCode::PayloadCharInvalid,
        ErrorCode::PayloadFillBitsInvalid,
        ErrorCode::PayloadTooShort,
        ErrorCode::PayloadTooLong,
        ErrorCode::BitfieldOutOfRange,
        ErrorCode::BitfieldWidthInvalid,
        ErrorCode::MultipartOutOfOrder,
        ErrorCode::MultipartDuplicate,
        ErrorCode::MultipartIncomplete,
        ErrorCode::MultipartCountInvalid,
        ErrorCode::MultipartSeqInvalid,
        ErrorCode::MultipartTalkerConflict,
        ErrorCode::MessageTypeUnknown,
        ErrorCode::MessageTypeUnsupported,
        ErrorCode::MessageDecodeFailure,
    };
    for (ErrorCode code : all_codes) {
        Error e{ code };
        EXPECT_FALSE(e.message().empty()) << "Empty message for code "
            << static_cast<int>(code);
    }
}

// =============================================================================
// Result<T> — success path
// =============================================================================

TEST(ResultT, ConstructFromValue) {
    Result<int> r{ 42 };
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_EQ(r.value(), 42);
}

TEST(ResultT, ConstructFromError) {
    Result<int> r{ Error{ ErrorCode::Unknown } };
    EXPECT_FALSE(r.ok());
    EXPECT_FALSE(static_cast<bool>(r));
}

TEST(ResultT, ErrorAccessor) {
    Result<int> r{ Error{ ErrorCode::PayloadEmpty, "test" } };
    EXPECT_EQ(r.error().code(), ErrorCode::PayloadEmpty);
    EXPECT_EQ(r.error().detail(), "test");
}

TEST(ResultT, ValueOr_Success) {
    Result<int> r{ 7 };
    EXPECT_EQ(r.value_or(99), 7);
}

TEST(ResultT, ValueOr_Failure) {
    Result<int> r{ Error{ ErrorCode::Unknown } };
    EXPECT_EQ(r.value_or(99), 99);
}

TEST(ResultT, MoveValue) {
    Result<std::string> r{ std::string("hello") };
    EXPECT_TRUE(r.ok());
    std::string moved = std::move(r).value();
    EXPECT_EQ(moved, "hello");
}

TEST(ResultT, ConstRef) {
    const Result<std::string> r{ std::string("const") };
    EXPECT_EQ(r.value(), "const");
}

TEST(ResultT, StringResult) {
    Result<std::string> r{ std::string("test_string") };
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.value(), "test_string");
}

// =============================================================================
// Result<void>
// =============================================================================

TEST(ResultVoid, Success) {
    auto r = Result<void>::success();
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(static_cast<bool>(r));
}

TEST(ResultVoid, Failure) {
    Result<void> r{ Error{ ErrorCode::BitfieldOutOfRange } };
    EXPECT_FALSE(r.ok());
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error().code(), ErrorCode::BitfieldOutOfRange);
}

// =============================================================================
// make_error
// =============================================================================

TEST(MakeError, BasicUsage) {
    auto r = make_error<int>(ErrorCode::SentenceChecksumBad, "test detail");
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), ErrorCode::SentenceChecksumBad);
    EXPECT_EQ(r.error().detail(), "test detail");
}

TEST(MakeError, NoDetail) {
    auto r = make_error<std::string>(ErrorCode::Unknown);
    EXPECT_FALSE(r.ok());
    EXPECT_TRUE(r.error().detail().empty());
}