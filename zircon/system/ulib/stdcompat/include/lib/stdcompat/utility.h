// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_STDCOMPAT_UTILITY_H_
#define LIB_STDCOMPAT_UTILITY_H_

#include <lib/stdcompat/internal/utility.h>

#include <utility>

namespace cpp17 {
// Use alias for cpp17 and above.
#if __cplusplus >= 201411L && !defined(LIB_STDCOMPAT_USE_POLYFILLS)

using std::in_place;
using std::in_place_t;

using std::in_place_index;
using std::in_place_index_t;

using std::in_place_type;
using std::in_place_type_t;

#else  // Provide provide polyfills for |in_place*| types and variables.

// Tag for requesting in-place initialization.
struct in_place_t {
  explicit constexpr in_place_t() = default;
};

// Tag for requesting in-place initialization by type.
template <typename T>
struct in_place_type_t {
  explicit constexpr in_place_type_t() = default;
};

// Tag for requesting in-place initialization by index.
template <size_t Index>
struct in_place_index_t final {
  explicit constexpr in_place_index_t() = default;
};

// Use inline variables if available.
#if __cpp_inline_variables >= 201606L && !defined(LIB_STDCOMPAT_USE_POLYFILLS)

constexpr in_place_t in_place{};

template <typename T>
constexpr in_place_type_t<T> in_place_type{};

template <size_t Index>
constexpr in_place_index_t<Index> in_place_index{};

#else  // Provide polyfill reference to provided variable storage.

static constexpr const in_place_t& in_place =
    internal::instantiate_templated_tag<in_place_t>::storage;

template <typename T>
static constexpr const in_place_type_t<T>& in_place_type =
    internal::instantiate_templated_tag<in_place_type_t<T>>::storage;

template <size_t Index>
static constexpr const in_place_index_t<Index>& in_place_index =
    internal::instantiate_templated_tag<in_place_index_t<Index>>::storage;

#endif  // __cpp_inline_variables >= 201606L && !defined(LIB_STDCOMPAT_USE_POLYFILLS)

#endif  // __cplusplus >= 201411L && !defined(LIB_STDCOMPAT_USE_POLYFILLS)

}  // namespace cpp17

#endif  // LIB_STDCOMPAT_UTILITY_H_
