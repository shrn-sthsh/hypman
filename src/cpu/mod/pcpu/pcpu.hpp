#pragma once

#include <cstddef>
#include <vector>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>

#include "vcpu/vcpu.hpp"


/**
 *  @brief pCPU Utility Header
 *
 *  @details Defines routines to pull pCPU details
 */
namespace libvirt
{

namespace pCPU 
{

// data and structure types
using rank_t = std::size_t;

typedef struct datum_t
{ 
    rank_t              pCPU_rank;
    util::stat::ulong_t usage_time;
    std::size_t         number_of_vCPUs;
} datum_t;

using data_t = std::vector<datum_t>;

// Structure creation routines
[[maybe_unused]]
status_code
data
(
    const connection_t  &connection,
    const vCPU::data_t  &vCPU_data,
          data_t        &pCPU_data
) noexcept;

namespace stat
{

// data types
using statistics_t = std::pair<std::double_t, std::double_t>;

// Statiscial calculation routines
[[nodiscard("Must use statistics calculated to call")]]
statistics_t
mean_and_deviation
(
    const data_t &data
) noexcept;

} // stat namespace

} // pCPU namespace

} // libvirt namespace
  
