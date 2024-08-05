#pragma once

#include <chrono>

#include <lib/libvirt.hpp>

#include "sys/scheduler.hpp"


namespace manager
{

[[nodiscard("Load balancer exit status must be checked")]]
status_code load_balancer
(
    const libvirt::connection_t     &connection,
    const std::chrono::milliseconds  interval
) noexcept;

} // manager namespace
