#ifndef TXSERVICE_VERSIONDB_REQUEST_HANDLER_RESULT_H_
#define TXSERVICE_VERSIONDB_REQUEST_HANDLER_RESULT_H_

namespace txservice::request
{
template <typename T>
class HandlerResult
{
public:
    HandlerResult()
    {
    }
    bool IsFinished() const
    {
        return is_finished_;
    }

    bool IsError() const
    {
        return is_error_;
    }

    void SetFinished()
    {
        is_finished_ = true;
    }

    void SetError()
    {
        is_error_ = true;
        SetFinished();
    }

    T &GetResult()
    {
        return result_;
    }

    void Reset()
    {
        is_finished_ = false;
        is_error_ = false;
    }

    T result_;
    bool is_finished_;
    bool is_error_;
    int ref_cnt;
};
}  // namespace txservice::request
#endif  // TXSERVICE_VERSIONDB_REQUEST_HANDLER_RESULT_H_

