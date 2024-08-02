#pragma once

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
    virDomain,
    std::function<void (virDomain *)>
>;
using list_t   = std::vector<domain_t>;

[[maybe_unused]]
status_code list
(
    const connection_t &connection,
          list_t       &domain_list
) noexcept;

} // domain namespace

} // libvirt namespace
