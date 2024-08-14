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

#include "domain/domain.hpp"
#include "pcpu/pcpu.hpp"
#include "vcpu/vcpu.hpp"

#include "cpuman.hpp"


static libvirt::vCPU::table_t prev_vCPU_table;
static util::stat::ulong_t    balancer_iteration = 0;
    
/**
 *  @brief Physical CPU Usage Manager
 *
 *  @details Operating systems running atop a hypervisor have virtualized 
 *  CPUs (vCPUs) mapped to physical hardware CPUs (pCPUs) which actually 
 *  execute task of and on the operating system.
 *
 *  A hypervisor will spread the multiple vCPUs requested to be supported 
 *  by any single OS across many pCPUs available in the hardware for all 
 *  the OS's, often leading to many vCPUs on any single pCPU.
 *  
 *  To balance the changing loads placed on any pCPU caused any or many of 
 *  changing loads of it's vCPUs, cpuman analyzes the spread of utilization
 *  amongst all pCPUs, and remaps vCPUs to pCPUs if necessary.
 */
int main(int argc, char *argv[])  
{
    /**************************** VALIDATE COMMAND ****************************/

    // Command should be provided with single interval argument
    if (argc != 2)
    {
        util::log::record
        (
            "Usage follows as ./cpuman <interval (ms)>", 
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
    

    /********************** CONNECT TO VIRTUALIZATION HOST ********************/

    // Make connection to hypervisor using libvirt
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


    /************************* ASSIGN INTERRUPT HANDLER ***********************/

    // Interrupt sets accessible exit flag
    static os::signal::signal_t exit_signal = os::signal::SIG_DEF;
    os::signal::signal
    (
        os::signal::SIG_INT,
        [](os::signal::signal_t interrupt)
        {
            exit_signal = os::signal::SIG_EXT;
        }
    );

    /************************** LAUNCH LOAD BALANCER **************************/

    // Run pCPU load balancer at every interval
    util::stat::ulong_t failures = 0, maximum_failures = 3;
    while (!static_cast<bool>(exit_signal))
    {
        // Launch load balancer
        manager::status_code status = manager::load_balancer(connection);
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Load balancer exited on terminating error after "
                    + std::to_string(balancer_iteration + 1) 
                    + " iterations", 
                util::log::type::ERROR
            );
            
            // Abort on too many failures
            if (++failures >= maximum_failures)
            {
                util::log::record
                (
                    "Reached maximum number of failures allowed; "
                    "aborting process",
                    util::log::type::ABORT
                );

                return EXIT_FAILURE;
            }
        }
        
        // Sleep until next interval
        std::this_thread::sleep_for(interval);
        ++balancer_iteration;
    }

    return EXIT_SUCCESS;
}


/**
 *  @brief pCPU Load Balancer
 *
 *  @param connection: hypervisor connection via libvirt
 *
 *  @details Balances pCPU loads from its vCPUs' demands by collecting data 
 *  about each domain and its vCPUs as well as data about hardware's active
 *  pCPUs and running a greedy scheduler mapping vCPUs to pCPUs. Scheduler
 *  executes based on an analysis of the likelyhood of better performance.
 *
 *  @return execution status code
 */
manager::status_code
manager::load_balancer
(
    const libvirt::connection_t &connection
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
            util::log::type::ABORT
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
            util::log::type::FLAG
        );

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
                util::log::type::ABORT
            );

            return EXIT_FAILURE;
        }

        // Change in some domains can still allow others to be scheduled
        util::log::record
        (
            "Minor change in intradomain architecture for one or more domains; "
            "will skip affected domains in scheduler",
            util::log::type::FLAG
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
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }

    // Save vCPU table for next iteration
    prev_vCPU_table = curr_vCPU_table;


    /**************************** pCPU INFORMATION ****************************/

    // Create list of active pCPUs       
    libvirt::pCPU::data_t curr_pCPU_data;
    status = libvirt::pCPU::data
    (
        connection,   
        curr_vCPU_data,
        curr_pCPU_data
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Unable to get number of pCPUs active in system",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }


    /*************************** SCHEDULER ALGORITHM **************************/

    // Run repinning scheduler
    status = manager::scheduler
    (
        curr_vCPU_data, 
        curr_pCPU_data
    );
    if (static_cast<bool>(status))
    {
        util::log::record
        (
            "Error incurred while running scheduler; exiting iteration",
            util::log::type::ABORT
        );

        return EXIT_FAILURE;
    }
 
    return EXIT_SUCCESS;
}
