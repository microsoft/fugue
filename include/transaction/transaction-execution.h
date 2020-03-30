#ifndef TXSERVICE_TRANSACTION_TRANSACTION_EXECUTION_H_
#define TXSERVICE_TRANSACTION_TRANSACTION_EXECUTION_H_

#include "data-store/data-store.h"
#include "transaction/local-state.h"
#include "transaction/operation-request.h"
#include "transaction/time-provider.h"
#include "transaction/transaction-operation.h"
#include "transaction/txn-id-generator.h"
#include "txlog/txlog.h"

namespace txservice::transaction
{
class TransactionExecution
{
public:
    static constexpr int64_t kMaxTxnExecutionTimeMS = 100000;

    using Pointer = std::unique_ptr<TransactionExecution>;

    TransactionExecution();

    TransactionExecution(int executor_id,
                         request::Handler *handler,
                         TxnIDGenerator *txn_id_generator,
                         TimeProvider *time_provider,
                         txlog::TxLog *tx_log);
    TransactionExecution(const TransactionExecution &that);

    void Call(TransactionOperation *operation,
              bool run_current_operation = true);
    bool MoveForward();
    void CleanStack();
    WriteSetEntry *FindInWriteSet(const TableName &table_name,
                                  const Key &key) const;
    ReadSetEntry *FindInReadSet(const TableName &table_name,
                                const Key &key) const;
    LocalState::KeyReadSetEntry *InsertReadSet();
    void InsertWriteSet(TableName *table_name,
                        Key *key,
                        int64_t version,
                        bool is_deleted,
                        Record *record,
                        ReadSetEntry *read_entry,
                        bool need_post_processing = false);
    const std::vector<LocalState::KeyReadSetEntry::Pointer> *GetAllReadSet()
        const;
    size_t GetReadSetSize() const;
    const std::vector<LocalState::KeyWriteSetEntry::Pointer> *GetAllWriteSet()
        const;
    size_t GetWriteSetSize() const;
    void ReleaseReadSet();
    void ReleaseWriteSet();
    bool EnableLog();
    void SetMaxCommitTsOfWriters(int64_t candidate);
    int64_t GetMaxCommitTsOfWriters();
    void SetCommitTs(int64_t candidate);
    int64_t GetCommitTs();
    void WriteLog(
        int64_t commit_timestamp,
        const std::vector<transaction::LocalState::KeyWriteSetEntry::Pointer>
            *write_entry,
        const size_t size,
        const TxnEntry *txn_entry,
        std::atomic<bool> *is_finish,
        bool sync = true);
    void Abort(std::string msg);
    void Recover(std::string msg);
    Result *Begin(int type);
    Result *Insert(TableName *table_name, Key *key, Record *record, void *);
    Result *Read(TableName *table_name, Key *key, Record *record, void *);
    // bring the record into transaction service for concurrency control
    Result *Read(TableName *table_name,
                 Key *key,
                 Record *record,
                 bool is_deleted,
                 void *);
    Result *ReadDataStore(TableName *table_name,
                          Key *key,
                          Record *record,
                          DataStore *driver,
                          bool need_to_read = false);
    Result *Update(TableName *table_name, Key *key, Record *record);
    Result *Upsert(TableName *table_name, Key *key, Record *record, void *);
    Result *Delete(TableName *table_name, Key *key);
    Result *Commit();
    Result *Abort();
    bool IsFinished();
    void SetFinished();
    TxnStatus GetTxnStatus();
    void SetTxnStatus(TxnStatus status);
    void PrepareAbort();
    void WaitForAbort();
    bool IsAborting();
    bool IsWaitForAborting();
    void Init();
    void Reset();
    void ResetTxnIDAndTime();
    void ResetTime();

    inline void SetCurrentRequest(std::shared_ptr<OperationRequest> request)
    {
        current_request_ = request;
    }

    inline OperationRequest *GetCurrentRequest() const
    {
        return current_request_.get();
    }

public:
    request::Handler *handler_;
    int64_t txn_id_;
    TxnEntry txn_entry_;
    int64_t commit_timestamp_local_;
    int type_;
    Upload upload_operation;
    std::vector<std::unique_ptr<UploadVersionEntry>> upload_version_entry_operation_vector;
    SetCommitTsOperation set_commit_ts_operation;
    Validate validate_operation;
    std::vector<std::unique_ptr<UpdateReadEntryMaxCommitTs>> update_read_entry_max_commit_ts_operation_vector;
    std::vector<std::unique_ptr<PushConflictTxnCommitTsLowerBound>>
        push_conflict_txn_commit_ts_lower_bound_operation_vector;
    WriteToLog write_to_log_operation;
    UpdateTxnStatusToCommit update_txn_status_to_commit_operation;
    PostProcessingAfterCommit post_processing_after_commit_operation;
    std::vector<std::unique_ptr<PostProcessingCommitEntryAfterCommit>>
        post_processing_commit_entry_after_commit_operation_vector;
    PostProcessingAfterAbort post_processing_after_abort_operation;
    std::vector<std::unique_ptr<PostProcessingDeleteEntryAfterAbort>>
        post_processing_delete_entry_after_abort_operation_vector;
    UpdateTxnStatusToAbort update_txn_status_to_abort_operation;
    ReadOutsideOperation read_outside_operation;
    InitTxnOperation init_txn_operation;
    InsertOperation insert_operation;
    UpsertOperation upsert_operation;
    UpdateOperation update_operation;
    DeleteOperation delete_operation;
    ReadDataStoreOperation read_ds_operation;
    ReleaseReadCounter release_read_counter_operation;
    std::vector<std::unique_ptr<ReleaseReadCounterForEachEntry>>
        release_read_counter_for_each_entry_operation_vector;
    Result result_;

private:
    std::vector<TransactionOperation *> operation_vector_;
    LocalState local_state_;
    int64_t max_commit_timestamp_of_writers_ = -1;
    int64_t commit_timestamp_;
    bool is_transaction_finished_;
    TxnStatus status_;
    TxnIDGenerator *txn_id_generator_;
    TimeProvider *time_provider_;
    txlog::TxLog *tx_log_;
    int64_t count = 0;
    int executor_id_;
    std::shared_ptr<OperationRequest> current_request_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_TRANSACTION_EXECUTION_H_
