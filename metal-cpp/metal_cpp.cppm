module;

#include <string>
#include <print>

export module lib;

export void print_hello() {
    std::println("hello, library_template");
}
