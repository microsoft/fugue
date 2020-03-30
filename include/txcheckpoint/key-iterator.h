#ifndef TXSERVICE_TXCHECKPOINT_KEY_ITERATOR_H_
#define TXSERVICE_TXCHECKPOINT_KEY_ITERATOR_H_

#include "versiondb/key.h"

namespace txservice::txcheckpoint
{
class KeyIterator
{
public:
    using Pointer = std::unique_ptr<KeyIterator>;
    virtual bool HasNext() = 0;
    virtual Key &Next() = 0;
    KeyIterator(void *key_deserializer)
    {
        this->key_deserializer_ = key_deserializer;
    }
    KeyIterator(const KeyIterator &) = delete;
    virtual ~KeyIterator()
    {
    }

protected:
    void *key_deserializer_;
    std::vector<Key::Pointer> key_container;
};
class ActiveKeyIterator : public KeyIterator
{
public:
    ActiveKeyIterator(int64_t checkpoint_ts, void *key_deserializer)
        : KeyIterator(key_deserializer)
    {
        this->checkpoint_ts_ = checkpoint_ts;
    }

protected:
    int64_t checkpoint_ts_;
};
}  // namespace txservice::txcheckpoint
#endif  // TXSERVICE_TXCHECKPOINT_KEY_ITERATOR_H_
