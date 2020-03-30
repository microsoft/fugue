#include "transaction/local-state.h"

namespace txservice::transaction
{
LocalState::LocalState(size_t capacity)
    : key_read_set_(capacity), key_write_set_(capacity)
{
}

LocalState::LocalState(const LocalState &that)
    : key_read_set_(that.key_read_set_), key_write_set_(that.key_write_set_)
{
}

LocalState::KeyReadSetEntry *LocalState::InsertReadSet()
{
    return key_read_set_.NewReadSetEntry();
}

void LocalState::InsertWriteSet(TableName *table_name,
                                Key *key,
                                int64_t version,
                                bool is_deleted,
                                Record* record,
                                ReadSetEntry *read_entry,
                                bool need_post_processing)
{
    key_write_set_.NewWriteSetEntry(table_name,
                                    key,
                                    version,
                                    is_deleted,
                                    record,
                                    read_entry,
                                    need_post_processing);
}

WriteSetEntry *LocalState::FindInWriteSet(const TableName &table_name,
                                          const Key &key) const
{
    return key_write_set_.FindInWriteSet(table_name, key);
}

ReadSetEntry *LocalState::FindInReadSet(const TableName &table_name,
                                        const Key &key) const
{
    return key_read_set_.FindInReadSet(table_name, key);
}

size_t LocalState::GetReadSetSize() const
{
    return key_read_set_.GetSize();
}

size_t LocalState::GetWriteSetSize() const
{
    return key_write_set_.GetSize();
}

const std::vector<LocalState::KeyReadSetEntry::Pointer>
    *LocalState::GetAllReadSet() const
{
    return key_read_set_.GetAllReadSet();
}

const std::vector<LocalState::KeyWriteSetEntry::Pointer>
    *LocalState::GetAllWriteSet() const
{
    return key_write_set_.GetAllWriteSet();
}

void LocalState::Reset()
{
    key_read_set_.Reset();
    key_write_set_.Reset();
}
}  // namespace txservice::transaction