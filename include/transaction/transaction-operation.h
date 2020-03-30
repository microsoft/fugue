#ifndef TXSERVICE_TRANSACTION_TRANSACTION_OPERATION_H_
#define TXSERVICE_TRANSACTION_TRANSACTION_OPERATION_H_

#include "data-store/data-store.h"
#include "transaction/local-state.h"
#include "transaction/result.h"
#include "versiondb/request/handler.h"

namespace txservice::transaction
{
class TransactionExecution;

struct TransactionOperation
{
    void Invoke(TransactionOperation *operation, 
            bool run_current_operation = true);

    void Call();

    virtual void CallImpl() = 0;

    TransactionOperation *Next();

    virtual TransactionOperation *NextImpl() = 0;

    TransactionOperation *PrepareAbort();

    TransactionOperation *Abort();

    virtual bool IsFinished() const = 0;

    virtual bool IsCascadeFinished() const = 0;

    void Init(TransactionExecution *execution);

    virtual ~TransactionOperation() = default;

protected:
    TransactionExecution *execution_;
    bool move_to_next_ = false;
    bool has_next_ = false;
};

struct ReadOutsideOperation : TransactionOperation
{
    void Reset(TableName *table_name, Key *key, Record *record, 
            bool is_deleted, void *);

    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void InternalRead();

    static VersionEntry *InternalPickVisibleVersion(VersionEntry &v1,
                                                    VersionEntry &v2);

    static VersionEntry *PickVisibleVersion(std::vector<VersionEntry> &list);

private:
    TableName *table_name_;
    Key *key_;
    void *callback_deserializer_;
    Record *record_;
    bool is_deleted_;
    LocalState::KeyReadSetEntry *key_read_set_entry_;
    request::HandlerResult<std::vector<VersionEntry>>
        result_of_get_version_list_;
};

struct Upload : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset();

private:
    const std::vector<LocalState::KeyWriteSetEntry::Pointer> *key_write_set_;
    size_t size_;
};

struct UploadVersionEntry : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(WriteSetEntry *write_set_entry,
            const LocalState::SetKey *set_key);

private:
    WriteSetEntry *write_set_entry_;
    const LocalState::SetKey *set_key_;
    VersionEntry version_entry_;
    request::HandlerResult<int64_t> result_of_upload_version_;
};

struct SetCommitTsOperation : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset();

private:
    request::HandlerResult<int64_t> result_of_set_commit_ts_;
};

struct Validate : TransactionOperation
{
    void CallImpl() override;

    TransactionOperation *NextImpl() override;

    bool IsFinished() const override;

    bool IsCascadeFinished() const override;

    void Reset();

private:
    const std::vector<LocalState::KeyReadSetEntry::Pointer> *key_read_set_;
    std::vector<size_t> read_index_;
};

struct UpdateReadEntryMaxCommitTs : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(ReadSetEntry *read_set_entry, const LocalState::SetKey *set_key,
            size_t index);

private:
    ReadSetEntry *read_set_entry_;
    const LocalState::SetKey *set_key_;
    request::HandlerResult<VersionEntry> result_of_update_max_commit_ts_;
    size_t index_;
};

struct PushConflictTxnCommitTsLowerBound : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(int64_t txn_id);

private:
    request::HandlerResult<TxnEntry> result_of_update_commit_lower_bound_;
    int64_t txn_id_;
};

struct WriteToLog : TransactionOperation
{
    WriteToLog();
    WriteToLog(const WriteToLog &that);
    void Reset();

    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

private:
    std::atomic<bool> is_finished_ = false;
};

struct UpdateTxnStatusToCommit : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset();

private:
    request::HandlerResult<Void> result_of_update_txn_status_to_commit_;
};

struct PostProcessingAfterCommit : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset();

private:
    const std::vector<LocalState::KeyWriteSetEntry::Pointer> *key_write_set_;
    std::vector<size_t> post_process_index_;
};

struct PostProcessingCommitEntryAfterCommit : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(WriteSetEntry *write_set_entry,
            const LocalState::SetKey *set_key);

private:
    WriteSetEntry *entry_;
    const LocalState::SetKey *set_key_;
    request::HandlerResult<Void>
        result_of_commit_version_in_post_processing_commit_;
};

struct PostProcessingAfterAbort : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset();

private:
    const std::vector<LocalState::KeyWriteSetEntry::Pointer> *key_write_set_;
    std::vector<size_t> post_process_index_;
};

struct PostProcessingDeleteEntryAfterAbort : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(const WriteSetEntry *entry, const LocalState::SetKey *set_key);

private:
    const WriteSetEntry *entry_;
    const LocalState::SetKey *set_key_;
    request::HandlerResult<Void>
        result_of_delete_version_in_post_processing_abort_;
};

struct ReleaseReadCounter : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset();

private:
    const std::vector<LocalState::KeyReadSetEntry::Pointer> *key_read_set_;
    std::vector<size_t> release_index_;
};

struct ReleaseReadCounterForEachEntry : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(ReadSetEntry *read_set_entry, const LocalState::SetKey *key);

private:
    ReadSetEntry *read_set_entry_;
    const LocalState::SetKey *set_key_;
    request::HandlerResult<Void> result_of_release_read_counter_;
};

struct UpdateTxnStatusToAbort : TransactionOperation
{
    void Reset();

    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

private:
    request::HandlerResult<Void> result_of_update_txn_status_to_abort_;
};

struct InsertOperation : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(TableName *table_name, Key *key, Record *record, void *);

private:
    TableName *table_name_;
    Key *key_;
    Record *record_;
    void *callback_deserializer_;
};

struct UpdateOperation : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(TableName *table_name, Key *key, Record *record);

private:
    TableName *table_name_;
    Key *key_;
    Record *record_;
};

struct UpsertOperation : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(TableName *table_name, Key *key, Record *record, void *);

private:
    TableName *table_name_;
    Key *key_;
    Record *record_;
    void *callback_deserializer_;
};

struct DeleteOperation : TransactionOperation
{
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(TableName *table_name, Key *key);

private:
    TableName *table_name_;
    Key *key_;
};

struct InitTxnOperation : TransactionOperation
{
    void Reset();
    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

private:
    request::HandlerResult<Void> result_of_new_txn_;
};

struct ReadDataStoreOperation : TransactionOperation
{
    void Reset(TableName *table_name, Key *key, Record *record, 
            DataStore *driver);

    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

private:
    TableName *table_name_;
    Key *key_;
    Record *record_;
    DataStore *driver_;
    request::HandlerResult<Record *> result_of_read_ds_;
};

struct InsertRangeOperation : TransactionOperation
{
    void Reset(TableName *table_name, Key *key, Record *record,
            DataStore *driver);

    virtual void CallImpl() override;

    virtual TransactionOperation *NextImpl() override;

    virtual bool IsFinished() const override;

    virtual bool IsCascadeFinished() const override;

    void Reset(TableName *table_name, const std::string &range_template);

private:
    TableName *table_name_;
    std::string range_template_;
};

} // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_TRANSACTION_OPERATION_H_