#pragma once

#include <cerridwen/annotations.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

struct CERRIDWEN_EXPORT_HOST LoggerHost {
    virtual ~LoggerHost() = default;

    virtual void log_message(const char* message) {
        std::cout << "[Host Logger]: " << message << "\n";
    }

    virtual int get_timestamp() {
        return 123456789;
    }
};

// Host class the plugin calls into; host code may throw.
struct CERRIDWEN_EXPORT_HOST Calculator {
    virtual ~Calculator() = default;

    virtual int add(int a, int b) { return a + b; }

    virtual int divide(int a, int b) {
        if (b == 0) throw std::invalid_argument("division by zero");
        return a / b;
    }
};

// Host class that plugin classes can SUBCLASS (see Bird in plugin.hpp).
struct CERRIDWEN_EXPORT_HOST Animal {
    virtual ~Animal() = default;

    virtual int legs() { return 4; }
    virtual int sound_id() { return 1; }
    virtual int describe() { return legs() * 10 + sound_id(); }
};
