#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <string>
#include <vector>

#include <lib/libvirt.hpp>

#include "domain/domain.hpp"


/**
 *  @brief vCPU Utility Header
 *
 *  @details Defines routines to pull vCPU details
 */
namespace libvirt 
{

namespace pCPU 
{

// data type redefined
using rank_t = std::size_t;

} // pCPU namespace

namespace vCPU 
{

// data and structure types
using rank_t  = std::size_t;
using list_t  = std::vector<virVcpuInfo>;
using table_t = std::unordered_map<domain::uuid_t, list_t>;

using uuid_set_t = std::unordered_set<domain::uuid_t>;
using table_diff_t = std::pair<bool, uuid_set_t>;

typedef struct datum_t
{ 
    explicit
    datum_t
    (
        rank_t                 vCPU_rank,
        pCPU::rank_t           pCPU_rank,
        domain::uuid_t         domain_uuid,
        domain::domain_t     &&domain,
        util::stat::ulong_t    usage_time
    ) noexcept;

    rank_t               vCPU_rank;
    pCPU::rank_t         pCPU_rank;
    domain::uuid_t       domain_uuid;
    domain::domain_t     domain;
    util::stat::ulong_t  usage_time;
} datum_t;

using data_t = std::vector<datum_t>;

// Structure creation routines
[[maybe_unused]]
status_code
table
(
    const domain::table_t &domain_table,
          vCPU::table_t   &vCPU_table 
) noexcept;

[[maybe_unused]]
status_code
data
(
    const table_t         &curr_vCPU_table,
    const table_t         &prev_vCPU_table,
    const uuid_set_t      &vPCU_table_diff,
          domain::table_t &curr_domain_table,
          data_t          &curr_vCPU_data
) noexcept;

// Change checking routines
[[nodiscard("Must use differences return to call")]]
table_diff_t
comparable_state
(
    const table_t &curr_vCPU_table,
    const table_t &prev_vCPU_table
) noexcept;

} // vCPU namespace

} // libvirt namespace
