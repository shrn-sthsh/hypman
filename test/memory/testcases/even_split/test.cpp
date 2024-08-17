#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <thread>
#include <unistd.h>


int 
main()
{
    const static std::size_t page_size 
        = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));

    void *curr_block = nullptr;
    void *next_block = nullptr;

    size_t index = 0;
    while (true) 
    {
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
