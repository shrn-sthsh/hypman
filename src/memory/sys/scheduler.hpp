#pragma once

#include <cstdint>

#include <stat/statistics.hpp>

#include "domain/domain.hpp"


namespace manager 
{

using status_code = std::uint8_t;

[[nodiscard("Scheduler exit status must be checked")]]
status_code
scheduler
(
    libvirt::domain::data_t &domain_data,
    util::stat::slong_t      hardware_memory_limit
);

} // manager namespace
