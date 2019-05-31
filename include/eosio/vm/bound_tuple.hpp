#pragma once

#include <eosio/vm/compile_time_string.hpp>

#include <limits>
#include <tuple>
#include <type_traits>

namespace eosio { namespace vm {
   namespace detail {
      struct no_type_found {};
      template <uint64_t N, typename Tup>
      inline constexpr auto _tuple_elem(std::enable_if_t < N<std::tuple_size_v<Tup>, int> _ = 0) {
         return std::tuple_element_t<N, Tup>{};
      }
      template <uint64_t N, typename Tup>
      inline constexpr auto _tuple_elem(std::enable_if_t<N >= std::tuple_size_v<Tup>, int> _ = 0) {
         return no_type_found{};
      }

      template <uint64_t N, typename Tup>
      using tuple_element_t = decltype(_tuple_elem<N, Tup>());

      template <uint64_t N, typename Tst, typename Nth, typename Bindings>
      struct _get_index {
         static constexpr uint64_t value = _get_index<N + 1, Tst, tuple_element_t<N + 1, Bindings>, Bindings>::value;
      };

      template <uint64_t N, typename Tst, typename Bindings>
      struct _get_index<N, Tst, Tst, Bindings> {
         static constexpr uint64_t value = N;
      };

      template <uint64_t N, typename Tst, typename Bindings>
      struct _get_index<N, Tst, no_type_found, Bindings> {
         static constexpr uint64_t value = std::numeric_limits<uint64_t>::max();
      };

      template <typename Tst, typename Bindings>
      struct get_index {
         static constexpr uint64_t value = _get_index<0, Tst, tuple_element_t<0, Bindings>, Bindings>::value;
      };

      // even and odd accessors
      template <typename... Args>
      struct get_odd_args;

      template <typename... Args>
      struct get_even_args;

      template <typename Arg0, typename Arg1, typename... Args>
      struct get_even_args<Arg0, Arg1, Args...> {
         using type = decltype(std::tuple_cat(std::declval<std::tuple<std::decay_t<Arg0>>>(),
                                              std::declval<typename get_even_args<Args...>::type>()));
      };

      template <typename Arg0, typename Arg1>
      struct get_even_args<Arg0, Arg1> {
         using type = std::tuple<std::decay_t<Arg0>>;
      };

      template <typename Arg0, typename Arg1, typename... Args>
      struct get_odd_args<Arg0, Arg1, Args...> {
         using type = decltype(
               std::tuple_cat(std::declval<std::tuple<Arg1>>(), std::declval<typename get_odd_args<Args...>::type>()));
      };

      template <typename Arg0, typename Arg1>
      struct get_odd_args<Arg0, Arg1> {
         using type = std::tuple<Arg1>;
      };

      template <typename... Args>
      using get_odd_args_t = typename get_odd_args<Args...>::type;

      template <typename... Args>
      using get_even_args_t = typename get_even_args<Args...>::type;

      template <typename Arg0, typename Arg1, typename... Args>
      inline constexpr auto _forward_odd_args(Arg0 arg0, Arg1 arg1, Args... args) {
         if constexpr (sizeof...(Args) > 0)
            return std::tuple_cat(std::make_tuple(std::move(arg1)), _forward_odd_args(args...));
         else
            return std::make_tuple(std::move(arg1));
      }

      template <typename... Args>
      inline constexpr auto forward_odd_args(Args... args) {
         return _forward_odd_args(args...);
      }

#if 0
     /**
      * Helper to determine if none of the fields are already taken.
      */
     template <size_t I, size_t N, typename Tst, typename BaseTup>
     struct _check_fields {
       static constexpr bool value = std::is_same_v<Tst, std::tuple_element_t<I, BaseTup>> ? false : _check_fields<I+1, M, N, Tst, BaseTup>::value;
     };
     template <size_t I, size_t N, typename Tst, typename BaseTup>
     struct _check_fields<I, I, N, Tst, BaseTup>
     template <typename Base, typename BndTup>
     struct check_fields {
     };
#endif
   } // namespace detail

   template <typename... Args>
   struct bound_tuple {
      using bindings = detail::get_even_args_t<Args...>;
      detail::get_odd_args_t<Args...> body;

      inline constexpr bound_tuple(Args... args) : body(detail::forward_odd_args(args...)) {}

      template <typename Str>
      inline constexpr auto&& get(const Str& str) && {
         return std::get<detail::get_index<Str, bindings>::value>(body);
      }

      template <typename Str>
      inline constexpr auto& get(const Str& str) {
         return std::get<detail::get_index<Str, bindings>::value>(body);
      }

      template <typename Str>
      inline constexpr const auto& get(const Str& str) const {
         return std::get<detail::get_index<Str, bindings>::value>(body);
      }
   };
}} // namespace eosio::vm

#define GENERATE_FIELDS(...)                                                                                           \
   using fields_t  = decltype(eosio::vm::bound_tuple(__VA_ARGS__));                                                    \
   fields_t fields = eosio::vm::bound_tuple(__VA_ARGS__);                                                              \
   template <uint64_t I>                                                                                               \
   using get_id = std::tuple_element_t<I, fields_t::bindings>;                                                         \
   static inline constexpr uint64_t get_num_of_fields() { return std::tuple_size_v<fields_t::bindings>; }              \
   template <typename Str>                                                                                             \
   using get_field_t = std::tuple_element_t<detail::get_index<Str, fields_t::bindings>::value,                         \
                                            decltype(std::declval<fields_t>().body)>;                                  \
   template <typename Str>                                                                                             \
   inline constexpr auto&& get(const Str& str)&& {                                                                     \
      return std::move(fields.get(str));                                                                               \
   }                                                                                                                   \
   template <typename Str>                                                                                             \
   inline constexpr auto& get(const Str& str) {                                                                        \
      return fields.get(str);                                                                                          \
   }                                                                                                                   \
   template <typename Str>                                                                                             \
   inline constexpr const auto& get(const Str& str) const {                                                            \
      return fields.get(str);                                                                                          \
   }                                                                                                                   \
   template <uint64_t I>                                                                                               \
   inline constexpr auto&& get()&& {                                                                                   \
      return std::move(std::get<I>(fields.body));                                                                      \
   }                                                                                                                   \
   template <uint64_t I>                                                                                               \
   inline constexpr auto& get() {                                                                                      \
      return std::get<I>(fields.body);                                                                                 \
   }                                                                                                                   \
   template <uint64_t I>                                                                                               \
   inline constexpr const auto& get() const {                                                                          \
      return std::get<I>(fields.body);                                                                                 \
   }

#define INHERIT_FIELDS(BASE, ...)                                                                                      \
   using fields_t  = decltype(eosio::vm::bound_tuple(__VA_ARGS__));                                                    \
   fields_t fields = eosio::vm::bound_tuple(__VA_ARGS__);                                                              \
   template <uint64_t I>                                                                                               \
   using get_id = std::tuple_element_t<I, fields_t::bindings>;                                                         \
   static inline constexpr uint64_t get_num_of_fields() { return std::tuple_size_v<fields_t::bindings>; }              \
   template <typename Str>                                                                                             \
   using get_field_t = std::tuple_element_t<detail::get_index<Str, fields_t::bindings>::value,                         \
                                            decltype(std::declval<fields_t>().body)>;                                  \
   static_assert(eosio::vm::detail::check_fields<BASE, fields>::value, "field already defined");                       \
   template <typename Str>                                                                                             \
   inline constexpr auto& get(const Str& str) {                                                                        \
      constexpr uint64_t index = detail::get_index<Str, decltype(fields)::bindings>::value;                            \
      if constexpr (index < (uint64_t)std::tuple_size_v < decltype(fields)::bindings)>)                                \
         return std::get<index>(fields.body);                                                                          \
      else                                                                                                             \
         return BASE::get(str);                                                                                        \
   }                                                                                                                   \
   template <typename Str>                                                                                             \
   inline constexpr auto&& get(const Str& str)&& {                                                                     \
      constexpr uint64_t index = detail::get_index<Str, decltype(fields)::bindings>::value;                            \
      if constexpr (index < (uint64_t)std::tuple_size_v<decltype(fields)::bindings>)                                   \
         return std::get<index>(fields.body);                                                                          \
      else                                                                                                             \
         return BASE::get(str);                                                                                        \
   }                                                                                                                   \
   template <typename Str>                                                                                             \
   inline constexpr const auto& get(const Str& str) const {                                                            \
      constexpr uint64_t index = detail::get_index<Str, decltype(fields)::bindings>::value;                            \
      if constexpr (index < (uint64_t)std::tuple_size_v<decltype(fields)::bindings>)                                   \
         return std::get<index>(fields.body);                                                                          \
      else                                                                                                             \
         return BASE::get(str);                                                                                        \
   }                                                                                                                   \
   template <uint64_t I>                                                                                               \
   inline constexpr auto&& get()&& {                                                                                   \
      return std::move(std::get<I>(fields.body));                                                                      \
   }                                                                                                                   \
   template <uint64_t I>                                                                                               \
   inline constexpr auto& get() {                                                                                      \
      return std::get<I>(fields.body);                                                                                 \
   }                                                                                                                   \
   template <uint64_t I>                                                                                               \
   inline constexpr const auto& get() const {                                                                          \
      return std::get<I>(fields.body);                                                                                 \
   }
