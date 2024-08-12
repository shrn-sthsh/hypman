#pragma once

#include <cstdint>
#include <string>


namespace util
{

namespace log
{

enum class type: std::uint32_t
{
    STATUS = 0x00,
    ERROR  = 0x01,
    START  = 0x02,
    STOP   = 0x04,
    FLAG   = 0x08,
    ABORT  = 0x10 
};

constexpr bool FLUSH = true;
constexpr bool ASYNC = true;

void
record
(
    const std::string &&message,
    const type          type  = type::STATUS,
    const bool          flush = ASYNC
) noexcept;

constexpr char SPACE[]    = " ";
constexpr char NEW_LINE[] = "\n";

} // log namespace

namespace clock
{

using time_t = std::string;

time_t 
time() noexcept;

} // clock namespace

} // util namespace
