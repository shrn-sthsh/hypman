#!/usr/bin/env python

from typing import Final

from lib.hyp_control import hypervisor_manager


DOMAIN_PREFIX: Final[str] = "test"
MEMORY_LIMIT: int = 2 * (1 << 10)

if __name__ == '__main__':
    manager: hypervisor_manager = hypervisor_manager()
    manager.set_filtered_domains_memory_limit(DOMAIN_PREFIX, MEMORY_LIMIT)
