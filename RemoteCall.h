#pragma once

#include "RemoteCallClient.h"
#include "RemoteCallServer.h"
#include "RemoteCallTransport.h"
#include "RemoteCallInterface.h"

// Declare remote function
#define REMOTE_FUNCTION_DECL(f) \
    f##DeclRemoteFunctionReturn(); \
    template <typename ...Args> \
    RemoteCall::Client::FunctionInfo<decltype(f##DeclRemoteFunctionReturn())> f(Args&&...args) \
        { return RemoteCall::Client::GetFunctionInfo<decltype(f##DeclRemoteFunctionReturn()), Args...>(#f, decltype(&f##RemoteFunction)(), args...); } \
    decltype(f##DeclRemoteFunctionReturn()) f##RemoteFunction

// Implement remote function
#define REMOTE_FUNCTION_IMPL(f) \
    static f##ImplRemoteFunctionReturn(); \
    static bool f##registerRemoteFunction = RemoteCall::Server::RegisterFunc(#f, &f##RemoteFunction); \
    static decltype(f##ImplRemoteFunctionReturn()) f##RemoteFunction


// Declare remote interface
#define REMOTE_INTERFACE(i) \
   namespace { extern const char i##remoteInterface[] = #i; } \
   struct i: public RemoteCall::TRemoteInterface<i, i##remoteInterface>

// Declare remote method
#define REMOTE_METHOD_DECL(m) \
    m##DeclMethodReturn() = 0; \
    using m##DeclMethodReturnType = decltype(RemoteCall::MethodReturnType(&m##DeclMethodReturn)); \
    template <typename ...Args> \
    RemoteCall::Client::MethodInfo<m##DeclMethodReturnType> m(Args&&...args) \
        { return RemoteCall::Client::GetMethodInfo<m##DeclMethodReturnType, Interface, Args...>(className_, instanceId_, #m, decltype(&Interface::m##Method)(), args...); } \
    virtual m##DeclMethodReturnType m##Method

// Declare remote class
#define REMOTE_CLASS(c) \
    class c; \
    static bool c##registerRemoteClass = RemoteCall::Server::RegisterClass<c>(#c); \
    class c

// Implement remote method
#define REMOTE_METHOD_IMPL(m) \
    m##DeclMethodReturn() override { return m##DeclMethodReturnType(); } \
    bool m##registerMethod = RemoteCall::Server::RegisterMethod((Interface*)this, #m, &Interface::m##Method); \
    m##DeclMethodReturnType m##Method

// CallLocation used in Exception (passing 2 variables in local calls is not much overhead in remote call)
#define REMOTE_CALL_LOCATION RemoteCall::CallLocation{__FILE__, __LINE__}

// Create class instance
#define REMOTE_NEW(className) GetClassAux(REMOTE_CALL_LOCATION, className)

// Delete class instance
#define REMOTE_DELETE(pRemoteInterface) DeleteInterface(REMOTE_CALL_LOCATION, pRemoteInterface)

// Call remote function
#define REMOTE_CALL(call) GetCallAux(REMOTE_CALL_LOCATION) = call

