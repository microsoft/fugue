#ifndef TXSERVICE_TRANSACTION_TRANSACTION_TASK_H_
#define TXSERVICE_TRANSACTION_TRANSACTION_TASK_H_

#include "transaction/transaction-execution.h"
#include "transaction/transaction-request.h"

namespace txservice::transaction
{
class TransactionTask
{
public:
    TransactionTask()
        : transaction_execution_(),
          transaction_request_(),
          in_use_(false),
          session_id_(0)
    {
    }

    TransactionTask(TransactionExecution transaction_execution,
                    TransactionRequest transaction_request)
        : transaction_execution_(transaction_execution),
          transaction_request_(transaction_request),
          in_use_(false),
          session_id_(0)
    {
    }

    TransactionExecution *GetTransactionExecution()
    {
        return &transaction_execution_;
    }

    TransactionRequest *GetTransactionRequest()
    {
        return &transaction_request_;
    }

    void Reset(int64_t session_id)
    {
        transaction_execution_.Reset();
        transaction_request_.Reset();
        in_use_ = true;
        session_id_ = session_id;
    }

    inline void Release()
    {
        in_use_ = false;
    }

    inline bool InUse()
    {
        return in_use_;
    }

    inline int64_t GetSessionID()
    {
        return session_id_;
    }

private:
    TransactionExecution transaction_execution_;
    TransactionRequest transaction_request_;
    bool in_use_;
    int64_t session_id_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_TRANSACTION_TASK_H_