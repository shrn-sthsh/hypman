#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace libvirt
{

#include "libvirt/libvirt.h"

using status_code = std::uint8_t;
using flag_code   = std::uint8_t;

constexpr flag_code FLAG_NULL = 0;

using connection_t = std::unique_ptr
<
    virConnect, 
    std::function<void (virConnect *)>
>;

} // libvirt namespace
