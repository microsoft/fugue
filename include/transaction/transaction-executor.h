#ifndef TXSERVICE_TRANSACTION_TRANSACTION_EXECUTOR_H_
#define TXSERVICE_TRANSACTION_TRANSACTION_EXECUTOR_H_

#include "transaction/transaction-execution.h"
#include "transaction/transaction-request.h"

namespace txservice::transaction
{
class TransactionExecutor
{
public:
    using Pointer = std::unique_ptr<TransactionExecutor>;
    TransactionExecutor(
    int executor_id,
    std::unique_ptr<TxnIDGenerator> txn_id_generator,
    request::Handler::Pointer handler,
    std::unique_ptr<TimeProvider> time_provider,
    txlog::TxLog *tx_log)
    : executor_id_(executor_id),
      txn_id_generator_(std::move(txn_id_generator)),
      handler_(std::move(handler)),
      time_provider_(std::move(time_provider)),
      tx_log_(tx_log){}
    virtual void Run() = 0;
    virtual void AddRequest(
        std::shared_ptr<OperationRequest> operation_request) = 0;
    virtual bool IsFinished() = 0;
    virtual void ShutDown() = 0;
    virtual void Statistics(int &commit, int &abort) = 0;
public:
    std::unique_ptr<DataStore> datastore_driver;
    virtual ~TransactionExecutor() = default;
    request::Handler::Pointer handler_;
    std::unique_ptr<TimeProvider> time_provider_;
    std::unique_ptr<TxnIDGenerator> txn_id_generator_;
    txlog::TxLog *tx_log_;
    int executor_id_;
    int commit_count_ = 0;
    int abort_count_ = 0;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_TRANSACTION_EXECUTOR_H_