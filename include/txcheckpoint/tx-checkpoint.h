#ifndef TXSERVICE_TXCHECKPOINT_TX_CHECKPOINT_H_
#define TXSERVICE_TXCHECKPOINT_TX_CHECKPOINT_H_

#include "txcheckpoint/key-iterator.h"
#include "txlog/txlog.h"
#include "versiondb/request/handler.h"

namespace txservice::txckpt
{
struct CheckpointEntry
{
    Key *key_;
    Record *record_;
    bool is_deleted_;
    CheckpointEntry(Key *key, Record *record, bool is_deleted)
        : key_(key), record_(record), is_deleted_(is_deleted)
    {
    }
};
class TxCheckpoint
{
public:
    using Pointer = std::unique_ptr<TxCheckpoint>;
    TxCheckpoint(const TableName table_name,
                 int partition,
                 txlog::TxLog::Pointer tx_log,
                 request::Handler::Pointer handler,
                 Record *infant,
                 void *record_deserializer = nullptr,
                 void *key_deserializer = nullptr,
                 int batch_size = 1)
        : batch_size_(batch_size)
    {
        this->table_name_ = table_name;
        this->partition_ = partition;
        this->tx_log_ = std::move(tx_log);
        this->handler_ = std::move(handler);
        this->key_deserializer_ = key_deserializer;
        this->record_deserailizer_ = record_deserializer;
        for (int idx = 0; idx < batch_size; idx++)
        {
            this->changes[0].push_back(std::move(infant->Copy()));
            this->changes[1].push_back(std::move(infant->Copy()));
            this->read_results.push_back(std::make_pair(
                nullptr, request::HandlerResult<VersionEntry>()));
            this->read_results[idx].second.result_.read_record_ =
                changes[0][idx].get();
        }
    }

    virtual ~TxCheckpoint() {};

    // start a new iteration of checkpoint
    virtual void NewCheckpoint(int64_t checkpoint_ts)
    {
        this->checkpoint_ts_ = checkpoint_ts;
        this->key_iterator_ =
            std::move(handler_->GetCheckpointKeys(this->table_name_,
                                                  this->partition_,
                                                  this->checkpoint_ts_,
                                                  this->key_deserializer_));
    }
    // go to the next change in this iteration.
    // Returns true if there is one, false otherwise indicating the end.
    virtual bool MoveNext() = 0;

    // return the current data. Note it shouldn't be called twice without
    // MoveNext()
    virtual CheckpointEntry Current() = 0;

    virtual void Flush()
    {
    }

    virtual void DeleteActiveEntry(Key *key)
    {
        handler_->CheckRemoveActEntry(
            this->table_name_, this->partition_, key, this->checkpoint_ts_);
    }

    void TruncateLog(int64_t truncate_ts)
    {
        this->tx_log_->CleanBefore(truncate_ts);
    }

public:
    void ReadPayload(int idx, Key &key)
    {
        this->read_results[idx].first = &key;
        this->handler_->GetVersionList(this->table_name_,
                                       key,
                                       this->read_results[idx].second,
                                       this->record_deserailizer_);
    }

    const int batch_size_;
    std::vector<Record::Pointer> changes[2];
    std::vector<
        std::pair<Key *, request::HandlerResult<VersionEntry>>>
        read_results;

    KeyIterator::Pointer key_iterator_;

    TableName table_name_;
    int partition_;
    int64_t checkpoint_ts_;
    txlog::TxLog::Pointer tx_log_;
    request::Handler::Pointer handler_;
    void *key_deserializer_;
    void *record_deserailizer_;
}; 
}  // namespace txservice::txckpt
#endif  // TXSERVICE_TXCHECKPOINT_KEY_ITERATOR_H_