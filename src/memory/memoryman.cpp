#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

#include <lib/signal.hpp>
#include <log/record.hpp>

#include "memoryman.hpp"


static os::signal::signal_t exit_signal = os::signal::SIG_NULL;
    
int main (int argc, char *argv[]) 
{
    // Validate command arguments
	if (argc != 2)
	{
        util::log::record("Usage follows as memoryman <interval (ms)>", util::log::ABORT);
		return 1;
	} 
    
    std::function<bool(const char *)> is_interval = [](const char *arg) 
    {
        return std::all_of
        (
            arg, arg + std::strlen(arg), 
            [](unsigned char ch) 
            {
                return std::isdigit(ch);
            }
        );
    };
    if (!is_interval(argv[1]))
    {
        util::log::record("Interval argument must be a positive integer", util::log::ABORT);
        return 1;
    }
    std::chrono::milliseconds interval(std::atoi(argv[1]));
    
    // Connect to QEMU
    libvirt::connection_t conn
    (
        libvirt::virConnectOpen("qemu:///system"),
        [](libvirt::virConnect *conn)
        {
            libvirt::virConnectClose(conn);
        }
    );
	if (conn == nullptr)
	{
        util::log::record("Unable to make connection to QEMU", util::log::ABORT);
		return 1;
	}

    // Check for exit signal
    os::signal::signal
    (
        os::signal::SIG_INT,
        [](os::signal::signal_t interrupt)
        {
            exit_signal = true; 
        }
    );

	while (!exit_signal)
	{
        // manager::load_balancer(conn, interval);
        std::this_thread::sleep_for(interval);
	}

	return 0;
}
