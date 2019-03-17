#if !defined(phmap_base_h_guard_)
#define phmap_base_h_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Includes work from abseil-cpp (https://github.com/abseil/abseil-cpp)
// with modifications.
// 
// Copyright 2018 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------

#include <stddef.h>
#include <functional>
#include <type_traits>

#include "phmap_config.h"

namespace phmap {

namespace type_traits_internal {

template <typename... Ts>
struct VoidTImpl {
  using type = void;
};

// This trick to retrieve a default alignment is necessary for our
// implementation of aligned_storage_t to be consistent with any implementation
// of std::aligned_storage.
// ---------------------------------------------------------------------------
template <size_t Len, typename T = std::aligned_storage<Len>>
struct default_alignment_of_aligned_storage;

template <size_t Len, size_t Align>
struct default_alignment_of_aligned_storage<Len,
                                            std::aligned_storage<Len, Align>> {
  static constexpr size_t value = Align;
};

// NOTE: The `is_detected` family of templates here differ from the library
// fundamentals specification in that for library fundamentals, `Op<Args...>` is
// evaluated as soon as the type `is_detected<Op, Args...>` undergoes
// substitution, regardless of whether or not the `::value` is accessed. That
// is inconsistent with all other standard traits and prevents lazy evaluation
// in larger contexts (such as if the `is_detected` check is a trailing argument
// of a `conjunction`. This implementation opts to instead be lazy in the same
// way that the standard traits are (this "defect" of the detection idiom
// specifications has been reported).
// ---------------------------------------------------------------------------

template <class Enabler, template <class...> class Op, class... Args>
struct is_detected_impl {
  using type = std::false_type;
};

template <template <class...> class Op, class... Args>
struct is_detected_impl<typename VoidTImpl<Op<Args...>>::type, Op, Args...> {
  using type = std::true_type;
};

template <template <class...> class Op, class... Args>
struct is_detected : is_detected_impl<void, Op, Args...>::type {};

template <class Enabler, class To, template <class...> class Op, class... Args>
struct is_detected_convertible_impl {
  using type = std::false_type;
};

template <class To, template <class...> class Op, class... Args>
struct is_detected_convertible_impl<
    typename std::enable_if<std::is_convertible<Op<Args...>, To>::value>::type,
    To, Op, Args...> {
  using type = std::true_type;
};

template <class To, template <class...> class Op, class... Args>
struct is_detected_convertible
    : is_detected_convertible_impl<void, To, Op, Args...>::type {};

template <typename T>
using IsCopyAssignableImpl =
    decltype(std::declval<T&>() = std::declval<const T&>());

template <typename T>
using IsMoveAssignableImpl = decltype(std::declval<T&>() = std::declval<T&&>());

}  // namespace type_traits_internal

template <typename T>
struct is_copy_assignable : type_traits_internal::is_detected<
                                type_traits_internal::IsCopyAssignableImpl, T> {
};

template <typename T>
struct is_move_assignable : type_traits_internal::is_detected<
                                type_traits_internal::IsMoveAssignableImpl, T> {
};

// ---------------------------------------------------------------------------
// void_t()
//
// Ignores the type of any its arguments and returns `void`. In general, this
// metafunction allows you to create a general case that maps to `void` while
// allowing specializations that map to specific types.
//
// This metafunction is designed to be a drop-in replacement for the C++17
// `std::void_t` metafunction.
//
// NOTE: `phmap::void_t` does not use the standard-specified implementation so
// that it can remain compatible with gcc < 5.1. This can introduce slightly
// different behavior, such as when ordering partial specializations.
// ---------------------------------------------------------------------------
template <typename... Ts>
using void_t = typename type_traits_internal::VoidTImpl<Ts...>::type;

// ---------------------------------------------------------------------------
// conjunction
//
// Performs a compile-time logical AND operation on the passed types (which
// must have  `::value` members convertible to `bool`. Short-circuits if it
// encounters any `false` members (and does not compare the `::value` members
// of any remaining arguments).
//
// This metafunction is designed to be a drop-in replacement for the C++17
// `std::conjunction` metafunction.
// ---------------------------------------------------------------------------
template <typename... Ts>
struct conjunction;

template <typename T, typename... Ts>
struct conjunction<T, Ts...>
    : std::conditional<T::value, conjunction<Ts...>, T>::type {};

template <typename T>
struct conjunction<T> : T {};

template <>
struct conjunction<> : std::true_type {};

// ---------------------------------------------------------------------------
// disjunction
//
// Performs a compile-time logical OR operation on the passed types (which
// must have  `::value` members convertible to `bool`. Short-circuits if it
// encounters any `true` members (and does not compare the `::value` members
// of any remaining arguments).
//
// This metafunction is designed to be a drop-in replacement for the C++17
// `std::disjunction` metafunction.
// ---------------------------------------------------------------------------
template <typename... Ts>
struct disjunction;

template <typename T, typename... Ts>
struct disjunction<T, Ts...> :
      std::conditional<T::value, T, disjunction<Ts...>>::type {};

template <typename T>
struct disjunction<T> : T {};

template <>
struct disjunction<> : std::false_type {};

// ---------------------------------------------------------------------------
// negation
//
// Performs a compile-time logical NOT operation on the passed type (which
// must have  `::value` members convertible to `bool`.
//
// This metafunction is designed to be a drop-in replacement for the C++17
// `std::negation` metafunction.
// ---------------------------------------------------------------------------
template <typename T>
struct negation : std::integral_constant<bool, !T::value> {};

// ---------------------------------------------------------------------------
// is_trivially_destructible()
//
// Determines whether the passed type `T` is trivially destructable.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_destructible()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: the extensions (__has_trivial_xxx) are implemented in gcc (version >=
// 4.3) and clang. Since we are supporting libstdc++ > 4.7, they should always
// be present. These  extensions are documented at
// https://gcc.gnu.org/onlinedocs/gcc/Type-Traits.html#Type-Traits.
// ---------------------------------------------------------------------------
template <typename T>
struct is_trivially_destructible
    : std::integral_constant<bool, __has_trivial_destructor(T) &&
                                   std::is_destructible<T>::value> 
{
#ifdef PHMAP_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
private:
    static constexpr bool compliant = std::is_trivially_destructible<T>::value ==
        is_trivially_destructible::value;
    static_assert(compliant || std::is_trivially_destructible<T>::value,
                  "Not compliant with std::is_trivially_destructible; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_destructible<T>::value,
                  "Not compliant with std::is_trivially_destructible; "
                  "Standard: true, Implementation: false");
#endif 
};

// ---------------------------------------------------------------------------
// is_trivially_default_constructible()
//
// Determines whether the passed type `T` is trivially default constructible.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_default_constructible()` metafunction for platforms that
// have incomplete C++11 support (such as libstdc++ 4.x). On any platforms that
// do fully support C++11, we check whether this yields the same result as the
// std implementation.
//
// NOTE: according to the C++ standard, Section: 20.15.4.3 [meta.unary.prop]
// "The predicate condition for a template specialization is_constructible<T,
// Args...> shall be satisfied if and only if the following variable
// definition would be well-formed for some invented variable t:
//
// T t(declval<Args>()...);
//
// is_trivially_constructible<T, Args...> additionally requires that the
// variable definition does not call any operation that is not trivial.
// For the purposes of this check, the call to std::declval is considered
// trivial."
//
// Notes from https://en.cppreference.com/w/cpp/types/is_constructible:
// In many implementations, is_nothrow_constructible also checks if the
// destructor throws because it is effectively noexcept(T(arg)). Same
// applies to is_trivially_constructible, which, in these implementations, also
// requires that the destructor is trivial.
// GCC bug 51452: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51452
// LWG issue 2116: http://cplusplus.github.io/LWG/lwg-active.html#2116.
//
// "T obj();" need to be well-formed and not call any nontrivial operation.
// Nontrivially destructible types will cause the expression to be nontrivial.
// ---------------------------------------------------------------------------
template <typename T>
struct is_trivially_default_constructible
    : std::integral_constant<bool, __has_trivial_constructor(T) &&
                                   std::is_default_constructible<T>::value &&
                                   is_trivially_destructible<T>::value> 
{
#ifdef PHMAP_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
private:
    static constexpr bool compliant =
        std::is_trivially_default_constructible<T>::value ==
        is_trivially_default_constructible::value;
    static_assert(compliant || std::is_trivially_default_constructible<T>::value,
                  "Not compliant with std::is_trivially_default_constructible; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_default_constructible<T>::value,
                  "Not compliant with std::is_trivially_default_constructible; "
                  "Standard: true, Implementation: false");
#endif
};

// ---------------------------------------------------------------------------
// is_trivially_copy_constructible()
//
// Determines whether the passed type `T` is trivially copy constructible.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_copy_constructible()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: `T obj(declval<const T&>());` needs to be well-formed and not call any
// nontrivial operation.  Nontrivially destructible types will cause the
// expression to be nontrivial.
// ---------------------------------------------------------------------------
template <typename T>
struct is_trivially_copy_constructible
    : std::integral_constant<bool, __has_trivial_copy(T) &&
                                   std::is_copy_constructible<T>::value &&
                                   is_trivially_destructible<T>::value> 
{
#ifdef PHMAP_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
private:
    static constexpr bool compliant =
        std::is_trivially_copy_constructible<T>::value ==
        is_trivially_copy_constructible::value;
    static_assert(compliant || std::is_trivially_copy_constructible<T>::value,
                  "Not compliant with std::is_trivially_copy_constructible; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_copy_constructible<T>::value,
                  "Not compliant with std::is_trivially_copy_constructible; "
                  "Standard: true, Implementation: false");
#endif
};

// ---------------------------------------------------------------------------
// is_trivially_copy_assignable()
//
// Determines whether the passed type `T` is trivially copy assignable.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_trivially_copy_assignable()` metafunction for platforms that have
// incomplete C++11 support (such as libstdc++ 4.x). On any platforms that do
// fully support C++11, we check whether this yields the same result as the std
// implementation.
//
// NOTE: `is_assignable<T, U>::value` is `true` if the expression
// `declval<T>() = declval<U>()` is well-formed when treated as an unevaluated
// operand. `is_trivially_assignable<T, U>` requires the assignment to call no
// operation that is not trivial. `is_trivially_copy_assignable<T>` is simply
// `is_trivially_assignable<T&, const T&>`.
// ---------------------------------------------------------------------------
template <typename T>
struct is_trivially_copy_assignable
    : std::integral_constant<
          bool, __has_trivial_assign(typename std::remove_reference<T>::type) &&
                    phmap::is_copy_assignable<T>::value> 
{
#ifdef PHMAP_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
private:
    static constexpr bool compliant =
        std::is_trivially_copy_assignable<T>::value ==
        is_trivially_copy_assignable::value;
    static_assert(compliant || std::is_trivially_copy_assignable<T>::value,
                  "Not compliant with std::is_trivially_copy_assignable; "
                  "Standard: false, Implementation: true");
    static_assert(compliant || !std::is_trivially_copy_assignable<T>::value,
                  "Not compliant with std::is_trivially_copy_assignable; "
                  "Standard: true, Implementation: false");
#endif
};

// -----------------------------------------------------------------------------
// C++14 "_t" trait aliases
// -----------------------------------------------------------------------------

template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T>
using remove_const_t = typename std::remove_const<T>::type;

template <typename T>
using remove_volatile_t = typename std::remove_volatile<T>::type;

template <typename T>
using add_cv_t = typename std::add_cv<T>::type;

template <typename T>
using add_const_t = typename std::add_const<T>::type;

template <typename T>
using add_volatile_t = typename std::add_volatile<T>::type;

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template <typename T>
using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T>
using add_pointer_t = typename std::add_pointer<T>::type;

template <typename T>
using make_signed_t = typename std::make_signed<T>::type;

template <typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type;

template <typename T>
using remove_extent_t = typename std::remove_extent<T>::type;

template <typename T>
using remove_all_extents_t = typename std::remove_all_extents<T>::type;

template <size_t Len, size_t Align = type_traits_internal::
                          default_alignment_of_aligned_storage<Len>::value>
using aligned_storage_t = typename std::aligned_storage<Len, Align>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <bool B, typename T, typename F>
using conditional_t = typename std::conditional<B, T, F>::type;

template <typename... T>
using common_type_t = typename std::common_type<T...>::type;

template <typename T>
using underlying_type_t = typename std::underlying_type<T>::type;

template <typename T>
using result_of_t = typename std::result_of<T>::type;

namespace type_traits_internal {

// ----------------------------------------------------------------------
// In MSVC we can't probe std::hash or stdext::hash because it triggers a
// static_assert instead of failing substitution. Libc++ prior to 4.0
// also used a static_assert.
// ----------------------------------------------------------------------
#if defined(_MSC_VER) || (defined(_LIBCPP_VERSION) && \
                          _LIBCPP_VERSION < 4000 && _LIBCPP_STD_VER > 11)
    #define PHMAP_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_ 0
#else
    #define PHMAP_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_ 1
#endif

#if !PHMAP_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
    template <typename Key, typename = size_t>
    struct IsHashable : std::true_type {};
#else   // PHMAP_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
    template <typename Key, typename = void>
    struct IsHashable : std::false_type {};

    template <typename Key>
    struct IsHashable<
        Key,
        phmap::enable_if_t<std::is_convertible<
            decltype(std::declval<std::hash<Key>&>()(std::declval<Key const&>())),
            std::size_t>::value>> : std::true_type {};
#endif

struct AssertHashEnabledHelper 
{
private:
    static void Sink(...) {}
    struct NAT {};

    template <class Key>
    static auto GetReturnType(int)
        -> decltype(std::declval<std::hash<Key>>()(std::declval<Key const&>()));
    template <class Key>
    static NAT GetReturnType(...);

    template <class Key>
    static std::nullptr_t DoIt() {
        static_assert(IsHashable<Key>::value,
                      "std::hash<Key> does not provide a call operator");
        static_assert(
            std::is_default_constructible<std::hash<Key>>::value,
            "std::hash<Key> must be default constructible when it is enabled");
        static_assert(
            std::is_copy_constructible<std::hash<Key>>::value,
            "std::hash<Key> must be copy constructible when it is enabled");
        static_assert(phmap::is_copy_assignable<std::hash<Key>>::value,
                      "std::hash<Key> must be copy assignable when it is enabled");
        // is_destructible is unchecked as it's implied by each of the
        // is_constructible checks.
        using ReturnType = decltype(GetReturnType<Key>(0));
        static_assert(std::is_same<ReturnType, NAT>::value ||
                      std::is_same<ReturnType, size_t>::value,
                      "std::hash<Key> must return size_t");
        return nullptr;
    }

    template <class... Ts>
    friend void AssertHashEnabled();
};

template <class... Ts>
inline void AssertHashEnabled
() 
{
    using Helper = AssertHashEnabledHelper;
    Helper::Sink(Helper::DoIt<Ts>()...);
}

}  // namespace type_traits_internal

}  // namespace phmap


// -----------------------------------------------------------------------------
//          hash_policy_traits
// -----------------------------------------------------------------------------
namespace phmap {
namespace container_internal {

// Defines how slots are initialized/destroyed/moved.
template <class Policy, class = void>
struct hash_policy_traits 
{
private:
    struct ReturnKey 
    {
        // We return `Key` here.
        // When Key=T&, we forward the lvalue reference.
        // When Key=T, we return by value to avoid a dangling reference.
        // eg, for string_hash_map.
        template <class Key, class... Args>
        Key operator()(Key&& k, const Args&...) const {
            return std::forward<Key>(k);
        }
    };

    template <class P = Policy, class = void>
    struct ConstantIteratorsImpl : std::false_type {};

    template <class P>
    struct ConstantIteratorsImpl<P, phmap::void_t<typename P::constant_iterators>>
        : P::constant_iterators {};

public:
    // The actual object stored in the hash table.
    using slot_type  = typename Policy::slot_type;

    // The type of the keys stored in the hashtable.
    using key_type   = typename Policy::key_type;

    // The argument type for insertions into the hashtable. This is different
    // from value_type for increased performance. See initializer_list constructor
    // and insert() member functions for more details.
    using init_type  = typename Policy::init_type;

    using reference  = decltype(Policy::element(std::declval<slot_type*>()));
    using pointer    = typename std::remove_reference<reference>::type*;
    using value_type = typename std::remove_reference<reference>::type;

    // Policies can set this variable to tell raw_hash_set that all iterators
    // should be constant, even `iterator`. This is useful for set-like
    // containers.
    // Defaults to false if not provided by the policy.
    using constant_iterators = ConstantIteratorsImpl<>;

    // PRECONDITION: `slot` is UNINITIALIZED
    // POSTCONDITION: `slot` is INITIALIZED
    template <class Alloc, class... Args>
    static void construct(Alloc* alloc, slot_type* slot, Args&&... args) {
        Policy::construct(alloc, slot, std::forward<Args>(args)...);
    }

    // PRECONDITION: `slot` is INITIALIZED
    // POSTCONDITION: `slot` is UNINITIALIZED
    template <class Alloc>
    static void destroy(Alloc* alloc, slot_type* slot) {
        Policy::destroy(alloc, slot);
    }

    // Transfers the `old_slot` to `new_slot`. Any memory allocated by the
    // allocator inside `old_slot` to `new_slot` can be transferred.
    //
    // OPTIONAL: defaults to:
    //
    //     clone(new_slot, std::move(*old_slot));
    //     destroy(old_slot);
    //
    // PRECONDITION: `new_slot` is UNINITIALIZED and `old_slot` is INITIALIZED
    // POSTCONDITION: `new_slot` is INITIALIZED and `old_slot` is
    //                UNINITIALIZED
    template <class Alloc>
    static void transfer(Alloc* alloc, slot_type* new_slot, slot_type* old_slot) {
        transfer_impl(alloc, new_slot, old_slot, 0);
    }

    // PRECONDITION: `slot` is INITIALIZED
    // POSTCONDITION: `slot` is INITIALIZED
    template <class P = Policy>
    static auto element(slot_type* slot) -> decltype(P::element(slot)) {
        return P::element(slot);
    }

    // Returns the amount of memory owned by `slot`, exclusive of `sizeof(*slot)`.
    //
    // If `slot` is nullptr, returns the constant amount of memory owned by any
    // full slot or -1 if slots own variable amounts of memory.
    //
    // PRECONDITION: `slot` is INITIALIZED or nullptr
    template <class P = Policy>
    static size_t space_used(const slot_type* slot) {
        return P::space_used(slot);
    }

    // Provides generalized access to the key for elements, both for elements in
    // the table and for elements that have not yet been inserted (or even
    // constructed).  We would like an API that allows us to say: `key(args...)`
    // but we cannot do that for all cases, so we use this more general API that
    // can be used for many things, including the following:
    //
    //   - Given an element in a table, get its key.
    //   - Given an element initializer, get its key.
    //   - Given `emplace()` arguments, get the element key.
    //
    // Implementations of this must adhere to a very strict technical
    // specification around aliasing and consuming arguments:
    //
    // Let `value_type` be the result type of `element()` without ref- and
    // cv-qualifiers. The first argument is a functor, the rest are constructor
    // arguments for `value_type`. Returns `std::forward<F>(f)(k, xs...)`, where
    // `k` is the element key, and `xs...` are the new constructor arguments for
    // `value_type`. It's allowed for `k` to alias `xs...`, and for both to alias
    // `ts...`. The key won't be touched once `xs...` are used to construct an
    // element; `ts...` won't be touched at all, which allows `apply()` to consume
    // any rvalues among them.
    //
    // If `value_type` is constructible from `Ts&&...`, `Policy::apply()` must not
    // trigger a hard compile error unless it originates from `f`. In other words,
    // `Policy::apply()` must be SFINAE-friendly. If `value_type` is not
    // constructible from `Ts&&...`, either SFINAE or a hard compile error is OK.
    //
    // If `Ts...` is `[cv] value_type[&]` or `[cv] init_type[&]`,
    // `Policy::apply()` must work. A compile error is not allowed, SFINAE or not.
    template <class F, class... Ts, class P = Policy>
    static auto apply(F&& f, Ts&&... ts)
        -> decltype(P::apply(std::forward<F>(f), std::forward<Ts>(ts)...)) {
        return P::apply(std::forward<F>(f), std::forward<Ts>(ts)...);
    }

    // Returns the "key" portion of the slot.
    // Used for node handle manipulation.
    template <class P = Policy>
    static auto key(slot_type* slot)
        -> decltype(P::apply(ReturnKey(), element(slot))) {
        return P::apply(ReturnKey(), element(slot));
    }

    // Returns the "value" (as opposed to the "key") portion of the element. Used
    // by maps to implement `operator[]`, `at()` and `insert_or_assign()`.
    template <class T, class P = Policy>
    static auto value(T* elem) -> decltype(P::value(elem)) {
        return P::value(elem);
    }

private:

    // Use auto -> decltype as an enabler.
    template <class Alloc, class P = Policy>
    static auto transfer_impl(Alloc* alloc, slot_type* new_slot,
                              slot_type* old_slot, int)
        -> decltype((void)P::transfer(alloc, new_slot, old_slot)) {
        P::transfer(alloc, new_slot, old_slot);
    }

    template <class Alloc>
    static void transfer_impl(Alloc* alloc, slot_type* new_slot,
                              slot_type* old_slot, char) {
        construct(alloc, new_slot, std::move(element(old_slot)));
        destroy(alloc, old_slot);
    }
};

}  // namespace container_internal
}  // namespace phmap

// -----------------------------------------------------------------------------
//          optional.h
// -----------------------------------------------------------------------------
#ifdef PHMAP_HAVE_STD_OPTIONAL

#include <optional>  // IWYU pragma: export

namespace phmap {
using std::bad_optional_access;
using std::optional;
using std::make_optional;
using std::nullopt_t;
using std::nullopt;
}  // namespace phmap

#else

#if defined(__clang__)
    #if __has_feature(cxx_inheriting_constructors)
        #define PHMAP_OPTIONAL_USE_INHERITING_CONSTRUCTORS 1
    #endif
#elif (defined(__GNUC__) &&                                       \
       (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 8)) || \
    (__cpp_inheriting_constructors >= 200802) ||                  \
    (defined(_MSC_VER) && _MSC_VER >= 1910)

    #define PHMAP_OPTIONAL_USE_INHERITING_CONSTRUCTORS 1
#endif

namespace phmap {

class bad_optional_access : public std::exception 
{
public:
    bad_optional_access() = default;
    ~bad_optional_access() override;
    const char* what() const noexcept override;
};

template <typename T>
class optional;

// --------------------------------
struct nullopt_t 
{
    struct init_t {};
    static init_t init;

    explicit constexpr nullopt_t(init_t& /*unused*/) {}
};

extern const nullopt_t nullopt;

namespace optional_internal {

// throw delegator
[[noreturn]] void throw_bad_optional_access();


struct empty_struct {};

// This class stores the data in optional<T>.
// It is specialized based on whether T is trivially destructible.
// This is the specialization for non trivially destructible type.
template <typename T, bool unused = std::is_trivially_destructible<T>::value>
class optional_data_dtor_base 
{
    struct dummy_type {
        static_assert(sizeof(T) % sizeof(empty_struct) == 0, "");
        // Use an array to avoid GCC 6 placement-new warning.
        empty_struct data[sizeof(T) / sizeof(empty_struct)];
    };

protected:
    // Whether there is data or not.
    bool engaged_;
    // Data storage
    union {
        dummy_type dummy_;
        T data_;
    };

    void destruct() noexcept {
        if (engaged_) {
            data_.~T();
            engaged_ = false;
        }
    }

    // dummy_ must be initialized for constexpr constructor.
    constexpr optional_data_dtor_base() noexcept : engaged_(false), dummy_{{}} {}

    template <typename... Args>
    constexpr explicit optional_data_dtor_base(in_place_t, Args&&... args)
        : engaged_(true), data_(phmap::forward<Args>(args)...) {}

    ~optional_data_dtor_base() { destruct(); }
};

// Specialization for trivially destructible type.
template <typename T>
class optional_data_dtor_base<T, true> 
{
    struct dummy_type {
        static_assert(sizeof(T) % sizeof(empty_struct) == 0, "");
        // Use array to avoid GCC 6 placement-new warning.
        empty_struct data[sizeof(T) / sizeof(empty_struct)];
    };

protected:
    // Whether there is data or not.
    bool engaged_;
    // Data storage
    union {
        dummy_type dummy_;
        T data_;
    };
    void destruct() noexcept { engaged_ = false; }

    // dummy_ must be initialized for constexpr constructor.
    constexpr optional_data_dtor_base() noexcept : engaged_(false), dummy_{{}} {}

    template <typename... Args>
    constexpr explicit optional_data_dtor_base(in_place_t, Args&&... args)
        : engaged_(true), data_(phmap::forward<Args>(args)...) {}
};

template <typename T>
class optional_data_base : public optional_data_dtor_base<T> 
{
protected:
    using base = optional_data_dtor_base<T>;
#if PHMAP_OPTIONAL_USE_INHERITING_CONSTRUCTORS
    using base::base;
#else
    optional_data_base() = default;

    template <typename... Args>
    constexpr explicit optional_data_base(in_place_t t, Args&&... args)
        : base(t, phmap::forward<Args>(args)...) {}
#endif

    template <typename... Args>
    void construct(Args&&... args) {
        // Use dummy_'s address to work around casting cv-qualified T* to void*.
        ::new (static_cast<void*>(&this->dummy_)) T(std::forward<Args>(args)...);
        this->engaged_ = true;
    }

    template <typename U>
    void assign(U&& u) {
        if (this->engaged_) {
            this->data_ = std::forward<U>(u);
        } else {
            construct(std::forward<U>(u));
        }
    }
};

// TODO(phmap-team): Add another class using
// std::is_trivially_move_constructible trait when available to match
// http://cplusplus.github.io/LWG/lwg-defects.html#2900, for types that
// have trivial move but nontrivial copy.
// Also, we should be checking is_trivially_copyable here, which is not
// supported now, so we use is_trivially_* traits instead.
template <typename T,
          bool unused = phmap::is_trivially_copy_constructible<T>::value&&
              phmap::is_trivially_copy_assignable<typename std::remove_cv<
                  T>::type>::value&& std::is_trivially_destructible<T>::value>
class optional_data;

// Trivially copyable types
template <typename T>
class optional_data<T, true> : public optional_data_base<T> 
{
protected:
#if PHMAP_OPTIONAL_USE_INHERITING_CONSTRUCTORS
    using optional_data_base<T>::optional_data_base;
#else
    optional_data() = default;

    template <typename... Args>
    constexpr explicit optional_data(in_place_t t, Args&&... args)
        : optional_data_base<T>(t, phmap::forward<Args>(args)...) {}
#endif
};

template <typename T>
class optional_data<T, false> : public optional_data_base<T> 
{
protected:
#if PHMAP_OPTIONAL_USE_INHERITING_CONSTRUCTORS
    using optional_data_base<T>::optional_data_base;
#else
    template <typename... Args>
    constexpr explicit optional_data(in_place_t t, Args&&... args)
        : optional_data_base<T>(t, phmap::forward<Args>(args)...) {}
#endif

    optional_data() = default;

    optional_data(const optional_data& rhs) : optional_data_base<T>() {
        if (rhs.engaged_) {
            this->construct(rhs.data_);
        }
    }

    optional_data(optional_data&& rhs) noexcept(
        phmap::default_allocator_is_nothrow::value ||
        std::is_nothrow_move_constructible<T>::value)
    : optional_data_base<T>() {
        if (rhs.engaged_) {
            this->construct(std::move(rhs.data_));
        }
    }

    optional_data& operator=(const optional_data& rhs) {
        if (rhs.engaged_) {
            this->assign(rhs.data_);
        } else {
            this->destruct();
        }
        return *this;
    }

    optional_data& operator=(optional_data&& rhs) noexcept(
        std::is_nothrow_move_assignable<T>::value&&
        std::is_nothrow_move_constructible<T>::value) {
        if (rhs.engaged_) {
            this->assign(std::move(rhs.data_));
        } else {
            this->destruct();
        }
        return *this;
    }
};

// Ordered by level of restriction, from low to high.
// Copyable implies movable.
enum class copy_traits { copyable = 0, movable = 1, non_movable = 2 };

// Base class for enabling/disabling copy/move constructor.
template <copy_traits>
class optional_ctor_base;

template <>
class optional_ctor_base<copy_traits::copyable> 
{
public:
    constexpr optional_ctor_base() = default;
    optional_ctor_base(const optional_ctor_base&) = default;
    optional_ctor_base(optional_ctor_base&&) = default;
    optional_ctor_base& operator=(const optional_ctor_base&) = default;
    optional_ctor_base& operator=(optional_ctor_base&&) = default;
};

template <>
class optional_ctor_base<copy_traits::movable> 
{
public:
    constexpr optional_ctor_base() = default;
    optional_ctor_base(const optional_ctor_base&) = delete;
    optional_ctor_base(optional_ctor_base&&) = default;
    optional_ctor_base& operator=(const optional_ctor_base&) = default;
    optional_ctor_base& operator=(optional_ctor_base&&) = default;
};

template <>
class optional_ctor_base<copy_traits::non_movable> 
{
public:
    constexpr optional_ctor_base() = default;
    optional_ctor_base(const optional_ctor_base&) = delete;
    optional_ctor_base(optional_ctor_base&&) = delete;
    optional_ctor_base& operator=(const optional_ctor_base&) = default;
    optional_ctor_base& operator=(optional_ctor_base&&) = default;
};

// Base class for enabling/disabling copy/move assignment.
template <copy_traits>
class optional_assign_base;

template <>
class optional_assign_base<copy_traits::copyable> 
{
public:
    constexpr optional_assign_base() = default;
    optional_assign_base(const optional_assign_base&) = default;
    optional_assign_base(optional_assign_base&&) = default;
    optional_assign_base& operator=(const optional_assign_base&) = default;
    optional_assign_base& operator=(optional_assign_base&&) = default;
};

template <>
class optional_assign_base<copy_traits::movable> 
{
public:
    constexpr optional_assign_base() = default;
    optional_assign_base(const optional_assign_base&) = default;
    optional_assign_base(optional_assign_base&&) = default;
    optional_assign_base& operator=(const optional_assign_base&) = delete;
    optional_assign_base& operator=(optional_assign_base&&) = default;
};

template <>
class optional_assign_base<copy_traits::non_movable> 
{
public:
    constexpr optional_assign_base() = default;
    optional_assign_base(const optional_assign_base&) = default;
    optional_assign_base(optional_assign_base&&) = default;
    optional_assign_base& operator=(const optional_assign_base&) = delete;
    optional_assign_base& operator=(optional_assign_base&&) = delete;
};

template <typename T>
constexpr copy_traits get_ctor_copy_traits() 
{
    return std::is_copy_constructible<T>::value
        ? copy_traits::copyable
        : std::is_move_constructible<T>::value ? copy_traits::movable
        : copy_traits::non_movable;
}

template <typename T>
constexpr copy_traits get_assign_copy_traits() 
{
    return phmap::is_copy_assignable<T>::value &&
                 std::is_copy_constructible<T>::value
             ? copy_traits::copyable
             : phmap::is_move_assignable<T>::value &&
                       std::is_move_constructible<T>::value
                   ? copy_traits::movable
                   : copy_traits::non_movable;
}

// Whether T is constructible or convertible from optional<U>.
template <typename T, typename U>
struct is_constructible_convertible_from_optional
    : std::integral_constant<
          bool, std::is_constructible<T, optional<U>&>::value ||
                    std::is_constructible<T, optional<U>&&>::value ||
                    std::is_constructible<T, const optional<U>&>::value ||
                    std::is_constructible<T, const optional<U>&&>::value ||
                    std::is_convertible<optional<U>&, T>::value ||
                    std::is_convertible<optional<U>&&, T>::value ||
                    std::is_convertible<const optional<U>&, T>::value ||
                    std::is_convertible<const optional<U>&&, T>::value> {};

// Whether T is constructible or convertible or assignable from optional<U>.
template <typename T, typename U>
struct is_constructible_convertible_assignable_from_optional
    : std::integral_constant<
          bool, is_constructible_convertible_from_optional<T, U>::value ||
                    std::is_assignable<T&, optional<U>&>::value ||
                    std::is_assignable<T&, optional<U>&&>::value ||
                    std::is_assignable<T&, const optional<U>&>::value ||
                    std::is_assignable<T&, const optional<U>&&>::value> {};

// Helper function used by [optional.relops], [optional.comp_with_t],
// for checking whether an expression is convertible to bool.
bool convertible_to_bool(bool);

// Base class for std::hash<phmap::optional<T>>:
// If std::hash<std::remove_const_t<T>> is enabled, it provides operator() to
// compute the hash; Otherwise, it is disabled.
// Reference N4659 23.14.15 [unord.hash].
template <typename T, typename = size_t>
struct optional_hash_base 
{
    optional_hash_base() = delete;
    optional_hash_base(const optional_hash_base&) = delete;
    optional_hash_base(optional_hash_base&&) = delete;
    optional_hash_base& operator=(const optional_hash_base&) = delete;
    optional_hash_base& operator=(optional_hash_base&&) = delete;
};

template <typename T>
struct optional_hash_base<T, decltype(std::hash<phmap::remove_const_t<T> >()(
                                 std::declval<phmap::remove_const_t<T> >()))> 
{
    using argument_type = phmap::optional<T>;
    using result_type = size_t;
    size_t operator()(const phmap::optional<T>& opt) const {
        phmap::type_traits_internal::AssertHashEnabled<phmap::remove_const_t<T>>();
        if (opt) {
            return std::hash<phmap::remove_const_t<T> >()(*opt);
        } else {
            return static_cast<size_t>(0x297814aaad196e6dULL);
        }
    }
};

}  // namespace optional_internal

// -----------------------------------------------------------------------------
// file utility.h
// -----------------------------------------------------------------------------

#include <tuple>
#include <utility>

// --------- identity.h
namespace phmap {
namespace internal {

template <typename T>
struct identity {
    typedef T type;
};

template <typename T>
using identity_t = typename identity<T>::type;

}  // namespace internal
}  // namespace phmap


// --------- inline_variable.h

#ifdef __cpp_inline_variables

#if defined(__clang__)
    #define PHMAP_INTERNAL_EXTERN_DECL(type, name) \
      extern const ::phmap::internal::identity_t<type> name;
#else  // Otherwise, just define the macro to do nothing.
    #define PHMAP_INTERNAL_EXTERN_DECL(type, name)
#endif  // defined(__clang__)

// See above comment at top of file for details.
#define PHMAP_INTERNAL_INLINE_CONSTEXPR(type, name, init) \
  PHMAP_INTERNAL_EXTERN_DECL(type, name)                  \
  inline constexpr ::phmap::internal::identity_t<type> name = init

#else

// See above comment at top of file for details.
//
// Note:
//   identity_t is used here so that the const and name are in the
//   appropriate place for pointer types, reference types, function pointer
//   types, etc..
#define PHMAP_INTERNAL_INLINE_CONSTEXPR(var_type, name, init)                  \
  template <class /*PhmapInternalDummy*/ = void>                               \
  struct PhmapInternalInlineVariableHolder##name {                             \
    static constexpr ::phmap::internal::identity_t<var_type> kInstance = init; \
  };                                                                          \
                                                                              \
  template <class PhmapInternalDummy>                                          \
  constexpr ::phmap::internal::identity_t<var_type>                            \
      PhmapInternalInlineVariableHolder##name<PhmapInternalDummy>::kInstance;   \
                                                                              \
  static constexpr const ::phmap::internal::identity_t<var_type>&              \
      name = /* NOLINT */                                                     \
      PhmapInternalInlineVariableHolder##name<>::kInstance;                    \
  static_assert(sizeof(void (*)(decltype(name))) != 0,                        \
                "Silence unused variable warnings.")

#endif  // __cpp_inline_variables


// ----------- invoke.h

namespace phmap {
namespace base_internal {

template <typename Derived>
struct StrippedAccept 
{
    template <typename... Args>
    struct Accept : Derived::template AcceptImpl<typename std::remove_cv<
                                                     typename std::remove_reference<Args>::type>::type...> {};
};

// (t1.*f)(t2, ..., tN) when f is a pointer to a member function of a class T
// and t1 is an object of type T or a reference to an object of type T or a
// reference to an object of a type derived from T.
struct MemFunAndRef : StrippedAccept<MemFunAndRef> 
{
    template <typename... Args>
    struct AcceptImpl : std::false_type {};

    template <typename R, typename C, typename... Params, typename Obj,
              typename... Args>
    struct AcceptImpl<R (C::*)(Params...), Obj, Args...>
        : std::is_base_of<C, Obj> {};

    template <typename R, typename C, typename... Params, typename Obj,
              typename... Args>
    struct AcceptImpl<R (C::*)(Params...) const, Obj, Args...>
        : std::is_base_of<C, Obj> {};

    template <typename MemFun, typename Obj, typename... Args>
    static decltype((std::declval<Obj>().*
                     std::declval<MemFun>())(std::declval<Args>()...))
    Invoke(MemFun&& mem_fun, Obj&& obj, Args&&... args) {
        return (std::forward<Obj>(obj).*
                std::forward<MemFun>(mem_fun))(std::forward<Args>(args)...);
    }
};

// ((*t1).*f)(t2, ..., tN) when f is a pointer to a member function of a
// class T and t1 is not one of the types described in the previous item.
struct MemFunAndPtr : StrippedAccept<MemFunAndPtr> 
{
    template <typename... Args>
    struct AcceptImpl : std::false_type {};

    template <typename R, typename C, typename... Params, typename Ptr,
              typename... Args>
    struct AcceptImpl<R (C::*)(Params...), Ptr, Args...>
        : std::integral_constant<bool, !std::is_base_of<C, Ptr>::value> {};

    template <typename R, typename C, typename... Params, typename Ptr,
              typename... Args>
    struct AcceptImpl<R (C::*)(Params...) const, Ptr, Args...>
        : std::integral_constant<bool, !std::is_base_of<C, Ptr>::value> {};

    template <typename MemFun, typename Ptr, typename... Args>
    static decltype(((*std::declval<Ptr>()).*
                     std::declval<MemFun>())(std::declval<Args>()...))
    Invoke(MemFun&& mem_fun, Ptr&& ptr, Args&&... args) {
        return ((*std::forward<Ptr>(ptr)).*
                std::forward<MemFun>(mem_fun))(std::forward<Args>(args)...);
    }
};

// t1.*f when N == 1 and f is a pointer to member data of a class T and t1 is
// an object of type T or a reference to an object of type T or a reference
// to an object of a type derived from T.
struct DataMemAndRef : StrippedAccept<DataMemAndRef> 
{
    template <typename... Args>
    struct AcceptImpl : std::false_type {};

    template <typename R, typename C, typename Obj>
    struct AcceptImpl<R C::*, Obj> : std::is_base_of<C, Obj> {};

    template <typename DataMem, typename Ref>
    static decltype(std::declval<Ref>().*std::declval<DataMem>()) Invoke(
        DataMem&& data_mem, Ref&& ref) {
        return std::forward<Ref>(ref).*std::forward<DataMem>(data_mem);
    }
};

// (*t1).*f when N == 1 and f is a pointer to member data of a class T and t1
// is not one of the types described in the previous item.
struct DataMemAndPtr : StrippedAccept<DataMemAndPtr> 
{
    template <typename... Args>
    struct AcceptImpl : std::false_type {};

    template <typename R, typename C, typename Ptr>
    struct AcceptImpl<R C::*, Ptr>
        : std::integral_constant<bool, !std::is_base_of<C, Ptr>::value> {};

    template <typename DataMem, typename Ptr>
    static decltype((*std::declval<Ptr>()).*std::declval<DataMem>()) Invoke(
        DataMem&& data_mem, Ptr&& ptr) {
        return (*std::forward<Ptr>(ptr)).*std::forward<DataMem>(data_mem);
    }
};

// f(t1, t2, ..., tN) in all other cases.
struct Callable
{
    // Callable doesn't have Accept because it's the last clause that gets picked
    // when none of the previous clauses are applicable.
    template <typename F, typename... Args>
    static decltype(std::declval<F>()(std::declval<Args>()...)) Invoke(
        F&& f, Args&&... args) {
        return std::forward<F>(f)(std::forward<Args>(args)...);
    }
};

// Resolves to the first matching clause.
template <typename... Args>
struct Invoker 
{
    typedef typename std::conditional<
        MemFunAndRef::Accept<Args...>::value, MemFunAndRef,
        typename std::conditional<
            MemFunAndPtr::Accept<Args...>::value, MemFunAndPtr,
            typename std::conditional<
                DataMemAndRef::Accept<Args...>::value, DataMemAndRef,
                typename std::conditional<DataMemAndPtr::Accept<Args...>::value,
                                          DataMemAndPtr, Callable>::type>::type>::
        type>::type type;
};

// The result type of Invoke<F, Args...>.
template <typename F, typename... Args>
using InvokeT = decltype(Invoker<F, Args...>::type::Invoke(
    std::declval<F>(), std::declval<Args>()...));

// Invoke(f, args...) is an implementation of INVOKE(f, args...) from section
// [func.require] of the C++ standard.
template <typename F, typename... Args>
InvokeT<F, Args...> Invoke(F&& f, Args&&... args) {
  return Invoker<F, Args...>::type::Invoke(std::forward<F>(f),
                                           std::forward<Args>(args)...);
}
}  // namespace base_internal
}  // namespace phmap


// ----------- utility.h

namespace phmap {

// integer_sequence
//
// Class template representing a compile-time integer sequence. An instantiation
// of `integer_sequence<T, Ints...>` has a sequence of integers encoded in its
// type through its template arguments (which is a common need when
// working with C++11 variadic templates). `phmap::integer_sequence` is designed
// to be a drop-in replacement for C++14's `std::integer_sequence`.
//
// Example:
//
//   template< class T, T... Ints >
//   void user_function(integer_sequence<T, Ints...>);
//
//   int main()
//   {
//     // user_function's `T` will be deduced to `int` and `Ints...`
//     // will be deduced to `0, 1, 2, 3, 4`.
//     user_function(make_integer_sequence<int, 5>());
//   }
template <typename T, T... Ints>
struct integer_sequence 
{
    using value_type = T;
    static constexpr size_t size() noexcept { return sizeof...(Ints); }
};

// index_sequence
//
// A helper template for an `integer_sequence` of `size_t`,
// `phmap::index_sequence` is designed to be a drop-in replacement for C++14's
// `std::index_sequence`.
template <size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

namespace utility_internal {

template <typename Seq, size_t SeqSize, size_t Rem>
struct Extend;

// Note that SeqSize == sizeof...(Ints). It's passed explicitly for efficiency.
template <typename T, T... Ints, size_t SeqSize>
struct Extend<integer_sequence<T, Ints...>, SeqSize, 0> {
  using type = integer_sequence<T, Ints..., (Ints + SeqSize)...>;
};

template <typename T, T... Ints, size_t SeqSize>
struct Extend<integer_sequence<T, Ints...>, SeqSize, 1> {
  using type = integer_sequence<T, Ints..., (Ints + SeqSize)..., 2 * SeqSize>;
};

// Recursion helper for 'make_integer_sequence<T, N>'.
// 'Gen<T, N>::type' is an alias for 'integer_sequence<T, 0, 1, ... N-1>'.
template <typename T, size_t N>
struct Gen {
  using type =
      typename Extend<typename Gen<T, N / 2>::type, N / 2, N % 2>::type;
};

template <typename T>
struct Gen<T, 0> {
  using type = integer_sequence<T>;
};

}  // namespace utility_internal

// Compile-time sequences of integers

// make_integer_sequence
//
// This template alias is equivalent to
// `integer_sequence<int, 0, 1, ..., N-1>`, and is designed to be a drop-in
// replacement for C++14's `std::make_integer_sequence`.
template <typename T, T N>
using make_integer_sequence = typename utility_internal::Gen<T, N>::type;

// make_index_sequence
//
// This template alias is equivalent to `index_sequence<0, 1, ..., N-1>`,
// and is designed to be a drop-in replacement for C++14's
// `std::make_index_sequence`.
template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

// index_sequence_for
//
// Converts a typename pack into an index sequence of the same length, and
// is designed to be a drop-in replacement for C++14's
// `std::index_sequence_for()`
template <typename... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

// Tag types

#ifdef PHMAP_HAVE_STD_OPTIONAL

using std::in_place_t;
using std::in_place;

#else  // PHMAP_HAVE_STD_OPTIONAL

// in_place_t
//
// Tag type used to specify in-place construction, such as with
// `phmap::optional`, designed to be a drop-in replacement for C++17's
// `std::in_place_t`.
struct in_place_t {};

PHMAP_INTERNAL_INLINE_CONSTEXPR(in_place_t, in_place, {});

#endif  // PHMAP_HAVE_STD_OPTIONAL

#if defined(PHMAP_HAVE_STD_ANY) || defined(PHMAP_HAVE_STD_VARIANT)
using std::in_place_type_t;
#else

// in_place_type_t
//
// Tag type used for in-place construction when the type to construct needs to
// be specified, such as with `phmap::any`, designed to be a drop-in replacement
// for C++17's `std::in_place_type_t`.
template <typename T>
struct in_place_type_t {};
#endif  // PHMAP_HAVE_STD_ANY || PHMAP_HAVE_STD_VARIANT

#ifdef PHMAP_HAVE_STD_VARIANT
using std::in_place_index_t;
#else

// in_place_index_t
//
// Tag type used for in-place construction when the type to construct needs to
// be specified, such as with `phmap::any`, designed to be a drop-in replacement
// for C++17's `std::in_place_index_t`.
template <size_t I>
struct in_place_index_t {};
#endif  // PHMAP_HAVE_STD_VARIANT

// Constexpr move and forward

// move()
//
// A constexpr version of `std::move()`, designed to be a drop-in replacement
// for C++14's `std::move()`.
template <typename T>
constexpr phmap::remove_reference_t<T>&& move(T&& t) noexcept {
  return static_cast<phmap::remove_reference_t<T>&&>(t);
}

// forward()
//
// A constexpr version of `std::forward()`, designed to be a drop-in replacement
// for C++14's `std::forward()`.
template <typename T>
constexpr T&& forward(
    phmap::remove_reference_t<T>& t) noexcept {  // NOLINT(runtime/references)
  return static_cast<T&&>(t);
}

namespace utility_internal {
// Helper method for expanding tuple into a called method.
template <typename Functor, typename Tuple, std::size_t... Indexes>
auto apply_helper(Functor&& functor, Tuple&& t, index_sequence<Indexes...>)
    -> decltype(phmap::base_internal::Invoke(
        phmap::forward<Functor>(functor),
        std::get<Indexes>(phmap::forward<Tuple>(t))...)) {
  return phmap::base_internal::Invoke(
      phmap::forward<Functor>(functor),
      std::get<Indexes>(phmap::forward<Tuple>(t))...);
}

}  // namespace utility_internal

// apply
//
// Invokes a Callable using elements of a tuple as its arguments.
// Each element of the tuple corresponds to an argument of the call (in order).
// Both the Callable argument and the tuple argument are perfect-forwarded.
// For member-function Callables, the first tuple element acts as the `this`
// pointer. `phmap::apply` is designed to be a drop-in replacement for C++17's
// `std::apply`. Unlike C++17's `std::apply`, this is not currently `constexpr`.
//
// Example:
//
//   class Foo {
//    public:
//     void Bar(int);
//   };
//   void user_function1(int, std::string);
//   void user_function2(std::unique_ptr<Foo>);
//   auto user_lambda = [](int, int) {};
//
//   int main()
//   {
//       std::tuple<int, std::string> tuple1(42, "bar");
//       // Invokes the first user function on int, std::string.
//       phmap::apply(&user_function1, tuple1);
//
//       std::tuple<std::unique_ptr<Foo>> tuple2(phmap::make_unique<Foo>());
//       // Invokes the user function that takes ownership of the unique
//       // pointer.
//       phmap::apply(&user_function2, std::move(tuple2));
//
//       auto foo = phmap::make_unique<Foo>();
//       std::tuple<Foo*, int> tuple3(foo.get(), 42);
//       // Invokes the method Bar on foo with one argument, 42.
//       phmap::apply(&Foo::Bar, tuple3);
//
//       std::tuple<int, int> tuple4(8, 9);
//       // Invokes a lambda.
//       phmap::apply(user_lambda, tuple4);
//   }
template <typename Functor, typename Tuple>
auto apply(Functor&& functor, Tuple&& t)
    -> decltype(utility_internal::apply_helper(
        phmap::forward<Functor>(functor), phmap::forward<Tuple>(t),
        phmap::make_index_sequence<std::tuple_size<
            typename std::remove_reference<Tuple>::type>::value>{})) {
  return utility_internal::apply_helper(
      phmap::forward<Functor>(functor), phmap::forward<Tuple>(t),
      phmap::make_index_sequence<std::tuple_size<
          typename std::remove_reference<Tuple>::type>::value>{});
}

// exchange
//
// Replaces the value of `obj` with `new_value` and returns the old value of
// `obj`.  `phmap::exchange` is designed to be a drop-in replacement for C++14's
// `std::exchange`.
//
// Example:
//
//   Foo& operator=(Foo&& other) {
//     ptr1_ = phmap::exchange(other.ptr1_, nullptr);
//     int1_ = phmap::exchange(other.int1_, -1);
//     return *this;
//   }
template <typename T, typename U = T>
T exchange(T& obj, U&& new_value)
{
    T old_value = phmap::move(obj);
    obj = phmap::forward<U>(new_value);
    return old_value;
}

}  // namespace phmap


// -----------------------------------------------------------------------------
// phmap::optional class definition
// -----------------------------------------------------------------------------

template <typename T>
class optional : private optional_internal::optional_data<T>,
                 private optional_internal::optional_ctor_base<
                     optional_internal::get_ctor_copy_traits<T>()>,
                 private optional_internal::optional_assign_base<
                     optional_internal::get_assign_copy_traits<T>()> 
{
    using data_base = optional_internal::optional_data<T>;

public:
    typedef T value_type;

    // Constructors

    // Constructs an `optional` holding an empty value, NOT a default constructed
    // `T`.
    constexpr optional() noexcept {}

    // Constructs an `optional` initialized with `nullopt` to hold an empty value.
    constexpr optional(nullopt_t) noexcept {}  // NOLINT(runtime/explicit)

    // Copy constructor, standard semantics
    optional(const optional& src) = default;

    // Move constructor, standard semantics
    optional(optional&& src) = default;

    // Constructs a non-empty `optional` direct-initialized value of type `T` from
    // the arguments `std::forward<Args>(args)...`  within the `optional`.
    // (The `in_place_t` is a tag used to indicate that the contained object
    // should be constructed in-place.)
    template <typename InPlaceT, typename... Args,
              phmap::enable_if_t<phmap::conjunction<
                                    std::is_same<InPlaceT, in_place_t>,
                                    std::is_constructible<T, Args&&...> >::value>* = nullptr>
        constexpr explicit optional(InPlaceT, Args&&... args)
        : data_base(in_place_t(), phmap::forward<Args>(args)...) {}

    // Constructs a non-empty `optional` direct-initialized value of type `T` from
    // the arguments of an initializer_list and `std::forward<Args>(args)...`.
    // (The `in_place_t` is a tag used to indicate that the contained object
    // should be constructed in-place.)
    template <typename U, typename... Args,
              typename = typename std::enable_if<std::is_constructible<
                                                     T, std::initializer_list<U>&, Args&&...>::value>::type>
        constexpr explicit optional(in_place_t, std::initializer_list<U> il,
                                    Args&&... args)
        : data_base(in_place_t(), il, phmap::forward<Args>(args)...) {
    }

    // Value constructor (implicit)
    template <
        typename U = T,
        typename std::enable_if<
            phmap::conjunction<phmap::negation<std::is_same<
                                                 in_place_t, typename std::decay<U>::type> >,
                              phmap::negation<std::is_same<
                                                 optional<T>, typename std::decay<U>::type> >,
                              std::is_convertible<U&&, T>,
                              std::is_constructible<T, U&&> >::value,
            bool>::type = false>
        constexpr optional(U&& v) : data_base(in_place_t(), phmap::forward<U>(v)) {}

    // Value constructor (explicit)
    template <
        typename U = T,
        typename std::enable_if<
            phmap::conjunction<phmap::negation<std::is_same<
                                                 in_place_t, typename std::decay<U>::type>>,
                              phmap::negation<std::is_same<
                                                 optional<T>, typename std::decay<U>::type>>,
                              phmap::negation<std::is_convertible<U&&, T>>,
                              std::is_constructible<T, U&&>>::value,
            bool>::type = false>
        explicit constexpr optional(U&& v)
        : data_base(in_place_t(), phmap::forward<U>(v)) {}

    // Converting copy constructor (implicit)
    template <typename U,
              typename std::enable_if<
                  phmap::conjunction<
                      phmap::negation<std::is_same<T, U> >,
                      std::is_constructible<T, const U&>,
                      phmap::negation<
                          optional_internal::
                          is_constructible_convertible_from_optional<T, U> >,
                      std::is_convertible<const U&, T> >::value,
                  bool>::type = false>
    optional(const optional<U>& rhs) {
        if (rhs) {
            this->construct(*rhs);
        }
    }

    // Converting copy constructor (explicit)
    template <typename U,
              typename std::enable_if<
                  phmap::conjunction<
                      phmap::negation<std::is_same<T, U>>,
                      std::is_constructible<T, const U&>,
                      phmap::negation<
                          optional_internal::
                          is_constructible_convertible_from_optional<T, U>>,
                      phmap::negation<std::is_convertible<const U&, T>>>::value,
                  bool>::type = false>
        explicit optional(const optional<U>& rhs) {
        if (rhs) {
            this->construct(*rhs);
        }
    }

    // Converting move constructor (implicit)
    template <typename U,
              typename std::enable_if<
                  phmap::conjunction<
                      phmap::negation<std::is_same<T, U> >,
                      std::is_constructible<T, U&&>,
                      phmap::negation<
                          optional_internal::
                          is_constructible_convertible_from_optional<T, U> >,
                      std::is_convertible<U&&, T> >::value,
                  bool>::type = false>
        optional(optional<U>&& rhs) {
        if (rhs) {
            this->construct(std::move(*rhs));
        }
    }

    // Converting move constructor (explicit)
    template <
        typename U,
        typename std::enable_if<
            phmap::conjunction<
                phmap::negation<std::is_same<T, U>>, std::is_constructible<T, U&&>,
                phmap::negation<
                    optional_internal::is_constructible_convertible_from_optional<
                        T, U>>,
                phmap::negation<std::is_convertible<U&&, T>>>::value,
            bool>::type = false>
        explicit optional(optional<U>&& rhs) {
        if (rhs) {
            this->construct(std::move(*rhs));
        }
    }

    // Destructor. Trivial if `T` is trivially destructible.
    ~optional() = default;

    // Assignment Operators

    // Assignment from `nullopt`
    //
    // Example:
    //
    //   struct S { int value; };
    //   optional<S> opt = phmap::nullopt;  // Could also use opt = { };
    optional& operator=(nullopt_t) noexcept {
        this->destruct();
        return *this;
    }

    // Copy assignment operator, standard semantics
    optional& operator=(const optional& src) = default;

    // Move assignment operator, standard semantics
    optional& operator=(optional&& src) = default;

    // Value assignment operators
    template <
        typename U = T,
        typename = typename std::enable_if<phmap::conjunction<
                                               phmap::negation<
                                                   std::is_same<optional<T>, typename std::decay<U>::type>>,
                                               phmap::negation<
                                                   phmap::conjunction<std::is_scalar<T>,
                                                                     std::is_same<T, typename std::decay<U>::type>>>,
                                               std::is_constructible<T, U>, std::is_assignable<T&, U>>::value>::type>
        optional& operator=(U&& v) {
        this->assign(std::forward<U>(v));
        return *this;
    }

    template <
        typename U,
        typename = typename std::enable_if<phmap::conjunction<
                                               phmap::negation<std::is_same<T, U>>,
                                               std::is_constructible<T, const U&>, std::is_assignable<T&, const U&>,
                                               phmap::negation<
                                                   optional_internal::
                                                   is_constructible_convertible_assignable_from_optional<
                                                       T, U>>>::value>::type>
        optional& operator=(const optional<U>& rhs) {
        if (rhs) {
            this->assign(*rhs);
        } else {
            this->destruct();
        }
        return *this;
    }

    template <typename U,
              typename = typename std::enable_if<phmap::conjunction<
                                                     phmap::negation<std::is_same<T, U>>, std::is_constructible<T, U>,
                                                     std::is_assignable<T&, U>,
                                                     phmap::negation<
                                                         optional_internal::
                                                         is_constructible_convertible_assignable_from_optional<
                                                             T, U>>>::value>::type>
        optional& operator=(optional<U>&& rhs) {
        if (rhs) {
            this->assign(std::move(*rhs));
        } else {
            this->destruct();
        }
        return *this;
    }

    // Modifiers

    // optional::reset()
    //
    // Destroys the inner `T` value of an `phmap::optional` if one is present.
    PHMAP_ATTRIBUTE_REINITIALIZES void reset() noexcept { this->destruct(); }

    // optional::emplace()
    //
    // (Re)constructs the underlying `T` in-place with the given forwarded
    // arguments.
    //
    // Example:
    //
    //   optional<Foo> opt;
    //   opt.emplace(arg1,arg2,arg3);  // Constructs Foo(arg1,arg2,arg3)
    //
    // If the optional is non-empty, and the `args` refer to subobjects of the
    // current object, then behaviour is undefined, because the current object
    // will be destructed before the new object is constructed with `args`.
    template <typename... Args,
              typename = typename std::enable_if<
                  std::is_constructible<T, Args&&...>::value>::type>
        T& emplace(Args&&... args) {
        this->destruct();
        this->construct(std::forward<Args>(args)...);
        return reference();
    }

    // Emplace reconstruction overload for an initializer list and the given
    // forwarded arguments.
    //
    // Example:
    //
    //   struct Foo {
    //     Foo(std::initializer_list<int>);
    //   };
    //
    //   optional<Foo> opt;
    //   opt.emplace({1,2,3});  // Constructs Foo({1,2,3})
    template <typename U, typename... Args,
              typename = typename std::enable_if<std::is_constructible<
                                                     T, std::initializer_list<U>&, Args&&...>::value>::type>
        T& emplace(std::initializer_list<U> il, Args&&... args) {
        this->destruct();
        this->construct(il, std::forward<Args>(args)...);
        return reference();
    }

    // Swaps

    // Swap, standard semantics
    void swap(optional& rhs) noexcept(
        std::is_nothrow_move_constructible<T>::value&&
        std::is_trivial<T>::value) {
        if (*this) {
            if (rhs) {
                using std::swap;
                swap(**this, *rhs);
            } else {
                rhs.construct(std::move(**this));
                this->destruct();
            }
        } else {
            if (rhs) {
                this->construct(std::move(*rhs));
                rhs.destruct();
            } else {
                // No effect (swap(disengaged, disengaged)).
            }
        }
    }

    // Observers

    // optional::operator->()
    //
    // Accesses the underlying `T` value's member `m` of an `optional`. If the
    // `optional` is empty, behavior is undefined.
    //
    // If you need myOpt->foo in constexpr, use (*myOpt).foo instead.
    const T* operator->() const {
        assert(this->engaged_);
        return std::addressof(this->data_);
    }
    T* operator->() {
        assert(this->engaged_);
        return std::addressof(this->data_);
    }

    // optional::operator*()
    //
    // Accesses the underlying `T` value of an `optional`. If the `optional` is
    // empty, behavior is undefined.
    constexpr const T& operator*() const & { return reference(); }
    T& operator*() & {
        assert(this->engaged_);
        return reference();
    }
    constexpr const T&& operator*() const && {
        return phmap::move(reference());
    }
    T&& operator*() && {
        assert(this->engaged_);
        return std::move(reference());
    }

    // optional::operator bool()
    //
    // Returns false if and only if the `optional` is empty.
    //
    //   if (opt) {
    //     // do something with opt.value();
    //   } else {
    //     // opt is empty.
    //   }
    //
    constexpr explicit operator bool() const noexcept { return this->engaged_; }

    // optional::has_value()
    //
    // Determines whether the `optional` contains a value. Returns `false` if and
    // only if `*this` is empty.
    constexpr bool has_value() const noexcept { return this->engaged_; }

// Suppress bogus warning on MSVC: MSVC complains call to reference() after
// throw_bad_optional_access() is unreachable.
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4702)
#endif  // _MSC_VER
    // optional::value()
    //
    // Returns a reference to an `optional`s underlying value. The constness
    // and lvalue/rvalue-ness of the `optional` is preserved to the view of
    // the `T` sub-object. Throws `phmap::bad_optional_access` when the `optional`
    // is empty.
    constexpr const T& value() const & {
        return static_cast<bool>(*this)
            ? reference()
            : (optional_internal::throw_bad_optional_access(), reference());
    }
    T& value() & {
        return static_cast<bool>(*this)
            ? reference()
            : (optional_internal::throw_bad_optional_access(), reference());
    }
    T&& value() && {  // NOLINT(build/c++11)
        return std::move(
            static_cast<bool>(*this)
            ? reference()
            : (optional_internal::throw_bad_optional_access(), reference()));
    }
    constexpr const T&& value() const && {  // NOLINT(build/c++11)
        return phmap::move(
            static_cast<bool>(*this)
            ? reference()
            : (optional_internal::throw_bad_optional_access(), reference()));
    }
#ifdef _MSC_VER
    #pragma warning(pop)
#endif  // _MSC_VER

    // optional::value_or()
    //
    // Returns either the value of `T` or a passed default `v` if the `optional`
    // is empty.
    template <typename U>
    constexpr T value_or(U&& v) const& {
        static_assert(std::is_copy_constructible<value_type>::value,
                      "optional<T>::value_or: T must by copy constructible");
        static_assert(std::is_convertible<U&&, value_type>::value,
                      "optional<T>::value_or: U must be convertible to T");
        return static_cast<bool>(*this)
            ? **this
            : static_cast<T>(phmap::forward<U>(v));
    }
    template <typename U>
    T value_or(U&& v) && {  // NOLINT(build/c++11)
        static_assert(std::is_move_constructible<value_type>::value,
                      "optional<T>::value_or: T must by copy constructible");
        static_assert(std::is_convertible<U&&, value_type>::value,
                      "optional<T>::value_or: U must be convertible to T");
        return static_cast<bool>(*this) ? std::move(**this)
            : static_cast<T>(std::forward<U>(v));
    }

private:
    // Private accessors for internal storage viewed as reference to T.
    constexpr const T& reference() const { return this->data_; }
    T& reference() { return this->data_; }

    // T constraint checks.  You can't have an optional of nullopt_t, in_place_t
    // or a reference.
    static_assert(
        !std::is_same<nullopt_t, typename std::remove_cv<T>::type>::value,
        "optional<nullopt_t> is not allowed.");
    static_assert(
        !std::is_same<in_place_t, typename std::remove_cv<T>::type>::value,
        "optional<in_place_t> is not allowed.");
    static_assert(!std::is_reference<T>::value,
                  "optional<reference> is not allowed.");
};

// Non-member functions

// swap()
//
// Performs a swap between two `phmap::optional` objects, using standard
// semantics.
//
// NOTE: we assume `is_swappable()` is always `true`. A compile error will
// result if this is not the case.
template <typename T,
          typename std::enable_if<std::is_move_constructible<T>::value,
                                  bool>::type = false>
void swap(optional<T>& a, optional<T>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

// make_optional()
//
// Creates a non-empty `optional<T>` where the type of `T` is deduced. An
// `phmap::optional` can also be explicitly instantiated with
// `make_optional<T>(v)`.
//
// Note: `make_optional()` constructions may be declared `constexpr` for
// trivially copyable types `T`. Non-trivial types require copy elision
// support in C++17 for `make_optional` to support `constexpr` on such
// non-trivial types.
//
// Example:
//
//   constexpr phmap::optional<int> opt = phmap::make_optional(1);
//   static_assert(opt.value() == 1, "");
template <typename T>
constexpr optional<typename std::decay<T>::type> make_optional(T&& v) {
    return optional<typename std::decay<T>::type>(phmap::forward<T>(v));
}

template <typename T, typename... Args>
constexpr optional<T> make_optional(Args&&... args) {
    return optional<T>(in_place_t(), phmap::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
constexpr optional<T> make_optional(std::initializer_list<U> il,
                                    Args&&... args) {
    return optional<T>(in_place_t(), il,
                       phmap::forward<Args>(args)...);
}

// Relational operators [optional.relops]

// Empty optionals are considered equal to each other and less than non-empty
// optionals. Supports relations between optional<T> and optional<U>, between
// optional<T> and U, and between optional<T> and nullopt.
//
// Note: We're careful to support T having non-bool relationals.

// Requires: The expression, e.g. "*x == *y" shall be well-formed and its result
// shall be convertible to bool.
// The C++17 (N4606) "Returns:" statements are translated into
// code in an obvious way here, and the original text retained as function docs.
// Returns: If bool(x) != bool(y), false; otherwise if bool(x) == false, true;
// otherwise *x == *y.
template <typename T, typename U>
constexpr auto operator==(const optional<T>& x, const optional<U>& y)
    -> decltype(optional_internal::convertible_to_bool(*x == *y)) {
    return static_cast<bool>(x) != static_cast<bool>(y)
             ? false
             : static_cast<bool>(x) == false ? true
                                             : static_cast<bool>(*x == *y);
}

// Returns: If bool(x) != bool(y), true; otherwise, if bool(x) == false, false;
// otherwise *x != *y.
template <typename T, typename U>
constexpr auto operator!=(const optional<T>& x, const optional<U>& y)
    -> decltype(optional_internal::convertible_to_bool(*x != *y)) {
    return static_cast<bool>(x) != static_cast<bool>(y)
             ? true
             : static_cast<bool>(x) == false ? false
                                             : static_cast<bool>(*x != *y);
}
// Returns: If !y, false; otherwise, if !x, true; otherwise *x < *y.
template <typename T, typename U>
constexpr auto operator<(const optional<T>& x, const optional<U>& y)
    -> decltype(optional_internal::convertible_to_bool(*x < *y)) {
    return !y ? false : !x ? true : static_cast<bool>(*x < *y);
}
// Returns: If !x, false; otherwise, if !y, true; otherwise *x > *y.
template <typename T, typename U>
constexpr auto operator>(const optional<T>& x, const optional<U>& y)
    -> decltype(optional_internal::convertible_to_bool(*x > *y)) {
    return !x ? false : !y ? true : static_cast<bool>(*x > *y);
}
// Returns: If !x, true; otherwise, if !y, false; otherwise *x <= *y.
template <typename T, typename U>
constexpr auto operator<=(const optional<T>& x, const optional<U>& y)
    -> decltype(optional_internal::convertible_to_bool(*x <= *y)) {
    return !x ? true : !y ? false : static_cast<bool>(*x <= *y);
}
// Returns: If !y, true; otherwise, if !x, false; otherwise *x >= *y.
template <typename T, typename U>
constexpr auto operator>=(const optional<T>& x, const optional<U>& y)
    -> decltype(optional_internal::convertible_to_bool(*x >= *y)) {
    return !y ? true : !x ? false : static_cast<bool>(*x >= *y);
}

// Comparison with nullopt [optional.nullops]
// The C++17 (N4606) "Returns:" statements are used directly here.
template <typename T>
constexpr bool operator==(const optional<T>& x, nullopt_t) noexcept {
    return !x;
}
template <typename T>
constexpr bool operator==(nullopt_t, const optional<T>& x) noexcept {
    return !x;
}
template <typename T>
constexpr bool operator!=(const optional<T>& x, nullopt_t) noexcept {
    return static_cast<bool>(x);
}
template <typename T>
constexpr bool operator!=(nullopt_t, const optional<T>& x) noexcept {
    return static_cast<bool>(x);
}
template <typename T>
constexpr bool operator<(const optional<T>&, nullopt_t) noexcept {
    return false;
}
template <typename T>
constexpr bool operator<(nullopt_t, const optional<T>& x) noexcept {
    return static_cast<bool>(x);
}
template <typename T>
constexpr bool operator<=(const optional<T>& x, nullopt_t) noexcept {
    return !x;
}
template <typename T>
constexpr bool operator<=(nullopt_t, const optional<T>&) noexcept {
    return true;
}
template <typename T>
constexpr bool operator>(const optional<T>& x, nullopt_t) noexcept {
    return static_cast<bool>(x);
}
template <typename T>
constexpr bool operator>(nullopt_t, const optional<T>&) noexcept {
    return false;
}
template <typename T>
constexpr bool operator>=(const optional<T>&, nullopt_t) noexcept {
    return true;
}
template <typename T>
constexpr bool operator>=(nullopt_t, const optional<T>& x) noexcept {
    return !x;
}

// Comparison with T [optional.comp_with_t]

// Requires: The expression, e.g. "*x == v" shall be well-formed and its result
// shall be convertible to bool.
// The C++17 (N4606) "Equivalent to:" statements are used directly here.
template <typename T, typename U>
constexpr auto operator==(const optional<T>& x, const U& v)
    -> decltype(optional_internal::convertible_to_bool(*x == v)) {
    return static_cast<bool>(x) ? static_cast<bool>(*x == v) : false;
}
template <typename T, typename U>
constexpr auto operator==(const U& v, const optional<T>& x)
    -> decltype(optional_internal::convertible_to_bool(v == *x)) {
    return static_cast<bool>(x) ? static_cast<bool>(v == *x) : false;
}
template <typename T, typename U>
constexpr auto operator!=(const optional<T>& x, const U& v)
    -> decltype(optional_internal::convertible_to_bool(*x != v)) {
    return static_cast<bool>(x) ? static_cast<bool>(*x != v) : true;
}
template <typename T, typename U>
constexpr auto operator!=(const U& v, const optional<T>& x)
    -> decltype(optional_internal::convertible_to_bool(v != *x)) {
    return static_cast<bool>(x) ? static_cast<bool>(v != *x) : true;
}
template <typename T, typename U>
constexpr auto operator<(const optional<T>& x, const U& v)
    -> decltype(optional_internal::convertible_to_bool(*x < v)) {
    return static_cast<bool>(x) ? static_cast<bool>(*x < v) : true;
}
template <typename T, typename U>
constexpr auto operator<(const U& v, const optional<T>& x)
    -> decltype(optional_internal::convertible_to_bool(v < *x)) {
    return static_cast<bool>(x) ? static_cast<bool>(v < *x) : false;
}
template <typename T, typename U>
constexpr auto operator<=(const optional<T>& x, const U& v)
    -> decltype(optional_internal::convertible_to_bool(*x <= v)) {
    return static_cast<bool>(x) ? static_cast<bool>(*x <= v) : true;
}
template <typename T, typename U>
constexpr auto operator<=(const U& v, const optional<T>& x)
    -> decltype(optional_internal::convertible_to_bool(v <= *x)) {
    return static_cast<bool>(x) ? static_cast<bool>(v <= *x) : false;
}
template <typename T, typename U>
constexpr auto operator>(const optional<T>& x, const U& v)
    -> decltype(optional_internal::convertible_to_bool(*x > v)) {
    return static_cast<bool>(x) ? static_cast<bool>(*x > v) : false;
}
template <typename T, typename U>
constexpr auto operator>(const U& v, const optional<T>& x)
    -> decltype(optional_internal::convertible_to_bool(v > *x)) {
    return static_cast<bool>(x) ? static_cast<bool>(v > *x) : true;
}
template <typename T, typename U>
constexpr auto operator>=(const optional<T>& x, const U& v)
    -> decltype(optional_internal::convertible_to_bool(*x >= v)) {
    return static_cast<bool>(x) ? static_cast<bool>(*x >= v) : false;
}
template <typename T, typename U>
constexpr auto operator>=(const U& v, const optional<T>& x)
    -> decltype(optional_internal::convertible_to_bool(v >= *x)) {
    return static_cast<bool>(x) ? static_cast<bool>(v >= *x) : true;
}

}  // namespace phmap

namespace std {

// std::hash specialization for phmap::optional.
template <typename T>
struct hash<phmap::optional<T> >
    : phmap::optional_internal::optional_hash_base<T> {};

}  // namespace std

#endif

// -----------------------------------------------------------------------------
//          common.h
// -----------------------------------------------------------------------------
namespace phmap {
namespace container_internal {

template <class, class = void>
struct IsTransparent : std::false_type {};
template <class T>
struct IsTransparent<T, phmap::void_t<typename T::is_transparent>>
    : std::true_type {};

template <bool is_transparent>
struct KeyArg 
{
    // Transparent. Forward `K`.
    template <typename K, typename key_type>
    using type = K;
};

template <>
struct KeyArg<false> 
{
    // Not transparent. Always use `key_type`.
    template <typename K, typename key_type>
    using type = key_type;
};

// The node_handle concept from C++17.
// We specialize node_handle for sets and maps. node_handle_base holds the
// common API of both.
// -----------------------------------------------------------------------
template <typename PolicyTraits, typename Alloc>
class node_handle_base 
{
protected:
    using slot_type = typename PolicyTraits::slot_type;

public:
    using allocator_type = Alloc;

    constexpr node_handle_base() {}

    node_handle_base(node_handle_base&& other) noexcept {
        *this = std::move(other);
    }

    ~node_handle_base() { destroy(); }

    node_handle_base& operator=(node_handle_base&& other) noexcept {
        destroy();
        if (!other.empty()) {
            alloc_ = other.alloc_;
            PolicyTraits::transfer(alloc(), slot(), other.slot());
            other.reset();
        }
        return *this;
    }

    bool empty() const noexcept { return !alloc_; }
    explicit operator bool() const noexcept { return !empty(); }
    allocator_type get_allocator() const { return *alloc_; }

protected:
    friend struct CommonAccess;

    node_handle_base(const allocator_type& a, slot_type* s) : alloc_(a) {
        PolicyTraits::transfer(alloc(), slot(), s);
    }

    void destroy() {
        if (!empty()) {
            PolicyTraits::destroy(alloc(), slot());
            reset();
        }
    }

    void reset() {
        assert(alloc_.has_value());
        alloc_ = phmap::nullopt;
    }

    slot_type* slot() const {
        assert(!empty());
        return reinterpret_cast<slot_type*>(std::addressof(slot_space_));
    }

    allocator_type* alloc() { return std::addressof(*alloc_); }

private:
    phmap::optional<allocator_type> alloc_;
    mutable phmap::aligned_storage_t<sizeof(slot_type), alignof(slot_type)>
    slot_space_;
};

// For sets.
// ---------
template <typename Policy, typename PolicyTraits, typename Alloc,
          typename = void>
class node_handle : public node_handle_base<PolicyTraits, Alloc> 
{
    using Base = typename node_handle::node_handle_base;

public:
    using value_type = typename PolicyTraits::value_type;

    constexpr node_handle() {}

    value_type& value() const { return PolicyTraits::element(this->slot()); }

    value_type& key() const { return PolicyTraits::element(this->slot()); }

private:
    friend struct CommonAccess;

    node_handle(const Alloc& a, typename Base::slot_type* s) : Base(a, s) {}
};

// For maps.
// ---------
template <typename Policy, typename PolicyTraits, typename Alloc>
class node_handle<Policy, PolicyTraits, Alloc,
                  phmap::void_t<typename Policy::mapped_type>>
    : public node_handle_base<PolicyTraits, Alloc> 
{
    using Base = typename node_handle::node_handle_base;

public:
    using key_type = typename Policy::key_type;
    using mapped_type = typename Policy::mapped_type;

    constexpr node_handle() {}

    auto key() const -> decltype(PolicyTraits::key(this->slot())) {
        return PolicyTraits::key(this->slot());
    }

    mapped_type& mapped() const {
        return PolicyTraits::value(&PolicyTraits::element(this->slot()));
    }

private:
    friend struct CommonAccess;

    node_handle(const Alloc& a, typename Base::slot_type* s) : Base(a, s) {}
};

// Provide access to non-public node-handle functions.
struct CommonAccess 
{
    template <typename Node>
    static auto GetSlot(const Node& node) -> decltype(node.slot()) {
        return node.slot();
    }

    template <typename Node>
    static void Reset(Node* node) {
        node->reset();
    }

    template <typename T, typename... Args>
    static T Make(Args&&... args) {
        return T(std::forward<Args>(args)...);
    }
};

// Implement the insert_return_type<> concept of C++17.
template <class Iterator, class NodeType>
struct InsertReturnType 
{
    Iterator position;
    bool inserted;
    NodeType node;
};

}  // namespace container_internal
}  // namespace phmap


#endif // phmap_base_h_guard_
