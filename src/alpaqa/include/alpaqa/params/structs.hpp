#pragma once

#include <any>
#include <map>
#include <string_view>
#include <type_traits>
#include <typeinfo>

namespace alpaqa::params {

/// Function wrapper to set attributes of a struct, type-erasing the type of the
/// attribute.
template <class S>
struct attribute_accessor;

/// Like std::any, but storing just the pointer, without any dynamic allocation.
class any_ptr {
  public:
    any_ptr() = default;
    template <class T>
    any_ptr(T *ptr)
        : ptr{const_cast<void *>(reinterpret_cast<const void *>(ptr))},
          ptr_type{&typeid(T)}, is_const{std::is_const_v<T>} {}

    template <class T>
    T *cast() const {
        if (!ptr_type)
            return nullptr;
        if (typeid(T) != *ptr_type)
            throw std::bad_any_cast();
        if (std::is_const_v<T> != is_const)
            throw std::bad_any_cast();
        return reinterpret_cast<T *>(ptr);
    }

  private:
    void *ptr                      = nullptr;
    const std::type_info *ptr_type = nullptr;
    bool is_const                  = true;
};

/// Dictionary that maps struct attribute names to type-erased functions that
/// set those attributes.
template <class S>
using attribute_table_t = std::map<std::string_view, attribute_accessor<S>>;

/// Specialize this type to define the attribute name to attribute setters
/// dictionaries for a struct type @p T.
template <class T, class S>
struct attribute_table;

/// Helper macro to easily specialize @ref alpaqa::params::attribute_table.
#define PARAMS_TABLE(type_, ...)                                               \
    template <class S>                                                         \
    struct attribute_table<type_, S> {                                         \
        using type = type_;                                                    \
        inline static const attribute_table_t<S> table{__VA_ARGS__};           \
    }

/// Helper macro to easily initialize a
/// @ref alpaqa::params::attribute_table_t.
#define PARAMS_MEMBER(name, ...)                                               \
    {                                                                          \
        #name, attribute_accessor<S>::template make<type>(&type::name,         \
                                                          __VA_ARGS__)         \
    }

/// Dictionary that maps struct attribute names to type-erased functions that
/// set those attributes.
template <class S>
using attribute_alias_table_t = std::map<std::string_view, std::string_view>;

/// Specialize this type to define the alternative attribute name to attribute
/// setters dictionaries for a struct type @p T.
template <class T, class S>
struct attribute_alias_table;

/// Helper macro to easily specialize @ref alpaqa::params::attribute_alias_table.
#define PARAMS_ALIAS_TABLE(type_, ...)                                         \
    template <class S>                                                         \
    struct attribute_alias_table<type_, S> {                                   \
        using type = type_;                                                    \
        inline static const attribute_alias_table_t<S> table{__VA_ARGS__};     \
    }

/// Helper macro to easily initialize a
/// @ref alpaqa::params::attribute_alias_table_t.
#define PARAMS_MEMBER_ALIAS(alias, name)                                       \
    { #alias, #name }

/// Function wrapper access the enumerators of an enum, type-erasing the type of
/// the enum.
template <class T, class S>
struct enum_accessor;

/// Dictionary that maps enumerator names to their values and documentation.
template <class T, class S>
using enum_table_t = std::map<std::string_view, enum_accessor<T, S>>;

/// Specialize this type to define the enumerator name to value dictionaries for
/// an enum type @p T.
template <class T, class S>
struct enum_table;

/// Helper macro to easily specialize @ref alpaqa::params::enum_table.
#define ENUM_TABLE(type_, ...)                                                 \
    template <class S>                                                         \
    struct enum_table<type_, S> {                                              \
        using type = type_;                                                    \
        inline static const enum_table_t<type, S> table{__VA_ARGS__};          \
    }

/// Helper macro to easily initialize a
/// @ref alpaqa::params::enum_table_t.
#define ENUM_MEMBER(name, ...)                                                 \
    {                                                                          \
        #name, { type::name, __VA_ARGS__ }                                     \
    }

} // namespace alpaqa::params