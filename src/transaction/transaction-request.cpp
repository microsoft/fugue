#include "transaction/transaction-request.h"

namespace txservice::transaction
{
TransactionRequest::TransactionRequest()
{
    current_ = 0;
}
TransactionRequest::TransactionRequest(const TransactionRequest &that)
    : operation_request_queue_(that.operation_request_queue_),
      current_(that.current_)
{
}

void TransactionRequest::PushRequest(std::shared_ptr<OperationRequest> operation_request)
{
    operation_request_queue_.push_back(operation_request);
}

std::shared_ptr<OperationRequest> TransactionRequest::CurrentRequest()
{
    if (current_ >= operation_request_queue_.size())
    {
        return nullptr;
    }
    else
    {
        return operation_request_queue_[current_++];
    }
}

void TransactionRequest::Reset()
{
    operation_request_queue_.clear();
    current_ = 0;
}
}  // namespace txservice::transaction