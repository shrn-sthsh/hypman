#!/usr/bin/env python

from __future__ import print_function

from lib.hypervisor_util import hypervisor_manager


VM_PREFIX: str = "test"
MEMORY_LIMIT: int = 2 * (1 << 10)

if __name__ == '__main__':
    manager = hypervisor_manager()
    manager.set_filtered_domains_memory_limit(VM_PREFIX, MEMORY_LIMIT)
