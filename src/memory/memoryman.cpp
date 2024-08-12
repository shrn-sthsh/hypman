#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

#include <lib/signal.hpp>
#include <log/record.hpp>

#include "domain/domain.hpp"
#include "hardware/hardware.hpp"
#include "sys/scheduler.hpp"

#include "memoryman.hpp"


static libvirt::domain::uuid_set_t prev_domain_uuids;
static util::stat::ulong_t      balancer_iteration = 1;

int
main 
(
    int argc, 
    char *argv[]
) 
{
    /* Validate command argument */
    // Must have a single argument -- an interval
    if (argc != 2)
    {
        util::log::record
        (
            "Usage follows as ./memoryman <interval (ms)>", 
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    } 
   
    // Interval argument must be a positive integer
    std::function<bool(const char *)> is_interval = [](const char *string) 
    {
        return std::all_of
        (
            string, string + std::strlen(string), 
            [](unsigned char character) 
            {
                return std::isdigit(character);
            }
        );
    };
    if (!is_interval(argv[1]))
    {
        util::log::record
        (
            "Interval argument must be a positive integer", 
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }
    std::chrono::milliseconds interval(std::atoi(argv[1]));
    

    /* Make connection to virtualization host using libvirt */
    libvirt::connection_t connection
    (
        libvirt::virConnectOpen("qemu:///system"),
        [](libvirt::virConnect *connection)
        {
            if (connection != nullptr)
                libvirt::virConnectClose(connection);
        }
    );
    if (connection == nullptr)
    {
        util::log::record
        (
            "Unable to make connection to QEMU", 
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }


    /* Assign a hangler for system interrupts */
    static os::signal::signal_t exit_signal = os::signal::SIG_DEF;
    os::signal::signal
    (
        os::signal::SIG_INT,
        [](os::signal::signal_t interrupt)
        {
            exit_signal = os::signal::SIG_EXT;
        }
    );

    /* Run memory load balancer at every interval */
    while (!static_cast<bool>(exit_signal))
    {
        // Call load_balancer
        manager::status_code status 
            = manager::load_balancer(connection, interval);

        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Scheduler exited on terminating error after "
                    + std::to_string(balancer_iteration) 
                    + " iterations", 
                util::log::type::ABORT
            );

            return EXIT_FAILURE;
        }
        
        // Sleep until next interval
        std::this_thread::sleep_for(interval);
        ++balancer_iteration;
    }

    return EXIT_SUCCESS;
}


manager::status_code
manager::load_balancer
(
    const libvirt::connection_t     &connection, 
    const std::chrono::milliseconds  interval
) noexcept
{
    libvirt::status_code status;

    /*************************** DOMAIN INFORMATION ***************************/

    // Get list of domains
    libvirt::domain::table_t curr_domain_table;
    status = libvirt::domain::table
    (
        connection, 
        curr_domain_table
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to retrieve data structure for domains",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }

    // Set statistics collection period if not set
    status = libvirt::domain::set_collection_period
    (
        curr_domain_table, 
        prev_domain_uuids,
        interval
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to set statistics period for domains",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }

    // Save current domain ids
    status = libvirt::domain::domain_uuids
    (
        curr_domain_table, 
        prev_domain_uuids
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable save current domain ids",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }
    
    // Get memory statistics for each domain and save table
    libvirt::domain::data_t curr_domain_data;
    curr_domain_data.reserve(curr_domain_table.size());
    status = libvirt::domain::data
    (
        curr_domain_table,
        curr_domain_data
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to retrieve memory statistics for domains",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }


    /*************************** SYSTEM INFORMATION ***************************/
    
    // Get hardware memory statistics
    util::stat::slong_t system_memory_limit;
    status = libvirt::hardware::system_memory_limit
    (
        connection, 
        system_memory_limit
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to retrieve hardware memory statistics",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }

    
    /*********************** MEMORY MOVEMENT SCHEDULER ************************/
    
    // Run scheduler to determine domains' memory sizes and execute reallocation
    status = manager::scheduler
    (
        curr_domain_data, 
        system_memory_limit
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Fault incurred in scheduler processing",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
