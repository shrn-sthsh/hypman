#pragma once

#include <cmath>
#include <cstdint>

#include "pcpu/pcpu.hpp"


/**
 *  @brief pCPU Scheduler Header
 *
 *  @details Defines scheduler's rountine
 */
namespace manager
{

using status_code = std::uint8_t;

[[nodiscard("Scheduler exit status must be checked")]]
status_code
scheduler
(
    libvirt::vCPU::data_t &curr_vCPU_data, 
    libvirt::pCPU::data_t &curr_pCPU_data
) noexcept;

static constexpr std::double_t DISPERSION_UPPER_BOUND = 0.115;
static constexpr std::double_t DISPERSION_LOWER_BOUND = 0.075;

[[nodiscard("Must use prediction result to call")]]
static 
bool
analyze_prediction
(
    const libvirt::pCPU::data_t &curr_data,
    const libvirt::pCPU::data_t &pred_data
) noexcept;

} // manager namespace
