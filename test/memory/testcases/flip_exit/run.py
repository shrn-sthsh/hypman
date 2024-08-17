#!/usr/bin/env python

from typing import Final

from env.lib.hyp_control import hypervisor_manager
from env.lib.test_util   import testing_utilities
 

DOMAIN_PREFIX: Final[str] = "test"

if __name__ == '__main__':
    manager: hypervisor_manager = hypervisor_manager()
    domain_names: list[str] = manager.domain_names(DOMAIN_PREFIX)

    manager.set_filtered_domains_memory(DOMAIN_PREFIX, 512)
    addresses: list[str] = testing_utilities.get_ip_addresses(domain_names)
    address_value_table: dict[str, str] = {}

    round: int = 0
    for address in addresses:
        # Barrier stop
        if round % 2 == 0: 
            address_value_table[address] = "T"

        else:
            address_value_table[address] = "F"

        round += 1

    testing_utilities.run_test_case(
        "~/test/memory/testcases/3/test {}", 
        address_value_table
    )
