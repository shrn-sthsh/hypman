#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <string>

#include <lib/libvirt.hpp>
#include <log/record.hpp>

#include "stat/statistics.hpp"

#include "domain.hpp"


/**
 *  @brief Domain ID to Domain Handle Table Producer
 *
 *  @param connection:   hypervisor connection via libvirt
 *  @param domain table: structure reference to write to
 *
 *  @details Creates a table mapping universally unique identifiers (uuid)
 *  of a domain to it's associated libvirt API domain handle
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::domain::table
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
        libvirt::domain::domains_active_running_flag
    );
    if (number_of_domains < 0)
    {
        util::log::record
        (
            "Unable to retrieve domain data through libvirt API",
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }

    // Transfer control of domain handles to table structure paired to uuids
    for (libvirt::domain::rank_t rank = 0; rank < number_of_domains; ++rank)
    {
        // Get UUID defined by libvirt
        char uuid[libvirt::domain::uuid_length];
        libvirt::status_code status 
            = libvirt::virDomainGetUUIDString(domains[rank], uuid);
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to retrieve domain id through libvirt API",
                util::log::type::FLAG
            );

            continue;
        }

        // Add UUID-domain-handle key-value pair to table
        domain_table[std::string(uuid)] = domain_t
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


/**
 *  @brief Statistics Collection Period Setter
 *
 *  @param current domain table:  current iteration UUID-to-domain table
 *  @param previous domain UUIDs: previous iteration domain UUIDs
 *  @param interval:              load balancer launching interval and 
 *                                collection period
 *
 *  @details Sets statistics collection period for any domains which have
 *  had the period set yet
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::domain::set_collection_period
(
          libvirt::domain::table_t    &curr_domain_table,
          libvirt::domain::uuid_set_t &prev_domain_uuids,
    const std::chrono::milliseconds   &interval
) noexcept
{
    // Validate tables are filled
    if (curr_domain_table.empty())
    {
        util::log::record
        (
            "Current iteration domain::table_t is empty", 
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }

    // Set all domains if none were set before
    using std::chrono::duration_cast;
    if (prev_domain_uuids.empty())
    {
        util::log::record
        (
            "Previous iteration domain::table_t is empty, thus all domains "
            "must have there collection periods set", 
            util::log::type::ERROR
        );

        // Set statistics collection period for all domains
        for (const auto &[uuid, domain]: curr_domain_table) 
        {
            status_code status = libvirt::virDomainSetMemoryStatsPeriod
            (
                domain.get(), 
                duration_cast<std::chrono::seconds>(interval).count(), 
                libvirt::domain::domain_affect_current_flag
            );

            if (static_cast<bool>(status))
            {
                util::log::record
                (
                    "Unable to set domain " + uuid
                        + "'s statistics collection period", 
                    util::log::type::FLAG
                );
            }
        }

        return EXIT_SUCCESS;
    }

    // Set statistics collection period for every new domain
    for (const auto &[uuid, domain]: curr_domain_table) 
    {
        // If domain already seen, continue to next
        if (prev_domain_uuids.find(uuid) != prev_domain_uuids.end())
            continue; 

        status_code status = libvirt::virDomainSetMemoryStatsPeriod
        (
            domain.get(), 
            duration_cast<std::chrono::seconds>(interval).count(), 
            libvirt::domain::domain_affect_current_flag
        );

        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to set domain " + uuid
                    + "'s statistics collection period", 
                util::log::type::FLAG
            );
        }
    }

    return EXIT_SUCCESS;
}


/**
 *  @brief Domain UUIDs Retriever
 *
 *  @param domain table: UUID-to-domain table to retrieve UUIDs from
 *  @param domain UUIDs: structure reference to write to
 *
 *  @details Copy over UUIDs keys of UUID-to-domain table into a set of UUIDs
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::domain::domain_uuids
(
    libvirt::domain::table_t    &domain_table,
    libvirt::domain::uuid_set_t &domain_uuids
) noexcept
{
    // Validate table is filled
    if (domain_table.empty())
    {
        util::log::record
        (
            "Provided domain::table_t is empty", 
            util::log::type::ERROR
        );

        return EXIT_FAILURE;
    }
    if (!domain_uuids.empty())
        domain_uuids.clear();

    // Copy over UUIDs
    for (const auto &[uuid, _]: domain_table) 
        domain_uuids.insert(uuid);

    return EXIT_SUCCESS;
}


/**
 *  @brief Domain Memory Data Collector
 *
 *  @param domain table: UUID-to-domain table to use domain refernces from
 *  @param domain data:  structure reference to write to
 *
 *  @details Collect data about domain memory for all domains required by 
 *  scheduler to determine reallocation memory chunks 
 *
 *  @return execution status code
 */
libvirt::status_code
libvirt::domain::data
(
     libvirt::domain::table_t &domain_table, 
     libvirt::domain::data_t  &domain_data
) noexcept
{
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

    // Check domain data capacity and size
    std::size_t number_of_domains = domain_table.size();
    if (domain_data.capacity() != number_of_domains || !domain_data.empty())
    {
        util::log::record
        (
            "domain::data_t recieved with incorrect number of domains capacity",
            util::log::type::FLAG
        );
        
        domain_data.clear();
        domain_data.reserve(number_of_domains); 
    }

    // Get memory statistics for each domain
    for (auto &[uuid, domain]: domain_table)
    { 
        libvirt::status_code status;

        // Pass on domain reference 
        libvirt::domain::datum_t datum;
        datum.uuid   = uuid;
        datum.domain = std::move(domain);

        // Get memory statistics for this domain 
        libvirt::domain::memory_statistics_t memory_statistics;
        status = libvirt::virDomainMemoryStats
        (
            datum.domain.get(),
            memory_statistics.data(),
            static_cast<util::stat::sint_t>
            (
                libvirt::domain::number_of_domain_memory_statistics
            ),
            libvirt::FLAG_DEF
        );
        if (static_cast<bool>(status))
        {
            util::log::record
            (
                "Unable to retrieve domain " + uuid
                    + "'s memory statistics through libvirt API", 
                util::log::type::FLAG
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
                "Unable to retrieve domain " + uuid
                    + "'s maxmimum memory limit through libvirt API", 
                util::log::type::FLAG
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
                "Unable to retrieve domain " + uuid
                    + "'s balloon driver's used memory through libvirt API", 
                util::log::type::ERROR
            );

            return EXIT_FAILURE;
        }
        if (!domain_extra_found)
        {
            util::log::record
            (
                "Unable to retrieve domain " + uuid
                    + "'s unused memory through libvirt API", 
                util::log::type::ERROR
            );
            
            return EXIT_FAILURE;
        } 
    }

    return EXIT_SUCCESS;
}


/**
 *  @brief Domain Datum Defualt Constructor
 *
 *  @details Create empty datum struct
 */
libvirt::domain::datum_t::datum_t(): 
    domain(nullptr) {}


/**
 *  @brief Domain Datum Move Constructor
 *
 *  @param UUID:                domain's UUID
 *  @param domain: domain's     libvirt API handle
 *  @param number_of_vCPUs:     amount of vCPUs running on this domain
 *  @param balloon memory used: memory used by domain's balloon driver in bytes
 *  @param domain memory extra: memory unused by domain in bytes
 *  @param domain memory limit: memory limit enforced by domain in bytes
 *  @param domain memory delta: memory change determined by scheduler in bytes
 *
 *  @details Copy over simple values and move domain handle to new object
 */
libvirt::domain::datum_t::datum_t
(
    libvirt::domain::datum_t &&other
) noexcept:
    uuid(other.uuid),
    domain(std::move(other.domain)), 
    number_of_vCPUs(other.number_of_vCPUs),
    balloon_memory_used(other.balloon_memory_used),
    domain_memory_extra(other.domain_memory_extra),
    domain_memory_limit(other.domain_memory_limit),
    domain_memory_delta(other.domain_memory_delta)
{}


/**
 *  @brief Domain Datum Move Assigment
 *
 *  @details Copy over simple values and move domain handle to existing object
 */
libvirt::domain::datum_t &
libvirt::domain::datum_t::operator=
(
    libvirt::domain::datum_t &&other
) noexcept
{
    if (this != &other)
    {
        this->uuid                = other.uuid;
        this->domain              = std::move(other.domain);
        this->number_of_vCPUs     = other.number_of_vCPUs; 
        this->balloon_memory_used = other.balloon_memory_used; 
        this->domain_memory_extra = other.domain_memory_extra; 
        this->domain_memory_extra = other.domain_memory_extra; 
        this->domain_memory_limit = other.domain_memory_limit; 
        this->domain_memory_delta = other.domain_memory_delta; 
    }

    return *this;
}
