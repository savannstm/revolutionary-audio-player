#pragma once

#include <QString>
#include <atomic>
#include <cstddef>
#include <deque>
#include <expected>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <qtversionchecks.h>
#include <qtypes.h>
#include <queue>
#include <ranges>
#include <rapidhash.h>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Qt::Literals::StringLiterals;
using namespace std::literals::string_view_literals;

namespace fs = std::filesystem;
namespace views = std::views;
namespace ranges = std::ranges;

using qi64 = qint64;
using qi32 = qint32;
using qu64 = quint64;

using usize = std::size_t;
using isize = std::intptr_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;
using str = char*;
using cstr = const char*;
using wchar = wchar_t;
using wcstr = const wchar*;

using fs::path;
using std::array;
using std::atomic;
using std::cerr;
using std::cout;
using std::deque;
using std::expected;
using std::format;
using std::make_shared;
using std::make_unique;
using std::map;
using std::nullopt;
using std::optional;
using std::println;
using std::queue;
using std::set;
using std::shared_ptr;
using std::span;
using std::string;
using std::string_view;
using std::to_string;
using std::tolower;
using std::toupper;
using std::tuple;
using std::unique_ptr;
using std::vector;
using std::wstring;
using std::wstring_view;

// math
using std::abs;
using std::clamp;
using std::log10f;
using std::log2f;
using std::max;
using std::min;
using std::powf;
using std::sqrtf;

using panic = std::runtime_error;
template <typename T, typename E>
using result = std::expected<T, E>;
template <typename E>
using err = std::unexpected<E>;
template <typename K, typename V>
using hashmap = std::unordered_map<K, V>;
template <typename K>
using hashset = std::unordered_set<K>;

template <typename O, typename T>
[[nodiscard]] constexpr auto as(T&& arg) -> O {
    return static_cast<O>(std::forward<T>(arg));
}

template <typename O, typename T>
[[nodiscard]] constexpr auto ras(T&& arg) -> O {
    return reinterpret_cast<O>(std::forward<T>(arg));
}

constexpr auto range(const usize from, const usize dest) {
    return views::iota(from, dest);
}

template <ranges::input_range R, typename T>
constexpr auto find_index(const R& range, const T& value) -> isize {
    const auto item = ranges::find(range, value);

    if (item == ranges::end(range)) {
        return -1;
    }

    return ranges::distance(ranges::begin(range), item);
}

constexpr auto operator""_qsv(const char16_t* chr, const size_t size)
    -> QStringView {
    return { chr, isize(size) };
}

// Concatenation compatibility
#if QT_VERSION_MINOR >= 9
constexpr auto operator""_qssv(const char16_t* chr, const size_t size)
    -> QStringView {
    return { chr, isize(size) };
}
#else
constexpr auto operator""_qssv(const char16_t* chr, const size_t size)
    -> QString {
    return QString(
        QStringPrivate(nullptr, const_cast<char16_t*>(chr), qsizetype(size))
    );
}
#endif

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
template <typename E>
using rapidhashset = std::unordered_set<E, RapidHasher>;

template <typename K, typename V>
class HashMap : public rapidhashmap<K, V> {
   public:
    using rapidhashmap<K, V>::rapidhashmap;

    [[nodiscard]] auto operator[](const K& key) const -> const V& {
        return rapidhashmap<K, V>::find(key)->second;
    }
};

template <typename E>
class HashSet : public rapidhashset<E> {
   public:
    using rapidhashset<E>::rapidhashset;
};

enum class TrackProperty : u8;
using TrackMetadata = HashMap<TrackProperty, QString>;