#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>

#include "record.hpp"


void util::log::record
(
    const std::string             &&message,
    const util::log::message_type   type,
    const bool                      flush
) noexcept
{
    const util::clock::time_t time = util::clock::time();

    std::string prefix;
    bool        error_type = false;

    switch (type) 
    {
    // Log types
    case STATUS:
        prefix = "STATUS:";
        break;
    case START:
        prefix = "START:";
        break;
    case STOP:
        prefix = "STOP:";
        break;

    // Error types
    case FLAG:
        prefix = "FLAG:";
        error_type = true;
        break;
    case ERROR:
        prefix = "ERROR:";
        error_type = true;
        break;
    case ABORT:
        prefix = "ABORT:";
        error_type = true;
        break;

    default:
        std::cerr << "ERROR: Unkown error type" << std::endl;
        return;
    }

    if (error_type)
    {
        std::cerr << time << log::SPACE << prefix << log::SPACE 
            << message << log::NEW_LINE;
        if (flush)
            std::cerr.flush();
    }

    else 
    {
        std::clog << time << log::SPACE << prefix << log::SPACE 
            << message << log::NEW_LINE;
        if (flush)
            std::clog.flush(); 
    }
}

util::clock::time_t util::clock::time() noexcept
{
    try
    {
        std::time_t  time  = std::time(nullptr);
        std::tm     *local = std::localtime(&time);

        std::stringstream stream;
        stream << "[" << std::put_time(local, "%Y-%m-%d %H:%M:%S") << "]";

        return stream.str();
    }

    catch (const std::exception &exception)
    {
        return "[N/A N/A]";
    }
}
