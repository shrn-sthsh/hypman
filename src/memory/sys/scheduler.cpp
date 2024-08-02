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
    util::stat::slong_t      hardware_memory_limit
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
	
	// Determine memory movement of each domain
	for (libvirt::domain::datum_t &domain_datum: domain_data)
    {
		// Domain's unused memory and memory limit
        double domain_rank_unused_memory = 
            static_cast<std::double_t>(domain_datum.domain_memory_extra);
		double domain_rank_memory_limit = 
            static_cast<std::double_t>(domain_datum.domain_memory_limit);

		// Movement thresholds
		double SUPPLY_THRESHOLD = SUPPLY_COEFFICIENT * domain_rank_memory_limit;
		double DEMAND_THRESHOLD = DEMAND_COEFFICIENT * domain_rank_memory_limit;
        
		// Domain can supply memory (domain loses memory)
        if (domain_rank_unused_memory > SUPPLY_THRESHOLD)
        {
            domain_datum.domain_memory_delta
				= -1 * MINIMUM_DOMAIN_MEMORY * CHANGE_COEFFICIENT;

            continue;
        }

		// Domain needs more memory (domain takes memory)
        if (domain_rank_unused_memory < DEMAND_THRESHOLD)
        {
            domain_datum.domain_memory_delta 
				= MINIMUM_DOMAIN_MEMORY * CHANGE_COEFFICIENT;

            continue;
        }

		// No change to domain's memory
        domain_datum.domain_memory_delta = 0.0;
    }


    /*********************** MEMORY REDISTRBUTION POLICY **********************/
	
	// Save the number of domains needing memory
    std::size_t number_of_requesting_domains = 0;

	// System reclaiming memory from supplying domains
    libvirt::status_code status;
	for (const libvirt::domain::datum_t &datum: domain_data)
	{
        util::stat::ulong_t memory_chunk 
            = datum.balloon_memory_used
			+ datum.domain_memory_delta;
		
		// Take back memory if domain is supplying
		if (datum.domain_memory_delta > 0)
        {
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

		// Add to number of domains if domain is requesting
		if (datum.domain_memory_delta < 0)
			number_of_requesting_domains += 1;
	}

	// System providing memory to requesting domains
	for (const libvirt::domain::datum_t &datum: domain_data)
	{
		util::stat::slong_t available_memory 
			= hardware_memory_limit - MINIMUM_SYSTEM_MEMORY;	
        util::stat::ulong_t maximum_chunk_size
            = static_cast<util::stat::ulong_t>(datum.domain_memory_limit);

        std::double_t domain_memory_delta 
            = datum.domain_memory_delta;
        std::double_t domain_memory_delta_magnitude
            = std::abs(domain_memory_delta);

		// Continue on to give memory to requesting domains
		if (domain_memory_delta > 0) 
            continue;

        // Requesting memory size is satisfiable by system
        if (domain_memory_delta_magnitude < available_memory)
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
            static_cast<std::double_t>(domain_memory_delta_magnitude)
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
