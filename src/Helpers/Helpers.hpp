#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <random>

using std::shared_ptr;
using std::make_shared;
using std::vector;
typedef unsigned char BYTE;
float random_float();

template <typename... Args>
std::size_t combine_hashes(const Args&... args) noexcept {
    std::size_t seed = 0;
    ((seed ^= std::hash<Args>{}(args) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2)), ...);
    return seed;
}