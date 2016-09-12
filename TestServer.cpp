#include "TestRemoteCall.h"

using namespace std;


// Server's transport class should be derived from 'RemoteCall::Transport'. It is used for asynchronous callbacks.
// It is described in TestClient.cpp in ClientTransport class.
struct ServerTransport: public RemoteCall::Transport<ServerTransport>
{
    // 'Send' is used for asynchronous call to send data. In server, it is used for reply to client's asynchronous call. It is invoked by REMOTE_CALL.
    bool Send(const std::vector<char>& vIn)
    {
        // For testing, emulates request to client (implemented in TestClient.cpp)
        extern void ClientRequestHandler(const std::vector<char>& vIn);
        ClientRequestHandler(vIn);

        return true;
    }
};


// This function is implemented on server, it handles Transport request from client  
void ServerRequestHandler(const std::vector<char>& vIn, std::vector<char>& vOut)
{
    // This function should be called when 'vIn' is received from client
    RemoteCall::ProcessCall(vIn, vOut);

    // If request is synchronous, server replies 'vOut' back to the client 
}


static ServerTransport s_transport;

static ITestCallback* s_pCallback;


std::tuple<int, std::string> REMOTE_FUNCTION_IMPL(TestSync)(std::vector<ABC>& vABC, const std::map<int, std::string>& m)
{
    for (auto& el: vABC)
    {
        el.n_ += 10;
        el.s_ += "!";
    }

    int sum = 0;
    string str;
    for (auto& el: m)
    {
        sum += el.first;
        str += el.second;
    }

    tuple<int, string> tpl;
    get<0>(tpl) = sum;
    get<1>(tpl) = str;

    return tpl;
}


ITest* s_pTest;

class CTest: public ITest
{
public:
    CTest(const string& s, int n)
        :s_(s), n_(n)
    {}

    ~CTest()
    {
	cout << "~CTest" << endl;
    }

    void REMOTE_METHOD_IMPL(UpdateData)(const std::string& s, int n) override
    {
        s_ += s;
        n_ += n;
    }

    bool REMOTE_METHOD_IMPL(GetData)(std::string& s, int& n) override
    {
        s = s_;
        n = n_ + 100;

        return true;
    }
private:
    string s_;
    int n_;
};


void REMOTE_FUNCTION_IMPL(SetTestCallback)(const std::string&, ITestCallback* pCallback, int n)
{
    // Save pCallback, after it is not used, it should be deleted to avoid memory leak
    s_pCallback = pCallback;
}


void REMOTE_FUNCTION_IMPL(TriggerTestCallback)()
{
    if (s_pCallback)
    {
        s_transport(s_pCallback->CallFromServer(12345));

        // delete to avoid local memory leak, it doesn't call server 
        delete s_pCallback;

        s_pCallback = 0;
    }
}


ITest* REMOTE_FUNCTION_IMPL(TestClassFactory)(const std::string& s, const std::string& c)
{
	auto pTest = new CTest(s, 10);
	pTest->DeleteWhenNoClient();

	s_pTest = pTest;

    return pTest;
}

