#include "TestRemoteCall.h"
#include <iostream>

using namespace std;

// Client needs to create a class which is derived from 'RemoteCall::Transport' and implements 'bool SendReceive(std::vector<char>& vChar)'
struct RemoteCallTransport: public RemoteCall::Transport
{
    // Sends to and receives array of bytes from server 
    bool SendReceive(std::vector<char>& vChar) override
    {
        // In this call client sends 'vChar' to server

        // Server, when it recevies 'vChar', calls 'RemoteCall::Server::CallFromClient(vChar);'
        RemoteCall::Server::CallFromClient(vChar);

        // Server replies 'vChar', modified in 'RemoteCall::Server::CallFromClient(vChar)', back to the client

        // Client returns true if 'vChar' was sucessfully received from the server, or false otherwise
        return true;
    }
};


int main(int argc, char* argv[])
{
    RemoteCallTransport rct;
	
    try 
    {
        string sInOut("ABC");
        int retF1 = rct.REMOTE_CALL(Func1)(sInOut, '!');

        vector<ABC> vInOut = { {"X", 1}, {"Y", 2} };
        tuple<int, string> retF2 = rct.REMOTE_CALL(Func2)(vInOut, ABC{"Z", 3});

        map<int, string> mInOut; 
        rct.REMOTE_CALL(Func3)(mInOut);

        for (auto& el: mInOut)
        {
            cout << el.first << " " << el.second;
        }

        ITest* pTest = rct.REMOTE_NEW("CTest");

        rct.REMOTE_CALL(pTest->Method1)();

        auto retM2 = rct.REMOTE_CALL(pTest->Method2)(sInOut, '*');

        rct.REMOTE_DELETE(pTest);
    }
    catch (const RemoteCall::Exception& e) 
    {
        cout << e.what() << endl;
    }

    return 0;
}



