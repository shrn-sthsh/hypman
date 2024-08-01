#pragma once

#include <stat/statistics.hpp>

#include "domain/domain.hpp"


namespace manager 
{

using status_code = int;

[[nodiscard("Scheduler exit status must be checked")]]
status_code scheduler
(
    libvirt::domain::list_t &domain_list,
    libvirt::domain::data_t &domain_data,
    util::stat::slong_t      hardware_memory_limit
);

} // manager namespace
