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


constexpr util::stat::slong_t MINIMUM_SYSTEM_MEMORY = 200 << 10;
constexpr util::stat::slong_t MINIMUM_DOMAIN_MEMORY = 100 << 10;

constexpr std::double_t SUPPLY_COEFFICIENT = 0.115;
constexpr std::double_t DEMAND_COEFFICIENT = 0.085;
constexpr std::double_t CHANGE_COEFFICIENT = 0.200;


manager::status_code manager::scheduler
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
            "Domain data is empty and unavailible",
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }


    /******************** DETERMINE HOW MEMORY NEEDS TO MOVE ******************/
    libvirt::domain::data_t suppliers; 
    suppliers.reserve(domain_data.size());

    libvirt::domain::data_t demanders; 
    demanders.reserve(domain_data.size());
    
    // Determine memory movement of each domain
    util::stat::slong_t available_memory 
        = system_memory_limit - MINIMUM_SYSTEM_MEMORY;

    libvirt::domain::data_t::iterator datum;
    for (datum = domain_data.begin(); datum != domain_data.end(); ++datum)
    {
        // Domain's unused memory and memory limit
        std::double_t domain_memory_extra = 
            static_cast<std::double_t>(datum->domain_memory_extra);
        std::double_t domain_memory_limit = 
            static_cast<std::double_t>(datum->domain_memory_limit);

        // Movement thresholds
        std::double_t SUPPLY_THRESHOLD = SUPPLY_COEFFICIENT * domain_memory_limit;
        std::double_t DEMAND_THRESHOLD = DEMAND_COEFFICIENT * domain_memory_limit;
        
        // Domain can supply memory (domain loses memory)
        if (domain_memory_extra > SUPPLY_THRESHOLD)
        {
            datum->domain_memory_delta
                = -1 * MINIMUM_DOMAIN_MEMORY * CHANGE_COEFFICIENT;
            suppliers.emplace_back(std::move(*datum));

            continue;
        }

        // Domain needs more memory (domain takes memory)
        if (domain_memory_extra < DEMAND_THRESHOLD)
        {
            datum->domain_memory_delta 
                = MINIMUM_DOMAIN_MEMORY * CHANGE_COEFFICIENT;
            demanders.emplace_back(std::move(*datum));

            continue;
        }

        // No change to domain's memory
        datum->domain_memory_delta = 0.0;
        available_memory -= datum->balloon_memory_used;
    }
    domain_data.clear();

    
    /******************** SORT DOMAINS BY MEMORY PRESSURE *********************/

    // Sort by memory pressure per vCPU in non-increasing order regardless 
    // of movement direction for an intertwined list
    std::sort
    (
        domain_data.begin(), domain_data.end(),
        [] 
        (
            const libvirt::domain::datum_t &datum_A, 
            const libvirt::domain::datum_t &datum_B
        )
        {
            std::double_t pressure_per_vCPU_A 
                = std::abs(datum_A.domain_memory_delta) 
                / datum_A.number_of_vCPUs;

            std::double_t pressure_per_vCPU_B
                = std::abs(datum_B.domain_memory_delta) 
                / datum_B.number_of_vCPUs;

            return pressure_per_vCPU_A > pressure_per_vCPU_B;
        }
    );


    /*********************** MEMORY REDISTRBUTION POLICY **********************/
    
    // Save the number of domains needing memory
    std::size_t number_of_requesting_domains = demanders.size();

    // System reclaiming memory from supplying domains
    libvirt::status_code status;
    for (const libvirt::domain::datum_t &datum: suppliers)
    {
        util::stat::ulong_t memory_chunk 
            = datum.balloon_memory_used
            + datum.domain_memory_delta;
        
        // Take back memory if domain is supplying
        status = libvirt::virDomainSetMemory
        (
            datum.domain.get(), 
            memory_chunk
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to set domain " + std::to_string(datum.rank)
                    + "'s memory to " + std::to_string(memory_chunk) 
                    + " bytes",
                util::log::ERROR
            );

            return EXIT_FAILURE;
        }
    }

    // System providing memory to requesting domains
    for (const libvirt::domain::datum_t &datum: demanders)
    {   
        util::stat::ulong_t maximum_chunk_size
            = static_cast<util::stat::ulong_t>(datum.domain_memory_limit);

        // Requesting memory size is satisfiable by system
        std::double_t domain_memory_delta = datum.domain_memory_delta;
        if (std::abs(domain_memory_delta) < available_memory)
        {   
            util::stat::ulong_t memory_chunk 
                = datum.balloon_memory_used + domain_memory_delta;

            // Set memory with condition it can only be as big as limit
            if (memory_chunk > maximum_chunk_size)
                memory_chunk = maximum_chunk_size;
            
            status = libvirt::virDomainSetMemory
            (
                datum.domain.get(), 
                memory_chunk
            );
            if (static_cast<bool>(status))
            {
                util::log::record
                (
                    "Unable to set domain " + std::to_string(datum.rank)
                        + "'s memory to " + std::to_string(memory_chunk) 
                        + " bytes",
                    util::log::ERROR
                );

                return EXIT_FAILURE;
            }
            available_memory -= memory_chunk;

            // Single domain served per iteration
            if (number_of_requesting_domains > 1)
                --number_of_requesting_domains;

            continue;
        }

        // Equally split remaining if size not satisfiable
        util::stat::ulong_t partitioned_chunk_size = std::ceil
        (
            static_cast<std::double_t>(std::abs(domain_memory_delta))
                / number_of_requesting_domains
        );
        bool domains_requesting = number_of_requesting_domains > 0;

        if (domains_requesting && partitioned_chunk_size < available_memory)
        {   
            util::stat::ulong_t memory_chunk = datum.balloon_memory_used
                + (domain_memory_delta / number_of_requesting_domains);
            
            // Set memory with condition it can only be as big as limit
            if (memory_chunk > maximum_chunk_size)
                memory_chunk = maximum_chunk_size;

            status = libvirt::virDomainSetMemory
            (
                datum.domain.get(), 
                memory_chunk
            );
            if (static_cast<bool>(status))
            {
                util::log::record
                (
                    "Unable to set domain " + std::to_string(datum.rank)
                        + "'s memory to " + std::to_string(memory_chunk) 
                        + " bytes",
                    util::log::ERROR
                );

                return EXIT_FAILURE;
            }           
            available_memory -= memory_chunk;

            // Single domain served per iteration
            if (number_of_requesting_domains > 1)
                --number_of_requesting_domains;
        }
    }

    return EXIT_SUCCESS;
}
