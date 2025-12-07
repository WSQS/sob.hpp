#pragma once
#include <sstream>
#include <iostream>
namespace sopho
{

    template <typename T>
    struct BuildTarget : public T
    {
        void build()
        {
            std::stringstream ss{};
            for (const auto &arg : this->args)
            {
                ss << arg << " ";
            }
            auto command = ss.str();
            std::system(command.data());
            std::cout << __FUNCTION__ << "finished" << std::endl;
        }
    };
}
