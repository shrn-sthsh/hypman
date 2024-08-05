#pragma once

#include <cmath>
#include <cstdint>

#include "pcpu/pcpu.hpp"
#include "vcpu/vcpu.hpp"


namespace manager
{

using status_code = std::uint8_t;

status_code scheduler
(
    libvirt::vCPU::data_t &curr_vCPU_data, 
    libvirt::pCPU::data_t &curr_pCPU_data
) noexcept;

constexpr std::double_t DISPERSION_UPPER_BOUND = 0.115;
constexpr std::double_t DISPERSION_LOWER_BOUND = 0.075;

bool analyze_prediction
(
    const libvirt::pCPU::data_t &curr_data,
    const libvirt::pCPU::data_t &pred_data
) noexcept;

} // manager namespace
