#ifndef TXSERVICE_VERSIONDB_KEY_H_
#define TXSERVICE_VERSIONDB_KEY_H_

#include <cstring>
#include "utility/types.h"
#include "utility/configuration.h"

namespace txservice
{
enum KeyType
{
    String,
    Int,
};

struct Key : Copyable<Key>, Hashable, EqualComparable<Key>, Serializable
{
    using Pointer = std::unique_ptr<Key>;

    virtual Pointer Copy() const = 0;
    virtual bool CopyFrom(const Key &that) = 0;
    virtual size_t Hash() const = 0;
    virtual size_t Serialize_Length() const = 0;
    virtual void SerializeToBuffer(char *buffer, size_t &offset) const = 0;
    virtual void DeserializeFromBuffer(const char *buffer, size_t &offset) = 0;
    virtual bool operator==(const Key &rhs) const = 0;
};

struct StringKey: Key
{
    StringKey(const std::string &s = "") : k(s)
    {
    }

    virtual Key::Pointer Copy() const override
    {
        return std::make_unique<StringKey>(k);
    }

    virtual bool CopyFrom(const Key &that) override
    {
        if (auto thatp = static_cast<const StringKey *>(&that))
        {
            k = thatp->k;
            return true;
        }
        return false;
    }

    virtual bool operator==(const Key &rhs) const override
    {
        if (auto rp = static_cast<const StringKey *>(&rhs))
        {
            return rp->k == k;
        }
        return false;
    }

    virtual size_t Hash() const override
    {
        return std::hash<std::string>{}(k);
    }

    virtual size_t Serialize_Length() const override
    {
        return k.size() + Constant::INT_LENGTH;
    }

    virtual void SerializeToBuffer(char *buffer, size_t &offset) const override
    {
        int k_size = k.size();
        memcpy(buffer + offset, &k_size, Constant::INT_LENGTH);
        offset += Constant::INT_LENGTH;
        k.copy(buffer + offset, k_size);
        offset += k_size;
    }

    virtual void DeserializeFromBuffer(const char *buffer, size_t &offset) override
    {
        int data_size = *((int *) (buffer + offset));
        offset += Constant::INT_LENGTH;
        k.assign(buffer + offset, data_size);
        offset += data_size;
    }

    std::string k;
};

struct IntKey : Key
{
    IntKey(const int64_t &val = 0) : k(val)
    {
    }

    virtual Key::Pointer Copy() const override
    {
        return std::make_unique<IntKey>(k);
    }

    virtual bool CopyFrom(const Key &that) override
    {
        if (auto thatp = static_cast<const IntKey *>(&that))
        {
            k = thatp->k;
            return true;
        }
        return false;
    }

    virtual bool operator==(const Key &rhs) const override
    {
        if (auto rp = static_cast<const IntKey *>(&rhs))
        {
            return rp->k == k;
        }
        return false;
    }

    virtual size_t Hash() const override
    {
        return std::hash<int64_t>{}(k);
    }

    virtual size_t Serialize_Length() const override
    {
        return Constant::INT64_T_LENGTH;
    }

    virtual void SerializeToBuffer(char *buffer, size_t &offset) const override
    {
        memcpy(buffer + offset, &k, Constant::INT64_T_LENGTH);
        offset += Constant::INT64_T_LENGTH;
    }

    virtual void DeserializeFromBuffer(const char *buffer, size_t &offset) override
    {
        k = *((int64_t *) (buffer + offset));
        offset += Constant::INT64_T_LENGTH;
    }

    int64_t k;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_KEY_H_