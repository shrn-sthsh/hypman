#pragma once

#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>

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

// data types and structure types
using rank_t = std::size_t;
using uuid_t = std::string;

using domain_t = std::unique_ptr
<
    virDomain,
    std::function<void (virDomain *)>
>;
using table_t = std::unordered_map
<
    uuid_t, 
    domain_t 
>;

// Structure creation routines
[[maybe_unused]]
status_code
table
(
    const connection_t &connection,
          table_t      &domain_table
) noexcept;

} // domain namespace

} // libvirt namespace
