#pragma once

#include <cerridwen/annotations.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace cerridwen {

// Host-side storage for plugins.
//
// A plugin never sees a real filesystem path. It names blobs with relative,
// '/'-separated names (e.g. "settings.bin", "cache/a.dat"). The host decides:
//   * the root directory every blob lives under (the plugin is jailed to it),
//   * the total number of bytes the plugin may keep there (quota).
// Names containing "..", absolute paths, backslashes, odd characters, or
// components that are symlinks are refused.
class CERRIDWEN_EXPORT_HOST CERRIDWEN_STORAGE_CLASS StorageHost {
public:
    enum Result : int {
        Ok = 0,
        InvalidName = -1,
        QuotaExceeded = -2,
        IoError = -3,
        NotFound = -4,
        NoStorage = -5,
    };

    static constexpr size_t kMaxNameLength = 255;
    static constexpr size_t kMaxDepth = 8;

    StorageHost(std::filesystem::path root, uint64_t quota_bytes)
        : _quota(quota_bytes) {
        std::error_code ec;
        std::filesystem::create_directories(root, ec);
        _root = std::filesystem::weakly_canonical(root, ec);
    }

    const std::filesystem::path& root() const { return _root; }
    uint64_t quota() const { return _quota; }

    uint64_t used() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return used_unlocked();
    }

    static bool valid_name(std::string_view name) {
        if (name.empty() || name.size() > kMaxNameLength) return false;
        size_t depth = 0, start = 0;
        while (start <= name.size()) {
            size_t end = name.find('/', start);
            if (end == std::string_view::npos) end = name.size();
            std::string_view comp = name.substr(start, end - start);
            if (comp.empty() || comp == "." || comp == "..") return false;
            for (char c : comp) {
                bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
                if (!ok) return false;
            }
            if (++depth > kMaxDepth) return false;
            start = end + 1;
        }
        return true;
    }

    int write(std::string_view name, const void* data, size_t len) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::filesystem::path p;
        if (!resolve(name, p)) return InvalidName;

        std::error_code ec;
        uint64_t existing = 0;
        if (std::filesystem::is_regular_file(p, ec)) existing = std::filesystem::file_size(p, ec);
        uint64_t used_now = used_unlocked();
        uint64_t base = used_now >= existing ? used_now - existing : 0;
        if (len > _quota || base > _quota - len) return QuotaExceeded;

        std::filesystem::create_directories(p.parent_path(), ec);
        if (ec) return IoError;
        std::ofstream f(p, std::ios::binary | std::ios::trunc);
        if (!f) return IoError;
        if (len) f.write(static_cast<const char*>(data), static_cast<std::streamsize>(len));
        return f.good() ? Ok : IoError;
    }

    // Returns the blob size (copying at most `cap` bytes into buf), or a negative Result.
    int64_t read(std::string_view name, void* buf, size_t cap) const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::filesystem::path p;
        if (!resolve(name, p)) return InvalidName;
        std::error_code ec;
        if (!std::filesystem::is_regular_file(p, ec)) return NotFound;
        uint64_t sz = std::filesystem::file_size(p, ec);
        if (ec) return IoError;
        if (cap && buf) {
            std::ifstream f(p, std::ios::binary);
            if (!f) return IoError;
            f.read(static_cast<char*>(buf), static_cast<std::streamsize>(std::min<uint64_t>(sz, cap)));
        }
        return static_cast<int64_t>(sz);
    }

    int64_t size(std::string_view name) const { return read(name, nullptr, 0); }
    bool exists(std::string_view name) const { return size(name) >= 0; }

    int remove(std::string_view name) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::filesystem::path p;
        if (!resolve(name, p)) return InvalidName;
        std::error_code ec;
        if (!std::filesystem::is_regular_file(p, ec)) return NotFound;
        return std::filesystem::remove(p, ec) ? Ok : IoError;
    }

private:
    std::filesystem::path _root;
    uint64_t _quota;
    mutable std::mutex _mutex;

    // Maps a plugin-visible name to a path under _root. Fails on anything that could
    // leave the jail, including symlinked components already on disk.
    bool resolve(std::string_view name, std::filesystem::path& out) const {
        if (!valid_name(name)) return false;
        std::filesystem::path p = _root;
        size_t start = 0;
        std::error_code ec;
        while (start <= name.size()) {
            size_t end = name.find('/', start);
            if (end == std::string_view::npos) end = name.size();
            p /= std::string(name.substr(start, end - start));
            auto st = std::filesystem::symlink_status(p, ec);
            if (!ec && std::filesystem::is_symlink(st)) return false;
            start = end + 1;
        }
        // Belt and braces: the lexical result must still be inside the root.
        auto rel = p.lexically_relative(_root);
        if (rel.empty() || *rel.begin() == "..") return false;
        out = std::move(p);
        return true;
    }

    uint64_t used_unlocked() const {
        uint64_t total = 0;
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(_root, ec);
             !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            std::error_code e2;
            if (it->is_regular_file(e2)) total += it->file_size(e2);
        }
        return total;
    }
};

} // namespace cerridwen
