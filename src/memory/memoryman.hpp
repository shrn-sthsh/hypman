#pragma once

#include <chrono>
#include <memory>

#include <lib/libvirt.hpp>


namespace libvirt 
{

using connection_t = std::shared_ptr<virConnect>;

} // libvirt namespace

namespace manager
{

using status_code = int;

status_code load_balancer
(
    const libvirt::connection_t     &conn,
    const std::chrono::milliseconds  interval
) noexcept;

} // manager namespace
