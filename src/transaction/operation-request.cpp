#include "transaction/operation-request.h"
#include <condition_variable>
#include <mutex>

namespace txservice::transaction
{
void OperationRequest::SetResult(Result *result)
{
    result_ = result;
}

Result *OperationRequest::GetResult() const
{
    return result_;
}

void OperationRequest::AddDependentRequest(OperationRequest *request)
{
    dependent_request_.push_back(request);
}

void OperationRequest::SetUp()
{
    if (request_processor_ != nullptr)
        request_processor_->Process(this);
}

void OperationRequest::PullResult()
{
    cache_->record_ = result_->record_;
    cache_->is_deleted_ = result_->is_deleted_;
    cache_->is_null_ = result_->is_null_;
    cache_->is_error_ = result_->is_error_;
}

void OperationRequest::Notify()
{
    std::unique_lock<std::mutex> lk(mutex_);
    is_finished_ = true;
    lk.unlock();
    cv_.notify_one();
}

void OperationRequest::Wait()
{
    // try fast path first
    if (is_finished_)
        return;
    // slow path, requires cv
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait(lk, [this] { return is_finished_; });
}

}  // namespace txservice::transaction