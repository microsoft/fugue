#include "transaction/transaction-operation.h"
#include "transaction/transaction-execution.h"

namespace txservice::transaction
{
void TransactionOperation::Invoke(TransactionOperation *operation,
                                  bool run_current_operation)
{
    execution_->Call(operation, run_current_operation);
}

void TransactionOperation::Call()
{
    move_to_next_ = false;
    CallImpl();
}

TransactionOperation* TransactionOperation::Next()
{
    move_to_next_ = true;
    has_next_ = true;
    return NextImpl();
}

TransactionOperation* TransactionOperation::PrepareAbort()
{
    execution_->WaitForAbort();
    has_next_ = false;
    return nullptr;
}

TransactionOperation* TransactionOperation::Abort()
{
    execution_->PrepareAbort();
    has_next_ = false;
    return nullptr;
}

void TransactionOperation::Init(TransactionExecution *execution)
{
    execution_ = execution;
}

void ReadOutsideOperation::Reset(TableName *table_name,
                                 Key *key,
                                 Record *record,
                                 bool is_deleted,
                                 void *callback_deserializer)
{
    table_name_ = table_name;
    key_ = key;
    record_ = record;
    is_deleted_ = is_deleted;
    callback_deserializer_ = callback_deserializer;
    key_read_set_entry_ = execution_->InsertReadSet();
    if (result_of_get_version_list_.result_.size() == 0)
    {
        VersionEntry v1, v2;
        result_of_get_version_list_.result_.push_back(v1);
        result_of_get_version_list_.result_.push_back(v2);
    }
    result_of_get_version_list_.Reset();
    result_of_get_version_list_.result_[0].Reset(
        execution_->GetCurrentRequest()->result_->record_,
        std::move(key_read_set_entry_->entry_->extension_));
    result_of_get_version_list_.result_[1].Reset(
        execution_->GetCurrentRequest()->result_->record_);
}

void ReadOutsideOperation::CallImpl()
{
    execution_->handler_->GetVersionList(
        *table_name_,
        *key_,
        this->execution_->commit_timestamp_local_,
        result_of_get_version_list_,
        this->callback_deserializer_);
}

bool ReadOutsideOperation::IsFinished() const
{
    return result_of_get_version_list_.IsFinished();
}

bool ReadOutsideOperation::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* ReadOutsideOperation::NextImpl()
{
    if (result_of_get_version_list_.IsError()) 
    {
        key_read_set_entry_->entry_->extension_ = std::move(result_of_get_version_list_.result_[0].extension_);
        execution_->ReleaseReadSet();
        execution_->GetCurrentRequest()->result_->SetError();
    }
    else
    {
        InternalRead();
    }
    has_next_ = false;
    return nullptr;
}

VersionEntry *ReadOutsideOperation::PickVisibleVersion(
    std::vector<VersionEntry> &version_list)
{
    assert(version_list.size() == 2);

    VersionEntry *result = nullptr;
    if (version_list[0].version_ > version_list[1].version_)
    {
        result = InternalPickVisibleVersion(version_list[0], version_list[1]);
    }
    else if (version_list[0].version_ < version_list[1].version_)
    {
        result = InternalPickVisibleVersion(version_list[1], version_list[0]);
    }

    return result;
}

VersionEntry *ReadOutsideOperation::InternalPickVisibleVersion(
    VersionEntry &v1, VersionEntry &v2)
{
    assert(v1.version_ > v2.version_);
    VersionEntry *result = nullptr;
    if (v1.end_ts_ != VersionEntry::kDefaultEndTs)
    {
        result = &v1;
    }
    else if (v2.version_ != VersionEntry::kDefaultVersion)
    {
        assert(v2.begin_ts_ != VersionEntry::kDefaultBeginTs);
        result = &v2;
    }
    if (result && (!result->is_deleted_ or result->version_ > 0))
    {
        return result;
    }
    else
    {
        return nullptr;
    }
}

void ReadOutsideOperation::InternalRead()
{
    VersionEntry *visible_version =
            PickVisibleVersion(
                result_of_get_version_list_.result_);

        LocalState::SetKey set_key(table_name_, key_);
        if (visible_version != nullptr)
        {
            bool is_deleted = visible_version->is_deleted_;
            Record *record = visible_version->read_record_;
            key_read_set_entry_->entry_->Reset(
                visible_version->version_,
                visible_version->tx_id_,
                visible_version->begin_ts_,
                visible_version->end_ts_,
                visible_version->is_deleted_,
                visible_version->read_record_,
                std::move(visible_version->extension_));

            key_read_set_entry_->key_->Reset(table_name_, key_);

            if (is_deleted)
            {
                execution_->GetCurrentRequest()->result_->SetDeleted();
            }
            else
            {
                execution_->GetCurrentRequest()->result_->SetRecord(record);
            }
        }
        else
        {
            key_read_set_entry_->entry_->Reset(0,
                                               VersionEntry::kEmptyTxId,
                                               0,
                                               VersionEntry::kMaxTimeStamp,
                                               true,
                                               nullptr,
                                               std::move(result_of_get_version_list_.result_[0].extension_));
            key_read_set_entry_->key_->Reset(table_name_, key_);

            this->execution_->GetCurrentRequest()->result_->SetNull();
        }
}

void Upload::Reset()
{
    key_write_set_ = execution_->GetAllWriteSet();
    size_ = execution_->GetWriteSetSize();
}

void Upload::CallImpl()
{
    if ( execution_->upload_version_entry_operation_vector.size() < size_)
    {
        for (int i =  execution_->upload_version_entry_operation_vector.size(); i < size_; i++)
        {
            std::unique_ptr<UploadVersionEntry> upload_version_entry = std::make_unique<UploadVersionEntry>();
            upload_version_entry->Init(execution_);
            execution_->upload_version_entry_operation_vector.push_back(std::move(upload_version_entry));
        }        
    }
    
    for (int i = 0; i < size_; i++)
    {
        execution_->upload_version_entry_operation_vector[i]->Reset(
            (*key_write_set_)[i]->entry_.get(),
            (*key_write_set_)[i]->key_.get());
        Invoke(execution_->upload_version_entry_operation_vector[i].get());
    }
}

TransactionOperation* Upload::NextImpl()
{
    if (execution_->IsWaitForAborting())
    {
        return Abort();
    }
    else
    {
        execution_->set_commit_ts_operation.Reset();
        return &(execution_->set_commit_ts_operation);
    }
}

bool Upload::IsFinished() const
{
    for (int i = 0; i < size_; i++)
    {
        if (!execution_->upload_version_entry_operation_vector[i]->IsCascadeFinished())
        {
            return false;
        }
    }
    return true;
}

bool Upload::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_; 
}

void UploadVersionEntry::Reset(WriteSetEntry *write_set_entry,
                               const LocalState::SetKey *set_key)
{
    write_set_entry_ = write_set_entry;
    set_key_ = set_key;
    result_of_upload_version_.Reset();
    version_entry_.Reset(write_set_entry_->version_,
                         execution_->txn_id_,
                         VersionEntry::kDefaultBeginTs,
                         VersionEntry::kDefaultEndTs,
                         0,
                         write_set_entry_->is_deleted_,
                         nullptr,
                         nullptr,
                         std::move(write_set_entry_->extension_));
}

void UploadVersionEntry::CallImpl()
{
    execution_->handler_->UploadVersion(
        *(set_key_->table_name),
        *(set_key_->key),
        version_entry_,
        write_set_entry_->read_entry_->extension_.get(),
        result_of_upload_version_);
}

bool UploadVersionEntry::IsFinished() const
{
    return result_of_upload_version_.IsFinished();
}

bool UploadVersionEntry::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* UploadVersionEntry::NextImpl()
{
    write_set_entry_->extension_ = std::move(version_entry_.extension_);
    if (result_of_upload_version_.IsError())
    {
        return PrepareAbort();
    }
    else
    {
        write_set_entry_->need_post_processing_ = true;
        int64_t max_commit_ts = result_of_upload_version_.result_;
        execution_->SetMaxCommitTsOfWriters(max_commit_ts);
    }
    has_next_ = false;
    return nullptr;
}

void SetCommitTsOperation::Reset()
{
    result_of_set_commit_ts_.Reset();
}

void SetCommitTsOperation::CallImpl()
{
    execution_->ResetTime();

    int64_t proposed_commit_ts =
        std::max(execution_->GetMaxCommitTsOfWriters() + 1,
                 execution_->commit_timestamp_local_);

    const std::vector<LocalState::KeyReadSetEntry::Pointer> *key_read_set_ =
        execution_->GetAllReadSet();
    size_t size_ = execution_->GetReadSetSize();

    for (size_t i = 0; i < size_; i++)
    {
        ReadSetEntry *entry = (*key_read_set_)[i]->entry_.get();
        if (execution_->FindInWriteSet(*((*key_read_set_)[i]->key_->table_name),
                                       *((*key_read_set_)[i]->key_->key)) !=
                                       nullptr)
        {
            proposed_commit_ts =
                std::max(proposed_commit_ts, entry->begin_ts_ + 1);
        }
        else
        {
            proposed_commit_ts = std::max(proposed_commit_ts, entry->begin_ts_);
        }
    }
    execution_->SetCommitTs(proposed_commit_ts);

    execution_->handler_->SetCommitTimestamp(
        execution_->txn_id_,
        proposed_commit_ts,
        execution_->txn_entry_.extension_.get(),
        result_of_set_commit_ts_);
}

bool SetCommitTsOperation::IsFinished() const
{
    return result_of_set_commit_ts_.IsFinished();
}

bool SetCommitTsOperation::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* SetCommitTsOperation::NextImpl()
{
    if (result_of_set_commit_ts_.IsError())
    {
        return Abort();
    }
    else
    {
        int64_t commit_time = result_of_set_commit_ts_.result_;
        if (commit_time < 0)
        {
            return Abort();
        }
        else
        {
            execution_->SetCommitTs(commit_time);
            execution_->validate_operation.Reset();
            return &(execution_->validate_operation);
        }
    }
}

void Validate::Reset()
{
    size_t size = execution_->GetReadSetSize();
    key_read_set_ = execution_->GetAllReadSet();
    read_index_.clear();
    for (int i = 0; i < size; i++)
    {
         if (!((*key_read_set_)[i]->entry_->is_updated_))
        {
            read_index_.push_back(i);
        }
    }
}

void Validate::CallImpl()
{
    if (execution_->update_read_entry_max_commit_ts_operation_vector.size() < read_index_.size())
    {
        for (int i = execution_->update_read_entry_max_commit_ts_operation_vector.size(); i < read_index_.size(); i++)
        {
            std::unique_ptr<UpdateReadEntryMaxCommitTs> update_read_entry_max_commit_ts = std::make_unique<UpdateReadEntryMaxCommitTs>();
            update_read_entry_max_commit_ts->Init(execution_);
            execution_->update_read_entry_max_commit_ts_operation_vector.push_back(std::move(update_read_entry_max_commit_ts));
        }        
    }

    if (execution_->push_conflict_txn_commit_ts_lower_bound_operation_vector.size() < read_index_.size())
    {
        for (int i = execution_->push_conflict_txn_commit_ts_lower_bound_operation_vector.size(); i < read_index_.size(); i++)
        {
            std::unique_ptr<PushConflictTxnCommitTsLowerBound> push_conflict_txn_commit_ts_lower_bound = std::make_unique<PushConflictTxnCommitTsLowerBound>();;
            push_conflict_txn_commit_ts_lower_bound->Init(execution_);
            execution_->push_conflict_txn_commit_ts_lower_bound_operation_vector.push_back(std::move(push_conflict_txn_commit_ts_lower_bound));
        }        
    }
    
    for (int i = 0; i < read_index_.size(); i++)
    {
        size_t index = read_index_[i];
        execution_->update_read_entry_max_commit_ts_operation_vector[i]->Reset(
            (*key_read_set_)[index]->entry_.get(),
            (*key_read_set_)[index]->key_.get(),
            i);
        Invoke(execution_->update_read_entry_max_commit_ts_operation_vector[i].get());
    }
}

TransactionOperation* Validate::NextImpl()
{
    if (execution_->IsWaitForAborting())
    {
        return Abort();
    }
    else
    {
        execution_->write_to_log_operation.Reset();
        return &(execution_->write_to_log_operation);
    }
}

bool Validate::IsFinished() const
{
    for (int i = 0; i < read_index_.size(); i++)
    {
        if (!execution_->update_read_entry_max_commit_ts_operation_vector[i]->IsCascadeFinished())
        {
            return false;
        }
    }
    return true;
}

bool Validate::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_; 
}

void UpdateReadEntryMaxCommitTs::Reset(ReadSetEntry *read_set_entry,
                                       const LocalState::SetKey *set_key,
                                       size_t index)
{
    read_set_entry_ = read_set_entry;
    set_key_ = set_key;
    result_of_update_max_commit_ts_.Reset();
    index_ = index;
}

void UpdateReadEntryMaxCommitTs::CallImpl()
{
    execution_->handler_->UpdateMaxCommitTsAndReread(
        *(set_key_->table_name),
        *(set_key_->key),
        read_set_entry_->version_,
        execution_->GetCommitTs(),
        read_set_entry_->extension_.get(),
        result_of_update_max_commit_ts_);
}

bool UpdateReadEntryMaxCommitTs::IsFinished() const
{
    return result_of_update_max_commit_ts_.IsFinished();
}

bool UpdateReadEntryMaxCommitTs::IsCascadeFinished() const
{
    if(!IsFinished() || !move_to_next_)
    {
        return false;
    }

    if(has_next_)
    {
        return execution_->push_conflict_txn_commit_ts_lower_bound_operation_vector[index_]->IsCascadeFinished();
    }
    
    return true;
}

TransactionOperation* UpdateReadEntryMaxCommitTs::NextImpl()
{
    if (result_of_update_max_commit_ts_.IsError())
    {
        return PrepareAbort();
    }
    else
    {
        read_set_entry_->need_release_ = false;
        VersionEntry &version_entry = result_of_update_max_commit_ts_.result_;
        if (version_entry.version_ == VersionEntry::kDefaultVersion)
        {
            return PrepareAbort();
        }
        assert(version_entry.max_commit_ts_ >= execution_->GetCommitTs());
        // Check whether the read version entry is locked by another txn.
        if (execution_->GetCommitTs() > version_entry.end_ts_)
        {
            return PrepareAbort();
        }
        else if (version_entry.tx_id_ != VersionEntry::kEmptyTxId)
        {
            execution_->push_conflict_txn_commit_ts_lower_bound_operation_vector[index_]
                ->Reset(version_entry.tx_id_);
            return execution_->push_conflict_txn_commit_ts_lower_bound_operation_vector[index_].get();
        }
    }
    has_next_ = false;
    return nullptr;
}

void PushConflictTxnCommitTsLowerBound::Reset(int64_t txn_id)
{
    result_of_update_commit_lower_bound_.Reset();
    txn_id_ = txn_id;
}

void PushConflictTxnCommitTsLowerBound::CallImpl()
{
    execution_->handler_->UpdateCommitLowerBound(
                txn_id_,
                execution_->GetCommitTs() + 1,
                result_of_update_commit_lower_bound_);
}

TransactionOperation* PushConflictTxnCommitTsLowerBound::NextImpl()
{
    if (result_of_update_commit_lower_bound_.IsError())
    {
        return PrepareAbort();
    }
    else
    {
        TxnEntry &txn_entry = result_of_update_commit_lower_bound_.result_;

        if (txn_entry.status == TxnStatus::kCommitted &&
                txn_entry.commit_ts <= execution_->GetCommitTs() ||
            txn_entry.status == TxnStatus::kOngoing &&
                (txn_entry.commit_ts != TxnEntry::kDefaultCommitTs &&
                 txn_entry.commit_ts <= execution_->GetCommitTs()))
        {
            return PrepareAbort();
        }
    }
    has_next_ = false;
    return nullptr;
}

bool PushConflictTxnCommitTsLowerBound::IsFinished() const
{
    return result_of_update_commit_lower_bound_.IsFinished();
}

bool PushConflictTxnCommitTsLowerBound::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

WriteToLog::WriteToLog()
{
}

WriteToLog::WriteToLog(const WriteToLog &that){
    is_finished_.store(that.is_finished_.load());
}

void WriteToLog::Reset()
{
    is_finished_.store(false);
}

void WriteToLog::CallImpl()
{
    if (execution_->GetWriteSetSize() == 0 || !Constant::ENABLE_LOG ||
        !execution_->EnableLog())
    {
        is_finished_.store(true);
    }
    else
    {
        execution_->txn_entry_.commit_ts = execution_->GetCommitTs();
        execution_->WriteLog(
            execution_->GetCommitTs(),
            execution_->GetAllWriteSet(),
            execution_->GetWriteSetSize(),
            &(execution_->txn_entry_),
            &(is_finished_),
            true);
    };
}

bool WriteToLog::IsFinished() const
{
    return is_finished_.load();
}

bool WriteToLog::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation*  WriteToLog::NextImpl()
{
    execution_->update_txn_status_to_commit_operation.Reset();
    return &(execution_->update_txn_status_to_commit_operation);
}

void UpdateTxnStatusToCommit::Reset()
{
    result_of_update_txn_status_to_commit_.Reset();
}

void UpdateTxnStatusToCommit::CallImpl()
{
    execution_->handler_->UpdateTxnStatus(
        execution_->txn_id_,
        TxnStatus::kCommitted,
        execution_->txn_entry_.extension_.get(),
        result_of_update_txn_status_to_commit_);
}

bool UpdateTxnStatusToCommit::IsFinished() const
{
    return result_of_update_txn_status_to_commit_.IsFinished();
}

bool UpdateTxnStatusToCommit::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* UpdateTxnStatusToCommit::NextImpl()
{
    if (result_of_update_txn_status_to_commit_.IsError())
    {
        execution_->Recover("Update Txn Commit Fail!");
        has_next_ = false;
        return nullptr;
    }
    else
    {
        execution_->GetCurrentRequest()->result_->SetStatus(
            TxnStatus::kCommitted);
        execution_->post_processing_after_commit_operation.Reset();
        return (&(execution_->post_processing_after_commit_operation));
    }
}

void PostProcessingAfterCommit::Reset()
{
    key_write_set_ = execution_->GetAllWriteSet();
    size_t size = execution_->GetWriteSetSize();
    post_process_index_.clear();

    for (int i = 0; i < size; i++)
    {
        if ((*key_write_set_)[i]->entry_->need_post_processing_)
        {
            post_process_index_.push_back(i);
        }
    }
}

void PostProcessingAfterCommit::CallImpl()
{
    if (execution_->post_processing_commit_entry_after_commit_operation_vector.size() < post_process_index_.size())
    {
        for (int i = execution_->post_processing_commit_entry_after_commit_operation_vector.size(); i < post_process_index_.size(); i++)
        {
            std::unique_ptr<PostProcessingCommitEntryAfterCommit> post_processing_commit_entry_after_commit = std::make_unique<PostProcessingCommitEntryAfterCommit>();
            post_processing_commit_entry_after_commit->Init(execution_);
            execution_->post_processing_commit_entry_after_commit_operation_vector.push_back(std::move(post_processing_commit_entry_after_commit));
        }        
    }
    
    for (int i = 0; i < post_process_index_.size(); i++)
    {
        size_t index = post_process_index_[i];
        execution_->post_processing_commit_entry_after_commit_operation_vector[i]->Reset(
            (*key_write_set_)[index]->entry_.get(),
            (*key_write_set_)[index]->key_.get());
        Invoke(execution_->post_processing_commit_entry_after_commit_operation_vector[i].get());
    }
}

TransactionOperation*  PostProcessingAfterCommit::NextImpl()
{
    execution_->SetTxnStatus(TxnStatus::kCommitted);
    execution_->SetFinished();
    has_next_ = false;
    return nullptr;
}

bool PostProcessingAfterCommit::IsFinished() const
{
    for (int i = 0; i < post_process_index_.size(); i++)
    {
        if (!execution_->post_processing_commit_entry_after_commit_operation_vector[i]->IsCascadeFinished())
        {
            return false;
        }
    }
    return true;
}

bool PostProcessingAfterCommit::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_; 
}

void PostProcessingCommitEntryAfterCommit::Reset(
    WriteSetEntry *entry, const LocalState::SetKey *set_key)
{
    entry_ = entry;
    set_key_ = set_key;
    result_of_commit_version_in_post_processing_commit_.Reset();
    result_of_commit_version_in_post_processing_commit_.ref_cnt = 2;
}

void PostProcessingCommitEntryAfterCommit::CallImpl()
{
    execution_->handler_->CommitVersion(
        *(set_key_->table_name),
        *(set_key_->key),
        entry_->version_,
        execution_->txn_id_,
        execution_->GetCommitTs(),
        VersionEntry::kMaxTimeStamp,
        VersionEntry::kEmptyTxId,
        entry_->extension_.get(),
        result_of_commit_version_in_post_processing_commit_,
        entry_->record_);
}

bool PostProcessingCommitEntryAfterCommit::IsFinished() const
{
    return result_of_commit_version_in_post_processing_commit_.ref_cnt == 0;
}

bool PostProcessingCommitEntryAfterCommit::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* PostProcessingCommitEntryAfterCommit::NextImpl()
{
    if (result_of_commit_version_in_post_processing_commit_.IsError())
    {
        execution_->Recover("Replace Entry Commit Fail!");
    }
    else
    {
        entry_->read_entry_->need_release_ = false;
    }
    has_next_ = false;
    return nullptr;
}

void PostProcessingAfterAbort::Reset()
{
    key_write_set_ = execution_->GetAllWriteSet();
    size_t size = execution_->GetWriteSetSize();
    post_process_index_.clear();

    for (int i = 0; i < size; i++)
    {
         if ((*key_write_set_)[i]->entry_->need_post_processing_)
        {
            post_process_index_.push_back(i);
        }
    }
}

void PostProcessingAfterAbort::CallImpl()
{
    if (execution_->post_processing_delete_entry_after_abort_operation_vector.size() < post_process_index_.size())
    {
        for (int i = execution_->post_processing_delete_entry_after_abort_operation_vector.size(); i < post_process_index_.size(); i++)
        {
            std::unique_ptr<PostProcessingDeleteEntryAfterAbort> post_processing_delete_entry_after_abort =
                std::make_unique<PostProcessingDeleteEntryAfterAbort>();
            post_processing_delete_entry_after_abort->Init(execution_);
            execution_->post_processing_delete_entry_after_abort_operation_vector.push_back(
                std::move(post_processing_delete_entry_after_abort));
        }        
    }
    
    for (int i = 0; i < post_process_index_.size(); i++)
    {
        size_t index = post_process_index_[i];
        execution_->post_processing_delete_entry_after_abort_operation_vector[i]->Reset(
            (*key_write_set_)[index]->entry_.get(),
            (*key_write_set_)[index]->key_.get());
        Invoke(execution_->post_processing_delete_entry_after_abort_operation_vector[i].get());
    }
}

TransactionOperation* PostProcessingAfterAbort::NextImpl()
{
    execution_->release_read_counter_operation.Reset();
    return &(execution_->release_read_counter_operation);
}

bool PostProcessingAfterAbort::IsFinished() const
{
    for (int i = 0; i < post_process_index_.size(); i++)
    {
        if (!execution_->post_processing_delete_entry_after_abort_operation_vector[i]->IsCascadeFinished())
        {
            return false;
        }
    }
    return true;
}

bool PostProcessingAfterAbort::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_; 
}

void PostProcessingDeleteEntryAfterAbort::Reset(
    const WriteSetEntry *entry, const LocalState::SetKey *set_key)
{
    entry_ = entry;
    set_key_ = set_key;
    result_of_delete_version_in_post_processing_abort_.Reset();
}

void PostProcessingDeleteEntryAfterAbort::CallImpl()
{
    execution_->handler_->DeleteVersion(
        *(set_key_->table_name),
        *(set_key_->key),
        entry_->version_,
        entry_->extension_.get(),
        result_of_delete_version_in_post_processing_abort_);
}

bool PostProcessingDeleteEntryAfterAbort::IsFinished() const
{
    return result_of_delete_version_in_post_processing_abort_.IsFinished();
}

bool PostProcessingDeleteEntryAfterAbort::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* PostProcessingDeleteEntryAfterAbort::NextImpl()
{
    if (result_of_delete_version_in_post_processing_abort_.IsError())
    {
        execution_->Recover("Delete Version Abort Fail!");
    }
    else if (entry_->read_entry_!= nullptr && entry_->read_entry_->need_post_processing_)
    {
        entry_->read_entry_->need_release_ = false;
    }

    has_next_ = false;
    return nullptr;
    
}

void ReleaseReadCounter::Reset()
{
    key_read_set_ = execution_->GetAllReadSet();
    size_t size = execution_->GetReadSetSize();
    release_index_.clear();
    for (int i = 0; i < size; i++)
    {
         if ((*key_read_set_)[i]->entry_->need_release_)
        {
            release_index_.push_back(i);
        }
    }
}

void ReleaseReadCounter::CallImpl()
{
    if (execution_->release_read_counter_for_each_entry_operation_vector.size() < release_index_.size())
    {
        for (int i = execution_->release_read_counter_for_each_entry_operation_vector.size(); i < release_index_.size(); i++)
        {
            std::unique_ptr<ReleaseReadCounterForEachEntry> release_read_counter_for_each_entry = 
                std::make_unique<ReleaseReadCounterForEachEntry>();
            release_read_counter_for_each_entry->Init(execution_);
            execution_->release_read_counter_for_each_entry_operation_vector.push_back(
                std::move(release_read_counter_for_each_entry));
        }        
    }

    for (int i = 0; i < release_index_.size(); i++)
    {
        size_t index = release_index_[i];
        execution_->release_read_counter_for_each_entry_operation_vector[i]->Reset(
            (*key_read_set_)[index]->entry_.get(),
            (*key_read_set_)[index]->key_.get());
        Invoke(execution_->release_read_counter_for_each_entry_operation_vector[i].get());
    }
}

TransactionOperation* ReleaseReadCounter::NextImpl()
{
    execution_->SetTxnStatus(TxnStatus::kAborted);
    execution_->SetFinished();
    has_next_ = false;
    return nullptr;
}

bool ReleaseReadCounter::IsFinished() const
{
    for (int i = 0; i < release_index_.size(); i++)
    {
        if (!execution_->release_read_counter_for_each_entry_operation_vector[i]->IsCascadeFinished())
        {
            return false;
        }
    }
    return true;
}

bool ReleaseReadCounter::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_; 
}

void ReleaseReadCounterForEachEntry::Reset(ReadSetEntry *read_set_entry,
                                           const LocalState::SetKey *set_key)
{
    read_set_entry_ = read_set_entry;
    set_key_ = set_key;
    result_of_release_read_counter_.Reset();
}

void ReleaseReadCounterForEachEntry::CallImpl()
{
    execution_->handler_->ReleaseReadCounter(
        *(set_key_->table_name),
        *(set_key_->key),
        read_set_entry_->version_,
        read_set_entry_->extension_.get(),
        result_of_release_read_counter_);;
}

bool ReleaseReadCounterForEachEntry::IsFinished() const
{
    return result_of_release_read_counter_.IsFinished();
}

bool ReleaseReadCounterForEachEntry::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* ReleaseReadCounterForEachEntry::NextImpl()
{
    if (result_of_release_read_counter_.IsError())
    {
        execution_->Recover("Release Counter Fail!");
        has_next_ = false;
        return nullptr;
    }
    else
    {
        read_set_entry_->need_release_ = false;
        has_next_ = false;
        return nullptr;
    }
}

void UpdateTxnStatusToAbort::Reset()
{
    result_of_update_txn_status_to_abort_.Reset();
}

void UpdateTxnStatusToAbort::CallImpl()
{
    execution_->handler_->UpdateTxnStatus(
        execution_->txn_id_,
        TxnStatus::kAborted,
        execution_->txn_entry_.extension_.get(),
        result_of_update_txn_status_to_abort_);
}

bool UpdateTxnStatusToAbort::IsFinished() const
{
    return result_of_update_txn_status_to_abort_.IsFinished();
}

bool UpdateTxnStatusToAbort::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* UpdateTxnStatusToAbort::NextImpl()
{
    if (result_of_update_txn_status_to_abort_.IsError())
    {
        execution_->Recover("Update Status Abort Fail!");
        has_next_ = false;
        return nullptr;
    }
    else
    {
        execution_->GetCurrentRequest()->result_->SetStatus(
            TxnStatus::kAborted);
        execution_->post_processing_after_abort_operation.Reset();
        return &(execution_->post_processing_after_abort_operation);
    }
}

void InsertOperation::Reset(TableName *table_name,
                            Key *key,
                            Record *record,
                            void *callback_deserializer)
{
    table_name_ = table_name;
    key_ = key;
    record_ = record;
    callback_deserializer_ = callback_deserializer;
}

void InsertOperation::CallImpl()
{
    execution_->read_outside_operation.Reset(table_name_,
                                             key_,
                                             record_,
                                             true,
                                             callback_deserializer_);
    Invoke(&(execution_->read_outside_operation));
}

bool InsertOperation::IsFinished() const
{
    return execution_->read_outside_operation.IsCascadeFinished();
}

bool InsertOperation::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* InsertOperation::NextImpl()
{
    ReadSetEntry *read_entry = execution_->FindInReadSet(*table_name_, *key_);

    if (read_entry != nullptr && read_entry->is_deleted_)
    {
        int64_t version_key = read_entry->version_ + 1;
        read_entry->is_updated_ = true;
        execution_->InsertWriteSet(
            table_name_, key_, version_key, false, record_, read_entry);

        execution_->GetCurrentRequest()->result_->SetFinished();
    }
    else
    {
        execution_->GetCurrentRequest()->result_->SetError();
    }
    has_next_ = false;
    return nullptr;
}

void UpsertOperation::Reset(TableName *table_name,
                            Key *key,
                            Record *record,
                            void *callback_deserializer)
{
    table_name_ = table_name;
    key_ = key;
    record_ = record;
    callback_deserializer_ = callback_deserializer;
}

void UpsertOperation::CallImpl()
{
    execution_->read_outside_operation.Reset(table_name_,
                                             key_,
                                             record_,
                                             true,
                                             callback_deserializer_);
    Invoke(&(execution_->read_outside_operation));
}

bool UpsertOperation::IsFinished() const
{
    return execution_->read_outside_operation.IsCascadeFinished();
}

bool UpsertOperation::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* UpsertOperation::NextImpl()
{
    ReadSetEntry *read_entry = execution_->FindInReadSet(*table_name_, *key_);
    if (read_entry != nullptr)
    {
        int64_t version_key = read_entry->version_ + 1;
        read_entry->is_updated_ = true;
        execution_->InsertWriteSet(
            table_name_, key_, version_key, false, record_, read_entry);

        execution_->GetCurrentRequest()->result_->SetFinished();
    }
    has_next_ = false;
    return nullptr;
}

void UpdateOperation::Reset(TableName *table_name, Key *key, Record *record)
{
    table_name_ = table_name;
    key_ = key;
    record_ = record;
}

void UpdateOperation::CallImpl()
{
    LocalState::SetKey set_key(table_name_, std::move(key_));

    ReadSetEntry *read_entry = execution_->FindInReadSet(*table_name_, *key_);

    if (read_entry != nullptr)
    {
        if (read_entry->is_deleted_ && read_entry->version_ > 0)
        {
            execution_->GetCurrentRequest()->result_->SetError();
        }
        else
        {
            int64_t version_key = read_entry->version_ + 1;
            read_entry->is_updated_ = true;

            execution_->InsertWriteSet(
                table_name_, key_, version_key, false, record_, read_entry);

            execution_->GetCurrentRequest()->result_->SetFinished();
        }
    }
    else
    {
        execution_->GetCurrentRequest()->result_->SetError();
    }
}

bool UpdateOperation::IsFinished() const
{
    return true;
}

bool UpdateOperation::IsCascadeFinished() const
{
    return true;
}

TransactionOperation* UpdateOperation::NextImpl()
{
    has_next_ = false;
    return nullptr;
}

void DeleteOperation::Reset(TableName *table_name, Key *key)
{
    table_name_ = table_name;
    key_ = key;
}

void DeleteOperation::CallImpl()
{
    LocalState::SetKey set_key(table_name_, std::move(key_));
    WriteSetEntry *write_entry =
        execution_->FindInWriteSet(*table_name_, *key_);
    ReadSetEntry *read_entry = execution_->FindInReadSet(*table_name_, *key_);

    if (write_entry != nullptr)
    {
        if (write_entry->is_deleted_)
        {
            execution_->GetCurrentRequest()->result_->SetError();
        }
        else
        {
            write_entry->is_deleted_ = true;
            write_entry->record_ = nullptr;
            execution_->GetCurrentRequest()->result_->SetFinished();
        }
    }
    else if (read_entry != nullptr)
    {
        if (read_entry->is_deleted_)
        {
            execution_->GetCurrentRequest()->result_->SetError();
        }
        else
        {
            int64_t version_key = read_entry->version_ + 1;
            read_entry->is_updated_ = true;

            execution_->InsertWriteSet(
                table_name_, key_, version_key, true, nullptr, read_entry);

            execution_->GetCurrentRequest()->result_->SetFinished();
        }
    }
    else
    {
        execution_->GetCurrentRequest()->result_->SetError();
    }
}

bool DeleteOperation::IsFinished() const
{
    return true;
}

bool DeleteOperation::IsCascadeFinished() const
{
    return true;
}
TransactionOperation* DeleteOperation::NextImpl()
{
    has_next_ = false;
    return nullptr;
}

void InitTxnOperation::Reset()
{
    result_of_new_txn_.Reset();
}

void InitTxnOperation::CallImpl()
{
    execution_->handler_->NewTxn(execution_->txn_entry_,
        execution_->commit_timestamp_local_,
        execution_->kMaxTxnExecutionTimeMS,
        result_of_new_txn_);
}

bool InitTxnOperation::IsFinished() const
{
    return result_of_new_txn_.IsFinished();
}

bool InitTxnOperation::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}

TransactionOperation* InitTxnOperation::NextImpl()
{
    if (result_of_new_txn_.IsError())
    {
        execution_->ResetTxnIDAndTime();
        execution_->init_txn_operation.Reset();
        return (&(execution_->init_txn_operation));
    }
    else
    {
        execution_->txn_id_ = execution_->txn_entry_.tx_id;
        execution_->GetCurrentRequest()->result_->SetFinished();
        has_next_ = false;
        return nullptr;
    }
}

void ReadDataStoreOperation::Reset(TableName *table_name, Key *key, Record *record, DataStore *driver)
{
    table_name_ = table_name;
    key_ = key;
    record_ = record;
    driver_ = driver;
    result_of_read_ds_.Reset();
    result_of_read_ds_.result_ = record;
}

void ReadDataStoreOperation::CallImpl()
{
    driver_->ReadRecord(*table_name_, key_, result_of_read_ds_);
}

bool ReadDataStoreOperation::IsFinished() const
{
    return result_of_read_ds_.IsFinished();
}

bool ReadDataStoreOperation::IsCascadeFinished() const
{
    return IsFinished() && move_to_next_;
}
TransactionOperation* ReadDataStoreOperation::NextImpl()
{
    if (result_of_read_ds_.IsError())
    {
        execution_->GetCurrentRequest()->result_->SetError();
    }
    else
    {
        if (result_of_read_ds_.result_ == nullptr)
        {
            execution_->GetCurrentRequest()->result_->SetNull();
        }
        else
        {
            execution_->GetCurrentRequest()->result_->SetRecord(
                result_of_read_ds_.result_);
        }
    }
    has_next_ = false;
    return nullptr;
}
}  // namespace txservice::transaction