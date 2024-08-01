#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>


namespace libvirt
{

namespace domain 
{

using rank_t   = std::size_t;
using domain_t = std::shared_ptr<libvirt::virDomain>;
using list_t   = std::vector<domain_t>;

[[maybe_unused]]
status_code list
(
    const connection_t &connection,
          list_t       &domain_list
) noexcept;

[[nodiscard("Collection period set action must be checked")]]
status_code set_collection_period
(
          list_t                    &domain_list, 
    const std::chrono::milliseconds &interval
) noexcept;


typedef struct
{
    util::stat::slong_t balloon_memory_used;
    util::stat::slong_t domain_memory_extra;
    util::stat::slong_t domain_memory_limit;
    std::double_t       domain_memory_delta;
} datum_t;

using data_t = std::vector<datum_t>;

[[maybe_unused]]
status_code ranking
(
    list_t &domain_list,
    data_t &domain_data
) noexcept;

constexpr std::size_t memory_statistic_balloon_used
    = static_cast<std::size_t>(VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON);
constexpr flag_code memory_statistic_domain_extra
    = static_cast<flag_code>(VIR_DOMAIN_MEMORY_STAT_UNUSED);
constexpr flag_code number_of_domain_memory_statistics 
    = static_cast<flag_code>(VIR_DOMAIN_MEMORY_STAT_NR);

using memory_statistics_t = std::array
<
    virDomainMemoryStatStruct, 
    number_of_domain_memory_statistics
>;

} // domain namespace

} // libvirt namespace
