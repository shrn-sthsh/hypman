#!/usr/bin/env python3

from typing import Final

from lib.hyp_control import hypervisor_manager
from lib.test_util   import testing_utilities

import sys


DOMAIN_PREFIX: Final[str] = "test"

def main(args: list[str]):
    if len(args) != 1:
        print('Usage: ./assign_file_to_all_vm.py [file]')
        exit(1)

    file_name:                str = sys.argv[1]
    manager:   hypervisor_manager = hypervisor_manager()

    domains:      list[str] = manager.domain_names()
    test_domains: list[str] = [machine for machine in domains if machine.startswith(DOMAIN_PREFIX)]
    if not test_domains:
        print(f"No virtual machines running with {DOMAIN_PREFIX} prefix; aborting process")
        exit(1)

    testing_utilities.copy_files(file_name, test_domains)


if __name__ == '__main__':
    main(sys.argv[1:])
