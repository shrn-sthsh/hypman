#include <cstdlib>

#include <lib/libvirt.hpp>

#include "hardware.hpp"
#include "stat/statistics.hpp"


libvirt::status_code libvirt::hardware::hardware_memory_limit
(
    const connection_t        &connection, 
          util::stat::slong_t &hardware_memory_limit
) noexcept
{
    libvirt::status_code status;

    // Get number of memory statistics
    int number_of_node_memory_statisitcs = 0;
    libvirt::virNodeGetMemoryStats
    (
        connection.get(), 
        VIR_NODE_MEMORY_STATS_ALL_CELLS, 
        nullptr, 
        &number_of_node_memory_statisitcs,
        libvirt::FLAG_NULL
    );
    if (number_of_node_memory_statisitcs < 1)
    {
        util::log::record
        (
            "Unable to retrieve number of hardware memory statistics "
            "through libvirt API",
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }
	
	// Get hardware memory statistics
    libvirt::hardware::memory_statistics_t memory_statistics
    (
        number_of_node_memory_statisitcs
    );
    status = libvirt::virNodeGetMemoryStats
    (
        connection.get(), 
        VIR_NODE_MEMORY_STATS_ALL_CELLS, 
        memory_statistics.data(), 
        &number_of_node_memory_statisitcs, 
        libvirt::FLAG_NULL
    );
    if (number_of_node_memory_statisitcs < 1)
    {
        util::log::record
        (
            "Unable to retrieve hardware memory statistics through libvirt API",
            util::log::ERROR
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
            hardware_memory_limit = static_cast<util::stat::slong_t>(value);
    }
    if (!hardware_memory_limit)
    {
        util::log::record
        (
            "Unable to retrieve hardware memory limit",
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
