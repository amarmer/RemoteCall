#pragma once

#include "RemoteCallClient.h"
#include "RemoteCallServer.h"
#include "RemoteCallTransport.h"

// DECLARE_REMOTE is used to declare remote function
#define DECLARE_REMOTE(f) \
   DeclRemoteCallReturn##f(); \
   template <typename ...Args> \
   RemoteCall::Client::CallInfo<decltype(DeclRemoteCallReturn##f())> RemoteCallProxy##f(Args&&...args) { \
      return RemoteCall::Client::GetCallInfo<decltype(DeclRemoteCallReturn##f()), Args...>(args..., #f, decltype(&RemoteCall##f)()); \
   }; \
   decltype(DeclRemoteCallReturn##f()) RemoteCall##f


// CALL_REMOTE is used to call remote function
#define CALL_REMOTE(f) GetAux() = RemoteCallProxy##f


// IMPLEMENT_REMOTE is used to implement remote function on server
#define IMPLEMENT_REMOTE(f) \
   static ImplRemoteCallReturn##f() { \
      static_assert(std::is_same<decltype(ImplRemoteCallReturn##f()), decltype(DeclRemoteCallReturn##f())>::value, "Return is different than declaration"); \
      return decltype(ImplRemoteCallReturn##f())(); \
   }; \
   static RemoteCall::Server::NameFunc s_RemoteCallNameFunc##f(#f, RemoteCall::Server::Func(&RemoteCall##f)); \
   static decltype(ImplRemoteCallReturn##f()) RemoteCall##f
