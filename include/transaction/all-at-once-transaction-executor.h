#ifndef TXSERVICE_TRANSACTION_ALL_AT_ONCE_TRANSACTION_EXECUTOR_H_
#define TXSERVICE_TRANSACTION_ALL_AT_ONCE_TRANSACTION_EXECUTOR_H_

#include "transaction/time-provider.h"
#include "transaction/transaction-execution.h"
#include "transaction/transaction-executor.h"
#include "transaction/transaction-request.h"
#include "transaction/txn-id-generator.h"
#include "transaction/transaction-task.h"
namespace txservice::transaction
{
class AllAtOnceTransactionExecutor : public TransactionExecutor
{
public:
    using Pointer = std::unique_ptr<AllAtOnceTransactionExecutor>;
    AllAtOnceTransactionExecutor(int executor_id,
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
    virtual void ShutDown() override;
    virtual void Statistics(int &commit, int &abort) override;

public:
    uint32_t concurrent_txn_count_;
    std::vector<TransactionTask> active_txn_;
    int active_txn_number_;
    std::vector<std::shared_ptr<OperationRequest>>
        request_queue_pool_;
    size_t push_index_;
    size_t pop_index_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_ALL_AT_ONCE_TRANSACTION_EXECUTOR_H_
