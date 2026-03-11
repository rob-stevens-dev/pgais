#include "aislib/error.h"

namespace aislib {

/**
 * @brief Returns a canonical description string for a given ErrorCode.
 *
 * @param code  The error code to describe.
 * @return      A null-terminated string literal.
 */
static const char* error_code_description(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Ok:                      return "success";
        case ErrorCode::Unknown:                 return "unknown internal error";

        case ErrorCode::SentenceEmpty:           return "sentence is empty";
        case ErrorCode::SentenceTooShort:        return "sentence is too short to be valid";
        case ErrorCode::SentenceInvalidStart:    return "sentence does not begin with '!' or '$'";
        case ErrorCode::SentenceMissingChecksum: return "checksum delimiter '*' not found";
        case ErrorCode::SentenceChecksumBad:     return "checksum mismatch";
        case ErrorCode::SentenceFieldMissing:    return "required sentence field is absent";
        case ErrorCode::SentenceFieldInvalid:    return "sentence field value is invalid";
        case ErrorCode::SentenceTalkerInvalid:   return "talker ID is not a recognised AIS talker";
        case ErrorCode::SentenceTypeInvalid:     return "sentence type is not VDM or VDO";

        case ErrorCode::PayloadEmpty:            return "payload string is empty";
        case ErrorCode::PayloadCharInvalid:      return "payload contains an invalid armour character";
        case ErrorCode::PayloadFillBitsInvalid:  return "fill bit count is not in [0, 5]";
        case ErrorCode::PayloadTooShort:         return "payload is shorter than the required minimum";
        case ErrorCode::PayloadTooLong:          return "payload exceeds the maximum permitted length";

        case ErrorCode::BitfieldOutOfRange:      return "bit field access is out of buffer range";
        case ErrorCode::BitfieldWidthInvalid:    return "bit field width must be in [1, 64]";

        case ErrorCode::MultipartOutOfOrder:     return "multi-part fragment arrived out of order";
        case ErrorCode::MultipartDuplicate:      return "duplicate multi-part fragment received";
        case ErrorCode::MultipartIncomplete:     return "reassembly requested before all fragments arrived";
        case ErrorCode::MultipartCountInvalid:   return "part count is zero or exceeds the protocol maximum";
        case ErrorCode::MultipartSeqInvalid:     return "sequential message identifier is out of range";
        case ErrorCode::MultipartTalkerConflict: return "fragment sentence type (VDM/VDO) conflicts with the first part of the sequence";

        case ErrorCode::MessageTypeUnknown:      return "message type ID is not in [1, 27]";
        case ErrorCode::MessageTypeUnsupported:  return "message type is known but not yet implemented";
        case ErrorCode::MessageDecodeFailure:    return "message decode failed";
    }
    return "unrecognised error code";
}

std::string Error::message() const {
    const char* base = error_code_description(code_);
    if (detail_.empty()) {
        return std::string(base);
    }
    std::string result;
    result.reserve(std::char_traits<char>::length(base) + 2 + detail_.size());
    result.append(base);
    result.append(": ");
    result.append(detail_);
    return result;
}

} // namespace aislib