#pragma once

#include <string>

namespace RemoteCall
{
    class Exception : public std::exception
    {
    public:
        enum ERROR { SERVER_ERROR = 0, NOT_IMPLEMENTED, RPC_ERROR };

        Exception(const std::string& fName, ERROR error, const std::string& description = "")
            : std::exception(info(fName, error, description).c_str()), fName_(fName), error_(error) {}

        std::string Function()
        {
            return fName_;
        }

        ERROR Error()
        {
            return error_;
        }

    private:
        static std::string info(const std::string& fName, ERROR error, const std::string& description = "")
        {
            char sError[128];
            sprintf_s(sError, "%d", error);

            return fName + ", error: " + sError + " " + description;
        }

    private:
        std::string fName_;
        ERROR error_;
    };
}