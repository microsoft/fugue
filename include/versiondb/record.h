#ifndef TXSERVICE_VERSIONDB_RECORD_H_
#define TXSERVICE_VERSIONDB_RECORD_H_

#include <cstring>
#include "utility/configuration.h"
#include "utility/types.h"

namespace txservice
{
struct Record : Copyable<Record>, Serializable
{
    using Pointer = std::unique_ptr<Record>;

    virtual Pointer Copy() const = 0;
    virtual bool CopyFrom(const Record &) = 0;
    virtual ~Record() = default;
    virtual void DeserializeFromBuffer(const char *buffer, size_t &offset) = 0;
};

struct StringRecord : Record
{
    StringRecord(const std::string &data) : data(data)
    {
    }

    virtual Pointer Copy() const override
    {
        return std::make_unique<StringRecord>(data);
    }

    virtual size_t Serialize_Length() const override
    {
        return data.size() + Constant::INT_LENGTH;
    }

    virtual void SerializeToBuffer(char *buffer, size_t &offset) const override
    {
        int data_size = data.size();
        memcpy(buffer + offset, &data_size, Constant::INT_LENGTH);
        offset += Constant::INT_LENGTH;
        data.copy(buffer + offset, data_size);
        offset += data_size;
    }

    virtual void DeserializeFromBuffer(const char *buffer,
                                       size_t &offset) override
    {
        int data_size = *((int *) (buffer + offset));
        offset += Constant::INT_LENGTH;
        data.assign(buffer + offset, data_size);
        offset += data_size;
    }

    virtual bool CopyFrom(const Record &that) override
    {
        if (auto thatp = static_cast<const StringRecord *>(&that))
        {
            data = thatp->data;
            return true;
        }
        return false;
    }

    std::string data;
};

struct IntRecord : Record
{
    IntRecord(const int64_t data) : data(data)
    {
    }

    virtual Pointer Copy() const override
    {
        return std::make_unique<IntRecord>(data);
    }

    virtual size_t Serialize_Length() const override
    {
        return Constant::INT64_T_LENGTH;
    }

    virtual void SerializeToBuffer(char *buffer, size_t &offset) const override
    {
        memcpy(buffer + offset, &data, Constant::INT64_T_LENGTH);
        offset += Constant::INT64_T_LENGTH;
    }

    virtual void DeserializeFromBuffer(const char *buffer,
                                       size_t &offset) override
    {
        data = *((int64_t *) (buffer + offset));
        offset += Constant::INT64_T_LENGTH;
    }

    virtual bool CopyFrom(const Record &that) override
    {
        if (auto thatp = static_cast<const IntRecord *>(&that))
        {
            data = thatp->data;
            return true;
        }
        return false;
    }

    int64_t data;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_RECORD_H_

