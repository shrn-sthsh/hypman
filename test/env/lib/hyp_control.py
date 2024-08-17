#!/usr/bin/env python

import libvirt
import time


"""
Provides control mechanisms for manually controlling hypervisor

Used to create environment for testing hypman
"""
class hypervisor_manager:
    def __init__(self, uri='qemu:///system'):
        self.conn: libvirt.virConnect = libvirt.open(uri)

    def __del__(self):
        self.conn.close()


    """ Domain Utilities """
    # List of filtered domain names
    def domain_names(self, filter=None) -> list[str]:
        domain_uuids: list[libvirt.libvirtmod.virDomainUUID] = self.conn.listDomainsID()
        domain_names: list[str] = [self.conn.lookupByID(id).name() for id in domain_uuids]

        if filter:
            return [domain_name for domain_name in domain_names if domain_name.startswith(filter)]

        return domain_names

    # Domain handler
    def __domain(self, domain_name: str) -> libvirt.virDomain:
        try:
            domain: libvirt.virDomain = self.conn.lookupByName(domain_name)

        except libvirt.libvirtError:
            error: str = f"Libvirt unable to find domain {domain_name}"
            print(error)
            raise(Exception(error))

        return domain

    """ Domain Controls """
    # Start up
    def start_domain(self, domain_name: str): 
        try:
            domain: libvirt.virDomain = self.__domain(domain_name)
            domain.create()

        except libvirt.libvirtError:
            print(f"Unable to start domain {domain_name}; check if already running")

    def start_filtered_domains(self, filter: str, interval=3):
        domain_names: list[str] = self.domain_names(filter)

        for domain in domain_names:
            self.start_domain(domain)

        time.sleep(interval)
 
    # Shutdown
    def shutdown_domain(self, domain_name: str):
        try:
            domain: libvirt.virDomain = self.__domain(domain_name)
            domain.shutdown()

        except Exception:
            print(f"Error incurred shutting down domain {domain_name}")
 
    def shutdown_filtered_domains(self, filter: str, interval=5):
        domain_names: list[str] = self.domain_names(filter)

        for domain in domain_names:
            self.shutdown_domain(domain)

        time.sleep(interval)

    # Kill
    def kill_domain(self, domain_name: str):
        try:
            domain: libvirt.virDomain = self.__domain(domain_name)
            domain.destroy()

        except Exception:
            print(f"Error incurred killing domain {domain_name}")
    
    def kill_filtered_domains(self, filter: str, interval=5):
        domain_names: list[str] = self.domain_names(filter)

        for domain in domain_names:
            self.kill_domain(domain)

        time.sleep(interval)
    
    # Memory
    def set_memory(self, domain_name: str, memory: int):
        try: 
            domain: libvirt.virDomain = self.__domain(domain_name)
            domain.setMemory(memory * (1 << 10))

        except Exception:
            print(f"Error incurred setting memory for domain {domain_name}")

    def set_filtered_domains_memory(self, filter: str, memory: int):
        domain_names: list[str] = self.domain_names(filter)

        for domain_name in domain_names:
            self.set_memory(domain_name, memory)

    # Memory Limit
    def set_memory_limit(self, domain_name: str, memory: int):
        try:
            domain: libvirt.virDomain = self.__domain(domain_name)
            domain.setMaxMemory(memory * (1 << 10))

        except Exception:
            print(f"Error incurred setting memory limit for domain {domain_name}")
  
    def set_filtered_domains_memory_limit(self, filter: str, memory: int):
        domain_names: list[str] = self.domain_names(filter)

        for domain in domain_names:
            self.set_memory_limit(domain, memory)



    """ CPU Utilities """
    # Active and running pCPU count
    def number_of_pCPUs(self) -> int:
        hostinfo: libvirt.libvirtmod.virNodeInfo = self.conn.getInfo()
        return hostinfo[4] * hostinfo[5] * hostinfo[6] * hostinfo[7]

    # pCPU bitmap for mapping to a pCPU
    def __pCPU_bitmap(self, rank: int) -> tuple[bool, ...]:
        mapping = [False] * self.number_of_pCPUs()
        mapping[rank] = True

        return tuple(mapping)

    """ CPU Controls """
    # Mapping a vCPU to a pCPU
    def map_vCPU_to_pCPU(self, domain_name: str, vCPU_rank: int, pCPU_rank: int):
        try:
            domain:  libvirt.virDomain = self.__domain(domain_name)
            mapping: tuple[bool, ...]  = self.__pCPU_bitmap(pCPU_rank)

            domain.pinVcpu(vCPU_rank, mapping)

        except:
            print(
                f"Error incurred mapping vCPU {vCPU_rank} "
                f"of domain {domain_name} to pCPU {pCPU_rank}"
            )

    # Unmapping a vCPU from a pCPU
    def unmap_vCPU(self, domain_name: str, vCPU_rank: int): 
        try:
            domain:  libvirt.virDomain = self.__domain(domain_name)
            mapping: tuple[bool, ...]  = tuple([True] * self.number_of_pCPUs())

            domain.pinVcpu(vCPU_rank, mapping)

        except Exception:
            print(f"Error incurred unmapping vCPU {vCPU_rank} of domain {domain_name}")

