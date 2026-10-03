#pragma once

#include <cerridwen/annotations.hpp>
#include <string>
#include <iostream>

struct CERRIDWEN_EXPORT_HOST LoggerHost {
    virtual ~LoggerHost() = default;

    virtual void log_message(const char* message) {
        std::cout << "[Host Logger]: " << message << "\n";
    }

    virtual int get_timestamp() {
        return 123456789;
    }
};
