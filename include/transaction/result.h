#ifndef TXSERVICE_TRANSACTION_RESULT_H_
#define TXSERVICE_TRANSACTION_RESULT_H_
#include <atomic>
#include <iostream>
#include "versiondb/record.h"
#include "versiondb/tx-entry.h"
namespace txservice::transaction
{
class OperationRequest;
struct Result
{
    using Pointer = std::unique_ptr<Result>;

    Result()
    {
        record_ = nullptr;
        is_deleted_ = false;
        is_null_ = false;
        is_error_ = false;
        operation_request_ = nullptr;
        status_ = TxnStatus::kOngoing;
    }

    Result(const Result &that)
    {
        record_ = that.record_;
        is_deleted_ = that.is_deleted_;
        is_null_ = that.is_null_;
        is_error_ = that.is_error_;
        operation_request_ = that.operation_request_;
        status_ = that.status_;
    }

    void Reset(Record *record, OperationRequest *operation_request);

    void Reset(OperationRequest *operation_request);

    void SetRecord(Record *record)
    {
        if (record_ != nullptr)
        {
            if (record_ != record)
            {
                record_->CopyFrom(*record);
            }
        }
        else
        {
            record_ = record;
        }
        SetFinished();
    }

    void SetNull()
    {
        is_null_ = true;
        SetFinished();
    }

    void SetDeleted()
    {
        is_deleted_ = true;
        SetFinished();
    }

    void SetError()
    {
        is_error_ = true;
        SetFinished();
    }

    void SetStatus(TxnStatus status)
    {
        status_ = status;
        SetFinished();
    }

    void SetFinished();

    bool IsDeleted()
    {
        return is_deleted_;
    }

    bool IsNull()
    {
        return is_null_;
    }

    bool IsError()
    {
        return is_error_;
    }

    bool IsCommitted()
    {
        return status_ == TxnStatus::kCommitted;
    }

    bool IsAbort()
    {
        return status_ == TxnStatus::kAborted;
    }

    Record *GetRecord()
    {
        return record_;
    }

    Record *record_;
    OperationRequest *operation_request_;
    bool is_deleted_;
    bool is_null_;
    bool is_error_;
    TxnStatus status_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_RESULT_H_
