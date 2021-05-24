#pragma once

#include <boost/date_time.hpp>
#include <iostream>

template <typename... TArgs>
concept CPrintable = requires(TArgs&&... args) {
    { (std::cout << ... << std::forward<TArgs>(args)) };
};

template <CPrintable... TArgs>
void Print(std::string_view s, TArgs&&... args) {
    std::cout << '[' << to_simple_string(boost::posix_time::microsec_clock::local_time()) << ']';
    std::cout << ' ' << s;
    (std::cout << ... << std::forward<TArgs>(args)) << std::endl;
}
