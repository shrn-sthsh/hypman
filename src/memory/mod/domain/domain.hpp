#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>


namespace libvirt
{

namespace domain 
{

using rank_t   = std::size_t;
using domain_t = std::unique_ptr
<
    libvirt::virDomain,
    std::function<void (virDomain *)>
>;
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


typedef struct datum_t
{
    // Default constructor 
    datum_t():
        domain(nullptr),
        balloon_memory_used(0),
        domain_memory_extra(0),
        domain_memory_limit(0),
        domain_memory_delta(0.0) 
    {}

    // Parameterized constructor
    datum_t
    (
        domain_t            domain, 
        util::stat::slong_t balloon_memory_used,
        util::stat::slong_t domain_memory_extra,
        util::stat::slong_t domain_memory_limit,
        std::double_t       domain_memory_delta
    ):
        domain(std::move(domain)), 
        balloon_memory_used(balloon_memory_used),
        domain_memory_extra(domain_memory_extra),
        domain_memory_limit(domain_memory_limit),
        domain_memory_delta(domain_memory_delta) 
    {}

    // Move constructor
    datum_t(datum_t &&other) noexcept:
        domain(std::move(other.domain)), 
        balloon_memory_used(other.balloon_memory_used),
        domain_memory_extra(other.domain_memory_extra),
        domain_memory_limit(other.domain_memory_limit),
        domain_memory_delta(other.domain_memory_delta)
    {}

    // Move assignment
    datum_t &operator=(datum_t &&other) noexcept
    {
        if (this != &other)
            this->domain = std::move(other.domain);

        return *this;
    }

    // Delete copy constructor and copy assignment
    datum_t(const datum_t &datum)            = delete;
    datum_t &operator=(const datum_t &datum) = delete;

    // Fields
    rank_t              rank;
    domain_t            domain;
    util::stat::slong_t balloon_memory_used;
    util::stat::slong_t domain_memory_extra;
    util::stat::slong_t domain_memory_limit;
    std::double_t       domain_memory_delta;
} datum_t;

using data_t = std::vector<datum_t>;

[[maybe_unused]]
status_code data
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
