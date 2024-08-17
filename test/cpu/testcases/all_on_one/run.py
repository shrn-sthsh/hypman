#!/usr/bin/env python

from typing import Final

from env.lib.hyp_control import hypervisor_manager
from env.lib.test_util   import testing_utilities
 

DOMAIN_PREFIX: Final[str] = "test"

if __name__ == '__main__':
    manager: hypervisor_manager = hypervisor_manager()
    domain_names: list[str] = manager.domain_names(DOMAIN_PREFIX)

    for domain_name in domain_names:
        manager.map_vCPU_to_pCPU(domain_name, 0, 0)

    addresses: list[str] = testing_utilities.get_ip_addresses(domain_names)
    address_value_table: dict[str, int] = {address: 100_000 for address in addresses}

    testing_utilities.run_test_case(
        "~/test/cpu/testcases/2/test {}", 
        address_value_table
    )
