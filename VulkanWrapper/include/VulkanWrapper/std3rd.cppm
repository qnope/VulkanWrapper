module;

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <ranges>
#include <set>
#include <source_location>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

export module std3rd;

export namespace std {
using std::apply;
using std::array;
using std::begin;
using std::byte;
using std::cerr;
using std::convertible_to;
using std::cout;
using std::distance;
using std::enable_shared_from_this;
using std::end;
using std::endl;
using std::erase_if;
using std::exception;
using std::exchange;
using std::exclusive_scan;
using std::find;
using std::find_if;
using std::format;
using std::forward;
using std::function;
using std::hash;
using std::ignore;
using std::integral_constant;
using std::is_standard_layout;
using std::is_standard_layout_v;
using std::is_trivially_copyable;
using std::is_trivially_copyable_v;
using std::make_shared;
using std::map;
using std::max;
using std::memcpy;
using std::min;
using std::move;
using std::nullopt;
using std::ostringstream;
using std::tuple;
using std::tuple_element;
using std::tuple_size;
using std::uint32_t;

using std::ceil;
using std::cos;
using std::cref;
using std::decay_t;
using std::get;
using std::ifstream;
using std::ios_base;
using std::is_base_of;
using std::is_base_of_v;
using std::is_same;
using std::is_same_v;
using std::less;
using std::log;
using std::log2;
using std::make_unique;
using std::mt19937;
using std::none_of;
using std::numeric_limits;
using std::optional;
using std::pair;
using std::pow;
using std::random_device;
using std::ref;
using std::reference_wrapper;
using std::same_as;
using std::set;
using std::shared_ptr;
using std::sin;
using std::size_t;
using std::sort;
using std::source_location;
using std::span;
using std::string;
using std::string_view;
using std::strong_ordering;
using std::swap;
using std::terminate;
using std::tolower;
using std::uniform_real_distribution;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::variant;
using std::vector;
using std::visit;

using std::operator==;
using std::operator!=;
using std::operator<=;
using std::operator<=>;
using std::operator>=;
using std::operator<;
using std::operator>;
using std::operator|;
using std::operator&;
using std::operator~;
using std::operator+;
using std::operator<<;
using std::operator>>;

namespace views = ranges::views;

} // namespace std

export namespace std::filesystem {
using std::filesystem::canonical;
using std::filesystem::create_directories;
using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::remove;
using std::filesystem::remove_all;
using std::filesystem::temp_directory_path;
} // namespace std::filesystem

export namespace std::ranges {
using std::ranges::copy;
using std::ranges::count_if;
using std::ranges::find;
using std::ranges::max_element;
using std::ranges::none_of;
using std::ranges::replace;
using std::ranges::sort;
using std::ranges::to;
using std::ranges::transform;
} // namespace std::ranges

export namespace std::ranges::views {
using std::ranges::views::filter;
using std::ranges::views::transform;
using std::ranges::views::zip;
} // namespace std::ranges::views

export using ::int32_t;
export using ::uint32_t;
export using ::uint64_t;
export using ::size_t;
