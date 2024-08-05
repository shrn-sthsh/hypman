#include <cstddef>
#include <cstdlib>
#include <string>

#include <lib/libvirt.hpp>
#include <log/record.hpp>

#include "domain.hpp"


libvirt::status_code libvirt::domain::table
(
    const libvirt::connection_t    &connection, 
          libvirt::domain::table_t &domain_table
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

    // Transfer control of domains data to list
    for (libvirt::domain::rank_t rank = 0; rank < number_of_domains; ++rank)
    {
        char domain_id[VIR_UUID_STRING_BUFLEN];
        libvirt::status_code status 
            = libvirt::virDomainGetUUIDString(domains[rank], domain_id);

        domain_table[std::string(domain_id)] = domain_t
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
