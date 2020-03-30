#pragma once
#include "txcheckpoint/key-iterator.h"
#include "versiondb/request/handler.h"

namespace txservice::kickout
{
class Kickout
{
public:
    using Pointer = std::unique_ptr<Kickout>;
    Kickout(request::Handler::Pointer handler,
            const TableName &table_name,
            void *key_deserializer)
        : handler_(std::move(handler)),
          table_name_(table_name),
          key_deserializer_(key_deserializer){};

    void NewKickout(int64_t kickout_ts)
    {
        this->key_iterator = this->handler_->GetAllCurrentKeys(
            this->table_name_, this->key_deserializer_);
        this->checkpoint_ts = kickout_ts;
        this->lru_ts = kickout_ts - txservice::Constant::TXN_EXPIRE_INTERVAL / 2;
        this->expire_ts = kickout_ts - txservice::Constant::TXN_EXPIRE_INTERVAL;
    }

    // do a round of scan-kick in that table; singleton thread for now.
    bool HasNext()
    {
        return this->key_iterator->HasNext();
    }

    void MoveNext(txservice::request::HandlerResult<bool> &handler_result)
    {
        this->handler_->KickoutVersion(this->table_name_,
                                            &this->key_iterator->Next(),
                                            this->checkpoint_ts,
                                            this->expire_ts,
                                            this->lru_ts,
                                            handler_result);
    }

    request::Handler::Pointer handler_;
    const TableName table_name_;
    txcheckpoint::KeyIterator::Pointer key_iterator;
    void *key_deserializer_;

    // calculated after a round of kickout on the table.
    // it depends on upper layers to decide what to do corresponding to the
    // rate.
    int total_num;
    int kickout_num;

    int64_t checkpoint_ts;
    int64_t expire_ts;
    int64_t lru_ts;
};
}  // namespace txservice::kickout