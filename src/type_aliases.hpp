#ifndef TYPE_ALIASES_HPP
#define TYPE_ALIASES_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
using str = char *;
using cstr = const char *;

namespace fs = std::filesystem;
namespace views = std::ranges::views;
namespace ranges = std::ranges;

using fs::path;
using std::array;
using std::cerr;
using std::cout;
using std::println;
using std::string;
using std::to_string;
using std::tuple;
using std::vector;
using panic = std::runtime_error;
template <typename K, typename V>
using hashmap = std::unordered_map<K, V>;
template <typename K, typename V>
using hashset = std::unordered_set<K, V>;

using std::map;
using std::set;

#endif  // TYPE_ALIASES_HPP