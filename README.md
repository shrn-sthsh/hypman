# hypman -- a Hypervisor Resource Manager

An open source manager for hypervisor computational and memory resources.


## Introduction

In a virtualized environment managed by a hypervisor, each virtual machine, also called a guest OS or a domain, is allocated a certain number of virtual CPUs, or vCPUs. These vCPUs are virtualized representations of the physical CPUs, or pCPUs, on the host machine.
Each virtual machine operates independently and interacts with its allocated vCPUs as if they were physical processors. 

Each domain is also dynamically allocated a portion of system memory using a buffering principle by the manager, changing in size as its own and other domains' demands change and in sync with other domains on the system.
The manager ensures security over each virtual machine allocated portion as well as providing memory control equivalent to that of what a native operating system has. 

This hypervisor resource manager handles the mapping and scheduling of these virtual processors onto the physical processors, ensuring that each virtual machine gets a fair share of computational resources based to its demands relative to others.
Similarly, it also provides each virtual machine with a share of the system memory relative to the demands placed on said virtual machine.
