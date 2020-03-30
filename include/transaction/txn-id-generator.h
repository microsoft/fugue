#ifndef TXSERVICE_TRANSACTION_TXN_ID_GENERATOR_H_
#define TXSERVICE_TRANSACTION_TXN_ID_GENERATOR_H_

#include <cstdint>
#include <cstdlib>
#include <ctime>

namespace txservice::transaction
{
class TxnIDGenerator
{
public:
    virtual ~TxnIDGenerator() = default;
    virtual int64_t GenerateID() = 0;
};

class SimpleTxnIDGenerator : public TxnIDGenerator
{
public:
    SimpleTxnIDGenerator(int64_t base, int64_t interval)
        : start_(base), end_(base + interval), id_(base)
    {
        srand (time(NULL));
    }

    int64_t GenerateID() override;

private:
    int64_t id_;
    int64_t start_;
    int64_t end_;
};
}// namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_TXN_ID_GENERATOR_H_