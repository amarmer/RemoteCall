#pragma once

#include "RemoteCall.h"

struct ABC
{
   std::string s_;
   int n_;
};

// Serialization functions of ABC
inline RemoteCall::Serializer& operator << (RemoteCall::Serializer& writer, const ABC& abc)
{
   return writer << abc.s_ << abc.n_;
}

inline RemoteCall::Serializer& operator >> (RemoteCall::Serializer& reader, ABC& abc)
{
   return reader >> abc.s_ >> abc.n_;
}


REMOTE_INTERFACE(ITestCallback)
{
    virtual void REMOTE_METHOD_DECL(CallFromServer)(int n) = 0;
};


REMOTE_INTERFACE(ITest)
{
    virtual void REMOTE_METHOD_DECL(UpdateData)(const std::string& s, int n) = 0;
    virtual bool REMOTE_METHOD_DECL(GetData)(std::string& s, int& n) = 0;
};


std::tuple<int, std::string> REMOTE_FUNCTION_DECL(TestSync)(std::vector<ABC>& vABC, const std::map<int, std::string>& m);
void REMOTE_FUNCTION_DECL(SetTestCallback)(const std::string&, ITestCallback*, int n);
void REMOTE_FUNCTION_DECL(TriggerTestCallback)();
ITest* REMOTE_FUNCTION_DECL(TestClassFactory)(const std::string& s, const std::string& c);

