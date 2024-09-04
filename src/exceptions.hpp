#include "definitions.h"

namespace ArbSimulation
{
    class Exception : public std::runtime_error
    {
    private:
        std::string _message;

    public:
        explicit Exception(const std::string &message) : _message(message), std::runtime_error(message) {};
        const char *what() const noexcept override
        {
            return _message.c_str();
        }
    };
}