#include <transaction/time-provider.h>
#include <utility/configuration.h>
#include <algorithm>
#include <chrono>
#include <ctime>
namespace txservice::transaction
{
LocalTimeProvider::LocalTimeProvider()
{
    count_ = 0;
    time_value_ = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
}
int64_t LocalTimeProvider::GetTime()
{
    if (count_ == Constant::TIME_PROVIDER_INTERVAL)
    {
        int64_t new_time =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        count_ = 0;

        time_value_ = std::max(time_value_, new_time);
    }

    return time_value_ + count_++;
}

void LocalTimeProvider::SetTime(int64_t time)
{
    time_value_ = std::max(time_value_ + count_, time);
    count_ = 0;
}
}  // namespace txservice::transaction
