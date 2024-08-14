#pragma once

#include <cstddef>
#include <vector>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>

#include "vcpu/vcpu.hpp"


namespace libvirt
{

namespace pCPU 
{

using rank_t = std::size_t;

typedef struct datum_t
{ 
    rank_t              pCPU_rank;
    util::stat::ulong_t usage_time;
    std::size_t         number_of_vCPUs;
} datum_t;

using data_t = std::vector<datum_t>;

status_code
data
(
    const connection_t  &connection,
    const vCPU::data_t  &vCPU_data,
          data_t        &pCPU_data
) noexcept;

namespace stat
{

using statistics_t = std::pair<std::double_t, std::double_t>;

statistics_t
mean_and_deviation
(
    const data_t &data
) noexcept;

} // stat namespace

} // pCPU namespace

} // libvirt namespace
  
