#ifndef TXSERVICE_UTILITY_CONFIGURATION_H_
#define TXSERVICE_UTILITY_CONFIGURATION_H_

#include <stddef.h>
#include <stdint.h>

namespace txservice
{
struct Constant
{
    static constexpr size_t INT64_T_LENGTH = sizeof(int64_t);
    static constexpr size_t INT_LENGTH = sizeof(int);
    static constexpr size_t SIZET_LENGTH = sizeof(size_t);
    static constexpr size_t FLOAT_LENGTH = sizeof(float);
    static constexpr size_t BOOL_LENGTH = sizeof(bool);
    static constexpr size_t TX_LOG_MAX_BATCH_THREAD_LOCAL_POOL = 1500;
    static constexpr bool ENABLE_LOG = false;
    static constexpr bool LOG_SYNC_BUFFER = true;
    static constexpr size_t MAX_TXN_TIME_MS = 10;
    static constexpr size_t MAX_TIME_SKEW_MS = 0;
    static constexpr int TIME_PROVIDER_INTERVAL = 1000;
    static constexpr int TXN_TABLE_MAX_CHECK_COUNT = 1000;
    static constexpr int VERSION_TABLE_MEMORY_RECYCLE_SIZE = 16;
    static constexpr size_t TPCC_STRING_SMALL = 24;
    static constexpr size_t TPCC_STRING_MIDDLE = 48;
    static constexpr size_t TPCC_STRING_LARGE = 100;
    static constexpr size_t ACTIVE_SET_PARTITION = 32;
    // 10 seconds as expire time.
    static constexpr size_t TXN_EXPIRE_INTERVAL = 10000000; 
    // whether to use local cache for data store.
    static constexpr bool ENABLE_VOLATILE_MAP = true; 
    // whether to use ds connector in executors.
    static constexpr bool USE_DATASTORE = true; 
};
}  // namespace txservice
#endif  // TXSERVICE_UTILITY_CONFIGURATION_H_