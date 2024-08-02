#pragma once

#include <functional>
#include <memory>

namespace libvirt
{

#include "libvirt/libvirt.h"

using status_code = int;
using flag_code   = int;

constexpr flag_code FLAG_NULL = 0;

using connection_t = std::unique_ptr
<
    virConnect, 
    std::function<void (virConnect *)>
>;

} // libvirt namespace
