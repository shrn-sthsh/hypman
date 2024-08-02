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
            "domain::list_t recieved with incorrect number of domains",
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
