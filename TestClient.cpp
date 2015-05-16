#include "TestAPI.h"

using namespace std;

bool Transport(std::vector<char>& vChar)
{
    // This is should be in the server 
    RemoteCall::Server::CallFromClient(vChar);

    return true;
}

struct RemoteCallTransport: public RemoteCall::Transport
{
   bool SendReceive(std::vector<char>& vChar) override
   {
      // Transport sends and receives vChar, and can use any transport like http, tcp/ip, named pipes, etc.
      return Transport(vChar);
   }
};


int main(int argc, char* argv[])
{
   RemoteCallTransport transport;
	
   try {
      transport.CALL_REMOTE(f0)();

      string s = "ABC";
      int n = 7;
      transport.CALL_REMOTE(f1)(s, '!', n);

      vector<ABC> vABC = { ABC("X", '0'), ABC("Y", '1') };
      tuple<int, string> ret2 = transport.CALL_REMOTE(f2)("!!!", vABC);
   }
   catch (const RemoteCall::Exception& e) {
      string what = e.what();
   }

   return 0;
}


