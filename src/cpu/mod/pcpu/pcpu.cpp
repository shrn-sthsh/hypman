#include <log/record.hpp>

#include "hardware/hardware.hpp"
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
