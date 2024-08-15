#pragma once

#include <string>
#include <vector>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>


/**
 *  @brief Hardware Utility Header
 *
 *  @details Defines routines to pull hardware details
 */
namespace libvirt 
{

namespace hardware
{

// Retrieve hardware memory limit
[[maybe_unused]]
status_code
memory_limit
(
    const connection_t        &connection,
          util::stat::slong_t &memory_limit
) noexcept;

// Statistics definitions
using memory_statistics_t = std::vector<virNodeMemoryStats>;

static const std::string 
node_memory_statistics_total = std::string(VIR_NODE_MEMORY_STATS_TOTAL);

static const util::stat::sint_t 
node_memory_all_statistics
    = static_cast<util::stat::sint_t>(VIR_NODE_MEMORY_STATS_ALL_CELLS);

} // hardware namespace

} // libvirt namespaceA
