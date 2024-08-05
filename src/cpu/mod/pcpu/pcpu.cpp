#include <cmath>
#include <cstddef>
#include <functional>
#include <log/record.hpp>
#include <numeric>

#include "hardware/hardware.hpp"
#include "stat/statistics.hpp"
#include "vcpu/vcpu.hpp"

#include "pcpu.hpp"


libvirt::status_code libvirt::pCPU::data
(
    const libvirt::connection_t  &connection,
          libvirt::vCPU::data_t  &vCPU_data, 
          libvirt::pCPU::data_t  &pCPU_data
) noexcept
{
    status_code status;

    // Validate vCPU data is filled
    if (vCPU_data.empty())
    {
        util::log::record
        (
            "vCPU::data_t is empty", 
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    // Get number of active pCPUs on system
    std::size_t number_of_pCPUs;
    status = libvirt::hardware::node_count
    (
        connection, 
        number_of_pCPUs
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to get number of pCPUs active in system",
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }
    pCPU_data.resize(number_of_pCPUs);

    // Set ranks for pCPUs
    for (libvirt::pCPU::rank_t rank = 0; rank < number_of_pCPUs; ++rank)
        pCPU_data[rank].pCPU_rank = rank;

    // Get pCPU usage times for every vCPU part of every domain
    for (const libvirt::vCPU::datum_t &vCPU_datum: vCPU_data)
    {
        // Get pCPU which this vCPU is pinned to
        const libvirt::pCPU::rank_t pCPU_rank 
            = static_cast<libvirt::pCPU::rank_t>(vCPU_datum.pCPU_rank);
        libvirt::pCPU::datum_t &pCPU_datum = pCPU_data[pCPU_rank];

        // Update pCPU statistics
        pCPU_datum.usage_time += vCPU_datum.usage_time;
        ++pCPU_datum.number_of_vCPUs;
    }

    return EXIT_SUCCESS;
}

libvirt::pCPU::stat::statistics_t libvirt::pCPU::stat::mean_and_deviation
(
    const data_t &data
) noexcept
{
    if (data.empty())
        return {0.0, 0.0};

    std::size_t number_of_pCPUs = data.size();

    // Compute mean
    util::stat::ulong_t sum = std::accumulate
    (
        data.begin(), data.end(), 0.0,
        [] (const util::stat::ulong_t sum, const libvirt::pCPU::datum_t &datum)
        {
            return sum + datum.usage_time;
        }
    );
    std::double_t mean = static_cast<std::double_t>(sum) / number_of_pCPUs;

    // Compute standard deviation
    std::double_t sum_of_squares = std::inner_product
    (
        data.begin(), data.end(), data.begin(), 0.0, std::plus<>(),
        [mean] 
        (
            const libvirt::pCPU::datum_t &datum, 
            const libvirt::pCPU::datum_t &
        )
        {
            std::double_t usage_time = static_cast<double>(datum.usage_time);
            return (usage_time - mean) * (usage_time - mean);
        }
    );
    std::double_t deviation = std::sqrt(sum_of_squares / number_of_pCPUs);

    return {mean, deviation};
}
