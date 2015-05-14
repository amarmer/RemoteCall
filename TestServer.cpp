#include "TestAPI.h"

using namespace std;


void IMPLEMENT_REMOTE(f0)()
{
}

void IMPLEMENT_REMOTE(f1)(std::string& s, char c, int& n)
{
   s += c;

   n += 10;
}


tuple<int, string> IMPLEMENT_REMOTE(f2)(const std::string& s, std::vector<ABC>& vABC)
{
   tuple<int, string> tpl;

   for (auto& el : vABC) {
      get<1>(tpl) += el.s_ + "_" + el.c_ + ".";
   }

   vABC.push_back(ABC(s, '.'));

   get<0>(tpl) = vABC.size();

   return tpl;
}
