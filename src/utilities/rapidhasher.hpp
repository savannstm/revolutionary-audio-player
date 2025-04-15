#pragma once

#include "aliases.hpp"
#include "rapidhash.h"

struct RapidHasher {
    template <typename T>
    constexpr auto operator()(const T& value) const -> u64 {
        return hash_impl(value);
    }

   private:
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    auto hash_impl(const T& value) -> u64 {
        return rapidhash(&value, sizeof(T));
    }

    static auto hash_impl(const string& string) -> u64 {
        return rapidhash(string.data(), string.size());
    }

    static auto hash_impl(const QString& path) -> u64 {
        return rapidhash(path.data(), path.size());
    }
};

template <typename K, typename V>
using rapidhashmap = std::unordered_map<K, V, RapidHasher>;
template <typename K>
using rapidhashset = std::unordered_set<K, RapidHasher>;
