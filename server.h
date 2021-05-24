#pragma once

#include "accept_connection.h"

class TServer {
public:
    explicit TServer(io::io_context& ctx, unsigned short port)
        : Acceptor{ctx, io::ip::tcp::endpoint{io::ip::tcp::v4(), port}}
        , Signals(ctx, SIGINT, SIGTERM)
        , Context{&ctx}
    {
        ScheduleTerm();
        ScheduleAccept();
        Print("Server started");
    }

    ~TServer() {
        Print("Server stopped");
    }

private:
    void ScheduleAccept() {
        auto handler = [this](sys::error_code err, io::ip::tcp::socket socket){
            Accept(err, std::move(socket));
        };
        Acceptor.async_accept(std::move(handler));
    }

    void ScheduleTerm() {
        auto handler = [this](sys::error_code err, int signal_number) {
            Print("Signal received ", signal_number);
            if (!err) {
                Context->stop();
            }
        };
        Signals.async_wait(handler);
    }

    void Accept(const sys::error_code& err, io::ip::tcp::socket socket) {
        if (!err) {
            AcceptConnection(std::move(socket));
        }
        ScheduleAccept();
    }

private:
    io::ip::tcp::acceptor Acceptor;
    boost::asio::signal_set Signals;
    io::io_context* Context;
};
