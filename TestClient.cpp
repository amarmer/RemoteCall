#include "TestRemoteCall.h"
#include <typeinfo.h>

using namespace std;

// Client's transport class should be derived from 'RemoteCall::Transport'. It is used for synchronous and asynchronous calls.
struct ClientTransport: public RemoteCall::Transport<ClientTransport>
{
    // 'SendReceive' is used for synchronous call to send and receive data. 
    // Synchronous call is implied if: 1. return type of declared function/method is not 'void' or 2. parameter declaration is non-const reference.
    //
    // Implementation of 'SendReceive' is not part of the framework, it is implemented via a particular transport, for instance: http(s), sockets, etc.
    //
    // vIn - sent to server 
    // vOut - received from server 
    // return - true if server call was sucessfull, or false otherwise
    bool SendReceive(const std::vector<char>& vIn, std::vector<char>& vOut)
    {
        // For testing, emulates request to server (implemented in TestServer.cpp)
        extern void ServerRequestHandler(const std::vector<char>& vIn, std::vector<char>& vOut);
        ServerRequestHandler(vIn, vOut);

        return true;
    }

    // 'Send' is used for asynchronous call to send data. 
    // Asynchronous call is implied if: 1. return type of function/method declaration is 'void' and 2. declarations of all parameters are not non-const reference.
    // Note: if SendReceive is implemented, it will be called instead of Send, since it provides more exception information from server, 
    // and in this case Send doesn't need to be implemented.
    //
    // Implementation of 'Send' is not part of the framework, it is implemented via a particular transport, for instance: named pipes, messages, etc.
    //
    // vIn - sent to server 
    // return - true if server call was sucessfull, or false otherwise
    bool Send(const std::vector<char>& vIn)
    {
        // For testing, emulates request to server (implemented in TestServer.cpp)
        extern void ServerRequestHandler(const std::vector<char>& vIn, std::vector<char>& vOut);
		std::vector<char> vOut;
		ServerRequestHandler(vIn, vOut);

        return true;
    }
};


// For testing, emulates request from server (used for callback replies to the client)
void ClientRequestHandler(const std::vector<char>& vIn)
{
    // 'RemoteCall::ProcessCall' function should be called when 'vIn' is received from server.
	std::vector<char> vOut;
    RemoteCall::ProcessCall(vIn, vOut);
}


class TestCallback: public ITestCallback
{
public:
    void REMOTE_METHOD_IMPL(CallFromServer)(int n) override
    {
        // n == 12345;
    }
} static s_testCallback;


int main(int argc, char* argv[])
{
    ClientTransport transport;

    try 
    {
        // TestSync is synchronous call since it has non-const reference parameter (vInOut) and it's return type is not 'void'
        vector<ABC> vInOut = { {"A", 0}, {"Z", 1}, };
        tuple<int, string> retTestSync = transport(TestSync(vInOut, map<int, string>{ { 1, "Return " }, { 9, "Test" } }));
        // vInOut == { {"A!", 10}, {"Z!", 11}};  retTestSync == (10, "Return Test");

		// SetTestCallback can be used in synchronous or asynchronous call since it doesn't have a non-const reference parameter and it's return type is 'void'
        transport(SetTestCallback("Test", &s_testCallback, 123));

        transport(TriggerTestCallback());

        ITest* pTest = transport(TestClassFactory("Test ", "Z"));

        transport(pTest->UpdateData("ABC", 5));

        string s;
        int n;
        transport(pTest->GetData(s, n));
        // s == "Test ABC"; n == 15

        transport(RemoteCall::Delete(pTest));
    }
    catch (const RemoteCall::Exception& e) 
    {
        cout << e.what() << endl;
    }

    return 0;
}
