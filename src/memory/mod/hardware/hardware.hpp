#pragma once

#include <vector>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>


namespace libvirt 
{

namespace hardware
{

[[maybe_unused]]
status_code hardware_memory_limit
(
    const connection_t        &connection,
          util::stat::slong_t &hardware_memory_limit
) noexcept;

using memory_statistics_t = std::vector<virNodeMemoryStats>;
const std::string node_memory_statistics_total
    = std::string(VIR_NODE_MEMORY_STATS_TOTAL);

} // hardware namespace

} // libvirt namespaceA
