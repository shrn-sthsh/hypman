#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lib/libvirt.hpp>

#include "domain/domain.hpp"


namespace libvirt 
{

namespace pCPU 
{

using rank_t = std::size_t;

} // pCPU namespace

namespace vCPU 
{

using rank_t = std::size_t;
using list_t  = std::vector<virVcpuInfo>;
using table_t = std::unordered_map
<
    domain::uuid_t, 
    list_t
>;

status_code table
(
    const domain::table_t &domain_table,
          vCPU::table_t   &vCPU_table 
) noexcept;

using uuid_set_t = std::unordered_set<domain::uuid_t>;
using table_diff_t = std::pair<bool, uuid_set_t>;

table_diff_t comparable_state
(
    const table_t &curr_table,
    const table_t &prev_table
) noexcept;


typedef struct datum_t
{ 
    datum_t
    (
        rank_t                 vCPU_rank,
        pCPU::rank_t           pCPU_rank,
        domain::uuid_t         domain_id,
        domain::domain_t     &&domain,
        util::stat::ulong_t    usage_time
    ) noexcept: 
        vCPU_rank(vCPU_rank), 
        pCPU_rank(pCPU_rank), 
        domain_id(domain_id), 
        domain(std::move(domain)), 
        usage_time(usage_time)
    {}

    rank_t               vCPU_rank;
    pCPU::rank_t         pCPU_rank;
    domain::uuid_t       domain_id;
    domain::domain_t     domain;
    util::stat::ulong_t  usage_time;
} datum_t;

using data_t = std::vector<datum_t>;

status_code data
(
    const table_t         &curr_vCPU_table,
    const table_t         &prev_vCPU_table,
    const uuid_set_t      &vPCU_table_diff,
          domain::table_t &domain_table,
          data_t          &data 
) noexcept;

} // vCPU namespace

} // libvirt namespace
