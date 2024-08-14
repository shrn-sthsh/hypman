#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>

#include <lib/libvirt.hpp>
#include <log/record.hpp>
#include <stat/statistics.hpp>

#include "domain/domain.hpp"

#include "scheduler.hpp"


// Mimimum memory limits
static constexpr util::stat::slong_t MINIMUM_SYSTEM_MEMORY = 200 << 10;
static constexpr util::stat::slong_t MINIMUM_DOMAIN_MEMORY = 100 << 10;

// Movement coefficients
static constexpr std::double_t SUPPLY_COEFFICIENT = 0.115;
static constexpr std::double_t DEMAND_COEFFICIENT = 0.085;
static constexpr std::double_t CHANGE_COEFFICIENT = 0.200;


/**
 *  @brief Memory Reallocation Scheduler 
 *
 *  @param domain data:         Collection of data about domains for scheduler's 
 *                              required reallocation policies
 *  @param system memory limit: Memory limit dictated by hardware
 *
 *  @details Determine how much and where to reallocate memory between domains
 *  to ensure fairness of performance amongst all domains.
 *
 *  Scheduler determines whether a domain can afford to supply or is in need of
 *  more memmory based much memory is unused by the balloon driver. Then it 
 *  proceeds to reclaim as much memory as possible from those domains which can 
 *  prvoided without degrading their performance.
 *
 *  Once done, the domains requiring more memory are sorted based on their
 *  memory pressures from how many vCPUs they have. Finally, domains are served 
 *  in this priority, and if the amount the require is unsatisfieable, they will 
 *  get a portion of what is left.
 *
 *  @return execution status code
 */
manager::status_code
manager::scheduler
(
    libvirt::domain::data_t &domain_data, 
    util::stat::slong_t      system_memory_limit
)
{
    // Check domain consistency 
    if (domain_data.empty())
    {
        util::log::record
        (
            "Domain data is empty and unavailable",
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }


    /******************** DETERMINE HOW MEMORY NEEDS TO MOVE ******************/
 
    libvirt::domain::data_t demanders; 
    demanders.reserve(domain_data.size());

    libvirt::domain::data_t suppliers; 
    suppliers.reserve(domain_data.size());
  
    // Memory ready to be consumed; domain changes subtract from system total
    util::stat::slong_t available_memory 
        = system_memory_limit - MINIMUM_SYSTEM_MEMORY;

    // Determine memory movement of each domain
    libvirt::domain::data_t::iterator datum;
    for (datum = domain_data.begin(); datum != domain_data.end(); ++datum)
    {
        // Domains current memory removed from total
        available_memory -= datum->balloon_memory_used;
        if (available_memory < 0)
        {
            util::log::record
            (
                "Currupted domain memory usage totals",
                util::log::type::ERROR
            );

            return EXIT_FAILURE;
        }

        // Domain's unused memory and memory limit
        const std::double_t domain_memory_extra = 
            static_cast<std::double_t>(datum->domain_memory_extra);
        const std::double_t domain_memory_limit = 
            static_cast<std::double_t>(datum->domain_memory_limit);

        // Movement thresholds
        const std::double_t SUPPLY_THRESHOLD 
            = SUPPLY_COEFFICIENT * domain_memory_limit;
        const std::double_t DEMAND_THRESHOLD 
            = DEMAND_COEFFICIENT * domain_memory_limit;
        
        // Domain can supply memory relative to its limit (domain loses memory)
        if (domain_memory_extra > SUPPLY_THRESHOLD)
        {
            datum->domain_memory_delta
                = -1 * MINIMUM_DOMAIN_MEMORY * CHANGE_COEFFICIENT;
            suppliers.emplace_back(std::move(*datum));

            continue;
        }

        // Domain needs more memory relative to it's limit (domain takes memory)
        if (domain_memory_extra < DEMAND_THRESHOLD)
        {
            datum->domain_memory_delta 
                = MINIMUM_DOMAIN_MEMORY * CHANGE_COEFFICIENT;
            demanders.emplace_back(std::move(*datum));

            continue;
        }
    }
    domain_data.clear();
 
    // Save the number of domains needing memory
    std::size_t number_of_requesting_domains = demanders.size();


    /********************** RECLAIM MEMORY FROM SUPPLIERS *********************/
     
    // System reclaiming memory from supplying domains
    libvirt::status_code status;
    for (const libvirt::domain::datum_t &datum: suppliers)
    {
        // Domain memory footprint with change
        util::stat::slong_t memory_chunk = datum.balloon_memory_used
                                         + datum.domain_memory_delta;
        if (memory_chunk < MINIMUM_DOMAIN_MEMORY)
            memory_chunk = MINIMUM_DOMAIN_MEMORY;

        // Check feasibilty of reallocation
        const util::stat::slong_t resultant_available_memory
            = available_memory - memory_chunk + datum.balloon_memory_used;
        if (resultant_available_memory < 0)
        {
            util::log::record
            (
                "Currupted domain memory usage totals",
                util::log::type::ERROR
            );

            return EXIT_FAILURE;
        }
        
        // Take back memory if from supplying domain
        status = libvirt::virDomainSetMemory
        (
            datum.domain.get(), 
            static_cast<util::stat::ulong_t>(memory_chunk)
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to set domain " + datum.uuid
                    + "'s memory to " + std::to_string(memory_chunk) 
                    + " bytes",
                util::log::type::FLAG
            );

            continue;
        }
        available_memory = resultant_available_memory; 
    }

 
    /**************** PRIOTITZE DEMANDERS BY MEMORY PRESSURE ******************/

    // Sort by memory pressure per vCPU in non-increasing order to prioritize 
    // domains that require the most memory per vCPU
    std::sort
    (
        demanders.begin(), demanders.end(), [] 
        (
            const libvirt::domain::datum_t &datum_A, 
            const libvirt::domain::datum_t &datum_B
        )
        {
            std::double_t pressure_per_vCPU_A 
                = datum_A.domain_memory_delta 
                / datum_A.number_of_vCPUs;

            std::double_t pressure_per_vCPU_B
                = datum_B.domain_memory_delta 
                / datum_B.number_of_vCPUs;

            return pressure_per_vCPU_A > pressure_per_vCPU_B;
        }
    );


    /*********************** PROVIDE MEMORY TO CONSUMERS **********************/

    // System providing memory to requesting domains
    for (const libvirt::domain::datum_t &datum: demanders)
    {   
        util::stat::slong_t maximum_chunk_size = datum.domain_memory_limit;

        // Requesting memory size is satisfiable by system
        std::double_t domain_memory_delta = datum.domain_memory_delta;
        if (std::abs(domain_memory_delta) < available_memory)
        {   
            util::stat::slong_t memory_chunk 
                = datum.balloon_memory_used + domain_memory_delta;

            // Memory chunk is limited by domain's set limit
            if (memory_chunk > maximum_chunk_size)
                memory_chunk = maximum_chunk_size;

            // Check feasibilty of reallocation
            const util::stat::slong_t resultant_available_memory
                = available_memory - memory_chunk + datum.balloon_memory_used;
            if (resultant_available_memory < 0 || memory_chunk < 0)
            {
                util::log::record
                (
                    "Currupted domain memory usage totals",
                    util::log::type::ERROR
                );

                return EXIT_FAILURE;
            }
            
            // Give memory to domain consuming
            status = libvirt::virDomainSetMemory
            (
                datum.domain.get(), 
                static_cast<util::stat::ulong_t>(memory_chunk)
            );
            if (static_cast<bool>(status))
            {
                util::log::record
                (
                    "Unable to set domain " + datum.uuid
                        + "'s memory to " + std::to_string(memory_chunk) 
                        + " bytes",
                    util::log::type::FLAG
                );

                continue;
            }
            available_memory = resultant_available_memory;

            // Single domain served per iteration
            if (number_of_requesting_domains > 1)
                --number_of_requesting_domains;

            continue;
        }

        // Equally split remaining if size not satisfiable
        util::stat::ulong_t partitioned_memory_chunk = std::ceil
        (
            static_cast<std::double_t>(std::abs(available_memory))
                / number_of_requesting_domains
        );
        bool domains_requesting = number_of_requesting_domains > 0;

        if (domains_requesting && partitioned_memory_chunk < available_memory)
        {   
            util::stat::slong_t memory_chunk = datum.balloon_memory_used
                + (domain_memory_delta / number_of_requesting_domains);
            
            // Memory chunk is limited by domain's set limit
            if (memory_chunk > maximum_chunk_size)
                memory_chunk = maximum_chunk_size;

            // Check feasibilty of reallocation
            const util::stat::slong_t resultant_available_memory
                = available_memory - memory_chunk + datum.balloon_memory_used;
            if (resultant_available_memory < 0 || memory_chunk < 0)
            {
                util::log::record
                (
                    "Currupted domain memory usage totals",
                    util::log::type::ERROR
                );

                return EXIT_FAILURE;
            }

            // Give memory to domain consuming
            status = libvirt::virDomainSetMemory
            (
                datum.domain.get(), 
                static_cast<util::stat::ulong_t>(memory_chunk)
            );
            if (static_cast<bool>(status))
            {
                util::log::record
                (
                    "Unable to set domain " + datum.uuid
                        + "'s memory to " + std::to_string(memory_chunk) 
                        + " bytes",
                    util::log::type::ERROR
                );

                return EXIT_FAILURE;
            }           
            available_memory = resultant_available_memory;

            // Single domain served per iteration
            if (number_of_requesting_domains > 1)
                --number_of_requesting_domains;
        }
    }

    return EXIT_SUCCESS;
}
