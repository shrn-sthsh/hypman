#pragma once

#include <cstddef>
#include <memory>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>

#include "vcpu/vcpu.hpp"


/**
 *  @brief Hardware Utility Header
 *
 *  @details Defines routines to pull hardware details
 */
namespace libvirt
{

namespace hardware
{

// data types
using node_t    = std::unique_ptr<virNodeInfo>;
using byte_t    = unsigned char;
using mapping_t = std::unique_ptr<byte_t>;

// Data collection routines
[[maybe_unused]]
status_code
node_count
(
    const connection_t &connection,
          std::size_t  &number_of_pCPUs
) noexcept;

// State modifer routines
[[maybe_unused]]
status_code
map
(
    const vCPU::datum_t &datum,
    const std::size_t   &number_of_pCPUs
) noexcept;

static inline 
void 
map_to_pCPU
(
    pCPU::rank_t  rank,
    mapping_t    &mapping
) noexcept;

[[nodiscard("Must use result length to call")]]
static inline 
util::stat::uint_t 
map_length
(
    std::size_t number_of_pCPUs
) noexcept;

} // hardware namespace

} // libvirt namespace
