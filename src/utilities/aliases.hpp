#pragma once

#include <QString>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <qtypes.h>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <typename O, typename T>
[[nodiscard]] constexpr auto as(T&& arg) -> O {
    return static_cast<O>(std::forward<T>(arg));
}

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

using qi64 = qint64;
using qi32 = qint32;
using qu64 = quint64;

namespace fs = std::filesystem;
namespace views = std::views;
namespace ranges = std::ranges;

using fs::path;
using std::abs;
using std::array;
using std::cerr;
using std::clamp;
using std::cout;
using std::format;
using std::make_unique;
using std::map;
using std::max;
using std::min;
using std::println;
using std::set;
using std::string;
using std::string_view;
using std::to_string;
using std::tuple;
using std::vector;

template <typename T, typename E>
using result = std::expected<T, E>;

template <typename E>
using err = std::unexpected<E>;

using std::tolower;
using std::toupper;

using panic = std::runtime_error;

using std::shared_ptr;
using std::unique_ptr;

using std::make_shared;
using std::make_unique;

template <typename K, typename V>
using hashmap = std::unordered_map<K, V>;
template <typename K>
using hashset = std::unordered_set<K>;

using namespace Qt::Literals::StringLiterals;
using namespace std::literals::string_view_literals;
