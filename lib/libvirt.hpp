#pragma once


#include "log/record.hpp"
#include <memory>

namespace libvirt
{

#include "libvirt/libvirt.h"

using status_code = int;
using flag_code   = int;

constexpr flag_code FLAG_NULL = 0;

using connection_t = std::shared_ptr<virConnect>;

} // libvirt namespace
