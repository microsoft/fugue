#ifndef TXSERVICE_TRANSACTION_OPERATION_REQUEST_H_
#define TXSERVICE_TRANSACTION_OPERATION_REQUEST_H_

#include <condition_variable>
#include <functional>
#include <mutex>
#include "transaction/result.h"
#include "versiondb/key.h"
#include "versiondb/record.h"
namespace txservice::transaction
{
enum OperationType
{
    Begin,
    Insert,
    Update,
    Upsert,
    Delete,
    Read,
    ReadOutside,
	ReadDataStore,
    Commit,
    Abort
};

class OperationRequest;

class RequestProcess
{
public:
    virtual ~RequestProcess() = default;

    virtual void Process(OperationRequest *request) = 0;

    using Pointer = std::unique_ptr<RequestProcess>;

    int choice;
};

class OperationRequest
{
public:
    using Pointer = std::shared_ptr<OperationRequest>;

    OperationRequest(
        int64_t session_id,
        const TableName table_name,
        Key::Pointer key,
        std::unique_ptr<Record> record,
        OperationType operation_type,
        std::unique_ptr<RequestProcess> request_processor = nullptr,
        void *callback_deserializer = nullptr)
        : session_id_(session_id),
          table_name_(table_name),
          key_(std::move(key)),
          record_(std::move(record)),
          operation_type_(operation_type),
          request_processor_(std::move(request_processor)),
          cache_(nullptr),
          result_(nullptr),
          is_finished_(false),
		  callback_deserializer_(callback_deserializer)
    {
        cache_ = std::make_unique<Result>();
    }

    OperationRequest(int64_t session_id, OperationType operation_type)
        : session_id_(session_id),
          operation_type_(operation_type),
          cache_(nullptr),
          result_(nullptr),
          is_finished_(false)
    {
        cache_ = std::make_unique<Result>();
    }

    ~OperationRequest()
    {
    }

    void SetResult(txservice::transaction::Result *result);

    Result *GetResult() const;

    void AddDependentRequest(OperationRequest *request);

    void SetUp();

    void PullResult();

    void Wait();

    void Notify();

    int type_ = 0;
    int64_t session_id_;
    TableName table_name_;
    Key::Pointer key_;
    std::unique_ptr<Result> cache_;
    Record::Pointer record_;
    OperationType operation_type_;
    Result *result_;
    std::vector<OperationRequest *> dependent_request_;
    std::unique_ptr<RequestProcess> request_processor_;
    void *callback_deserializer_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool is_finished_;
    // this is only used in read data store.
    bool need_to_read_outside_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_OPERATION_REQUEST_H_