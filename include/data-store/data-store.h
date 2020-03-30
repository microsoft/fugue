#ifndef TXSERVICE_DATA_STORE_DATA_STORE_H_
#define TXSERVICE_DATA_STORE_DATA_STORE_H_

#include <memory>
#include "versiondb/key.h"
#include "versiondb/record.h"
#include "versiondb/request/handler-result.h"

namespace txservice
{
class DataStore
{
public:
    using Pointer = std::unique_ptr<DataStore>;

    virtual ~DataStore() = default;

    virtual void ReadRecord(const TableName table_name,
                            Key *key,
                            request::HandlerResult<Record *> &result_) = 0;
    virtual void Update(const TableName table_name,
                        Key *key,
                        Record *record) = 0;
    virtual void Insert(const TableName table_name,
                        Key *key,
                        Record *recordp) = 0;
    virtual void Delete(const TableName table_name,
                        Key *key) = 0;
};
}  // namespace txservice
#endif  // TXSERVICE_DATA_STORE_DATA_STORE_H_