#ifndef LC_VARIANT_WRAPPER_HPP
#define LC_VARIANT_WRAPPER_HPP

#include <optional>
#include <variant>

// Variant wrapper with visitor pattern
// This struct wraps a std::variant and provides utilities to visit all possible cases with a set of handlers.
// It also provides a method to get a specific type from the variant if it matches.
template<typename Variant>
struct VariantWrapper {
private:
    Variant m_variant;
public:
    template<typename... Ts>
    struct overload : Ts... {
        using Ts::operator()...;
    };

    template<typename... Ts>
    overload(Ts...) -> overload<Ts...>;

    VariantWrapper() = default;

    explicit VariantWrapper(Variant variant) noexcept : m_variant(std::move(variant)) {}

    auto visit_all_cases(auto &&... handlers) const noexcept {
        try {
            return std::visit(overload<std::decay_t<decltype(handlers)>...>{
                              std::forward<decltype(handlers)>(handlers)...
                          }, m_variant);
        } catch (...) {
            throw std::runtime_error("Failed to visit all cases. Should never occur.");
        }
    }

    template<typename T>
    std::optional<T> get_if() const noexcept {
        try {
            if (const auto *ptr = std::get_if<T>(&m_variant)) {
                return *ptr;
            }
        } catch (...) {
            throw std::runtime_error("Failed to get if a case. Should never occur.");
        }
        return std::nullopt;
    }
};

#endif // LC_VARIANT_WRAPPER_HPP
