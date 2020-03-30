#include "transaction/txn-id-generator.h"

namespace txservice::transaction
{
int64_t SimpleTxnIDGenerator::GenerateID()
{
    id_++;
    if (id_ == end_)
    {
        id_ = start_;
    }
    return id_;
}
}  // namespace txservice::transaction