// Standard includes used throughout the project.
#pragma once

// Allow glog macros to operate on unordered_{map, set}.
#define GLOG_STL_LOGGING_FOR_UNORDERED

#include <glog/logging.h>
#include <glog/stl_logging.h>

#include <algorithm>
#include <climits>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

using namespace std::string_literals;

using std::deque;
using std::make_shared;
using std::make_unique;
using std::map;
using std::move;
using std::nullopt;
using std::optional;
using std::pair;
using std::priority_queue;
using std::queue;
using std::set;
using std::shared_ptr;
using std::stack;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::variant;
using std::vector;

// Generic way to combine multiple hash functions.
template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
  // constexpr uint32_t hash_combine_constant32 = 0x9e3779b9;
  constexpr uint64_t hash_combine_constant64 = 0x9e3779b97f4a7c17;
  seed ^=
      std::hash<T>{}(v) + hash_combine_constant64 + (seed << 6) + (seed >> 2);
  (hash_combine(seed, rest), ...);
}

// Similar, but for vectors.
template <typename T>
void hash_combine(std::size_t& seed, const vector<T>& v) {
  // constexpr uint32_t hash_combine_constant32 = 0x9e3779b9;
  constexpr uint64_t hash_combine_constant64 = 0x9e3779b97f4a7c17;
  for (const auto& el : v) {
    seed ^= std::hash<T>{}(el) + hash_combine_constant64 + (seed << 6) +
            (seed >> 2);
  }
}

// Needed for using various datastructures as keys for unordered_map and
// unordered_set.
namespace std {

// Needed for pair<int, string>.
template <>
struct hash<pair<int, string>> {
  inline size_t operator()(const pair<int, string>& p) const {
    std::size_t val = 0;
    hash_combine(val, p.first, p.second);
    return val;
  }
};

// Needed for set<string>
template <>
struct hash<set<string>> {
  inline size_t operator()(const set<string>& strings) const {
    std::size_t val = 0;
    for (const auto& str : strings) hash_combine(val, str);
    return val;
  }
};

}  // namespace std
