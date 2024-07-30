#pragma once


namespace os
{

namespace signal
{

#include <signal.h>

using signal_t = int;

constexpr signal_t SIG_NULL = 0x0000;
constexpr signal_t SIG_DEF  = 0x0001;
constexpr signal_t SIG_INT  = SIGINT;

} // signal namespace

} // os namespace
