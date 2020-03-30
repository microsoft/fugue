#ifndef TXSERVICE_UTILITY_TYPE_H_
#define TXSERVICE_UTILITY_TYPE_H_

#include <stddef.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "crtp.h"
#include "meta.h"

namespace txservice
{
using TableName = std::string;

struct Void
{
};

template <typename Derived>
struct Copyable : Crtp<Derived, Copyable<Derived>>
{
    std::unique_ptr<Derived> Copy() const
    {
        return this->GetImpl().Copy();
    }

    bool CopyFrom(const Copyable &that)
    {
        return this->GetImpl().CopyFrom(that);
    }
};

// this function is used for the copy of extension
template <typename P,
          typename T = meta::DerefT<P>,
          std::enable_if_t<std::is_base_of_v<Copyable<T>, T>, int> = 0>
auto CopyPtr(const P &p) -> std::unique_ptr<meta::DerefT<P>>
{
    if (p == nullptr)
    {
        return nullptr;
    }
    return p->Copy();
}

struct Serializable
{
    virtual ~Serializable() = default;
    virtual size_t Serialize_Length() const = 0;
    virtual void SerializeToBuffer(char *buffer, size_t &offset) const = 0;
};

template <typename Derived>
struct EqualComparable
{
    bool Equals(const Derived &that) const
    {
        return static_cast<const Derived &>(*this) == that;
    }
};

struct Hashable
{
    virtual size_t Hash() const = 0;
    virtual ~Hashable() = default;
};
}  // namespace txservice
#endif  // TXSERVICE_UTILITY_TYPE_H_