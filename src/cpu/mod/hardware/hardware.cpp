#include <cstdlib>
#include <string>

#include <log/record.hpp>
#include <stat/statistics.hpp>

#include "hardware.hpp"


/**
 *  @brief Node Counter
 *
 *  @param connection:      hypervisor connection via libvirt
 *  @param number of pCPUs: variable reference to write to
 *
 *  @details Retreives number of active pCPUs in hardware
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::hardware::node_count
(
    const connection_t &connection,
          std::size_t  &number_of_pCPUs
) noexcept
{
    status_code status;

    // Get hardware node handle
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
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }

    // Get number of pCPUs in hardware
    number_of_pCPUs = node.get()->cpus;
    
    return EXIT_SUCCESS;
}


/**
 *  @brief vCPU to pCPU Mapper
 *
 *  @param vCPU datum:      vCPU data relavent to mapping to designated pCPU
 *  @param number of pCPUs: number of active pCPUs in hardware
 *
 *  @details Retreives number of active pCPUs in hardware
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::hardware::map
(
    const vCPU::datum_t &datum,
    const std::size_t   &number_of_pCPUs
) noexcept
{
    libvirt::status_code status;

    // Create mapping
    libvirt::hardware::mapping_t mapping;
    libvirt::hardware::map_to_pCPU
    (
        datum.pCPU_rank,
        mapping
    );

    // Execute mapping
    status = libvirt::virDomainPinVcpu
    (
        datum.domain.get(),
        datum.vCPU_rank,
        mapping.get(), 
        libvirt::hardware::map_length(number_of_pCPUs)
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to map vCPU " + std::to_string(datum.vCPU_rank)
                + " on domain " + datum.domain_uuid
                + " to pCPU " + std::to_string(datum.pCPU_rank),
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


/**
 *  @brief Set up Map for Specific pCPU to be Mapped
 *
 *  @param rank:    rank of pCPU to map
 *  @param mapping: variable reference to write to
 *
 *  @details Flips pCPU bit in provided map based on provied pCPU rank
 */
static inline 
void 
libvirt::hardware::map_to_pCPU
(
    libvirt::pCPU::rank_t         rank,
    libvirt::hardware::mapping_t &mapping
) noexcept
{
    mapping.get()[rank / 8] |= 1 << rank % 8;
}


/**
 *  @brief Get Mapping Length
 *
 *  @param number of pCPUs: number of active pCPUs in hardware
 *
 *  @details Calculates length of bitmap based on number of pCPUs
 *
 *  @return bit map length in bytes
 */
static inline
util::stat::uint_t 
libvirt::hardware::map_length
(
    std::size_t number_of_pCPUs
) noexcept
{
    return static_cast<util::stat::uint_t>((number_of_pCPUs + 7) / 8);
}
