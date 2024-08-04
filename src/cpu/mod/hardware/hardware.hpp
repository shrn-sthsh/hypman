#pragma once

#include <memory>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>


namespace libvirt
{

namespace hardware
{

using node_t = std::unique_ptr<virNodeInfo>;

status_code node_count
(
    const connection_t       &connection,
          util::stat::uint_t &number_of_pCPUs
) noexcept;

} // hardware namespace

} // libvirt namespace
