#include <cstdlib>

#include <log/record.hpp>
#include <stat/statistics.hpp>
#include <string>

#include "hardware.hpp"


libvirt::status_code libvirt::hardware::node_count
(
    const connection_t &connection,
          std::size_t  &number_of_pCPUs
) noexcept
{
    status_code status;

    libvirt::hardware::node_t node;
    status = libvirt::virNodeGetInfo
    (
        connection.get(),
        node.get()
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to retreive hardware information",
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    // Get number of pCPUs in hardware
    number_of_pCPUs = node.get()->cpus;
    
    return EXIT_SUCCESS;
}

libvirt::status_code libvirt::hardware::remap
(
    const vCPU::datum_t &datum,
    const std::size_t   &number_of_pCPUs
) noexcept
{
    libvirt::status_code status;

    // Create mapping
    libvirt::hardware::mapping_t mapping;
    VIR_USE_CPU(mapping.get(), datum.pCPU_rank);

    // Execute mapping
    status = libvirt::virDomainPinVcpu
    (
        datum.domain.get(),
        datum.vCPU_rank,
        mapping.get(), 
        VIR_CPU_MAPLEN(number_of_pCPUs)
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to map vCPU " + std::to_string(datum.vCPU_rank)
                + " on domain " + datum.domain_id
                + " to pCPU " + std::to_string(datum.pCPU_rank),
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
