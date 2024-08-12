#pragma once

#include <chrono>
#include <cstdint>

#include <lib/libvirt.hpp>


namespace manager
{

using status_code = std::uint8_t;

[[nodiscard("Load balancer exit status must be checked")]]
status_code 
load_balancer
(
    const libvirt::connection_t     &connection,
    const std::chrono::milliseconds &interval
) noexcept;

} // manager namespace
