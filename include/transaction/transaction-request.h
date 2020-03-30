#ifndef TXSERVICE_TRANSACTION_TRANSACTION_REQUEST_H_
#define TXSERVICE_TRANSACTION_TRANSACTION_REQUEST_H_

#include <memory>
#include "transaction/operation-request.h"

namespace txservice::transaction
{
class TransactionRequest
{
public:
    using Pointer = std::unique_ptr<TransactionRequest>;
    TransactionRequest();
    TransactionRequest(const TransactionRequest &that);
    void PushRequest(std::shared_ptr<OperationRequest> operation_request);
    std::shared_ptr<OperationRequest> CurrentRequest();
    void Reset();

    std::vector<std::shared_ptr<OperationRequest>> operation_request_queue_;
    int current_ = 0;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_TRANSACTION_REQUEST_H_