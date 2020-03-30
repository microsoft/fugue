#include "transaction/result.h"
#include "transaction/operation-request.h"

namespace txservice::transaction
{
void Result::SetFinished()
{
    operation_request_->Notify();
}

void Result::Reset(Record *record, OperationRequest *operation_request)
{
    record_ = record;
    is_deleted_ = false;
    is_null_ = false;
    is_error_ = false;
    status_ = TxnStatus::kOngoing;
    operation_request_ = operation_request;
    operation_request_->SetResult(this);
}

void Result::Reset(OperationRequest *operation_request)
{
    is_deleted_ = false;
    is_null_ = false;
    is_error_ = false;
    status_ = TxnStatus::kOngoing;
    operation_request_ = operation_request;
    record_ = nullptr;
    operation_request_->SetResult(this);
}
}  // namespace txservice::transaction