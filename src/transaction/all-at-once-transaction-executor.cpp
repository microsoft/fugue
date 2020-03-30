#include "transaction/all-at-once-transaction-executor.h"

namespace txservice::transaction
{
AllAtOnceTransactionExecutor::AllAtOnceTransactionExecutor(
    int executor_id,
    uint32_t concurrent_txn_count,
    std::unique_ptr<TxnIDGenerator> txn_id_generator,
    request::Handler::Pointer handler,
    std::unique_ptr<TimeProvider> time_provider,
    txlog::TxLog *tx_log,
    size_t capacity)
    : TransactionExecutor(executor_id, 
      std::move(txn_id_generator),
      std::move(handler), 
      std::move(time_provider), 
      tx_log),
      concurrent_txn_count_(concurrent_txn_count),
      request_queue_pool_(capacity),
      push_index_(0),
      pop_index_(0)
{
    active_txn_number_ = 0;
    for (int i = 0; i < concurrent_txn_count; i++)
    {
        TransactionExecution transaction_execution(executor_id,
                                                   handler_.get(),
                                                   txn_id_generator_.get(),
                                                   time_provider_.get(),
                                                   tx_log);
        TransactionRequest transaction_request;
        TransactionTask transaction_task(transaction_execution,
                                         transaction_request);
        active_txn_.push_back(transaction_task);
    }
}

void AllAtOnceTransactionExecutor::AddRequest(
    std::shared_ptr<OperationRequest> operation_request)
{
    request_queue_pool_[push_index_] = operation_request;
    push_index_++;
}

// transaction operation request must be next to each other.
void AllAtOnceTransactionExecutor::LaunchRequests()
{
    size_t last_index = 0;
    while (pop_index_ < push_index_)
    {
        std::shared_ptr<OperationRequest> operation_request =
            request_queue_pool_[pop_index_];

        int64_t session_id = operation_request->session_id_;

        assert(operation_request->operation_type_ == OperationType::Begin);
        if (active_txn_number_ < concurrent_txn_count_)
        {
            bool find_empty_txn_task = false;
            for (; last_index < concurrent_txn_count_; last_index++)
            {
                if (!active_txn_[last_index].InUse())
                {
                    active_txn_[last_index].Reset(session_id);
                    active_txn_[last_index]
                        .GetTransactionRequest()
                        ->PushRequest(operation_request);
                    find_empty_txn_task = true;
                    active_txn_number_++;

                    while (pop_index_ < push_index_)
                    {
                        pop_index_++;
                        std::shared_ptr<OperationRequest> operation_request =
                            request_queue_pool_[pop_index_];

                        int64_t session_id = operation_request->session_id_;
                        assert(active_txn_[last_index].GetSessionID() ==
                               session_id);

                        active_txn_[last_index]
                            .GetTransactionRequest()
                            ->PushRequest(operation_request);
                        if (operation_request->operation_type_ ==
                                OperationType::Commit ||
                            operation_request->operation_type_ ==
                                OperationType::Abort)
                        {
                            break;
                        }
                    }

                    break;
                }
            }
            if (!find_empty_txn_task)
                throw "no empty txn task";

            last_index++;
            pop_index_++;
        }
        else
        {
            break;
        }
    }
}

void AllAtOnceTransactionExecutor::Advance()
{
    bool has_finished_txn = false;

    while (!has_finished_txn)
    {
        for (int i = 0; i < concurrent_txn_count_; i++)
        {
            if (active_txn_[i].InUse())
            {
                TransactionExecution *execution =
                    active_txn_[i].GetTransactionExecution();

                bool is_idle = execution->MoveForward();
                if (is_idle && !execution->IsFinished())
                {
                    TransactionRequest *transaction_request =
                        active_txn_[i].GetTransactionRequest();
                    std::shared_ptr<OperationRequest> operation_request =
                        transaction_request->CurrentRequest();

                    if (operation_request != nullptr)
                    {
                        operation_request->SetUp();
                        execution->SetCurrentRequest(operation_request);
                        switch (operation_request->operation_type_)
                        {
                        case OperationType::Begin:
                            execution->Begin(operation_request->type_);
                            break;
                        case OperationType::Insert:
                            execution->Insert(
                                &(operation_request->table_name_),
                                operation_request->key_.get(),
                                std::move(operation_request->record_.get()),
                                operation_request->callback_deserializer_);
                            break;
                        case OperationType::Update:
                            execution->Update(
                                &(operation_request->table_name_),
                                operation_request->key_.get(),
                                std::move(operation_request->record_.get()));
                            break;
                        case OperationType::Upsert:
                            execution->Upsert(
                                &(operation_request->table_name_),
                                operation_request->key_.get(),
                                std::move(operation_request->record_.get()),
                                operation_request->callback_deserializer_);
                            break;
                        case OperationType::Delete:
                            execution->Delete(&(operation_request->table_name_),
                                              operation_request->key_.get());
                            break;
                        case OperationType::Read:
                        {
                            Result *result = execution->Read(
                                &(operation_request->table_name_),
                                operation_request->key_.get(),
                                operation_request->record_.get(),
                                operation_request->callback_deserializer_);
                            operation_request->SetResult(result);
                            break;
                        }
                        case OperationType::ReadOutside:
                        {
                            Result *result = execution->Read(
                                &(operation_request->table_name_),
                                operation_request->key_.get(),
                                operation_request->record_.get(),
                                false,
                                operation_request->callback_deserializer_);
                            operation_request->SetResult(result);
                            break;
                        }
                        case OperationType::ReadDataStore:
                        {
                            if (this->datastore_driver)
                            {
                                Result *result = execution->ReadDataStore(
                                    &(operation_request->table_name_),
                                    operation_request->key_.get(),
                                    operation_request->record_.get(),
                                    datastore_driver.get(),
                                    operation_request->need_to_read_outside_);
                                operation_request->SetResult(result);
                            }
                            else  // in that case we assume a default record.
                            {
                                execution->result_.Reset(
                                    operation_request->record_.get(),
                                    execution->GetCurrentRequest());
                                operation_request->SetResult(
                                    &execution->result_);
                            }
                            break;
                        }
                        case OperationType::Commit:
                            execution->Commit();
                            break;
                        case OperationType::Abort:
                            execution->Abort();
                            break;
                        default:
                            break;
                        }
                    }
                }

                if (execution->IsFinished())
                {
                    if (execution->GetTxnStatus() ==
                        txservice::TxnStatus::kAborted)
                    {
                        abort_count_++;
                    }
                    if (execution->GetTxnStatus() ==
                        txservice::TxnStatus::kCommitted)
                    {
                        commit_count_++;
                    }
                    active_txn_[i].Release();
                    active_txn_number_--;
                    has_finished_txn = true;
                }
            }
        }
        handler_->SendBatch();
    }
}

void AllAtOnceTransactionExecutor::Statistics(int &commit, int &abort)
{
    commit = commit_count_;
    abort = abort_count_;
}

void AllAtOnceTransactionExecutor::Run()
{
    while (pop_index_ != push_index_ || active_txn_number_ > 0)
    {
        LaunchRequests();
        Advance();
    }
}

bool AllAtOnceTransactionExecutor::IsFinished()
{
    return pop_index_ == push_index_ && active_txn_number_ == 0;
}

void AllAtOnceTransactionExecutor::ShutDown()
{
}
}  // namespace txservice::transaction
