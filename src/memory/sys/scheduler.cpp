#include <cmath>
#include <cstddef>
#include <cstdlib>

#include <lib/libvirt.hpp>
#include <string>

#include "scheduler.hpp"
#include "domain/domain.hpp"
#include "stat/statistics.hpp"


constexpr util::stat::slong_t MINIMUM_SYSTEM_MEMORY = 200 << 10;
constexpr util::stat::slong_t MINIMUM_DOMAIN_MEMORY = 100 << 10;

constexpr std::double_t SUPPLY_COEFFICIENT = 0.115;
constexpr std::double_t DEMAND_COEFFICIENT = 0.085;
constexpr std::double_t CHANGE_COEFFICIENT = 0.200;


manager::status_code manager::scheduler
(
    libvirt::domain::list_t &domain_list, 
    libvirt::domain::data_t &domain_data, 
    util::stat::slong_t      hardware_memory_limit
)
{
    // Check domain consistency 
    std::size_t number_of_domains = domain_list.size();
    if (domain_data.size() != number_of_domains)
    {
        util::log::record
        (
            "Domain data structure have diffrent number of domain",
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
	for (libvirt::domain::rank_t rank = 0; rank < number_of_domains; ++rank)
	{
        libvirt::domain::domain_t &domain = domain_list[rank];
        libvirt::domain::datum_t  &data   = domain_data[rank];

        util::stat::ulong_t memory_chunk 
            = data.balloon_memory_used
			+ data.domain_memory_delta;
		
		// Take back memory if domain is supplying
		if (data.domain_memory_delta > 0)
        {
			status = libvirt::virDomainSetMemory
            (
                domain.get(), 
                memory_chunk
            );
            if (static_cast<bool>(status))
            {
                util::log::record
                (
                    "Unable to set domain " + std::to_string(rank)
                        + "'s memory to " + std::to_string(memory_chunk) 
                        + " bytes",
                    util::log::ERROR
                );

                return EXIT_FAILURE;
            }
        }

		// Add to number of domains if domain is requesting
		if (data.domain_memory_delta < 0)
			number_of_requesting_domains += 1;
	}

	// System providing memory to requesting domains
	for (libvirt::domain::rank_t rank = 0; rank < number_of_domains; ++rank)
	{
        libvirt::domain::domain_t &domain = domain_list[rank];
        libvirt::domain::datum_t  &datum  = domain_data[rank];

		util::stat::slong_t available_memory 
			= hardware_memory_limit - MINIMUM_SYSTEM_MEMORY;	
        util::stat::ulong_t maximum_chunk_size
            = static_cast<util::stat::ulong_t>(datum.domain_memory_limit);

        std::double_t domain_memory_delta 
            = datum.domain_memory_delta;
        std::double_t domain_memory_delta_magnitude
            = std::abs(domain_memory_delta);

		// Give away memory if domain is requesting
		if (domain_memory_delta < 0) 
		{
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
                    domain.get(), 
                    memory_chunk
                );
                if (static_cast<bool>(status))
                {
                    util::log::record
                    (
                        "Unable to set domain " + std::to_string(rank)
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
                    domain.get(), 
                    memory_chunk
                );
                if (static_cast<bool>(status))
                {
                    util::log::record
                    (
                        "Unable to set domain " + std::to_string(rank)
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
	}

    return EXIT_SUCCESS;
}
