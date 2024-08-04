#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

#include <lib/signal.hpp>
#include <log/record.hpp>
#include <stat/statistics.hpp>

#include "sys/scheduler.hpp"
#include "domain/domain.hpp"
#include "hardware/hardware.hpp"
#include "vcpu/vcpu.hpp"

#include "cpuman.hpp"


static os::signal::signal_t   exit_signal = os::signal::SIG_NULL;
static libvirt::vCPU::table_t prev_vCPU_table;
static util::stat::ulong_t    balancer_iteration = 0;
    
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

    // Set signal handler
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
        ++balancer_iteration;
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
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }


    /**************************** vCPU INFORMATION ****************************/

    // Create vCPU-domain table 
    libvirt::vCPU::table_t curr_vCPU_table(curr_domain_table.size());
    status = libvirt::vCPU::table
    (
        curr_domain_table, 
        curr_vCPU_table
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to create a table of vCPU information sorted by domain",
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }

    // Determine whether domain architecture is the same
    const auto &[comparable, vCPU_table_diff] = libvirt::vCPU::comparable_state
    (
        curr_vCPU_table, 
        prev_vCPU_table
    );

    // Save and exit iteration if first
    if (balancer_iteration == 0)
    {
        util::log::record
        (
            "First iteration has no base data to estimate on; exiting current "
            "iteration after saving data",
            util::log::FLAG
        );

        // Save data captured 
        prev_vCPU_table = curr_vCPU_table;
        
        return EXIT_SUCCESS;
    }

    // Note if changes present
    if (!comparable)
    {
        // Change in number of domains requres reset
        if (vCPU_table_diff.empty())
        {
            util::log::record
            (
                "Significant change in domain architecture requires skip of "
                "scheduler iteration",
                util::log::ABORT
            );

            return EXIT_FAILURE;
        }

        // Change in some domains can still allow others to be scheduled
        util::log::record
        (
            "Minor change in intradomain architecture for one or more domains; "
            "will skip affected domains in scheduler",
            util::log::FLAG
        );
    }

    // Create list of schedulable vCPUs
    libvirt::vCPU::data_t curr_vCPU_data;
    status = libvirt::vCPU::data
    (
        curr_vCPU_table, 
        prev_vCPU_table, 
        vCPU_table_diff, 
        curr_domain_table, 
        curr_vCPU_data
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to gather vCPU data from vCPU tables and domain tables",
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }


	/************************** HARDWARE INFORMATION **************************/

    util::stat::uint_t number_of_pCPUs;
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
            util::log::ABORT
        );

        return EXIT_FAILURE;
    }


    /**************************** pCPU INFORMATION ****************************/

    // Save data captured 
    prev_vCPU_table = curr_vCPU_table;

    return EXIT_SUCCESS;
}
