#include <cstddef>
#include <cstdlib>
#include <string>

#include <lib/libvirt.hpp>
#include <log/record.hpp>

#include "domain/domain.hpp"
#include "stat/statistics.hpp"

#include "vcpu.hpp"


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
            util::log::ERROR
        );

        return EXIT_FAILURE;
    }

    // Collect vCPU information for vCPUs on each domain 
    for (const auto &[domain_id, domain]: domain_table)
    {
        util::stat::sint_t number_of_vCPUs 
            = libvirt::virDomainGetMaxVcpus(domain.get());
        if (number_of_vCPUs < 1)
        {
            util::log::record
            (
                "Domain " + domain_id + " has no available vCPUs", 
                util::log::STATUS
            );

            continue;
        }
        libvirt::vCPU::list_t vCPU_list
        (
            static_cast<std::size_t>(number_of_vCPUs)
        );

        status = libvirt::virDomainGetVcpus
        (
            domain.get(), 
            vCPU_list.data(),
            number_of_vCPUs,
            nullptr,
            libvirt::FLAG_NULL
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to retrieve domain information for domain " + domain_id,
                util::log::ERROR
            );
        }

        vCPU_table.emplace(domain_id, vCPU_list);
    }

    return EXIT_SUCCESS;
}


libvirt::vCPU::table_diff_t
libvirt::vCPU::comparable_state
(
    const libvirt::vCPU::table_t &curr_table, 
    const libvirt::vCPU::table_t &prev_table
) noexcept
{
    // Not equal some if either or both are empty
    if (curr_table.empty())
    {
        util::log::record
        (
            "First table is empty",
            util::log::STATUS
        );

        return libvirt::vCPU::table_diff_t(false, {});
    }
    if (prev_table.empty())
    {
        util::log::record
        (
            "Second table is empty",
            util::log::STATUS
        );

        return libvirt::vCPU::table_diff_t(false, {});
    }

    // Not equal if have different number of domains
    if(curr_table.size() != prev_table.size())
    {
        util::log::record
        (
            "Tables have different number of domains",
            util::log::STATUS
        );

        return libvirt::vCPU::table_diff_t(false, {});
    }

    // Not equal if any same ranked domains have a different number of vCPUs
    uuid_set_t diff;
    for (const auto &[domain_id, list_A]: curr_table)
    {
        // Must have the same key
        const libvirt::vCPU::table_t::const_iterator iterator 
            = prev_table.find(domain_id);
        if (iterator == prev_table.end())
        {
            diff.emplace(domain_id);
            continue;
        }

        // Must have same number of vCPUs
        const libvirt::vCPU::list_t list_B = iterator->second;
        if (list_A.size() != list_B.size())
            diff.emplace(domain_id);
    }

    if (!diff.empty())
    {
        std::string domain_id_list;
        uuid_set_t::iterator domain_id = diff.begin();

        domain_id_list += *(domain_id++); 
        while (domain_id != diff.end())
            domain_id_list += ", " + *(domain_id++); 

        util::log::record
        (
            "Number of vCPUs is inconsistent for domains of the UUIDs " 
                + domain_id_list,
            util::log::STATUS
        );

        return libvirt::vCPU::table_diff_t(false, diff);
    }
 
    return libvirt::vCPU::table_diff_t(true, diff);
}


libvirt::status_code 
libvirt::vCPU::data
(
    const libvirt::vCPU::table_t    &curr_vCPU_table, 
    const libvirt::vCPU::table_t    &prev_vCPU_table,
    const libvirt::vCPU::uuid_set_t &vCPU_table_diff,
          libvirt::domain::table_t  &domain_table,
          libvirt::vCPU::data_t     &data
) noexcept
{
    // Validate tables are filled 
    if (curr_vCPU_table.empty())
    {
        util::log::record
        (
            "Current iteration vCPU table is empty",
            util::log::STATUS
        );

        return EXIT_FAILURE;
    }
    if (prev_vCPU_table.empty())
    {
        util::log::record
        (
            "Previous iteration vCPU table is empty",
            util::log::STATUS
        );

        return EXIT_FAILURE;
    }
    if (curr_vCPU_table.empty())
    {
        util::log::record
        (
            "Current iteration vCPU table is empty",
            util::log::STATUS
        );

        return EXIT_FAILURE;
    }

    // Process each domain and it's vCPUs to create schedulable vCPU list
    for (auto &[domain_id, curr_list]: curr_vCPU_table)
    {
        // Skip any domains marked as different between two tables
        if (vCPU_table_diff.find(domain_id) != vCPU_table_diff.end())
             continue;
        const libvirt::vCPU::table_t::const_iterator iterator 
            = prev_vCPU_table.find(domain_id);
        if (iterator == prev_vCPU_table.end())
            continue;

        // Process each vCPUs in domains present in both tables
        libvirt::vCPU::list_t prev_list = iterator->second;
        for (libvirt::vCPU::rank_t rank = 0; rank < curr_list.size(); ++rank)
        {
            const libvirt::virVcpuInfo &curr_info = curr_list[rank];
            const libvirt::virVcpuInfo &prev_info = prev_list[rank];

            util::stat::slong_t curr_usage_time
                = static_cast<util::stat::slong_t>(curr_info.cpuTime);
            util::stat::slong_t prev_usage_time
                = static_cast<util::stat::slong_t>(prev_info.cpuTime);

            // Usage time diffrence between iterations
            util::stat::slong_t usage_time_norm 
                = curr_usage_time - prev_usage_time; 
            if (usage_time_norm < 0)
            {
                util::log::record
                (
                    "Usage time currupted for vCPU " + std::to_string(rank)
                        + " on domain " + domain_id + "; using zero in place",
                    util::log::FLAG
                );
                
                usage_time_norm = 0;
            }

            // Set data and move over domain control to data
            data.emplace_back
            (
                curr_info.number,
                curr_info.cpu,
                domain_id,
                std::move(domain_table[domain_id]),
                usage_time_norm
            );
        }
    }

    return EXIT_SUCCESS;
}
