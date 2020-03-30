#ifndef TXSERVICE_VERSIONDB_REQUEST_HANDLER_H_
#define TXSERVICE_VERSIONDB_REQUEST_HANDLER_H_

#include <memory>
#include <vector>
#include "versiondb/request/handler-result.h"
#include "versiondb/entry-extension.h"
#include "versiondb/key.h"
#include "versiondb/tx-entry.h"
#include "versiondb/version-entry.h"
#include "txcheckpoint/key-iterator.h"

namespace txservice::request
{
class Handler
{
public:
    using Pointer = std::unique_ptr<Handler>;

    virtual ~Handler() = default;

    /**
     * Upload a new version of a existing version list
     * @return true  if the version is successfully uploaded
     *         false if the version specified in the request exists
     *                  (indicates a w-w conflict)
     */
    virtual void UploadVersion(const TableName &table_name,
                               const Key &key,
                               VersionEntry &version_entry,
                               EntryExtension *extension,
                               HandlerResult<int64_t> &) = 0;

    /**
     * Delete the dirty version of a version list.
     * The sender tx of this request should be the one created the dirty version
     * @return it shouldn't fail if everything goes right,
     *         any failure of this request is considered a error.
     */
    virtual void DeleteVersion(const TableName &table_name,
                               const Key &key,
                               int64_t version_key,
                               EntryExtension *extension,
                               HandlerResult<Void> &) = 0;

    /**
    Clean the old version entry
    */
    virtual void CleanStaleVersion(const TableName &table_name,
                                   int64_t end_time,
                                   HandlerResult<Void> &) = 0;

    /**
    Clean the old txn entry
    */
    virtual void CleanStaleTxn(int64_t end_time, HandlerResult<Void> &) = 0;

    /**
     * Initialize the version list of a new key.
     * @return true  if the new version list is successfully created
     *         false if there is a existing version list with the same key
     */
    virtual void InitVersionList(const TableName &table_name,
                                 const Key &key,
                                 VersionEntry &version_entry,
                                 HandlerResult<bool> &) = 0;

    /**
     * Replace the begin_ts, end_ts, tx_id of a specific version.
     * Used in postprocessing operation.
     */
    virtual void CommitVersion(const TableName &table_name,
                                const Key &key,
                                int64_t version_key,
                                int64_t expect_txn_id,
                                int64_t target_begin_ts,
                                int64_t target_end_ts,
                                int64_t target_tx_id,
                                EntryExtension *extension,
                                HandlerResult<Void> &,
                                Record *record = nullptr,
                                bool commited = false) = 0;

    /**
     * Comparing the commit_ts to the remote commit_ts and keep the greater one
     * @return the updated version entry
     */
    virtual void UpdateMaxCommitTsAndReread(const TableName &table_name,
                                            const Key &key,
                                            int64_t version_key,
                                            int64_t max_commit_ts,
                                            EntryExtension *extension,
                                            HandlerResult<VersionEntry> &) = 0;

    /**
     * Decrease the read counter
     */
    virtual void ReleaseReadCounter(const TableName &table_name,
                                    const Key &key,
                                    int64_t version_key,
                                    EntryExtension *extension,
                                    HandlerResult<Void> &) = 0;

    /**
     * If the key not exist, then init the version list and return the initial
     * pseudo version; otherwise, get the latest existing versions (more
     * than 2) in the version list and increase counter.
     */
    virtual void GetVersionList(const TableName &table_name,
                                const Key &key,
                                const int64_t time,
                                HandlerResult<std::vector<VersionEntry>> &,
                                void *) = 0;

    /**
     * Retrieve the specified TxEntry
     */
    virtual void GetTxn(int64_t txn_id, HandlerResult<TxnEntry> &) = 0;

    /**
     * Recycle the specified transaction record
     * Reset the TxEntry whether the tx_id exists
     */
    virtual void NewTxn(TxnEntry &entry,
                        int64_t local_time,
                        int64_t max_txn_execution_time_ms,
                        HandlerResult<Void> &) = 0;

    /**
     * Set commit timestamp of specified transaction
     * @return the new commit timestamp
     */
    virtual void SetCommitTimestamp(int64_t txn_id,
                                    int64_t commit_ts,
                                    EntryExtension *extension,
                                    HandlerResult<int64_t> &) = 0;

    /**
     * Update lower bound of commit timestamp of specified transaction to be
     * the maximum of commit_ts and the original one
     * @return the new lower bound of commit time
     */
    virtual void UpdateCommitLowerBound(int64_t txn_id,
                                        int64_t commit_ts_lower_bound,
                                        HandlerResult<TxnEntry> &) = 0;

    /**
     * Set the transaction status
     */
    virtual void UpdateTxnStatus(int64_t txn_id,
                                 TxnStatus status,
                                 EntryExtension *extension,
                                 HandlerResult<Void> &) = 0;

    virtual void GetAllCurrentKeys(int64_t txn_id,
                                   const TableName &table_name,
                                   HandlerResult<std::vector<StringKey>> &)
    {
        // empty implementation for this to compile
    }

    virtual txcheckpoint::KeyIterator::Pointer GetAllCurrentKeys(
        const TableName &, void *) = 0;

    virtual void InsertRangeEntry(int64_t txn_id,
                                  const TableName &table_name,
                                  const std::string &range,
                                  int64_t commit_ts,
                                  HandlerResult<Void> &)
    {
        // empty implementation for this to compile
    }

    virtual void UpdateRangeEntry(int64_t txn_id,
                                  int64_t commit_ts,
                                  HandlerResult<Void> &)
    {
        // empty implementation for this to compile
    }

    virtual txcheckpoint::KeyIterator::Pointer GetCheckpointKeys(TableName &,
		                                                   int,
                                                           int64_t,
                                                           void *) = 0;

	virtual void CheckRemoveActEntry(TableName &, int, Key *, int64_t,
    HandlerResult<Void> &) = 0;

    virtual void KickoutVersion(const TableName &, const Key *, int64_t, int64_t, int64_t,
    HandlerResult<bool> &) = 0;

    virtual void GetVisibleVersionPromise(const TableName &table_name,
                             const Key &key,
                             HandlerResult<VersionEntry> &,
                             void *) = 0;

    virtual void SendBatch()
    {
    }
};
}  // namespace txservice::request
#endif  // TXSERVICE_VERSIONDB_REQUEST_HANDLER_H_