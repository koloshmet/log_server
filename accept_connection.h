#pragma once

#include "task.h"
#include "connection.h"

#include "print.h"

inline TTask AcceptConnection(io::ip::tcp::socket socket) {
    using namespace std::literals;

    TConnection connection(std::move(socket));
    Print("Connection started");
    try {
        while (true) {
            connection.SetReadTimeout(5s);
            std::optional<THttpRequest> request = co_await connection.AsyncRead();
            if (!request) {
                Print("Timeout expired");
                break;
            }
            Print("Request has been read\n", *request);

            THttpResponse response;
            response.version(request->version());
            response.result(bst::http::status::ok);
            co_await connection.AsyncWrite(response);
            Print("Response has been written");

            if (!request->keep_alive()) {
                break;
            }
        }
    } catch (const std::exception& error) {
        Print("Error: ", error.what());
    }

    Print("Connection closed");
    co_return;
}
