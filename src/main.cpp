#include "burstline/application.hpp"

#include <cstddef>
#include <span>

int main(const int argc, char* argv[]) {
    return burstline::Application{}.run(
        std::span<char* const>{argv, static_cast<std::size_t>(argc)});
}
