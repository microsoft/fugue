#ifndef TXSERVICE_VERSIONDB_TX_ENTRY_H_
#define TXSERVICE_VERSIONDB_TX_ENTRY_H_

#include <stdint.h>
#include "versiondb/entry-extension.h"
#include "versiondb/version-entry.h"
#include "utility/configuration.h"

namespace txservice
{
enum TxnStatus
{
    kOngoing,
    kCommitted,
    kWaitForAborting,
    kAborting,
    kAborted
};

struct TxnEntry : Serializable
{
    static constexpr int64_t kDefaultCommitTs = -1;

    TxnEntry()
    {
        tx_id = VersionEntry::kEmptyTxId;
        status = TxnStatus::kOngoing;
        commit_ts = kDefaultCommitTs;
        commit_lower_bound = kDefaultCommitTs;
        extension_ = nullptr;
    }

    TxnEntry(const TxnEntry &that)
    {
        tx_id = that.tx_id;
        status = that.status;
        commit_ts = that.commit_ts;
        commit_lower_bound = that.commit_lower_bound;
        extension_ = std::move(CopyPtr(that.extension_));
    }

    TxnEntry(int64_t tx_id_val,
             TxnStatus status_val,
             int64_t commit_ts_val,
             int64_t commit_lower_bound_val,
             EntryExtension::Pointer extension_val = nullptr)
        : tx_id(tx_id_val),
          status(status_val),
          commit_ts(commit_ts_val),
          commit_lower_bound(commit_lower_bound_val),
          extension_(std::move(extension_val))
    {
    }

    void Reset(int64_t tx_id_val, int64_t lower_bound)
    {
        tx_id = tx_id_val;
        status = TxnStatus::kOngoing;
        commit_ts = kDefaultCommitTs;
        commit_lower_bound = lower_bound;
    }

    void Reset(int64_t lower_bound)
    {
        commit_lower_bound = lower_bound;
    }

    size_t Serialize_Length() const
    {
        return 2 * Constant::INT64_T_LENGTH;
    }

    void SerializeToBuffer(char *buffer, size_t &offset) const
    {
        memcpy(buffer + offset, &tx_id, Constant::INT64_T_LENGTH);
        offset += Constant::INT64_T_LENGTH;
        memcpy(buffer + offset, &commit_ts, Constant::INT64_T_LENGTH);
        offset += Constant::INT64_T_LENGTH;
    }

    int64_t tx_id;
    TxnStatus status;
    int64_t commit_ts;
    int64_t commit_lower_bound;
    EntryExtension::Pointer extension_;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_TX_ENTRY_H_
