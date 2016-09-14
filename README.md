#### RemoteCall - C++ framework for synchronous and asynchronous IPC

RemoteCall allows to declare, implement and call remote C++ functions similarly 
to how it is done locally in the same process. RemoteCall supports synchronous and asynchronous 
calls, and supports functions, interfaces, methods.

#####Framework
*RemoteCall.h* contains framework. This header should be included in declarion header file.

For declaration and implementation used macros REMOTE_FUNCTION_DECL, REMOTE_FUNCTION_IMPL, REMOTE_INTERFACE, REMOTE_METHOD_DECL, REMOTE_METHOD_IMPL.

Function call used as transport(Function(Parameters)),  method call used as transport(interfacePointer->Method(Parameters)).


####Local and Remote comparison examples:

#####1. Function 'GetLength':

#####Declaration:
```C++
//Local
int GetLength(const std::string& s);

// Remote
int REMOTE_FUNCTION_DECL(GetLength)(const std::string& s);
```

#####Implementation:
```C++

// Local
int GetLength(const std::string& s) { return s.size(); }

// Remote
int REMOTE_FUNCTION_IMPL(GetLength)(const std::string& s)  { return s.size(); }
```

#####Call:
```C++
// Local  
int size = GetLength("abc");

// Remote
int size = transport(GetLength("abc"));
```

#####2. Interface 'ITest' and and it's method 'GetLength' example:

#####Declaration:
```C++
// Local  
struct ITest
{
   virtual int GetLength(const std::string& s) = 0;
};

// Remote  
REMOTE_INTERFACE(ITest)
{
   virtual int REMOTE_METHOD_DECL(GetLength)(const std::string& s) = 0;
};

ITest* REMOTE_FUNCTION_DECL(TestClassFactory)();
```

#####Implementation:
```C++
//Local
struct Test: public ITest
{
   int GetLength(const std::string& s) override
   {
      return s.size();
   }
};

// Remote
struct Test: public ITest
{
   int REMOTE_METHOD_IMPL(GetLength)(const std::string& s) override
   {
      return s.size();
   }
};

ITest* REMOTE_FUNCTION_IMPL(TestClassFactory)()
{
   return new Test;
}
```

#####Call:
```C++
// Local
pTest = new Test;
int size = pTest->GetLength("abc");
   
// Remote
ITest* pTest = transport(TestClassFactory());
int size = transport(pTest->GetLength("abc"));
```


#####Transport

RemoteCall can use any transport for IPC, for instance socket, pipes, etc. 
Transport implementation is not part of the framework.
Client's transport class should be derived from 'RemoteCall::Transport'. It is used for synchronous and asynchronous calls.
For synchronous communication should be implemented SendReceive, for asynchronous should be implemented Send or SendReceive.
It is described and implemented in TestClient.cpp. Bellow instance of Transport class is refered as 'transport'.



#####Function declaration: REMOTE_FUNCTION_DECL(FunctionName)
```C++
tuple<std::string, int> REMOTE_FUNCTION_DECL(Test)(std::vector<std::string>& vInOut, int n, const string& s);
```
#####Function implementation: REMOTE_FUNCTION_IMPL(FunctionName)

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
#####Function call: transport(FunctionName(parameters)). 

```C++
vector<string> vInOut = {"In1", "In2"};
auto ret = transport(Test(vInOut, 12345, "Test"));
```
#####Interface declaration: REMOTE_INTERFACE(InterfaceName)

```C++
REMOTE_INTERFACE(ITest) 
{
};
```

#####Method declaration: REMOTE_METHOD_DECL(MethodName)

```C++
REMOTE_INTERFACE(ITest)
{
    virtual int REMOTE_METHOD_DECL(TestMethod)(string& s) = 0;
};
```

#####Method Implementation: REMOTE_METHOD_IMPL(MethodName)

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
#####Method call: transport(interfacePointer->MethodName(parameters)) 
        
```C++
strins sInOut = "Test";
auto ret = transport(pTest->TestMethod(sInOut));
```
#####Class instance destruction: transport(Delete(interfacePointer))

```C++
transport(Delete(pTest));
```

#####Callback

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
transport(pTest->TestCallback("Test", new TestCallback));
```

Server calls callback method like:
```C++
serverTRansport(pTestCallback->CallFromServer("Reply"));
```

#####Restrictions 
1. In functions and methods, pointers cannot be used in return and in parameters (except pointers to REMOTE_INTERFACE)
2. If a parameter is passed as non-const reference, it is In/Out parameter

#####Exceptions
All calls can throw RemoteCall::Exception

#####Non built-in types
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

TestRemoteCall.h contains test functions, interface and methods declarations.
TestServer.cpp contains test functions, class and methods implementattions.
TestClient.cpp contains test functions, constructor, destructor and methods calls.

