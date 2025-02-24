//
// Created by RussBaz on 20/02/2025.
//

#ifndef CUSTOM_ERROR_CODE_HPP
#define CUSTOM_ERROR_CODE_HPP

#include <string>
#include <cerrno>
#include <system_error> 

// Custom error codes enumeration
// Note: More error codes will be added as needed
enum class CustomErrorCode {
    Success = 0,
    Disconnected = 1,
    UnknownError
};

// Custom error category implementation
class CustomErrorCategory final : public std::error_category {
public:
    // The name of the category
    [[nodiscard]] constexpr const char* name() const noexcept override {
        return "CustomErrorCategory";
    }

    // Convert error code to human-readable message
    [[nodiscard]] std::string message(int ev) const override {
        switch (static_cast<CustomErrorCode>(ev)) {
            case CustomErrorCode::Success:
                return "Success";
            case CustomErrorCode::Disconnected:
                return "Disconnected";
            case CustomErrorCode::UnknownError:
                return "Unknown error";
            default:
                return "Unrecognised error";
        }
    }

    // Map custom errors to standard error conditions
    [[nodiscard]] std::error_condition default_error_condition(int ev) const noexcept override {
        switch (static_cast<CustomErrorCode>(ev)) {
            case CustomErrorCode::Success:
                return { 0, std::generic_category() };
            case CustomErrorCode::Disconnected:
                return { EINVAL, std::generic_category() };
            case CustomErrorCode::UnknownError:
                return { EINVAL, std::generic_category() };
            default:
                return { ev, *this };
        }
    }

    // Custom comparison logic for error conditions
    [[nodiscard]] bool equivalent(const int code, const std::error_condition& condition) const noexcept override {
        return default_error_condition(code) == condition;
    }
};

// Get the singleton instance of the custom error category
[[nodiscard]] inline const std::error_category& custom_error_category() {
    static constinit CustomErrorCategory instance;
    return instance;
}

// Convert custom error codes to std::error_code
[[nodiscard]] inline std::error_code make_error_code(CustomErrorCode e) noexcept {
    return { static_cast<int>(e), custom_error_category() };
}

// Mark CustomErrorCode as an error code enum
template <>
struct std::is_error_code_enum<CustomErrorCode> : true_type {};

#endif // CUSTOM_ERROR_CODE_HPP
