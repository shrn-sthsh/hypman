#pragma once

#include <string>


namespace util
{

namespace log
{

enum message_type
{
    STATUS = 0x0000,
    ERROR  = 0x0001,
    START  = 0x0002,
    STOP   = 0x0004,
    FLAG   = 0x0008,
    ABORT  = 0x0010 
};

constexpr bool FLUSH = true;
constexpr bool ASYNC = true;

void
record
(
    const std::string &&message,
    const message_type  type = STATUS,
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
