#include <cstdlib>

#include <log/record.hpp>
#include <stat/statistics.hpp>

#include "hardware.hpp"


libvirt::status_code libvirt::hardware::node_count
(
    const connection_t       &connection,
          util::stat::uint_t &number_of_pCPUs
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
    }

    // Get number of pCPUs in hardware
    number_of_pCPUs = node.get()->cpus;
    
    return EXIT_SUCCESS;
}
