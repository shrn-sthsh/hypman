#!/usr/bin/env python

from typing import Final

from env.lib.hyp_control import hypervisor_manager
from env.lib.test_util   import testing_utilities
 

DOMAIN_PREFIX: Final[str] = "test"

if __name__ == '__main__':
    manager:      hypervisor_manager = hypervisor_manager()
    domain_names: list[str]          = manager.domain_names(DOMAIN_PREFIX)

    addresses: list[str] = testing_utilities.get_ip_addresses(domain_names)
    address_value_table: dict[str, list[int]] = {address: [] for address in addresses}

    testing_utilities.run_test_case(
        "killall run", 
        address_value_table
    )
