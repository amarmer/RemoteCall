#pragma once

#include "RemoteCall.h"

struct ABC
{
   std::string s_;
   int n_;
};


inline RemoteCall::Serializer& operator << (RemoteCall::Serializer& writer, const ABC& abc)
{
   return writer << abc.s_ << abc.n_;
}

inline RemoteCall::Serializer& operator >> (RemoteCall::Serializer& reader, ABC& abc)
{
   return reader >> abc.s_ >> abc.n_;
}


int REMOTE_FUNCTION_DECL(Func1)(std::string& s, char c);
std::tuple<int, std::string> REMOTE_FUNCTION_DECL(Func2)(std::vector<ABC>& vABC, const ABC& abc);
void REMOTE_FUNCTION_DECL(Func3)(std::map<int, std::string>& m);

REMOTE_INTERFACE(ITest)
{
    virtual void REMOTE_METHOD_DECL(Method1)() = 0;
    virtual int REMOTE_METHOD_DECL(Method2)(std::string& s, char c) = 0;
};


