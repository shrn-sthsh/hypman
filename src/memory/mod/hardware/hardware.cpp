#include <cstdlib>

#include <lib/libvirt.hpp>
#include <log/record.hpp>
#include <stat/statistics.hpp>

#include "hardware.hpp"


/**
 *  @brief Hardware Memory Limit Determiner
 *
 *  @param connection:   hypervisor connection via libvirt
 *  @param memory limit: variable reference to write to
 *
 *  @details Determine hardware memory limit of system for scheduler's use
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::hardware::memory_limit
(
    const connection_t        &connection, 
          util::stat::slong_t &memory_limit
) noexcept
{
    libvirt::status_code status;

    // Get number of memory statistics
    util::stat::sint_t number_of_node_memory_statistics = 0;
    libvirt::virNodeGetMemoryStats
    (
        connection.get(), 
        VIR_NODE_MEMORY_STATS_ALL_CELLS, 
        nullptr, 
        &number_of_node_memory_statistics,
        libvirt::FLAG_DEF
    );
    if (number_of_node_memory_statistics < 1)
    {
        util::log::record
        (
            "Unable to retrieve number of hardware memory statistics "
            "through libvirt API",
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }
	
	// Get hardware memory statistics
    libvirt::hardware::memory_statistics_t memory_statistics
    (
        number_of_node_memory_statistics
    );
    status = libvirt::virNodeGetMemoryStats
    (
        connection.get(), 
        VIR_NODE_MEMORY_STATS_ALL_CELLS, 
        memory_statistics.data(), 
        &number_of_node_memory_statistics, 
        libvirt::FLAG_DEF
    );
    if (number_of_node_memory_statistics < 1)
    {
        util::log::record
        (
            "Unable to retrieve hardware memory statistics through libvirt API",
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }
    
    // Look through returned resulted and save hardware memory limit
    bool memory_limit_found = false;
    for (const auto &[field, value]: memory_statistics)
    {
        memory_limit_found = static_cast<bool>
        (
            std::string(field).compare(node_memory_statistics_total)
        );
        if (memory_limit_found)
            memory_limit = static_cast<util::stat::slong_t>(value);
    }
    if (!memory_limit)
    {
        util::log::record
        (
            "Unable to retrieve hardware memory limit",
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
