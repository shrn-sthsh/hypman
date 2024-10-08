#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>


/**
 *  @brief Domain Utility Header
 *
 *  @details Defines routines to pull domain memory data
 */
namespace libvirt
{

namespace domain 
{

// Domain data constants
static constexpr util::stat::uint_t 
domains_active_running_flag = static_cast<util::stat::uint_t>
(
    VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING
);

static constexpr std::size_t 
uuid_length = static_cast<std::size_t>(VIR_UUID_STRING_BUFLEN);

static constexpr util::stat::uint_t 
domain_affect_current_flag
    = static_cast<util::stat::uint_t>(VIR_DOMAIN_AFFECT_CURRENT);

// Memory statistics constants
static constexpr std::size_t 
memory_statistic_balloon_used
    = static_cast<std::size_t>(VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON);

static constexpr flag_code 
memory_statistic_domain_extra
    = static_cast<flag_code>(VIR_DOMAIN_MEMORY_STAT_UNUSED);

static constexpr flag_code 
number_of_domain_memory_statistics 
    = static_cast<flag_code>(VIR_DOMAIN_MEMORY_STAT_NR);

// data types and structure types
using rank_t   = std::size_t;
using domain_t = std::unique_ptr
<
    virDomain,
    std::function<void (virDomain *)>
>;

using uuid_t     = std::string;
using uuid_set_t = std::unordered_set<uuid_t>;
using table_t    = std::unordered_map<uuid_t, domain_t>;

typedef struct datum_t
{
    // Default constructor 
    explicit
    datum_t();

    // Move constructor
    datum_t(datum_t &&other) noexcept;

    // Move assignment 
    datum_t &operator=(datum_t &&other) noexcept; 

    // Delete copy constructor and copy assignment
    datum_t(const datum_t &datum)            = delete;
    datum_t &operator=(const datum_t &datum) = delete;

    // Fields
    uuid_t              uuid;
    domain_t            domain;
    std::size_t         number_of_vCPUs;
    util::stat::slong_t balloon_memory_used;
    util::stat::slong_t domain_memory_extra;
    util::stat::slong_t domain_memory_limit;
    std::double_t       domain_memory_delta;
} datum_t;

using data_t = std::vector<datum_t>;

using memory_statistics_t = std::array
<
    virDomainMemoryStatStruct, 
    number_of_domain_memory_statistics
>;

// Structure creation routines
[[maybe_unused]]
status_code
table
(
    const connection_t &connection,
          table_t      &table
) noexcept;

status_code
domain_uuids
(
    table_t    &domain_table,
    uuid_set_t &domain_uuids
) noexcept;

[[maybe_unused]]
status_code
data
(
    table_t &domain_table,
    data_t  &domain_data
) noexcept;

// State modfier rountines
[[nodiscard("Collection period set action must be checked")]]
status_code
set_collection_period
(
          table_t                   &curr_domain_table,
          uuid_set_t                &prev_domain_uuids,
    const std::chrono::milliseconds &interval
) noexcept;

} // domain namespace

} // libvirt namespace
