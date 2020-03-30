#ifndef TXSERVICE_TXLOG_TXLOG_H_
#define TXSERVICE_TXLOG_TXLOG_H_

#include "transaction/local-state.h"
#include "versiondb/tx-entry.h"
namespace txservice::txlog
{
class TxLog
{
public:
    using Pointer = std::unique_ptr<TxLog>;

    virtual ~TxLog() = default;

    virtual void Append(
        int executor_id,
        const std::vector<transaction::LocalState::KeyWriteSetEntry::Pointer>
            *write_entry,
        const size_t size,
        const TxnEntry *txn_entry,
        bool sync = true) = 0;

    virtual void AsyncAppend(
        int executor_id,
        const std::vector<transaction::LocalState::KeyWriteSetEntry::Pointer>
            *write_entry,
        const size_t size,
        const TxnEntry *txn_entry,
        std::atomic<bool> *is_finish,
        bool sync = true) = 0;

    virtual void AsyncAppend() = 0;

    virtual void CleanBefore(int64_t timestamp) = 0;

    virtual void Close() = 0;
};
}  // namespace txservice::txlog
#endif  // TXSERVICE_TXLOG_TXLOG_H_