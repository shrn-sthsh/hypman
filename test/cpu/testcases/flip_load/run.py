#!/usr/bin/env python

from typing import Final

from env.lib.hyp_control import hypervisor_manager
from env.lib.test_util   import testing_utilities


DOMAIN_PREFIX: Final[str] = "test"

if __name__ == '__main__':
    manager: hypervisor_manager = hypervisor_manager()
    domain_names: list[str] = manager.domain_names(DOMAIN_PREFIX)

    for domain_name in domain_names:
        manager.unmap_vCPU(domain_name, 0)

    addresses: list[str] = testing_utilities.get_ip_addresses(domain_names)
    address_value_table: dict[str, int] = {}

    round: int = 0
    for address in addresses:
        # Process running at 100%
        if round % 2 == 0: 
            address_value_table[address] = 250_000

        # Process running at 50%
        else:
            address_value_table[address] = 50_000

        round += 1

    testing_utilities.run_test_case(
        "~/test/cpu/testcases/3/test {}",
        address_value_table
    )
