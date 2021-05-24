#include "server.h"

#include <charconv>

namespace {
    unsigned short ExtractPort(std::string_view portArg) {
        unsigned short res;
        std::from_chars(std::to_address(portArg.begin()), std::to_address(portArg.end()), res);
        return res;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "log_server <port>" << std::endl;
        return 0;
    }
    io::io_context context;
    TServer server{context, ExtractPort(argv[1])};

    context.run();

    return 0;
}
