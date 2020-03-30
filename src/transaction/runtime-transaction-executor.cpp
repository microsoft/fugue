#include "transaction/runtime-transaction-executor.h"

namespace txservice::transaction
{
RuntimeTransactionExecutor::RuntimeTransactionExecutor(
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
      current_request_(nullptr)
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

void RuntimeTransactionExecutor::AddRequest(
    std::shared_ptr<OperationRequest> operation_request)
{
    request_queue_pool_.write(operation_request);
}

// transaction operation request must be next to each other.
void RuntimeTransactionExecutor::LaunchRequests()
{
    size_t last_index = -1;
    while (true)
    {
        if (current_request_ == nullptr)
        {
            if (!request_queue_pool_.read(current_request_))
            {
                break;
            }
        }
        int64_t session_id = current_request_->session_id_;
        int concurrent_txn_index;

        if (current_request_->operation_type_ == OperationType::Begin)
        {
            // find an empty slot if we are beginning a new transaction
            if (active_txn_number_ >= concurrent_txn_count_)
            {
                return;
            }
            for (concurrent_txn_index = 0;
                 concurrent_txn_index < concurrent_txn_count_;
                 concurrent_txn_index++)
            {
                if (!active_txn_[concurrent_txn_index].InUse())
                {
                    active_txn_[concurrent_txn_index].Reset(session_id);
                    active_txn_number_++;
                    last_index = concurrent_txn_index;
                    break;
                }
            }
            if (concurrent_txn_index == concurrent_txn_count_)
            {
                throw "no empty txn task";
            }
        }
        else
        {
            for (concurrent_txn_index = 0;
                 concurrent_txn_index < concurrent_txn_count_;
                 concurrent_txn_index++)
            {
                if (active_txn_[concurrent_txn_index].GetSessionID() ==
                    session_id)
                {
                    last_index = concurrent_txn_index;
                    break;
                }
            }
            if (concurrent_txn_index == concurrent_txn_count_)
            {
                throw "no session task";
            }
        }

        if (last_index == -1)
        {
            throw "not found suitable transaction";
        }

        active_txn_[last_index].GetTransactionRequest()->PushRequest(
            current_request_);
        current_request_ = nullptr;
    }
}

void RuntimeTransactionExecutor::Advance()
{
    bool has_finished_txn = false;

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
                auto operation_request = transaction_request->CurrentRequest();

                if (operation_request != nullptr)
                {
                    Result *result = nullptr;
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
                        execution->Read(&(operation_request->table_name_),
                                        operation_request->key_.get(),
                                        operation_request->record_.get(),
                            operation_request->callback_deserializer_);

                        break;
                    }
                    case OperationType::ReadOutside:
                    {
                        execution->Read(&(operation_request->table_name_),
                                        operation_request->key_.get(),
                                        operation_request->record_.get(),
                                        false, operation_request->callback_deserializer_);
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
                if (execution->GetTxnStatus() == txservice::TxnStatus::kAborted)
                {
                    abort_count_++;
                }
                if (execution->GetTxnStatus() == txservice::TxnStatus::kCommitted)
                {
                    commit_count_++;
                }

                active_txn_[i].Release();
                active_txn_number_--;
                has_finished_txn = true;
            }
        }
    }
}

void RuntimeTransactionExecutor::Statistics(int &commit, int &abort)
{
    commit = commit_count_;
    abort = abort_count_;
}

void RuntimeTransactionExecutor::Run()
{
    while (!request_queue_pool_.isEmpty() || active_txn_number_ > 0)
    {
        LaunchRequests();
        Advance();
        handler_->SendBatch();
    }
}

bool RuntimeTransactionExecutor::IsFinished()
{
    return request_queue_pool_.isEmpty() && active_txn_number_ == 0;
}

void RuntimeTransactionExecutor::ShutDown() 
{    
}
}  // namespace txservice::transaction

