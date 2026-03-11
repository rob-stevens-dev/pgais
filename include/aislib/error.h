#pragma once

#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

namespace aislib {

/**
 * @file error.h
 * @brief Typed error handling primitives for aislib.
 *
 * aislib avoids throwing exceptions across library boundaries to remain
 * embeddable within PostgreSQL extension modules, which use longjmp-based
 * error handling incompatible with C++ stack unwinding.  All fallible
 * operations return a Result<T> that carries either a value or an Error.
 *
 * Usage pattern:
 * @code
 *   Result<Sentence> r = Sentence::parse(raw);
 *   if (!r) {
 *       log(r.error().message());
 *       return;
 *   }
 *   Sentence& s = r.value();
 * @endcode
 */

// -----------------------------------------------------------------------------
// ErrorCode
// -----------------------------------------------------------------------------

/**
 * @brief Enumeration of all error conditions produced by aislib.
 *
 * Each enumerator maps to a distinct, actionable failure mode.  The numeric
 * values are stable across library versions and may be persisted or logged.
 */
enum class ErrorCode : int {
    // General
    Ok                     = 0,  ///< No error.
    Unknown                = 1,  ///< Unclassified internal error.

    // Sentence-level errors
    SentenceEmpty          = 100, ///< Input string is empty.
    SentenceTooShort       = 101, ///< Input is shorter than the minimum valid NMEA sentence.
    SentenceInvalidStart   = 102, ///< Sentence does not begin with '!' or '$'.
    SentenceMissingChecksum= 103, ///< Checksum delimiter '*' not found.
    SentenceChecksumBad    = 104, ///< Computed checksum does not match the transmitted value.
    SentenceFieldMissing   = 105, ///< A required comma-delimited field is absent.
    SentenceFieldInvalid   = 106, ///< A field value fails type or range validation.
    SentenceTalkerInvalid  = 107, ///< Talker ID is not a recognised AIS talker.
    SentenceTypeInvalid    = 108, ///< Sentence type identifier is not VDM or VDO.

    // Payload / encoding errors
    PayloadEmpty           = 200, ///< Encoded payload string is empty.
    PayloadCharInvalid     = 201, ///< Payload contains a character outside the 6-bit ASCII armour range.
    PayloadFillBitsInvalid = 202, ///< Fill-bit count is out of the valid range [0, 5].
    PayloadTooShort        = 203, ///< Decoded payload is shorter than required for the claimed message type.
    PayloadTooLong         = 204, ///< Decoded payload exceeds the maximum permitted length.

    // Bit-field errors
    BitfieldOutOfRange     = 300, ///< A bit-field read or write would exceed the buffer bounds.
    BitfieldWidthInvalid   = 301, ///< A requested field width of zero or >64 bits was specified.

    // Multi-part reassembly errors
    MultipartOutOfOrder    = 400, ///< A fragment arrived with an unexpected part number.
    MultipartDuplicate     = 401, ///< A fragment with this sequence number was already received.
    MultipartIncomplete    = 402, ///< Reassembly was requested before all fragments arrived.
    MultipartCountInvalid  = 403, ///< Part count field is zero or exceeds the protocol maximum.
    MultipartSeqInvalid    = 404, ///< Sequential message identifier is outside [0, 9].
    MultipartTalkerConflict= 405, ///< A fragment carries a different VDM/VDO type than the first part of the sequence.

    // Message-level errors
    MessageTypeUnknown     = 500, ///< Message type ID is not in the range [1, 27].
    MessageTypeUnsupported = 501, ///< Message type is recognised but not yet implemented.
    MessageDecodeFailure   = 502, ///< Decoding failed for a reason specific to the message type.
};

// -----------------------------------------------------------------------------
// Error
// -----------------------------------------------------------------------------

/**
 * @brief Carries a single error code and a human-readable description.
 *
 * Error objects are lightweight value types intended to be returned by value.
 * The description is optional; when absent, @c message() returns a canonical
 * string derived from the error code.
 */
class Error {
public:
    /**
     * @brief Constructs an Error with a code and an optional detail string.
     * @param code    The error code identifying the failure.
     * @param detail  Additional context, e.g. the offending field value.
     */
    explicit Error(ErrorCode code, std::string detail = {}) noexcept
        : code_(code), detail_(std::move(detail)) {}

    /** @brief Returns the error code. */
    [[nodiscard]] ErrorCode code() const noexcept { return code_; }

    /**
     * @brief Returns a human-readable description of the error.
     *
     * If a detail string was provided at construction it is appended to the
     * canonical code description, separated by ": ".
     */
    [[nodiscard]] std::string message() const;

    /** @brief Returns the optional detail string, which may be empty. */
    [[nodiscard]] const std::string& detail() const noexcept { return detail_; }

    /** @brief Equality compares by code only; detail strings are ignored. */
    bool operator==(const Error& other) const noexcept { return code_ == other.code_; }
    bool operator!=(const Error& other) const noexcept { return code_ != other.code_; }

private:
    ErrorCode   code_;
    std::string detail_;
};

// -----------------------------------------------------------------------------
// Result<T>
// -----------------------------------------------------------------------------

/**
 * @brief A discriminated union holding either a value of type T or an Error.
 *
 * Result<T> is modelled after std::expected (C++23) but targets C++17 to
 * remain compatible with PostgreSQL 11's build toolchain requirements.
 *
 * A default-constructed Result is invalid; always construct via the static
 * factory helpers or the converting constructors.
 *
 * @tparam T  The value type on the success path.  Must be move-constructible.
 */
template <typename T>
class Result {
public:
    // ----- Construction -----

    /**
     * @brief Constructs a successful Result holding the given value.
     * @param value  The success value, forwarded into internal storage.
     */
    /* implicit */ Result(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : storage_(std::move(value)) {}

    /**
     * @brief Constructs a failed Result holding the given error.
     * @param err  The Error describing the failure.
     */
    /* implicit */ Result(Error err) noexcept
        : storage_(std::move(err)) {}

    // ----- Observers -----

    /**
     * @brief Returns true if the Result holds a value (success path).
     */
    [[nodiscard]] bool ok() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    /** @brief Equivalent to @c ok(). */
    explicit operator bool() const noexcept { return ok(); }

    /**
     * @brief Returns a reference to the contained value.
     * @pre   ok() must be true; behaviour is undefined otherwise.
     */
    [[nodiscard]] T& value() & {
        return std::get<T>(storage_);
    }

    /** @brief Const overload of value(). */
    [[nodiscard]] const T& value() const & {
        return std::get<T>(storage_);
    }

    /** @brief Rvalue overload of value(). */
    [[nodiscard]] T value() && {
        return std::get<T>(std::move(storage_));
    }

    /**
     * @brief Returns the contained Error.
     * @pre   !ok() must be true; behaviour is undefined otherwise.
     */
    [[nodiscard]] const Error& error() const & noexcept {
        return std::get<Error>(storage_);
    }

    /**
     * @brief Returns the value, or @p fallback if the result is an error.
     * @param fallback  Returned when !ok().
     */
    [[nodiscard]] T value_or(T fallback) const & {
        return ok() ? value() : std::move(fallback);
    }

private:
    std::variant<T, Error> storage_;
};

// -----------------------------------------------------------------------------
// Result<void> specialisation
// -----------------------------------------------------------------------------

/**
 * @brief Specialisation of Result for operations that produce no value.
 *
 * Mirrors the primary template API except that value() is absent.  A
 * successful void result is constructed with Result<void>::success().
 */
template <>
class Result<void> {
public:
    /** @brief Constructs a successful void result. */
    static Result<void> success() noexcept { return Result<void>(true); }

    /** @brief Constructs a failed void result. */
    /* implicit */ Result(Error err) noexcept
        : error_(std::move(err)), ok_(false) {}

    /** @brief Returns true if the operation succeeded. */
    [[nodiscard]] bool ok() const noexcept { return ok_; }

    /** @brief Equivalent to ok(). */
    explicit operator bool() const noexcept { return ok_; }

    /**
     * @brief Returns the contained Error.
     * @pre   !ok() must be true.
     */
    [[nodiscard]] const Error& error() const noexcept { return error_; }

private:
    explicit Result(bool /*tag*/) noexcept : ok_(true) {}

    Error error_{ ErrorCode::Ok };
    bool  ok_;
};

// -----------------------------------------------------------------------------
// Convenience factory
// -----------------------------------------------------------------------------

/**
 * @brief Constructs a failed Result<T> from a code and optional detail string.
 *
 * This helper reduces boilerplate at call sites:
 * @code
 *   return make_error<Sentence>(ErrorCode::SentenceChecksumBad, raw);
 * @endcode
 */
template <typename T>
[[nodiscard]] Result<T> make_error(ErrorCode code, std::string detail = {}) {
    return Result<T>{ Error{ code, std::move(detail) } };
}

} // namespace aislib