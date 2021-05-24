#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>

#include <chrono>
#include <functional>
#include <optional>

namespace io = boost::asio;
namespace bst = boost::beast;
namespace sys = boost::system;

using THttpRequest = bst::http::request<bst::http::dynamic_body>;
using THttpResponse = bst::http::response<bst::http::dynamic_body>;

class TConnectionData : public std::enable_shared_from_this<TConnectionData> {
    class TReadAwaiter {
        enum class EReadResult : unsigned char {
            Empty,
            Success,
            Timeout
        };

    public:
        explicit TReadAwaiter(std::shared_ptr<TConnectionData> connection)
            : Req{}
            , Connection(std::move(connection))
            , Error{}
            , ReadResult{EReadResult::Empty}
        {}

        bool await_ready() {
            return false;
        }

        void await_suspend(std::coroutine_handle<> coro) {
            auto rdHandler = [this, conn = Connection, coro](sys::error_code error, std::size_t) {
                if (error) [[unlikely]] {
                    if (error == io::error::operation_aborted) {
                        return;
                    }
                    Error = error;
                    Connection->Timer.cancel();
                    coro.resume();
                } else if (ReadResult == EReadResult::Empty) {
                    Connection->Timer.cancel();
                    ReadResult = EReadResult::Success;
                    coro.resume();
                }
            };
            auto tmHandler = [this, conn = Connection, coro](sys::error_code error) {
                if (error) [[unlikely]] {
                    if (error == io::error::operation_aborted) {
                        return;
                    }
                    Error = error;
                    coro.resume();
                } else if (ReadResult == EReadResult::Empty) {
                    ReadResult = EReadResult::Timeout;
                    coro.resume();
                }
            };

            Req.clear();
            Error.clear();
            ReadResult = EReadResult::Empty;
            Connection->Timer.async_wait(std::move(tmHandler));
            bst::http::async_read(Connection->Socket, Connection->Buff, Req, std::move(rdHandler));
        }

        std::optional<THttpRequest> await_resume() {
            if (Error) {
                throw sys::system_error(Error);
            }
            switch (ReadResult) {
                case EReadResult::Success:
                    return std::move(Req);
                case EReadResult::Timeout:
                    return {};
                default:
                    throw std::logic_error{"Coroutine was resumed incorrect"};
            }
        }

    private:
        THttpRequest Req;
        std::shared_ptr<TConnectionData> Connection;
        sys::error_code Error;
        EReadResult ReadResult;
    };

    class TWriteAwaiter {
    public:
        TWriteAwaiter(std::shared_ptr<TConnectionData> connection, THttpResponse& resp)
            : Connection(std::move(connection))
            , Error{}
            , Resp{&resp}
        {}

        bool await_ready() {
            return false;
        }

        void await_suspend(std::coroutine_handle<> coro) {
            auto wrHandler = [this, conn = Connection, coro](sys::error_code error, std::size_t) {
                if (error) [[unlikely]] {
                    if (error == io::error::operation_aborted) {
                        return;
                    }
                    Error = error;
                }
                coro.resume();
            };
            bst::http::async_write(Connection->Socket, *Resp, std::move(wrHandler));
        }

        void await_resume() {
            if (Error) {
                throw sys::system_error(Error);
            }
        }

    private:
        std::shared_ptr<TConnectionData> Connection;
        sys::error_code Error;
        THttpResponse* Resp;
    };

public:
    explicit TConnectionData(io::ip::tcp::socket socket)
        : Timer(socket.get_executor())
        , Socket(std::move(socket))
        , Buff{}
    {}

    auto AsyncRead() {
        return TReadAwaiter{shared_from_this()};
    }

    auto AsyncWrite(THttpResponse& resp) {
        return TWriteAwaiter{shared_from_this(), resp};
    }

    void SetReadTimeout(std::chrono::seconds time) {
        Timer.expires_from_now(boost::posix_time::seconds{time.count()});
    }

    void Close() {
        Socket.close();
    }

private:
    io::deadline_timer Timer;
    io::ip::tcp::socket Socket;
    bst::flat_buffer Buff;
};

class TConnection {
public:
    explicit TConnection(io::ip::tcp::socket socket)
        : Data(std::make_shared<TConnectionData>(std::move(socket)))
    {}

    ~TConnection() {
        Data->Close();
    }

    auto AsyncRead() {
        return Data->AsyncRead();
    }

    auto AsyncWrite(THttpResponse& resp) {
        return Data->AsyncWrite(resp);
    }

    void SetReadTimeout(std::chrono::seconds time) {
        Data->SetReadTimeout(time);
    }

private:
    std::shared_ptr<TConnectionData> Data;
};
