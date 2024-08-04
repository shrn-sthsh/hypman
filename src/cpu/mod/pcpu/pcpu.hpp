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

status_code data
(
    const connection_t  &connection,
          vCPU::data_t  &vCPU_data,
          data_t        &pCPU_data
) noexcept;

} // pCPU namespace

} // libvirt namespace
