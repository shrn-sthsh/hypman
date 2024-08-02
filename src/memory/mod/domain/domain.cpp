#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <string>

#include <lib/libvirt.hpp>
#include <log/record.hpp>

#include "domain.hpp"


libvirt::status_code libvirt::domain::list
(
    const libvirt::connection_t   &connection, 
          libvirt::domain::list_t &domain_list
) noexcept
{
    // Use libvirt API to get the collection of domains
    libvirt::virDomain **domains = nullptr;
    std::size_t number_of_domains = libvirt::virConnectListAllDomains
	(
		connection.get(), &domains,
	    VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING
	);
    if (number_of_domains < 0)
    {
        util::log::record
        (
            "Unable to retrieve domain data through libvirt API",
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    // Validate list size
    if (domain_list.size() != number_of_domains)
    {
        util::log::record
        (
            "domain::list recieved with incorrect number of domains",
            util::log::FLAG
        );
        
        domain_list.clear();
        domain_list.resize(number_of_domains);

        std::fill
        (
            domain_list.begin(), 
            domain_list.end(), 
            nullptr
        );
    }

    // Transfer control of domains data to list
    for (libvirt::domain::rank_t rank = 0; rank < number_of_domains; ++rank)
    {
        domain_list[rank] = domain_t
        (
            domains[rank],
            [](libvirt::virDomain *domain)
            {
                if (domain != nullptr)
                    libvirt::virDomainFree(domain);
            }
        ); 
    }

    // Free API collection
    std::free(domains);

    return EXIT_SUCCESS;
}


libvirt::status_code libvirt::domain::set_collection_period
(
          libvirt::domain::list_t   &domain_list,
    const std::chrono::milliseconds &interval
) noexcept
{
    // Check list size
    if (domain_list.empty())
    {
        util::log::record
        (
            "domain::list is empty", 
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    // Set statistics collection period for every domain
    for (libvirt::domain::rank_t rank = 0; rank < domain_list.size(); ++rank)
    {
        status_code status = libvirt::virDomainSetMemoryStatsPeriod
        (
            domain_list[rank].get(), 
            std::chrono::duration_cast<std::chrono::seconds>(interval).count(), 
            VIR_DOMAIN_AFFECT_CURRENT
        );

        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to set domain " + std::to_string(rank) 
                    + "'s statistics collection period", 
                util::log::FLAG
            );
        }
    }

    return EXIT_SUCCESS;
}


libvirt::status_code libvirt::domain::data
(
     libvirt::domain::list_t &domain_list, 
     libvirt::domain::data_t &domain_data
) noexcept
{
    // Check list size
    std::size_t number_of_domains = domain_list.size();
    if (number_of_domains == 0)
    {
        util::log::record
        (
            "domain::list is empty", 
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    // Check domaib ranking size
    if (domain_data.size() != number_of_domains)
    {
        util::log::record
        (
            "domain::list recieved with incorrect number of domains",
            util::log::FLAG
        );
        
        domain_data.clear();
        domain_data.resize(number_of_domains); 
    }

    // Get memory statistics for each domain
    for (libvirt::domain::rank_t rank = 0; rank < number_of_domains; ++rank)
    { 
        libvirt::status_code status; 

        // Pass on domain reference 
        libvirt::domain::datum_t &datum = domain_data[rank];
        datum.rank   = rank;
        datum.domain = std::move(domain_list[rank]);

        // Get memory statistics for this domain 
        libvirt::domain::memory_statistics_t memory_statistics;
        status = libvirt::virDomainMemoryStats
        (
            datum.domain.get(),
            memory_statistics.data(),
            libvirt::domain::number_of_domain_memory_statistics,
            libvirt::FLAG_NULL
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to retrieve domain " + std::to_string(rank) 
                    + "'s memory statistics through libvirt API", 
                util::log::FLAG
            );
        }

        // Get domain's maxmimum memory limit and number of vCPUs
        libvirt::virDomainInfo information; 
        status = libvirt::virDomainGetInfo
        (
            datum.domain.get(), 
            &information
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to retrieve domain " + std::to_string(rank) 
                    + "'s maxmimum memory limit through libvirt API", 
                util::log::FLAG
            );
        }
        datum.domain_memory_limit = information.maxMem;
        datum.number_of_vCPUs     = information.nrVirtCpu;

        // Get remaining statistics 
        bool domain_extra_found = false, balloon_used_found = false;
        using memory_statistic_t = libvirt::virDomainMemoryStatStruct;
        for (const memory_statistic_t &statistic: memory_statistics)
        {
            libvirt::flag_code flag 
                = static_cast<libvirt::flag_code>(statistic.tag);

            // Get the memory used up by balloon
            if (flag == memory_statistic_balloon_used)
            {
                datum.balloon_memory_used
                    = static_cast<util::stat::slong_t>(statistic.val);

                balloon_used_found = true;
            }

            // Get the memory unused by domain
            if (flag == memory_statistic_domain_extra)
            {
                datum.domain_memory_extra
                    = static_cast<util::stat::slong_t>(statistic.val);

                domain_extra_found = true;
            }
        }
        if (!balloon_used_found)
        {
            util::log::record
            (
                "Unable to retrieve domain " + std::to_string(rank) 
                    + "'s balloon driver's used memory through libvirt API", 
                util::log::ERROR
            );

            return EXIT_FAILURE;
        }
        if (!domain_extra_found)
        {
            util::log::record
            (
                "Unable to retrieve domain " + std::to_string(rank) 
                    + "'s unused memory through libvirt API", 
                util::log::ERROR
            );
            
            return EXIT_FAILURE;
        } 
    }

    return EXIT_SUCCESS;
}
