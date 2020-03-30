#ifndef TXSERVICE_TRANSACTION_TIME_PROVIDER_H_
#define TXSERVICE_TRANSACTION_TIME_PROVIDER_H_
#include <cstdint>

namespace txservice::transaction
{
class TimeProvider
{
public:
    virtual ~TimeProvider() = default;
    virtual int64_t GetTime() = 0;
    virtual void SetTime(int64_t time) = 0;
};

class LocalTimeProvider : public TimeProvider
{
public:
    LocalTimeProvider();
    virtual int64_t GetTime() override;
    virtual void SetTime(int64_t time) override;

private:
    int64_t time_value_;
    int count_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_RUNTIME_TRANSACTION_EXECUTOR_H_