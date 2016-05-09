#pragma once

#include <string>

namespace RemoteCall
{
    struct RemoteInterface
    {
        virtual std::string GetInterfaceName() { return std::string(); }

        virtual ~RemoteInterface() {}

        std::string className_;
        std::string instanceId_;
    };


    template <typename I, const char* interfaceName>
    struct TRemoteInterface: public RemoteInterface
    {
        typedef I Interface;

        static std::string InterfaceName()
        {
            return interfaceName;
        }

        std::string GetInterfaceName() override
        { 
            static_assert(sizeof(I) == sizeof(RemoteInterface), "Member variables are not allowed in an interface");
            
            return interfaceName; 
        }
    };
}