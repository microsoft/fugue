#ifndef TXSERVICE_TRANSACTION_RUNTIME_TRANSACTION_EXECUTOR_H_
#define TXSERVICE_TRANSACTION_RUNTIME_TRANSACTION_EXECUTOR_H_

#include <memory>
#include <folly/MPMCQueue.h>
#include "transaction/transaction-executor.h"
#include "transaction/transaction-task.h"

namespace txservice::transaction
{
class RuntimeTransactionExecutor : public TransactionExecutor
{
public:
    RuntimeTransactionExecutor(int executor_id,
                        uint32_t concurrent_txn_count,
                        std::unique_ptr<TxnIDGenerator> txn_id_generator,
                        request::Handler::Pointer handler,
                        std::unique_ptr<TimeProvider> time_provider,
                        txlog::TxLog *tx_log,
                        size_t capacity);
    virtual void Run() override;
    virtual void AddRequest(
        std::shared_ptr<OperationRequest> operation_request) override;
    void LaunchRequests();
    void Advance();
    virtual bool IsFinished() override;
    virtual void Statistics(int &commit, int &abort) override;
    virtual void ShutDown() override;

public:
    uint32_t concurrent_txn_count_;
    std::vector<TransactionTask> active_txn_;
    int active_txn_number_;
    folly::MPMCQueue<std::shared_ptr<OperationRequest>> request_queue_pool_;
    std::shared_ptr<OperationRequest> current_request_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_RUNTIME_TRANSACTION_EXECUTOR_H_