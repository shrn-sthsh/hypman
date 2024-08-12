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
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }

    // Collect vCPU information for vCPUs on each domain 
    for (const auto &[domain_uuid, domain]: domain_table)
    {
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
                "Unable to retrieve domain information for domain " + domain_uuid,
                util::log::type::ERROR
            );
        }

        vCPU_table.emplace(domain_uuid, vCPU_list);
    }

    return EXIT_SUCCESS;
}


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
        util::log::record("First table is empty");
        return libvirt::vCPU::table_diff_t(false, {});
    }
    if (prev_vCPU_table.empty())
    {
        util::log::record("Second table is empty");
        return libvirt::vCPU::table_diff_t(false, {});
    }

    // Not equal if have different number of domains
    if(curr_vCPU_table.size() != prev_vCPU_table.size())
    {
        util::log::record("Tables have different number of domains");
        return libvirt::vCPU::table_diff_t(false, {});
    }

    // Not equal if any same ranked domains have a different number of vCPUs
    uuid_set_t diff;
    for (const auto &[domain_uuid, list_A]: curr_vCPU_table)
    {
        // Must have the same key
        const libvirt::vCPU::table_t::const_iterator iterator 
            = prev_vCPU_table.find(domain_uuid);
        if (iterator == prev_vCPU_table.end())
        {
            diff.emplace(domain_uuid);
            continue;
        }

        // Must have same number of vCPUs
        const libvirt::vCPU::list_t list_B = iterator->second;
        if (list_A.size() != list_B.size())
            diff.emplace(domain_uuid);
    }

    if (!diff.empty())
    {
        std::string domain_uuid_list;
        uuid_set_t::iterator domain_uuid = diff.begin();

        domain_uuid_list += *(domain_uuid++); 
        while (domain_uuid != diff.end())
            domain_uuid_list += ", " + *(domain_uuid++); 

        util::log::record
        (
            "Number of vCPUs is inconsistent for domains of the UUIDs " 
                + domain_uuid_list
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
          libvirt::vCPU::data_t     &vCPU_data
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
    if (domain_table.empty())
    {
        util::log::record("Current iteration domain table is empty");
        return EXIT_FAILURE;
    }

    // Check vCPU data size
    if (!vCPU_data.empty())
    {
        util::log::record
        (
            "vCPU::data_t recieved with incorrect number of vCPUs",
            util::log::type::FLAG
        );
        
        vCPU_data.clear();
    }

    // Process each domain and it's vCPUs to create schedulable vCPU list
    for (auto &[domain_uuid, curr_vCPU_list]: curr_vCPU_table)
    {
        // Skip any domains marked as different between two tables
        if (vCPU_table_diff.find(domain_uuid) != vCPU_table_diff.end())
             continue;
        const libvirt::vCPU::table_t::const_iterator iterator 
            = prev_vCPU_table.find(domain_uuid);
        if (iterator == prev_vCPU_table.end())
            continue;

        // Process each vCPUs in domains present in both tables
        libvirt::vCPU::list_t prev_vCPU_list = iterator->second;
        for (libvirt::vCPU::rank_t rank = 0; rank < curr_vCPU_list.size(); ++rank)
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
                        + " on domain " + domain_uuid + "; using zero in place",
                    util::log::type::FLAG
                );
                
                usage_time_norm = 0;
            }

            // Set data and move over domain control to data
            vCPU_data.emplace_back
            (
                curr_vCPU_info.number,
                curr_vCPU_info.cpu,
                domain_uuid,
                std::move(domain_table[domain_uuid]),
                usage_time_norm
            );
        }
    }

    return EXIT_SUCCESS;
}
