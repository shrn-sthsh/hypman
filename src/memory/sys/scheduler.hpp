#pragma once


#include "domain/domain.hpp"
#include "stat/statistics.hpp"

namespace manager 
{

using status_code = int;

status_code scheduler
(
    libvirt::domain::list_t &domain_list,
    libvirt::domain::data_t &domain_data,
    util::stat::slong_t      hardware_memory_limit
);

} // manager namespace
