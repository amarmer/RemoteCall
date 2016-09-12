#pragma once

#include "RemoteCallClient.h"
#include "RemoteCallServer.h"

// Declare remote function
#define REMOTE_FUNCTION_DECL(f) \
    f##DeclRemoteFunctionReturn(); \
    using f##DeclRemoteFunctionReturnType = decltype(f##DeclRemoteFunctionReturn()); \
    template <typename ...Args> \
	RemoteCall::FunctionInfo<RemoteCall::UseSendReceive<f##DeclRemoteFunctionReturnType, Args...>(), f##DeclRemoteFunctionReturnType> \
	f(Args&&...args) { return RemoteCall::GetFunctionInfo<Args...>(#f, decltype(&f##RemoteFunction)(), args...); } \
    decltype(f##DeclRemoteFunctionReturn()) f##RemoteFunction

// Implement remote function
#define REMOTE_FUNCTION_IMPL(f) \
    f##ImplRemoteFunctionReturn(); \
    static bool f##registerRemoteFunction = RemoteCall::RegisterFunc(#f, &f##RemoteFunction); \
    static decltype(f##ImplRemoteFunctionReturn()) f##RemoteFunction


// Declare remote interface
#define REMOTE_INTERFACE(i) \
   struct i: public RemoteCall::TRemoteInterface<i>

// Declare remote method
#define REMOTE_METHOD_DECL(m) \
    m##MethodReturn() = 0; \
    using m##MethodReturnType = decltype(RemoteCall::MethodReturnType(&Interface::m##MethodReturn)); \
    template <typename ...Args> \
	RemoteCall::MethodInfo<RemoteCall::UseSendReceive<m##MethodReturnType, Args...>(), m##MethodReturnType> \
	m(Args&&...args) { return RemoteCall::GetMethodInfo<m##MethodReturnType, Interface, Args...>(instanceId_, #m, decltype(&Interface::m##Method)(), args...); } \
    virtual m##MethodReturnType m##Method

// Implement remote method
#define REMOTE_METHOD_IMPL(m) \
    m##MethodReturn() override { return m##MethodReturnType(); } \
    bool m##registerMethod = RemoteCall::RegisterMethod((Interface*)this, #m, &Interface::m##Method); \
    m##MethodReturnType m##Method

