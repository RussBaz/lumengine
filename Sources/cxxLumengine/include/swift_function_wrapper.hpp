#ifndef LE_SWIFT_FUNCTION_WRAPPER_HPP
#define LE_SWIFT_FUNCTION_WRAPPER_HPP

#include <functional>
#include <string>

// Forward declarations for Swift interop
extern "C" void *pass_swift_closure_to_cpp(void * (*closure)(void *));
extern "C" void release_swift_closure(void *closure);

// Error logging helper
inline void log_error(const std::string &message) noexcept {
    std::fprintf(stderr, "[Error] %s\n", message.c_str());
}

// Generic Swift function wrapper
// This struct wraps a Swift function to be callable from C++ with the specified Inputs and Output types.
// It takes a Swift function pointer, wraps it in a C++ std::function, and handles calling and releasing the Swift closure.
template<typename Output, typename... Inputs>
struct SwiftFunctionWrapper {
    std::function<Output(Inputs...)> m_function;
    void *m_swift_closure;

    explicit SwiftFunctionWrapper(void *swift_function) noexcept
        : m_swift_closure(swift_function) {
        // Wrap the Swift function to match the C++ function signature
        m_function = [swift_function](Inputs... inputs) -> Output {
            try {
                auto input_tuple = std::make_tuple(std::move(inputs)...);
                auto swift_call = reinterpret_cast<Output(*)(void *)>(swift_function);
                return swift_call(static_cast<void *>(&input_tuple));
            } catch (...) {
                throw std::runtime_error("Swift closure threw exception");
            }
        };
    }

    ~SwiftFunctionWrapper() noexcept {
        // Release the Swift closure when the wrapper is destroyed
        release_swift_closure(m_swift_closure);
    }

    template<typename... Args>
    Output call(Args &&... args) const noexcept {
        try {
            if (!m_function) {
                throw std::runtime_error("Function not initialised");
            }
            try {
                return m_function(std::forward<Args>(args)...);
            } catch (...) {
                throw std::runtime_error("Failed to call swift_function");
            }
        } catch (...) {
            std::terminate();
        }
    }
};

// Overloaded constructor for SwiftFunctionWrapper with no inputs
// This specialization handles Swift functions that do not take any inputs and returns an Output type.
template<typename Output>
struct SwiftFunctionWrapper<Output> {
    std::function<Output()> m_function;
    void *m_swift_closure;

    explicit SwiftFunctionWrapper(void *swift_function) noexcept
        : m_swift_closure(swift_function) {
        m_function = [swift_function]() -> Output {
            try {
                auto swift_call = reinterpret_cast<Output(*)(void *)>(swift_function);
                return swift_call(nullptr);
            } catch (...) {
                throw std::runtime_error("Swift closure threw exception");
            }
        };
    }

    Output call() const noexcept {
        try {
            if (!m_function) {
                throw std::runtime_error("Function not initialised");
            }
            try {
                return m_function();
            } catch (...) {
                throw std::runtime_error("Failed to call swift_function");
            }
        } catch (...) {
            std::terminate();
        }
    }
};

// Void input/output specialisation for SwiftFunctionWrapper
// This specialisation handles Swift functions that do not take any inputs and do not return any outputs (void).
template<>
struct SwiftFunctionWrapper<void, void> {
    std::function<void()> m_function;
    void *m_swift_closure;

    explicit SwiftFunctionWrapper(void *swift_function) noexcept
        : m_swift_closure(swift_function) {
        m_function = [swift_function] {
            try {
                reinterpret_cast<void(*)(void *)>(swift_function)(nullptr);
            } catch (...) {
                throw std::runtime_error("Swift closure threw exception");
            }
        };
    }

    void call() const noexcept {
        try {
            if (!m_function) {
                throw std::runtime_error("Function not initialised");
            }
            try {
                m_function();
            } catch (...) {
                throw std::runtime_error("Failed to call swift_function");
            }
        } catch (...) {
            std::terminate();
        }
    }
};

#endif //LE_SWIFT_FUNCTION_WRAPPER_HPP
