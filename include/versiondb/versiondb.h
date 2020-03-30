#ifndef TXSERVICE_VERSIONDB_VERSIONDB_H_
#define TXSERVICE_VERSIONDB_VERSIONDB_H_

#include "versiondb/request/handler.h"

namespace txservice
{
class  VersionDb
{
public:
    using Pointer = std::unique_ptr<VersionDb>;
    // there are situations where a handler is bound to a special eventloop, such as redis.
    virtual request::Handler::Pointer MakeHandler() = 0;

    virtual bool CreateVersionTable(const TableName &table_name) = 0;
    
    virtual bool DropVersionTable(const TableName &table_name) = 0;
    virtual bool ClearTransactions()
    {
        return true;
    }
    virtual bool ClearVersions()
    {
        return true;
    }

    virtual ~VersionDb() = default;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_VERSIONDB_H_
