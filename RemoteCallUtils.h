#pragma once

#include <string>
#include <sstream> 

#include "RemoteCallUtils.h"

namespace RemoteCall
{
    template<typename C, typename Ret, typename ...Args>
    auto MethodReturnType(Ret(C::*)(Args...)) -> Ret;

    struct ToString: public std::stringstream
    {
        template <typename T>
        ToString& operator << (const T& t)
        {
            ((std::stringstream&)*this) << t;

            return *this;
        }

        operator std::string()
        {
            return ((std::stringstream&)(*this)).str();
        }
    };

    struct CallLocation
    {
        const char* fName_;
        int line_;

        operator std::string() const
        {
            return ToString() << fName_ << ':' << line_ << ".";
        }
    };
}
