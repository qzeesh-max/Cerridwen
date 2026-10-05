#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <memory>
#include <type_traits>
#include <cstring>
#include <stdexcept>

namespace cerridwen {

struct Buffer {
    std::vector<uint8_t> data;
    size_t offset = 0;

    void write(const void* ptr, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(ptr);
        data.insert(data.end(), p, p + size);
    }

    void read(void* ptr, size_t size) {
#ifndef __EMSCRIPTEN__
        if (offset + size > data.size()) throw std::runtime_error("Buffer underflow");
#endif
        std::memcpy(ptr, data.data() + offset, size);
        offset += size;
    }
};

template <typename T> void serialize(Buffer& buf, const T& value);
template <typename T> void deserialize(Buffer& buf, T& value);

inline void serialize(Buffer& buf, const std::string& value);
inline void deserialize(Buffer& buf, std::string& value);

template <typename T> void serialize(Buffer& buf, const std::vector<T>& value);
template <typename T> void deserialize(Buffer& buf, std::vector<T>& value);

template <typename T> void serialize(Buffer& buf, const std::list<T>& value);
template <typename T> void deserialize(Buffer& buf, std::list<T>& value);

template <typename T> void serialize(Buffer& buf, const std::deque<T>& value);
template <typename T> void deserialize(Buffer& buf, std::deque<T>& value);

template <typename T> void serialize(Buffer& buf, const std::set<T>& value);
template <typename T> void deserialize(Buffer& buf, std::set<T>& value);

template <typename T> void serialize(Buffer& buf, const std::unordered_set<T>& value);
template <typename T> void deserialize(Buffer& buf, std::unordered_set<T>& value);

template <typename K, typename V> void serialize(Buffer& buf, const std::map<K, V>& value);
template <typename K, typename V> void deserialize(Buffer& buf, std::map<K, V>& value);

template <typename K, typename V> void serialize(Buffer& buf, const std::unordered_map<K, V>& value);
template <typename K, typename V> void deserialize(Buffer& buf, std::unordered_map<K, V>& value);

template <typename... Ts> void serialize(Buffer& buf, const std::variant<Ts...>& value);
template <typename... Ts> void deserialize(Buffer& buf, std::variant<Ts...>& value);

template <typename T>
void serialize(Buffer& buf, const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        buf.write(&value, sizeof(T));
    } else {
        static_assert(std::is_arithmetic_v<T>, "Type not supported for serialization");
    }
}

template <typename T>
void deserialize(Buffer& buf, T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        buf.read(&value, sizeof(T));
    } else {
        static_assert(std::is_arithmetic_v<T>, "Type not supported for deserialization");
    }
}

// std::string
inline void serialize(Buffer& buf, const std::string& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    buf.write(value.data(), size);
}

inline void deserialize(Buffer& buf, std::string& value) {
    uint32_t size;
    deserialize(buf, size);
    value.resize(size);
    buf.read(value.data(), size);
}

// std::vector
template <typename T>
void serialize(Buffer& buf, const std::vector<T>& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    for (const auto& item : value) serialize(buf, item);
}

template <typename T>
void deserialize(Buffer& buf, std::vector<T>& value) {
    uint32_t size;
    deserialize(buf, size);
    value.resize(size);
    for (auto& item : value) deserialize(buf, item);
}

// std::list
template <typename T>
void serialize(Buffer& buf, const std::list<T>& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    for (const auto& item : value) serialize(buf, item);
}

template <typename T>
void deserialize(Buffer& buf, std::list<T>& value) {
    uint32_t size;
    deserialize(buf, size);
    value.resize(size);
    for (auto& item : value) deserialize(buf, item);
}

// std::deque
template <typename T>
void serialize(Buffer& buf, const std::deque<T>& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    for (const auto& item : value) serialize(buf, item);
}

template <typename T>
void deserialize(Buffer& buf, std::deque<T>& value) {
    uint32_t size;
    deserialize(buf, size);
    value.resize(size);
    for (auto& item : value) deserialize(buf, item);
}

// std::set
template <typename T>
void serialize(Buffer& buf, const std::set<T>& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    for (const auto& item : value) serialize(buf, item);
}

template <typename T>
void deserialize(Buffer& buf, std::set<T>& value) {
    uint32_t size;
    deserialize(buf, size);
    value.clear();
    for (uint32_t i = 0; i < size; ++i) {
        T item;
        deserialize(buf, item);
        value.insert(item);
    }
}

// std::unordered_set
template <typename T>
void serialize(Buffer& buf, const std::unordered_set<T>& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    for (const auto& item : value) serialize(buf, item);
}

template <typename T>
void deserialize(Buffer& buf, std::unordered_set<T>& value) {
    uint32_t size;
    deserialize(buf, size);
    value.clear();
    for (uint32_t i = 0; i < size; ++i) {
        T item;
        deserialize(buf, item);
        value.insert(item);
    }
}

// std::map
template <typename K, typename V>
void serialize(Buffer& buf, const std::map<K, V>& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    for (const auto& pair : value) {
        serialize(buf, pair.first);
        serialize(buf, pair.second);
    }
}

template <typename K, typename V>
void deserialize(Buffer& buf, std::map<K, V>& value) {
    uint32_t size;
    deserialize(buf, size);
    value.clear();
    for (uint32_t i = 0; i < size; ++i) {
        K k; V v;
        deserialize(buf, k);
        deserialize(buf, v);
        value[k] = v;
    }
}

// std::unordered_map
template <typename K, typename V>
void serialize(Buffer& buf, const std::unordered_map<K, V>& value) {
    uint32_t size = value.size();
    serialize(buf, size);
    for (const auto& pair : value) {
        serialize(buf, pair.first);
        serialize(buf, pair.second);
    }
}

template <typename K, typename V>
void deserialize(Buffer& buf, std::unordered_map<K, V>& value) {
    uint32_t size;
    deserialize(buf, size);
    value.clear();
    for (uint32_t i = 0; i < size; ++i) {
        K k; V v;
        deserialize(buf, k);
        deserialize(buf, v);
        value[k] = v;
    }
}

// std::variant
template <typename... Ts>
void serialize(Buffer& buf, const std::variant<Ts...>& value) {
    uint32_t index = value.index();
    serialize(buf, index);
    std::visit([&buf](const auto& v) { serialize(buf, v); }, value);
}

template <typename... Ts>
void deserialize(Buffer& buf, std::variant<Ts...>& value) {
    uint32_t index;
    deserialize(buf, index);
    
    auto deserialize_impl = [&buf, &index, &value](auto& self, auto I) -> void {
        constexpr size_t i_val = decltype(I)::value;
        if constexpr (i_val < sizeof...(Ts)) {
            if (index == i_val) {
                std::variant_alternative_t<i_val, std::variant<Ts...>> item;
                deserialize(buf, item);
                value = std::move(item);
                return;
            }
            self(self, std::integral_constant<size_t, i_val + 1>{});
        }
    };
    deserialize_impl(deserialize_impl, std::integral_constant<size_t, 0>{});
}

} // namespace cerridwen
