#ifndef TXSERVICE_VERSIONDB_READ_SET_ENTRY_H_
#define TXSERVICE_VERSIONDB_READ_SET_ENTRY_H_

#include "versiondb/version-entry.h"

namespace txservice
{
struct ReadSetEntry : Copyable<ReadSetEntry>
{
    using Pointer = std::unique_ptr<ReadSetEntry>;

    ReadSetEntry()
    {
        version_ = VersionEntry::kDefaultVersion;
        tx_id_ = VersionEntry::kEmptyTxId;
        begin_ts_ = VersionEntry::kDefaultBeginTs;
        end_ts_ = VersionEntry::kDefaultEndTs;
        is_deleted_ = true;
        is_updated_ = false;
        record_ = nullptr;
        extension_ = nullptr;
        need_post_processing_ = false;
        need_release_ = true;
    }

    ReadSetEntry(int64_t version,
                 int64_t tx_id,
                 int64_t begin_ts,
                 int64_t end_ts,
                 bool is_deleted,
                 Record* record,
                 EntryExtension::Pointer extension = nullptr,
                 bool need_post_processing = false,
                 bool need_release =true,
                 bool is_updated = false)
        : version_(version),
          tx_id_(tx_id),
          begin_ts_(begin_ts),
          end_ts_(end_ts),
          is_deleted_(is_deleted),
          is_updated_(is_updated),
          record_(record),
          need_post_processing_(need_post_processing),
          need_release_(need_release),
          extension_(std::move(extension))
    {
    }

    Pointer Copy() const
    {
        return std::make_unique<ReadSetEntry>(version_,
                                              tx_id_,
                                              begin_ts_,
                                              end_ts_,
                                              is_deleted_,
                                              record_,
                                              CopyPtr(extension_),
                                              need_post_processing_,
                                              need_release_,
                                              is_updated_);
    }

    void Reset(int64_t version,
               int64_t tx_id,
               int64_t begin_ts,
               int64_t end_ts,
               bool is_deleted,
               Record* record,
               EntryExtension::Pointer extension = nullptr,
               bool need_post_processing = false,
               bool need_release =true,
               bool is_updated = false)
    {
        version_ = version;
        tx_id_ = tx_id;
        begin_ts_ = begin_ts;
        end_ts_ = end_ts;
        is_deleted_ = is_deleted;
        is_updated_ = is_updated;
        record_ = record;
        extension_ = std::move(extension);
        need_post_processing_ = need_post_processing;
        need_release_ = need_release;
    }

    int64_t version_;
    int64_t tx_id_;
    int64_t begin_ts_;
    int64_t end_ts_;
    bool is_deleted_;
    bool is_updated_;
    Record* record_;
    bool need_post_processing_;
    bool need_release_;
    EntryExtension::Pointer extension_;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_READ_SET_ENTRY_H_