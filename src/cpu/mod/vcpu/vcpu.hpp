#pragma once

#include <vector>

#include <lib/libvirt.hpp>

#include "domain/domain.hpp"

namespace libvirt 
{

namespace vCPU 
{

using list_t  = std::vector<virVcpuInfo>;
using table_t = std::vector<list_t>;

status_code table
(
    const domain::list_t &domain_list,
          vCPU::table_t  &vCPU_table 
) noexcept;

}

}
