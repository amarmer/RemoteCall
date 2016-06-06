#pragma once

#include "RemoteCallUtils.h"

namespace RemoteCall
{
    using NoException = std::string;

    class Exception : public std::exception
    {
    public:
        enum ErrorType {NoError, TransportError, ServerError, InvalidFunction, InvalidClassInstance, InvalidMethod};

        Exception() {}

        Exception(ErrorType error, const std::string& description = "")
            : std::exception(format(error, description).c_str()), error_(error)
         {}

        Exception(int err, const std::string& what)
            : std::exception(what.c_str()), error_((Exception::ErrorType)err)
        {}

        ErrorType Error() const
        { 
            return error_; 
        }

    private:
        std::string format(ErrorType error, const std::string& description)
        {
            return ToString() << "Error " << error << ". " << description;
        }

    private:
        ErrorType error_ = NoError;
    };


    inline Serializer& operator << (Serializer& writer, const Exception& e)
    {
        return writer << (int)e.Error() << e.what();
    }

    inline Serializer& operator >> (Serializer& reader, Exception& e)
    {
        std::string what;
        int err;
        reader >> err >> what;

        e = Exception(err, what);

        return reader;
    }
}
