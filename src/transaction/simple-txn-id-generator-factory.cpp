#include "transaction/txn-id-generator-factory.h"

namespace txservice::transaction
{
std::unique_ptr<TxnIDGenerator> SimpleTxnIDGeneratorFactory::GetTxnIDGenerator(
    int executor_id)
{
    return std::make_unique<SimpleTxnIDGenerator>(interval_ * executor_id,
                                                  interval_);
}
}  // namespace txservice::transaction