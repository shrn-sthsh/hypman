#!/usr/bin/env python

from typing import Final

from env.lib.hyp_control import hypervisor_manager
from env.lib.test_util   import testing_utilities
 

DOMAIN_PREFIX: Final[str] = "test"

if __name__ == '__main__':
    manager:      hypervisor_manager = hypervisor_manager()
    domain_names: list[str]          = manager.domain_names(DOMAIN_PREFIX)

    manager.set_filtered_domains_memory(DOMAIN_PREFIX, 512) 
    addresses: list[str] = testing_utilities.get_ip_addresses(domain_names)
    address_value_table: dict[str, list[int]] = {addresses[0]: []}

    testing_utilities.run_test_case(
        "~/test/memory/testcases/1/test", 
        address_value_table
    )
