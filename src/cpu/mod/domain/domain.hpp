#pragma once

#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>


namespace libvirt
{

namespace domain 
{

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

[[maybe_unused]]
status_code
table
(
    const connection_t &connection,
          table_t      &domain_table
) noexcept;

} // domain namespace

} // libvirt namespace
