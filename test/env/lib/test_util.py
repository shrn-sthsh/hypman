#!/usr/bin/env python

import os
import subprocess


class testing_utilities:
    @staticmethod
    def get_ip_addresses(domain_names: list[str]) -> list[str]:
        command = 'uvt-kvm ip {}'
        return [os.popen(command.format(domain)).read().strip() for domain in domain_names]

    @staticmethod
    def copy_files(file_path: str, domain_names: list[str]):
        addresses = testing_utilities.get_ip_addresses(domain_names) 

        for address in addresses:
            print('Copying {} to {}.'.format(file_path, address))
            os.popen('scp -r {} ubuntu@{}:~/'.format(file_path, address))

    @staticmethod
    def run_test_case(
            command: str, 
            address_value_table: dict[str, list[str]]
    ) -> dict[str, subprocess.Popen]:

        null_device = open(os.devnull, 'w')
        process_table = dict()

        for address, arguments in address_value_table.items():
            ssh_command = "ssh ubuntu@{} '".format(address)
            ssh_command += command + "'"

            try:
                process = subprocess.Popen(
                    ssh_command.format(arguments), 
                    stdout=null_device, 
                    shell=True
                )
                process_table[address] = process
            except OSError:
                raise Exception(
                    "Problems when executing the command {} with arguments {}"
                        .format(ssh_command, arguments)
                )
            except ValueError:
                raise Exception(
                    "Invalid arguments when executing the command {} with arguments {}"
                        .format(ssh_command, arguments)
                )

        return process_table



