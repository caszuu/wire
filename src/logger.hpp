#include <iostream>

// a simple server side logger

namespace wire {
    // ansii escape seqences
    constexpr static const char* reset_escape = "";
    constexpr static const char* warn_escape = "";
    constexpr static const char* error_escape = "";
    
    void log_info(std::string_view message) noexcept {
        std::cout << "[info] " << message << '\n';
    }
    
    void log_warn(std::string_view message) noexcept {
        std::cout << '[' << warn_escape << "warn" << reset_escape << "] " << message << '\n';
    }
    
    void log_err(std::string_view message) noexcept {
        std::cout << '[' << error_escape << "err" << reset_escape << "] " << message << '\n';
    }
}