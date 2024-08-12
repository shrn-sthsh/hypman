#pragma once

#include <cstdint>
#include <functional>
#include <memory>


/**
 *  @brief Libvirt C library namespace wrapper
 *
 *  @details Wraps libvirt -- a platform virtualization management 
 *  tool -- in a namespace for seperation of names as well as space
 *  for virtualization related custom subroutines
 */
namespace libvirt
{

#include "libvirt/libvirt.h"

// Codes used within subroutines or to report on subroutines
using status_code = std::uint8_t;
using flag_code   = std::uint8_t;

constexpr flag_code FLAG_DEF = 0;

// RAII connection pointer
using connection_t = std::unique_ptr
<
    virConnect, 
    std::function<void (virConnect *)>
>;

} // libvirt namespace
