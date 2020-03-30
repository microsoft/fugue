#ifndef TXSERVICE_TRANSACTION_LOCAL_STATE_H_
#define TXSERVICE_TRANSACTION_LOCAL_STATE_H_

#include <assert.h>
#include <vector>
#include <iostream>
#include "versiondb/key.h"
#include "versiondb/record.h"
#include "versiondb/read-set-entry.h"
#include "versiondb/write-set-entry.h"

namespace txservice::transaction
{
class LocalState
{
public:
    struct SetKey : Hashable, EqualComparable<SetKey>
    {
        using Pointer = std::unique_ptr<SetKey>;

        SetKey() : table_name(nullptr), key(nullptr)
        {
        }

        SetKey(TableName *table_name, Key *key)
            : table_name(table_name), key(key)
        {
        }

        SetKey(const SetKey &that) : table_name(that.table_name), key(that.key)
        {
        }

        void Reset(TableName *table_name_val, Key *key_val)
        {
            table_name = table_name_val;
            key = key_val;
        }

        SetKey &operator=(const SetKey &that)
        {
            table_name = that.table_name;
            key = that.key;
            return *this;
        }

        SetKey(SetKey &&that) = default;

        Pointer Copy() const
        {
            return std::make_unique<SetKey>(table_name, key);
        }

        SetKey &operator=(SetKey &&that) = default;

        bool operator==(const SetKey &rhs) const
        {
            return key->Equals(*rhs.key) and *table_name == *(rhs.table_name);
        }

        bool operator!=(const SetKey &rhs) const
        {
            return not(*this == rhs);
        }

        virtual size_t Hash() const override
        {
            return key->Hash() * 23 + std::hash<TableName>()(*table_name);
        }

        size_t Serialize_Length() const
        {
            return table_name->size() + Constant::INT_LENGTH +
                   key->Serialize_Length();
        }

        void SerializeToBuffer(char *buffer, size_t &offset) const
        {
            // table name size
            int name_size = table_name->size();
            memcpy(buffer + offset, &name_size, Constant::INT_LENGTH);
            offset += Constant::INT_LENGTH;
            // table name
            table_name->copy(buffer + offset, name_size);
            offset += name_size;
            key->SerializeToBuffer(buffer, offset);
        }

        TableName *table_name;
        Key *key;
    };

    struct KeyReadSetEntry
    {
        using Pointer = std::unique_ptr<KeyReadSetEntry>;

        KeyReadSetEntry(SetKey::Pointer key, ReadSetEntry::Pointer entry)
            : key_(std::move(key)), entry_(std::move(entry))
        {
        }

        Pointer Copy() const
        {
            return std::make_unique<KeyReadSetEntry>(std::move(key_->Copy()),
                                                     std::move(entry_->Copy()));
        }

        SetKey::Pointer key_;
        ReadSetEntry::Pointer entry_;
    };

    struct KeyReadSetEntryPool
    {
    public:
        using Pointer = std::unique_ptr<KeyReadSetEntryPool>;

        KeyReadSetEntryPool(size_t capacity)
        {
            capacity_ = capacity;
            pop_index_ = 0;

            for (size_t i = 0; i < capacity; i++)
            {
                SetKey::Pointer key = std::make_unique<SetKey>();
                ReadSetEntry::Pointer read_set_entry =
                    std::make_unique<ReadSetEntry>();
                KeyReadSetEntry::Pointer key_read_set_entry =
                    std::make_unique<KeyReadSetEntry>(
                        std::move(key), std::move(read_set_entry));
                key_read_set_entry_pool_.push_back(
                    std::move(key_read_set_entry));
            }
        }

        KeyReadSetEntryPool(const KeyReadSetEntryPool &that)
        {
            capacity_ = that.capacity_;
            pop_index_ = that.pop_index_;
            for (size_t i = 0; i < capacity_; i++)
            {
                KeyReadSetEntry::Pointer key_read_set_entry =
                    std::move(that.key_read_set_entry_pool_[i]->Copy());
                key_read_set_entry_pool_.push_back(
                    std::move(key_read_set_entry));
            }
        }

        KeyReadSetEntry *NewReadSetEntry()
        {
            if (pop_index_ == capacity_)
            {
                Resize();
            }
            KeyReadSetEntry *key_read_set_entry =
                key_read_set_entry_pool_[pop_index_].get();
            pop_index_++;
            return key_read_set_entry;
        }

        void Resize()
        {
            size_t new_capacity = 1 + capacity_;
            for (size_t i = capacity_; i < new_capacity; i++)
            {
                SetKey::Pointer key = std::make_unique<SetKey>();
                ReadSetEntry::Pointer read_set_entry =
                    std::make_unique<ReadSetEntry>();
                KeyReadSetEntry::Pointer key_read_set_entry =
                    std::make_unique<KeyReadSetEntry>(
                        std::move(key), std::move(read_set_entry));
                key_read_set_entry_pool_.push_back(
                    std::move(key_read_set_entry));
            }
            capacity_ = new_capacity;
        }

        ReadSetEntry *FindInReadSet(const TableName &table_name,
                                    const Key &key) const
        {
            for (int i = pop_index_ - 1; i >= 0; i--)
            {
                KeyReadSetEntry *entry = key_read_set_entry_pool_[i].get();
                assert(entry->key_ != nullptr);
                assert(entry->key_->table_name != nullptr);
                assert(entry->key_->key != nullptr);
                if (table_name == *(entry->key_->table_name) &&
                    key == *(entry->key_->key))
                {
                    return entry->entry_.get();
                }
            }
            return nullptr;
        }

        void Release()
        {
            pop_index_--;
        }

        void Reset()
        {
            pop_index_ = 0;
        }
        size_t GetSize() const
        {
            return pop_index_;
        }

        const std::vector<KeyReadSetEntry::Pointer> *GetAllReadSet() const
        {
            return &key_read_set_entry_pool_;
        }

    private:
        std::vector<KeyReadSetEntry::Pointer> key_read_set_entry_pool_;
        size_t capacity_;
        size_t pop_index_;
    };

    struct KeyWriteSetEntry
    {
        using Pointer = std::unique_ptr<KeyWriteSetEntry>;

        KeyWriteSetEntry(SetKey::Pointer key, WriteSetEntry::Pointer entry)
            : key_(std::move(key)), entry_(std::move(entry))
        {
        }
        Pointer Copy() const
        {
            return std::make_unique<KeyWriteSetEntry>(
                std::move(key_->Copy()), std::move(entry_->Copy()));
        }

        SetKey::Pointer key_;
        WriteSetEntry::Pointer entry_;
    };

    struct KeyWriteSetEntryPool
    {
    public:
        using Pointer = std::unique_ptr<KeyWriteSetEntryPool>;

        KeyWriteSetEntryPool(size_t capacity)
        {
            capacity_ = capacity;
            pop_index_ = 0;

            for (size_t i = 0; i < capacity; i++)
            {
                SetKey::Pointer key = std::make_unique<SetKey>();
                WriteSetEntry::Pointer write_set_entry =
                    std::make_unique<WriteSetEntry>();
                KeyWriteSetEntry::Pointer key_write_set_entry =
                    std::make_unique<KeyWriteSetEntry>(
                        std::move(key), std::move(write_set_entry));
                key_write_set_entry_pool_.push_back(
                    std::move(key_write_set_entry));
            }
        }

        KeyWriteSetEntryPool(const KeyWriteSetEntryPool &that)
        {
            capacity_ = that.capacity_;
            pop_index_ = that.pop_index_;
            for (size_t i = 0; i < capacity_; i++)
            {
                KeyWriteSetEntry::Pointer key_write_set_entry =
                    std::move(that.key_write_set_entry_pool_[i]->Copy());
                key_write_set_entry_pool_.push_back(
                    std::move(key_write_set_entry));
            }
        }

        KeyWriteSetEntry *NewWriteSetEntry(TableName *table_name,
                                           Key *key,
                                           int64_t version,
                                           bool is_deleted,
                                           Record *record,
                                           ReadSetEntry *read_entry,
                                           bool need_post_processing = false)
        {
            if (pop_index_ == capacity_)
            {
                Resize();
            }
            KeyWriteSetEntry *key_write_set_entry =
                key_write_set_entry_pool_[pop_index_].get();
            key_write_set_entry->key_->Reset(table_name, key);
            key_write_set_entry->entry_->Reset(
                version, is_deleted, record, read_entry, need_post_processing);
            pop_index_++;
            return key_write_set_entry;
        }

        void Resize()
        {
            size_t new_capacity = 1 + capacity_;
            for (size_t i = capacity_; i < new_capacity; i++)
            {
                SetKey::Pointer key = std::make_unique<SetKey>();
                WriteSetEntry::Pointer write_set_entry =
                    std::make_unique<WriteSetEntry>();
                KeyWriteSetEntry::Pointer key_write_set_entry =
                    std::make_unique<KeyWriteSetEntry>(
                        std::move(key), std::move(write_set_entry));
                key_write_set_entry_pool_.push_back(
                    std::move(key_write_set_entry));
            }
            capacity_ = new_capacity;
        }

        WriteSetEntry *FindInWriteSet(const TableName &table_name,
                                      const Key &key) const
        {
            for (int i = pop_index_ - 1; i >= 0; i--)
            {
                KeyWriteSetEntry *entry = key_write_set_entry_pool_[i].get();
                assert(entry->key_ != nullptr);
                assert(entry->key_->table_name != nullptr);
                assert(entry->key_->key != nullptr);
                if (table_name == *(entry->key_->table_name) &&
                    key == *(entry->key_->key))
                {
                    return entry->entry_.get();
                }
            }
            return nullptr;
        }

        void Release()
        {
            pop_index_--;
        }

        void Reset()
        {
            pop_index_ = 0;
        }

        size_t GetSize() const
        {
            return pop_index_;
        }

        const std::vector<KeyWriteSetEntry::Pointer> *GetAllWriteSet() const
        {
            return &key_write_set_entry_pool_;
        }

    private:
        std::vector<KeyWriteSetEntry::Pointer> key_write_set_entry_pool_;
        size_t capacity_;
        size_t pop_index_;
    };

    LocalState(size_t capacity);

    LocalState(const LocalState &that);

    KeyReadSetEntry *InsertReadSet();

    void InsertWriteSet(TableName *table_name,
                        Key *key,
                        int64_t version,
                        bool is_deleted,
                        Record *record,
                        ReadSetEntry *read_entry,
                        bool need_post_processing = false);

    WriteSetEntry *FindInWriteSet(const TableName &table_name,
                                  const Key &key) const;

    ReadSetEntry *FindInReadSet(const TableName &table_name,
                                const Key &key) const;

    const std::vector<KeyReadSetEntry::Pointer> *GetAllReadSet() const;

    size_t GetReadSetSize() const;

    const std::vector<KeyWriteSetEntry::Pointer> *GetAllWriteSet() const;

    size_t GetWriteSetSize() const;

    void Reset();

    void ReleaseReadSet()
    {
        key_read_set_.Release();
    }

    void ReleaseWriteSet()
    {
        key_write_set_.Release();
    }

private:
    /// version entries retrieved from the remote kv
    KeyReadSetEntryPool key_read_set_;
    /// dirty version entries which are to be uploaded
    KeyWriteSetEntryPool key_write_set_;
};
}  // namespace txservice::transaction
#endif  // TXSERVICE_TRANSACTION_LOCAL_STATE_H_