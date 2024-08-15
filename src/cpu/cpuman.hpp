#pragma once

#include <lib/libvirt.hpp>

#include "sys/scheduler.hpp"


namespace manager
{

[[nodiscard("Load balancer exit status must be checked")]]
static
status_code 
load_balancer
(
    const libvirt::connection_t &connection
) noexcept;

} // manager namespace
