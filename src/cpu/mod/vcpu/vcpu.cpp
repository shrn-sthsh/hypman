#include <cstddef>
#include <cstdlib>
#include <string>

#include <lib/libvirt.hpp>
#include <log/record.hpp>

#include "domain/domain.hpp"
#include "stat/statistics.hpp"

#include "vcpu.hpp"


/**
 *  @brief Domain ID to Domain's vCPU list table
 *
 *  @param connection: hypervisor connection via libvirt
 *  @param vCPU table: structure reference to write to
 *
 *  @details Creates a table mapping universally unique identifiers (uuid)
 *  of a domain to a list of all of its vCPUs' libvirt API handles
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::vCPU::table
(
    const libvirt::domain::table_t &domain_table, 
          libvirt::vCPU::table_t   &vCPU_table
) noexcept
{
    libvirt::status_code status;

    // Validate table is filled
    if (domain_table.empty())
    {
        util::log::record
        (
            "domain::table_t is empty", 
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }

    // Collect vCPU information for vCPUs on each domain 
    for (const auto &[domain_uuid, domain]: domain_table)
    {
        // Get domain's number of vCPUs
        util::stat::sint_t number_of_vCPUs 
            = libvirt::virDomainGetMaxVcpus(domain.get());
        if (number_of_vCPUs < 1)
        {
            util::log::record
            (
                "Domain " + domain_uuid + " has no available vCPUs"
            );

            continue;
        }
        libvirt::vCPU::list_t vCPU_list
        (
            static_cast<std::size_t>(number_of_vCPUs)
        );

        // Get domain's vCPUs' information
        status = libvirt::virDomainGetVcpus
        (
            domain.get(), 
            vCPU_list.data(),
            number_of_vCPUs,
            nullptr,
            libvirt::FLAG_DEF
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to retrieve domain information for domain " 
                    + domain_uuid,
                util::log::type::ERROR
            );
        }

        // Add domain-id-vCPU-information key-value pair to table
        vCPU_table.emplace(domain_uuid, vCPU_list);
    }

    return EXIT_SUCCESS;
}


/**
 *  @brief Determine whether domain-vCPU architecture is iterable for scheduler
 *
 *  @param current vCPU table:  domain-vCPUs table of current iteration
 *  @param previous vCPU table: domain-vCPUs table from previous iteration
 *
 *  @details Checks that the domain-vCPU tables between the previous and current
 *  iteration only differ about how many vCPUs each domain many have and no new
 *  domains have not been added. Differing domains will be skipped and returned.
 *
 *  An addition of domains requires another iteration to guarantee the domain 
 *  has had time to stabilize. Domains with additonal vCPUs, however, can be
 *  skipped to allow stablization. A reduction of vCPUs has no issues either.
 *
 *  @return whether tables are comparable and the set of differing domains
 */
libvirt::vCPU::table_diff_t
libvirt::vCPU::comparable_state
(
    const libvirt::vCPU::table_t &curr_vCPU_table, 
    const libvirt::vCPU::table_t &prev_vCPU_table
) noexcept
{
    // Not equal some if either or both are empty
    if (curr_vCPU_table.empty())
    {
        util::log::record
        (
            "Current table is empty",
            util::log::type::FLAG
        );

        return libvirt::vCPU::table_diff_t(false, {});
    }
    if (prev_vCPU_table.empty())
    {
        util::log::record
        (
            "Previous table is empty",
            util::log::type::FLAG
        );

        return libvirt::vCPU::table_diff_t(false, {});
    }

    // Not equal if have different number of domains
    if(curr_vCPU_table.size() != prev_vCPU_table.size())
    {
        util::log::record
        (
            "Tables have different number of domains",
            util::log::type::FLAG
        );

        return libvirt::vCPU::table_diff_t(false, {});
    }

    // Not equal if any same ranked domains have a different number of vCPUs or
    // if there is an unknown domain by UUID
    uuid_set_t diff;
    bool diff_number_of_domains = false;
    for (const auto &[curr_domain_uuid, curr_vCPU_list]: curr_vCPU_table)
    {
        // The previous table must have the domain in the current table
        const libvirt::vCPU::table_t::const_iterator iterator 
            = prev_vCPU_table.find(curr_domain_uuid);
        if (iterator == prev_vCPU_table.end())
        {
            diff_number_of_domains = true;

            util::log::record
            (
                "Current iteration has new domain " + curr_domain_uuid,
                util::log::type::FLAG
            );

            continue;
        }

        // Both domains should also have the same number of vCPUs
        const libvirt::vCPU::list_t prev_vCPU_list = iterator->second;
        if (curr_vCPU_list.size() != prev_vCPU_list.size())
            diff.emplace(curr_domain_uuid);
    }

    // A filled diff set requires a note
    if (!diff.empty())
    {
        std::string domain_uuid_list;
        uuid_set_t::iterator domain_uuid = diff.begin();

        domain_uuid_list += *(domain_uuid++); 
        while (domain_uuid != diff.end())
            domain_uuid_list += ", " + *(domain_uuid++); 

        util::log::record
        (
            "Number of vCPUs is inconsistent for " + std::to_string(diff.size())
                + " domains of the UUIDs " + domain_uuid_list
        );

        return libvirt::vCPU::table_diff_t(false, diff);
    }
 
    return libvirt::vCPU::table_diff_t(true, diff);
}


/**
 *  @brief vCPU Data Collector
 *
 *  @param current vCPU table:    domain-vCPUs table of current iteration
 *  @param previous vCPU table:   domain-vCPUs table from previous iteration
 *  @param vCPU table difference: set of domains in current vCPU table but not
 *                                in the previous vCPU table
 *  @param current domain table:  current domains' UUIDs to libvirt API domain 
 *                                handles table
 *  @param current vCPU data:     structure reference to write to
 *
 *  @details Collect data about vCPUs status' for all domains required by 
 *  scheduler to determine remapping

 *
 *  @return execution status code
 */
libvirt::status_code 
libvirt::vCPU::data
(
    const libvirt::vCPU::table_t    &curr_vCPU_table, 
    const libvirt::vCPU::table_t    &prev_vCPU_table,
    const libvirt::vCPU::uuid_set_t &vCPU_table_diff,
          libvirt::domain::table_t  &curr_domain_table,
          libvirt::vCPU::data_t     &curr_vCPU_data
) noexcept
{
    // Validate tables are filled 
    if (curr_vCPU_table.empty())
    {
        util::log::record("Current iteration vCPU table is empty");
        return EXIT_FAILURE;
    }
    if (prev_vCPU_table.empty())
    {
        util::log::record("Previous iteration vCPU table is empty");
        return EXIT_FAILURE;
    }
    if (curr_domain_table.empty())
    {
        util::log::record("Current iteration domain table is empty");
        return EXIT_FAILURE;
    }

    // Check vCPU data size
    if (!curr_vCPU_data.empty())
    {
        util::log::record
        (
            "vCPU::data_t recieved with incorrect number of vCPUs",
            util::log::type::FLAG
        );
        
        curr_vCPU_data.clear();
    }

    // Process each domain and it's vCPUs to create schedulable vCPU list
    for (auto &[curr_domain_uuid, curr_vCPU_list]: curr_vCPU_table)
    {
        // Skip any domains marked as having different number of vCPUs
        if (vCPU_table_diff.find(curr_domain_uuid) != vCPU_table_diff.end())
             continue;

        // Process each vCPU in domains present in both tables
        libvirt::vCPU::list_t prev_vCPU_list 
            = prev_vCPU_table.find(curr_domain_uuid)->second;

        libvirt::vCPU::rank_t rank;
        for (rank = 0; rank < curr_vCPU_list.size(); ++rank)
        {
            const libvirt::virVcpuInfo &curr_vCPU_info = curr_vCPU_list[rank];
            const libvirt::virVcpuInfo &prev_vCPU_info = prev_vCPU_list[rank];

            util::stat::slong_t curr_usage_time
                = static_cast<util::stat::slong_t>(curr_vCPU_info.cpuTime);
            util::stat::slong_t prev_usage_time
                = static_cast<util::stat::slong_t>(prev_vCPU_info.cpuTime);

            // Usage time diffrence between iterations
            util::stat::slong_t usage_time_norm 
                = curr_usage_time - prev_usage_time; 
            if (usage_time_norm < 0)
            {
                util::log::record
                (
                    "Usage time currupted for vCPU " + std::to_string(rank)
                        + " on domain " + curr_domain_uuid 
                        + "; using zero in place",
                    util::log::type::FLAG
                );
                
                usage_time_norm = 0;
            }

            // Set data and move over domain control to data
            curr_vCPU_data.emplace_back
            (
                curr_vCPU_info.number,
                curr_vCPU_info.cpu,
                curr_domain_uuid,
                std::move(curr_domain_table[curr_domain_uuid]),
                usage_time_norm
            );
        }
    }

    return EXIT_SUCCESS;
}


/**
 *  @brief vCPU Data Collector
 *
 *  @param vCPU rank:   vCPU rank amongst domain's vCPUs
 *  @param pCPU rank:   pCPU rank to which vCPU is pinned to
 *  @param domain UUID: domain UUID of domain which vCPU is apart of
 *  @param domain:      libvirt API domain handle for which vCPU is apart of
 *  @param usage time:  vCPU's active pCPU usage time between iterations
 *
 *  @details Copy over simple values and move domain handle to new object
 */
libvirt::vCPU::datum_t::datum_t
(
    libvirt::vCPU::rank_t       vCPU_rank,
    libvirt::pCPU::rank_t       pCPU_rank,
    libvirt::domain::uuid_t     domain_uuid,
    libvirt::domain::domain_t &&domain,
    util::stat::ulong_t         usage_time
) noexcept: 
    vCPU_rank(vCPU_rank), 
    pCPU_rank(pCPU_rank), 
    domain_uuid(domain_uuid), 
    domain(std::move(domain)), 
    usage_time(usage_time)
{}
