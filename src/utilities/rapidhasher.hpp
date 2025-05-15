#pragma once

#include "aliases.hpp"
#include "rapidhash.h"

struct RapidHasher {
    template <typename T>
    constexpr auto operator()(const T& value) const -> u64 {
        if constexpr (std::is_same_v<T, QString>) {
            return rapidhash(value.toUtf8().constData(), value.size());
        } else if constexpr (std::is_same_v<T, string>) {
            return rapidhash(value.data(), value.size());
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            return rapidhash(&value, sizeof(T));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for RapidHasher");
        }
    }
};

template <typename K, typename V>
using rapidhashmap = std::unordered_map<K, V, RapidHasher>;
template <typename K>
using rapidhashset = std::unordered_set<K, RapidHasher>;

template <typename K, typename V>
class HashMap : public rapidhashmap<K, V> {
   public:
    using rapidhashmap<K, V>::rapidhashmap;

    [[nodiscard]] auto operator[](const K& key) const -> const V& {
        return rapidhashmap<K, V>::find(key)->second;
    }
};
