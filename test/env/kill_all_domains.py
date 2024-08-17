#!/usr/bin/env python

from typing import Final

from lib.hyp_control import hypervisor_manager


DOMAIN_PREFIX: Final[str] = "test"

if __name__ == '__main__':
    manager: hypervisor_manager = hypervisor_manager()
    manager.kill_filtered_domains(DOMAIN_PREFIX)

