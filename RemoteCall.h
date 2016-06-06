#pragma once

#include "RemoteCallClient.h"
#include "RemoteCallServer.h"

#define REMOTE_FUNCTION_DECL(f) \
    f##DeclRemoteFunctionReturn(); \
    using f##DeclRemoteFunctionReturnType = decltype(f##DeclRemoteFunctionReturn()); \
    template <typename Transport> \
    struct f##RemoteFunctionStruct \
    { \
        const Transport& trt_; \
	f##RemoteFunctionStruct(const Transport& trt) : trt_(trt) {} \
	template <typename ...Args> \
	f##DeclRemoteFunctionReturnType operator () (Args&&...args) \
	    { return trt_.Call(RemoteCall::GetFunctionInfo<f##DeclRemoteFunctionReturnType, Args...>(#f, decltype(&f##RemoteFunction)(), args...)); } \
    }; \
    template <typename Transport> \
    f##RemoteFunctionStruct<Transport> f(const Transport& trt) { return f##RemoteFunctionStruct<Transport>(trt); } \
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
    using m##MethodReturnType = decltype(RemoteCall::MethodReturnType(&m##MethodReturn)); \
    template <typename Transport> \
    struct m##MethodStruct \
	{ \
		const Transport& trt_; \
		std::string instanceId_; \
		m##MethodStruct(const Transport& trt, const std::string& instanceId) : trt_(trt), instanceId_(instanceId) {} \
		template <typename ...Args> \
		m##MethodReturnType operator() (Args&&...args) \
			{ return trt_.Call(RemoteCall::GetMethodInfo<m##MethodReturnType, Interface, Args...>(instanceId_, #m, decltype(&Interface::m##Method)(), args...)); } \
	}; \
	template <typename Transport> \
	m##MethodStruct<Transport> m(const Transport& trt) { return m##MethodStruct<Transport>(trt, instanceId_); } \
	virtual m##MethodReturnType m##Method

// Implement remote method
#define REMOTE_METHOD_IMPL(m) \
    m##MethodReturn() override { return m##MethodReturnType(); } \
    bool m##registerMethod = RemoteCall::RegisterMethod((Interface*)this, #m, &Interface::m##Method); \
    m##MethodReturnType m##Method
