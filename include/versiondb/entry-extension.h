#ifndef TXSERVICE_VERSIONDB_ENTRY_EXTENSION_H_
#define TXSERVICE_VERSIONDB_ENTRY_EXTENSION_H_

#include "utility/types.h"

namespace txservice
{
struct EntryExtension : Copyable<EntryExtension>
{
    using Pointer = std::unique_ptr<EntryExtension>;

    virtual Pointer Copy() const = 0;
    virtual bool CopyFrom(const EntryExtension &) = 0;
    virtual ~EntryExtension() = default;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_ENTRY_EXTENSION_H_