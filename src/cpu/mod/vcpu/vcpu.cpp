#include <cstddef>
#include <cstdlib>
#include <string>

#include <log/record.hpp>

#include "domain/domain.hpp"
#include "stat/statistics.hpp"

#include "vcpu.hpp"


libvirt::status_code libvirt::vCPU::table
(
    const libvirt::domain::list_t &domain_list, 
          libvirt::vCPU::table_t  &vCPU_table
) noexcept
{
    libvirt::status_code status;

    // Check list size
    if (domain_list.empty())
    {
        util::log::record
        (
            "domain::list_t is empty", 
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }
    
    // Check domain data size
    std::size_t number_of_domains = domain_list.size();
    if (vCPU_table.size() != number_of_domains)
    {
        util::log::record
        (
            "vCPU::table_t recieved with incorrect number of domains",
            util::log::FLAG
        );
        
        vCPU_table.clear();
        vCPU_table.resize(number_of_domains); 
    } 

    // Collect vCPU information for vCPUs on each domain 
    for (libvirt::domain::rank_t rank = 0; rank < number_of_domains; ++rank)
    {
        const libvirt::domain::domain_t &domain    = domain_list[rank];
              libvirt::vCPU::list_t     &vCPU_list = vCPU_table[rank];

        util::stat::sint_t number_of_vCPUs 
            = libvirt::virDomainGetMaxVcpus(domain.get());
        if (number_of_vCPUs < 1)
        {
            util::log::record
            (
                "Domain " + std::to_string(rank) + " has no available vCPUs", 
                util::log::STATUS
            );

            continue;
        }
        vCPU_list.resize(static_cast<std::size_t>(number_of_vCPUs));

        status = libvirt::virDomainGetVcpus
        (
            domain.get(), 
            vCPU_list.data(),
            static_cast<util::stat::sint_t>(vCPU_list.size()),
            nullptr,
            libvirt::FLAG_NULL
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to retrieve domain information for domain "
                    + std::to_string(rank),

                util::log::ERROR
            );
        }
    }

    return EXIT_SUCCESS;
}
