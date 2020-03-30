#ifndef TXSERVICE_TRANSACTION_TXN_ID_GENERATOR_FACTORY_H_
#define TXSERVICE_TRANSACTION_TXN_ID_GENERATOR_FACTORY_H_

#include <memory>
#include "transaction/txn-id-generator.h"

namespace txservice::transaction
{
class TxnIDGeneratorFactory
{
public:
    virtual std::unique_ptr<TxnIDGenerator> GetTxnIDGenerator(
        int executor_id) = 0;
    virtual ~TxnIDGeneratorFactory() = default;
};

class SimpleTxnIDGeneratorFactory : public TxnIDGeneratorFactory
{
public:
    SimpleTxnIDGeneratorFactory(int count)
        : count_(count), interval_(INT64_MAX / count)
    {
    }

    std::unique_ptr<TxnIDGenerator> GetTxnIDGenerator(
        int executor_id) override;

private:
    int count_;
    int64_t interval_;
};
}// namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_TXN_ID_GENERATOR_FACTORY_H_
