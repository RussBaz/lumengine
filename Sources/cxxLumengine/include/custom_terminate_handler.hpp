#ifndef LE_CUSTOM_TERMINATE_HANDLER_HPP
#define LE_CUSTOM_TERMINATE_HANDLER_HPP

#include <iostream>
#include <exception>
#include <cstdlib>

// Custom terminate handler implementation
[[noreturn]] inline void custom_terminate_handler() {
    // Attempt to get the current exception
    if (const auto ex = std::current_exception()) {
        try {
            // Re-throw the current exception to capture its type
            std::rethrow_exception(ex);
        } catch (const std::exception &e) {
            // Log the exception information
            std::cerr << "Unhandled exception: " << e.what() << std::endl;
        } catch (...) {
            // Handle non-standard exceptions
            std::cerr << "Unhandled non-standard exception" << std::endl;
        }
    } else {
        std::cerr << "Terminate called without an active exception" << std::endl;
    }
    std::abort(); // Terminate the program
}

// Initialisation function to set the custom terminate handler
inline void initialise_terminate_handler() {
    std::set_terminate(custom_terminate_handler);
}

#endif // LE_CUSTOM_TERMINATE_HANDLER_HPP
