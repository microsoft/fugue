#ifndef TXSERVICE_VERSIONDB_WRITE_SET_ENTRY_H_
#define TXSERVICE_VERSIONDB_WRITE_SET_ENTRY_H_

#include "versiondb/version-entry.h"

namespace txservice
{
struct WriteSetEntry : Copyable<WriteSetEntry>
{
    using Pointer = std::unique_ptr<WriteSetEntry>;

    WriteSetEntry()
    {
        version_ = VersionEntry::kDefaultVersion;
        is_deleted_ = true;
        record_ = nullptr;
        read_entry_ = nullptr;
        need_post_processing_ = false;
        extension_ = nullptr;
    }

    WriteSetEntry(int64_t version,
                  bool is_deleted,
                  Record* record,
                  ReadSetEntry *read_entry,
                  bool need_post_processing = false,
                  EntryExtension::Pointer extension = nullptr)
        : version_(version),
          is_deleted_(is_deleted),
          record_(record),
          read_entry_(read_entry),
          need_post_processing_(need_post_processing),
          extension_(std::move(extension))
    {
    }

    Pointer Copy() const
    {
        return std::make_unique<WriteSetEntry>(version_,
                                               is_deleted_,
                                               record_,
                                               read_entry_,
                                               need_post_processing_,
                                               CopyPtr(extension_));
    }

    void Reset(int64_t version,
               bool is_deleted,
               Record* record,
               ReadSetEntry *read_entry,
               bool need_post_processing = false)
    {
        version_ = version;
        is_deleted_ = is_deleted;
        record_ = record;
        read_entry_ = read_entry;
        need_post_processing_ = need_post_processing;
    }

    size_t Serialize_Length() const
    {
        size_t length = 0;
        length += Constant::INT_LENGTH;
        length += Constant::BOOL_LENGTH;
        length += record_->Serialize_Length();
        return length;
    }

    void SerializeToBuffer(char *buffer, size_t &offset) const
    {
        memcpy(buffer + offset, &version_, Constant::INT64_T_LENGTH);
        offset += Constant::INT64_T_LENGTH;
        memcpy(buffer + offset, &is_deleted_, Constant::BOOL_LENGTH);
        offset += Constant::BOOL_LENGTH;
        record_->SerializeToBuffer(buffer, offset);
    }

    int64_t version_;
    bool is_deleted_;
    Record* record_;
    ReadSetEntry *read_entry_;
    bool need_post_processing_;
    EntryExtension::Pointer extension_;
};
}  // namespace txservice
#endif  // TXSERVICE_VERSIONDB_WRITE_SET_ENTRY_H_
