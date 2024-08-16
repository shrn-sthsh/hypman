#!/usr/bin/env python

from __future__ import print_function

from lib.hypervisor_util import hypervisor_manager


VM_PREFIX: str = "test"

if __name__ == '__main__':
    manager = hypervisor_manager()
    manager.shutdown_filtered_domains(VM_PREFIX)
