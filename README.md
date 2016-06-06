#### RemoteCall - C++ IPC framework for synchronous and asynchronous communication

There are many ways to use IPC/RPC, as it is described in http://en.wikipedia.org/wiki/Remote_procedure_call, 
for instance XML-RPC, JSON-RPC, Protocol Buffers (protobufs), COM/DCOM, CORBA, etc.

Advantage of RemoteCall framework, it is strognly typed and declares, implements and calls remote C++ functions identically 
to how they are used locally in the same process. RemoteCall supports synchronous and asynchronous 
calls, and supports functions, interfaces, methods.

Bellow is an explanation how to use it.

#####Framework
*RemoteCall.h* contains framework macros. This header should be included in declarion header file.

#####Transport

RemoteCall can use any transport for IPC, for instance socket, pipes, etc. 
Transport implementation is not part of the framework.
Client's transport class should be derived from 'RemoteCall::Transport'. It is used for synchronous and asynchronous calls.
For synchrnous communication should be implemented SendReceive, for asynchrnous should be implemented Send or SendReceive.
It is described and implemented in TestClient.cpp. Bellow instance of Transport class is refered as 'transport'.


#####RemoteCall macros descriptions

######Function declaration: REMOTE_FUNCTION_DECL(funcName)
```C++
tuple<std::string, int> REMOTE_FUNCTION_DECL(Test)(std::vector<std::string>& vInOut, int n, const string& s);
```
######Function implementation: REMOTE_FUNCTION_IMPL(funcName)

```C++
tuple<string, int> REMOTE_FUNCTION_IMPL(Test)(std::vector<std::string>& vInOut, int n, const string& s)
{
   vInOut.push_back(s);
   
   tuple<string, int> tpl;
   get<0>(tpl) = s;
   get<1>(tpl) = n;

   return tpl;
}
```
######Function call: FunctionName(transport)(parameters). 

```C++
vector<string> vInOut = {"In1", "In2"};
auto ret = Test(transport)(vInOut, 12345, "Test");
```
######Interface declaration: REMOTE_INTERFACE(interfaceName)

```C++
REMOTE_INTERFACE(ITest) 
{
};
```

######Method declaration: REMOTE_METHOD_DECL(className)

```C++
REMOTE_INTERFACE(ITest)
{
    virtual int REMOTE_METHOD_DECL(TestMethod)(string& s) = 0;
};
```

######Method Implementation: REMOTE_METHOD_IMPL(methodName)

```C++
struct CTest: public ITest 
{
    int REMOTE_METHOD_IMPL(TestMethod)(string& s) override
    {
        s += "Server";
        
        return s.size();
    }
};
```
######Method call: InterfacePointer->MethodName(trt)(parameters) 
        
```C++
strins sInOut = "Test";
auto ret = pTest->TestMethod(transport)(sInOut);
```
######Class instance destruction: Delete(trt)(interfacePointer)

```C++
Delete(trt)(pTest);
```

######Callback

Callback is just a class described above:

```C++
REMOTE_INTERFACE(ITestCallback)
{
    virtual void REMOTE_METHOD_DECL(CallFromServer)(const string& s) = 0;
};

class TestCallback: public ITestCallback
{   
public:
    void REMOTE_METHOD_IMPL(CallFromServer)(const string& s) override
    {
    }
};
```

A client calls server and pass pointer to instance of a callback class, for instance:
```C++
pTest->TestCallback(transport)("Test", new TestCallback);
```

Server calls callback method like:
```C++
pTestCallback->CallFromServer(serverTransport)("Reply");
```

######Restrictions 
1. In functions and methods, pointers cannot be used in return and in parameters (except pointers to RemoterInterface)
2. If a parameter is passed as non-const reference, it is In/Out parameter

######Exceptions
All calls can throw RemoteCall::Exception

######Non built-in types
For non built-in types, for instance type T, 2 serialization functions needs to be implemented:

```C++
RemoteCall::Serializer& operator << (RemoteCall::Serializer& writer, const T& t)
{
   return writer << data of 't' parameter;
}

RemoteCall::Serializer& operator >> (RemoteCall::Serializer& reader, T& t)
{
   return reader >> data of 't' parameter;
}
```

For instance for a structure ABC:

```C++
struct ABC
{
   std::string s_;
   char c_;
};


inline RemoteCall::Serializer& operator << (RemoteCall::Serializer& writer, const ABC& abc)
{
   return writer << abc.s_ << abc.n_;
}

inline RemoteCall::Serializer& operator >> (RemoteCall::Serializer& reader, ABC& abc)
{
   return reader >> abc.s_ >> abc.n_;
}
```
######Example

Declarations in TestRemoteCall.h:

```C++
#include "RemoteCall.h"

int REMOTE_FUNCTION_DECL(Func)(std::string& s, char c);

REMOTE_INTERFACE(ITest)
{
    virtual void REMOTE_METHOD_DECL(Method)(int& count) = 0;
};
```

Implementations on server:

```C++
#include "TestRemoteCall.h"

int REMOTE_FUNCTION_IMPL(Func)(string& s, char c)
{
   s += c;

   return s.size();
}

REMOTE_CLASS(CTest): public ITest
{
    void REMOTE_METHOD_IMPL(Method)(int& count) override
    {
		count = ++count_;
    }

	int count_ = 0;
};
```

Calls:

```C++
#include "TestRemoteCall.h"

int main(int argc, char* argv[])
{
    // RemoteCallTransport described above
    RemoteCallTransport rct;
	
    try 
    {
        string sInOut("ABC");
        int ret = rct.REMOTE_CALL(Func)(sInOut, '!');
		// sInOut == "ABC!", ret == 4

		ITest* pTest = rct.REMOTE_NEW("CTest");

		int count = 0;
		rct.REMOTE_CALL(pTest->Method)(count);
		// count == 1
		
		rct.REMOTE_DELETE(pTest); 
	}
    catch (const RemoteCall::Exception& e) 
    {
        cout << e.what() << endl;
    }
}
```

TestRemoteCall.h contains test functions, interface and methods declarations.
TestServer.cpp contains test functions, class and methods implementattions.
TestClient.cpp contains test functions, constructor, destructor and methods calls.

