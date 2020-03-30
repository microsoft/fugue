#ifndef TXSERVICE_UTILITY_CRTP_H_
#define TXSERVICE_UTILITY_CRTP_H_

namespace txservice
{
template <typename Impl, typename Tag = void>
struct Crtp
{
    Impl &GetImpl()
    {
        return static_cast<Impl &>(*this);
    }
    const Impl &GetImpl() const
    {
        return static_cast<const Impl &>(*this);
    }
};
}  // namespace txservice
#endif  // TXSERVICE_UTILITY_CRTP_H_