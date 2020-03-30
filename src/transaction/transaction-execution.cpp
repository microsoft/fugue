#include "transaction/transaction-execution.h"

namespace txservice::transaction
{
TransactionExecution::TransactionExecution()
    : handler_(nullptr),
      txn_id_generator_(nullptr),
      time_provider_(nullptr),
      tx_log_(nullptr),
      local_state_(10)
{
}

TransactionExecution::TransactionExecution(int executor_id,
                                           request::Handler *handler,
                                           TxnIDGenerator *txn_id_generator,
                                           TimeProvider *time_provider,
                                           txlog::TxLog *txlog)
    : executor_id_(executor_id),
      handler_(handler),
      txn_id_generator_(txn_id_generator),
      time_provider_(time_provider),
      tx_log_(txlog),
      local_state_(10),
      current_request_(nullptr)
{
    is_transaction_finished_ = false;
    Init();
}

TransactionExecution::TransactionExecution(const TransactionExecution &that)
    : result_(that.result_),
      local_state_(that.local_state_),
      max_commit_timestamp_of_writers_(that.max_commit_timestamp_of_writers_),
      commit_timestamp_(that.commit_timestamp_),
      is_transaction_finished_(that.is_transaction_finished_),
      status_(that.status_),
      txn_id_generator_(that.txn_id_generator_),
      time_provider_(that.time_provider_),
      tx_log_(that.tx_log_),
      handler_(that.handler_),
      txn_id_(that.txn_id_),
      txn_entry_(that.txn_entry_),
      commit_timestamp_local_(that.commit_timestamp_local_),
      executor_id_(that.executor_id_),
      current_request_(nullptr)
{
    Init();
}

void TransactionExecution::Init()
{
    upload_operation.Init(this);
    for (int i = 0; i < upload_version_entry_operation_vector.size(); i++)
    {
        upload_version_entry_operation_vector[i]->Init(this);
    }
    set_commit_ts_operation.Init(this);
    validate_operation.Init(this);
    for (int i = 0; i < update_read_entry_max_commit_ts_operation_vector.size(); i++)
    {
        update_read_entry_max_commit_ts_operation_vector[i]->Init(this);
    }
    for (int i = 0; i < push_conflict_txn_commit_ts_lower_bound_operation_vector.size(); i++)
    {
        push_conflict_txn_commit_ts_lower_bound_operation_vector[i]->Init(this);
    }
    write_to_log_operation.Init(this);
    update_txn_status_to_commit_operation.Init(this);
    post_processing_after_commit_operation.Init(this);
    for (int i = 0; i < post_processing_commit_entry_after_commit_operation_vector.size(); i++)
    {
        post_processing_commit_entry_after_commit_operation_vector[i]->Init(this);
    }
    post_processing_after_abort_operation.Init(this);
    for (int i = 0; i < post_processing_delete_entry_after_abort_operation_vector.size(); i++)
    {
        post_processing_delete_entry_after_abort_operation_vector[i]->Init(this);
    }
    update_txn_status_to_abort_operation.Init(this);
    read_outside_operation.Init(this);
    init_txn_operation.Init(this);
    insert_operation.Init(this);
    upsert_operation.Init(this);
    update_operation.Init(this);
    delete_operation.Init(this);
    read_ds_operation.Init(this);
    release_read_counter_operation.Init(this);
    for (int i = 0; i < release_read_counter_for_each_entry_operation_vector.size(); i++)
    {
        release_read_counter_for_each_entry_operation_vector[i]->Init(this);
    }
}

void TransactionExecution::Reset()
{
    local_state_.Reset();
    current_request_ = nullptr;
    is_transaction_finished_ = false;
    status_ = TxnStatus::kOngoing;
    commit_timestamp_ = -1;
    max_commit_timestamp_of_writers_ = -1;
    ResetTxnIDAndTime();
}

void TransactionExecution::ResetTxnIDAndTime()
{
    txn_id_ = txn_id_generator_->GenerateID();
    commit_timestamp_local_ = time_provider_->GetTime();
    txn_entry_.Reset(txn_id_, commit_timestamp_local_);
}

void TransactionExecution::ResetTime()
{
    commit_timestamp_local_ = time_provider_->GetTime();
    txn_entry_.Reset(commit_timestamp_local_);
}

void TransactionExecution::Call(TransactionOperation *operation,
                                bool run_current_operation)
{
    operation_vector_.push_back(operation);
    operation->Call();
}

bool TransactionExecution::MoveForward()
{
    if (operation_vector_.empty())
    {
        return true;
    }
    else
    {
        bool can_be_deleted = true;
        TransactionOperation *operation;

        for (int i = operation_vector_.size() - 1; i >= 0; i--)
        {   
            if (operation_vector_[i] != nullptr && operation_vector_[i]->IsFinished())
            {
                operation = operation_vector_[i]->Next();
                operation_vector_[i] = operation;
                if (operation_vector_[i] != nullptr)
                {
                    operation_vector_[i]->Call();
                }
                else if (IsAborting())
                {
                    break;
                }
            }
        }

        if(IsAborting())
        {
            SetTxnStatus(TxnStatus::kAborted);
            Abort();
        }
        else
        {
            for (int i = operation_vector_.size() - 1; i >= 0; i--)
            {
                if (operation_vector_[i] == nullptr)
                {
                    operation_vector_.pop_back();
                }
                else
                {
                    break;
                }
            }
        }
    }
    return operation_vector_.empty();
}

void TransactionExecution::CleanStack()
{
    operation_vector_.clear();
}

void TransactionExecution::WriteLog(
    int64_t commit_timestamp,
    const std::vector<transaction::LocalState::KeyWriteSetEntry::Pointer>
        *write_entry,
    const size_t size,
    const TxnEntry *txn_entry,
    std::atomic<bool> *is_finish,
    bool sync)
{
    tx_log_->AsyncAppend(
        executor_id_, write_entry, size, txn_entry, is_finish, sync);
}

WriteSetEntry *TransactionExecution::FindInWriteSet(const TableName &table_name,
                                                    const Key &key) const
{
    return local_state_.FindInWriteSet(table_name, key);
}

ReadSetEntry *TransactionExecution::FindInReadSet(const TableName &table_name,
                                                  const Key &key) const
{
    return local_state_.FindInReadSet(table_name, key);
}

LocalState::KeyReadSetEntry *TransactionExecution::InsertReadSet()
{
    return local_state_.InsertReadSet();
}
void TransactionExecution::InsertWriteSet(TableName *table_name,
                                          Key *key,
                                          int64_t version,
                                          bool is_deleted,
                                          Record *record,
                                          ReadSetEntry *read_entry,
                                          bool need_post_processing)
{
    return local_state_.InsertWriteSet(table_name,
                                       key,
                                       version,
                                       is_deleted,
                                       record,
                                       read_entry,
                                       need_post_processing);
}

const std::vector<LocalState::KeyReadSetEntry::Pointer>
    *TransactionExecution::GetAllReadSet() const
{
    return local_state_.GetAllReadSet();
}
size_t TransactionExecution::GetReadSetSize() const
{
    return local_state_.GetReadSetSize();
}

const std::vector<LocalState::KeyWriteSetEntry::Pointer>
    *TransactionExecution::GetAllWriteSet() const
{
    return local_state_.GetAllWriteSet();
}
size_t TransactionExecution::GetWriteSetSize() const
{
    return local_state_.GetWriteSetSize();
}

void TransactionExecution::ReleaseReadSet()
{
    local_state_.ReleaseReadSet();
}

void TransactionExecution::ReleaseWriteSet()
{
    local_state_.ReleaseWriteSet();
}

bool TransactionExecution::EnableLog()
{
    return tx_log_ != nullptr;
}

void TransactionExecution::SetMaxCommitTsOfWriters(int64_t candidate)
{
    max_commit_timestamp_of_writers_ =
        std::max(max_commit_timestamp_of_writers_, candidate);
}

int64_t TransactionExecution::GetMaxCommitTsOfWriters()
{
    return max_commit_timestamp_of_writers_;
}

void TransactionExecution::SetCommitTs(int64_t candidate)
{
    commit_timestamp_ = candidate;
    time_provider_->SetTime(candidate);
}

int64_t TransactionExecution::GetCommitTs()
{
    return commit_timestamp_;
}

void TransactionExecution::Recover(std::string msg)
{
    throw std::runtime_error(
        "unhandle exception and need to be recovered manually");
}

Result *TransactionExecution::Insert(TableName *table_name,
                                     Key *key,
                                     Record *record,
                                     void *callback_deserializer)
{
    result_.Reset(record, GetCurrentRequest());
    insert_operation.Reset(table_name, key, record, callback_deserializer);
    Call(&(insert_operation));
    return &result_;
}

Result *TransactionExecution::Upsert(TableName *table_name,
                                     Key *key,
                                     Record *record,
                                     void *callback_deserializer)
{
    result_.Reset(record, GetCurrentRequest());
    upsert_operation.Reset(table_name, key, record, callback_deserializer);
    Call(&(upsert_operation));
    return &result_;
}

Result *TransactionExecution::Update(TableName *table_name,
                                     Key *key,
                                     Record *record)
{
    result_.Reset(GetCurrentRequest());
    update_operation.Reset(table_name, key, record);
    Call(&(update_operation));
    return &result_;
}

Result *TransactionExecution::Delete(TableName *table_name, Key *key)
{
    result_.Reset(GetCurrentRequest());
    delete_operation.Reset(table_name, key);
    Call(&(delete_operation));
    return &result_;
}

Result *TransactionExecution::Read(TableName *table_name,
                                   Key *key,
                                   Record *record,
                                   void *callback_deserializer)
{
    result_.Reset(record, GetCurrentRequest());
    read_outside_operation.Reset(
        table_name, key, record, false, callback_deserializer);
    Call(&(read_outside_operation));
    return &result_;
}

Result *TransactionExecution::Read(TableName *table_name,
                                   Key *key,
                                   Record *record,
                                   bool is_deleted,
                                   void *callback_deserializer)
{
    result_.Reset(record, GetCurrentRequest());
    read_outside_operation.Reset(table_name,
                                 key,
                                 record,
                                 is_deleted,
                                 callback_deserializer);
    Call(&(read_outside_operation));
    return &result_;
}

Result *TransactionExecution::ReadDataStore(TableName *table_name,
                                            Key *key,
                                            Record *record,
                                            DataStore *driver,
                                            bool need_to_read)
{
    result_.Reset(record, GetCurrentRequest());
    if (need_to_read)
    {
        read_ds_operation.Reset(table_name, key, record, driver);
        Call(&(read_ds_operation));
    }
    else
    {
        result_.SetFinished();
    }

    return &result_;
}

Result *TransactionExecution::Commit()
{
    result_.Reset(GetCurrentRequest());
    upload_operation.Reset();
    Call(&(upload_operation));
    return &result_;
}

void TransactionExecution::Abort(std::string message)
{
    Abort();
} 

Result *TransactionExecution::Abort()
{
    CleanStack();
    result_.Reset(GetCurrentRequest());
    update_txn_status_to_abort_operation.Reset();
    Call(&(update_txn_status_to_abort_operation));
    return &result_;
}

Result *TransactionExecution::Begin(int type)
{
    result_.Reset(GetCurrentRequest());
    type_ = type;
    init_txn_operation.Reset();
    Call(&(init_txn_operation));
    return &result_;
}

bool TransactionExecution::IsFinished()
{
    return is_transaction_finished_;
}

void TransactionExecution::SetFinished()
{
    is_transaction_finished_ = true;
}

TxnStatus TransactionExecution::GetTxnStatus()
{
    return status_;
}
void TransactionExecution::SetTxnStatus(TxnStatus status)
{
    status_ = status;
}

void TransactionExecution::PrepareAbort()
{
    SetTxnStatus(TxnStatus::kAborting);
}
void TransactionExecution::WaitForAbort()
{
    SetTxnStatus(TxnStatus::kWaitForAborting);
}

bool TransactionExecution::IsAborting()
{
    return status_ == TxnStatus::kAborting;
}

bool TransactionExecution::IsWaitForAborting()
{
    return status_ == TxnStatus::kWaitForAborting;
}
}  // namespace txservice::transaction