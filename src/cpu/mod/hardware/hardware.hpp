#pragma once

#include <cstddef>
#include <memory>

#include <lib/libvirt.hpp>
#include <stat/statistics.hpp>

#include "vcpu/vcpu.hpp"


namespace libvirt
{

namespace hardware
{

using node_t = std::unique_ptr<virNodeInfo>;

status_code
node_count
(
    const connection_t &connection,
          std::size_t  &number_of_pCPUs
) noexcept;

using byte_t = unsigned char;
using mapping_t = std::unique_ptr<byte_t>;

status_code
map
(
    const vCPU::datum_t &datum,
    const std::size_t   &number_of_pCPUs
) noexcept;

} // hardware namespace

} // libvirt namespace
