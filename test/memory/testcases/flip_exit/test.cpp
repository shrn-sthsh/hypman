#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <thread>
#include <unistd.h>

#include <log/record.hpp>

int 
main(int argc, char *argv[])
{
    if (argc != 2)
    {
        util::log::record
        (
            "Must provide argument for barrier exit", 
            util::log::type::ABORT
        );

        return EXIT_FAILURE; 
    }
    bool barrier_exit = std::string(argv[1]) == "T";

    const static std::size_t page_size 
        = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    const static std::size_t barrier_amount
        = (512 << 20) / page_size;

    void *curr_block = nullptr;
    void *next_block = nullptr;

    size_t index = 0;
    while (true) 
    {
        // Exit if barrier is reached and set
        if (index >= barrier_amount && barrier_exit)
            break;

        // Acquire new block if possible
        next_block = std::realloc(curr_block, page_size * (index + 1));
        if (next_block == nullptr) 
            break;

        // Mark block
        curr_block = next_block;
        static_cast<char *>(curr_block)[page_size * index++] = 'a';

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    std::free(curr_block);

    return EXIT_SUCCESS;
}
