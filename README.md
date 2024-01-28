# Hypervisor Resource Manager

An open source manager for hypervisor computation and memory resources.


## Introduction

In a virtualized environment managed by a hypervisor, each virtual machine, or domain, is allocated a certain number of virtual CPUs, or vCPUs. These vCPUs are virtualized representations of the physical CPUs, or pCPUs, in the host machine. Each virtual machine operates independently within its own domain and interacts with its allocated vCPUs as if they were physical processors. 

Each domain is also dynamically allocated a portion of system memory using a buffering principle by the manager, changing in size as demands change and in sync with other domains on the system.  The manager ensures security over each virtual machine allocated portion as well as providing memory control equivalent to that of what a native operating system has. 

This hypervisor resource manager handles the mapping and scheduling of these virtual processors onto the physical processors, ensuring that each virtual machine gets its fair share of computational resources.  It also provides each virtual machine with a share of the system memory relative to the demands placed on said virtual machine.