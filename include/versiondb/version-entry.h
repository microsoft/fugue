#ifndef TXSERVICE_VERSIONDB_VERSION_ENTRY_H_
#define TXSERVICE_VERSIONDB_VERSION_ENTRY_H_

#include <stdint.h>
#include "versiondb/entry-extension.h"
#include "versiondb/record.h"

namespace txservice
{
struct VersionEntry
{
    using Pointer = std::unique_ptr<VersionEntry>;
    /// initial value of begin timestamp
    static constexpr int64_t kDefaultBeginTs = -1;
    /// initial value of end timestamp
    static constexpr int64_t kDefaultEndTs = -1;
    static constexpr int64_t kDefaultMaxTs = 0;
    static constexpr int64_t kUnSetCommitTs = -2;
    static constexpr int64_t kMaxTimeStamp = INT64_MAX;
    static constexpr int64_t kDefaultVersion = -1;
    /// version number of the first version in the version list.
    static constexpr int64_t kFirstVersion = 0;
    /// used in commit post-processing, setting tx_id of latest version
    static constexpr int64_t kEmptyTxId = -1;
    static constexpr int64_t kMaxVersionKeyStartIndex = -1;

    bool operator<(const VersionEntry &other) const
    {
        if (version_ < other.version_)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    VersionEntry &operator=(VersionEntry other)
    {
        version_ = other.version_;
        tx_id_ = other.tx_id_;
        begin_ts_ = other.begin_ts_;
        end_ts_ = other.end_ts_;
        max_commit_ts_ = other.max_commit_ts_;
        is_deleted_ = other.is_deleted_;
        read_record_ = other.read_record_;
        write_record_ = other.write_record_;
        extension_ = std::move(other.extension_);
        return *this;
    }

    VersionEntry(int64_t version,
                 int64_t tx_id,
                 int64_t begin_ts,
                 int64_t end_ts,
                 int64_t max_commit_ts,
                 bool is_deleted,
                 Record *read_record,
                 Record *write_record,
                 EntryExtension::Pointer extension = nullptr)
        : version_(version),
          tx_id_(tx_id),
          begin_ts_(begin_ts),
          end_ts_(end_ts),
          max_commit_ts_(max_commit_ts),
          is_deleted_(is_deleted),
          read_record_(read_record),
          write_record_(write_record),
          pool_(nullptr),
          extension_(std::move(extension))
    {
    }

    VersionEntry(const VersionEntry &other)
    {
        version_ = other.version_;
        tx_id_ = other.tx_id_;
        begin_ts_ = other.begin_ts_;
        end_ts_ = other.end_ts_;
        max_commit_ts_ = other.max_commit_ts_;
        is_deleted_ = other.is_deleted_;
        read_record_ = other.read_record_;
        write_record_ = other.write_record_;
        pool_ = CopyPtr(other.write_record_);
        extension_ = CopyPtr(other.extension_);
    }

    VersionEntry()
    {
        version_ = kDefaultVersion;
        tx_id_ = kEmptyTxId;
        begin_ts_ = kDefaultBeginTs;
        end_ts_ = kDefaultEndTs;
        max_commit_ts_ = -1;
        is_deleted_ = true;
        read_record_ = nullptr;
        write_record_ = nullptr;
        extension_ = nullptr;

    }

    void Reset(Record *record = nullptr,
               EntryExtension::Pointer extension = nullptr)
    {
        version_ = kDefaultVersion;
        tx_id_ = kEmptyTxId;
        begin_ts_ = kDefaultBeginTs;
        end_ts_ = kDefaultEndTs;
        max_commit_ts_ = -1;
        is_deleted_ = true;
        read_record_ = record;
        write_record_ = nullptr;
        extension_ = std::move(extension);
    }

    void Reset(int64_t version,
               int64_t tx_id,
               int64_t begin_ts,
               int64_t end_ts,
               int64_t max_commit_ts,
               bool is_deleted,
               Record *read_record,
               Record *write_record,
               EntryExtension::Pointer extension)
    {
        version_ = version;
        tx_id_ = tx_id;
        begin_ts_ = begin_ts;
        end_ts_ = end_ts;
        max_commit_ts_ = max_commit_ts;
        is_deleted_ = is_deleted;
        read_record_ = read_record;
        write_record_ = write_record;
        extension_ = std::move(extension);
    }

    int64_t version_;
    int64_t tx_id_;
    int64_t begin_ts_;
    int64_t end_ts_;
    int64_t max_commit_ts_;
    bool is_deleted_;
    Record *read_record_;
    Record *write_record_;
    Record::Pointer pool_;
    EntryExtension::Pointer extension_;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_VERSION_ENTRY_H_