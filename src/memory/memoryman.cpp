#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

#include <lib/signal.hpp>
#include <log/record.hpp>

#include "sys/scheduler.hpp"
#include "domain/domain.hpp"
#include "hardware/hardware.hpp"

#include "memoryman.hpp"


static os::signal::signal_t exit_signal  = os::signal::SIG_NULL;
static bool statistics_collection_period = false;
    
int main (int argc, char *argv[]) 
{
    // Validate command arguments
	if (argc != 2)
	{
        util::log::record
        (
            "Usage follows as memoryman <interval (ms)>", 
            util::log::ABORT
        );

		return EXIT_FAILURE;
	} 
    
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
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }
    std::chrono::milliseconds interval(std::atoi(argv[1]));
    
    // Connect to QEMU
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
            util::log::ABORT
        );

		return EXIT_FAILURE;
	}

    // Check for exit signal
    os::signal::signal
    (
        os::signal::SIG_INT,
        [](os::signal::signal_t interrupt)
        {
            exit_signal = os::signal::SIG_DEF; 
        }
    );

    // Run memory load balancer
	while (!static_cast<bool>(exit_signal))
	{
        manager::status_code status 
            = manager::load_balancer(connection, interval);

        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Scheduler exited on terminating error", 
                util::log::ABORT
            );

            return EXIT_FAILURE;
        }
        
        std::this_thread::sleep_for(interval);
	}

	return EXIT_SUCCESS;
}

manager::status_code manager::load_balancer
(
    const libvirt::connection_t     &connection, 
    const std::chrono::milliseconds  interval
) noexcept
{
    libvirt::status_code status;

    /*************************** DOMAIN INFORMATION ***************************/

    // Get list of domains
    libvirt::domain::list_t domain_list;
    status = libvirt::domain::list
    (
        connection, 
        domain_list
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to retrieve data structure for domains",
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }

    // Set statistics collection period
    if (!statistics_collection_period)
    {
        status = libvirt::domain::set_collection_period
        (
            domain_list, 
            interval
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to set statistics period for domains",
                util::log::ABORT
            );

            return EXIT_FAILURE;
        }

        statistics_collection_period = true;
    }

    // Get memory statistics for each domain
    std::size_t number_of_domains = domain_list.size();
    libvirt::domain::data_t domain_data
    (
        number_of_domains, 
        libvirt::domain::datum_t()
    );

    status = libvirt::domain::ranking
    (
        domain_list,
        domain_data
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to retrieve memory statistics for domains",
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }


    /*************************** SYSTEM INFORMATION ***************************/
    
    // Get hardware memory statistics
    util::stat::slong_t hardware_memory_limit;
    status = libvirt::hardware::hardware_memory_limit
    (
        connection, 
        hardware_memory_limit
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to retrieve hardware memory statistics",
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }

    
    /*********************** MEMORY MOVEMENT SCHEDULER ************************/
    
    // Run scheduler to determine domains' memory sizes and execute reallocation
    status = manager::scheduler
    (
        domain_list, 
        domain_data, 
        hardware_memory_limit
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Fault incurred in scheduler processing",
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
