#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <thread>

#include <log/record.hpp>
#include <stat/statistics.hpp>

int 
main(int argc, char *argv[]) 
{
    util::stat::uint_t round = 0;
    int count = 100'000;

    // Count argument must be a positive integer if provided
    if (argc == 2)
    {
        std::function<bool(const char *)> is_count = [](const char *string) 
        {
            return std::all_of
            (
                string, string + std::strlen(string), 
                [](unsigned char character) 
                {
                    return std::isdigit(character);
                }
            );
        };

        if (!is_count(argv[1]))
        {
            util::log::record
            (
                "Count argument must be a positive integer", 
                util::log::type::ABORT
            );

            return EXIT_FAILURE;
        }

        std::chrono::microseconds interval(std::atoi(argv[1]));
    }

    // Busy spin
    while (true) 
    {
        if (++round < count) 
            continue;

        round = 0;
        std::this_thread::sleep_for(std::chrono::microseconds(400));
    }

    return EXIT_SUCCESS;
}
