#pragma once

#include <limits.h>


/**
 *  @brief C System call wrapper
 *
 *  @details Wraps signal.h system call header in namespace as
 *  well as providing addtional flags used in catching system 
 *  interrupts which are custom defined as well
 */
namespace os
{

namespace signal
{

#include <signal.h>

// Singal type compatible with system call 
using signal_t = int;

// Additional flags based on system integer size
#if INT_MAX == 0x7F
constexpr signal_t SIG_DEF = 0x00;
constexpr signal_t SIG_EXT = 0x01;

#elif INT_MAX == 0x7FFF
constexpr signal_t SIG_DEF = 0x0000;
constexpr signal_t SIG_EXT = 0x0001;

#elif INT_MAX == 0x7FFFFFFF
constexpr signal_t SIG_DEF = 0x00000000;
constexpr signal_t SIG_EXT = 0x00000001;

#elif INT_MAX >= 0x7FFFFFFFFFFFFFFF
constexpr signal_t SIG_DEF = 0x0000000000000000;
constexpr signal_t SIG_EXT = 0x0000000000000001;
#endif

constexpr signal_t SIG_INT = SIGINT;

} // signal namespace

} // os namespace
