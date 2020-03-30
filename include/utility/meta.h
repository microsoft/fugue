#ifndef TXSERVICE_UTILITY_META_H_
#define TXSERVICE_UTILITY_META_H_

#include <functional>
#include <type_traits>

namespace txservice::meta
{
template <typename P>
using DerefT = std::remove_reference_t<decltype(*std::declval<P>())>;

template <typename P, typename T, typename = void>
struct IsPointerOf : std::bool_constant<false>
{
};

// SFINAE requring a valid dereference operator
template <typename P, typename T>
struct IsPointerOf<
    P,
    T,
    std::enable_if_t<std::is_base_of_v<T, std::remove_cv_t<DerefT<P>>>>>
    : std::bool_constant<true>
{
};
}  // namespace txservice::meta
#endif  // TXSERVICE_UTILITY_META_H_

