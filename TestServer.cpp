#include "TestRemoteCall.h"

using namespace std;

int REMOTE_FUNCTION_IMPL(Func1)(string& s, char c)
{
   s += c;

   return s.size();
}

tuple<int, string> REMOTE_FUNCTION_IMPL(Func2)(vector<ABC>& vABC, const ABC& abc)
{
   vABC.push_back(abc);

   tuple<int, string> tpl;

   for (auto& el : vABC) 
   {
       get<0>(tpl) += el.n_;
       get<1>(tpl) += el.s_;
   }

   return tpl;
}

void REMOTE_FUNCTION_IMPL(Func3)(std::map<int, std::string>& m)
{
    m = {{1, "A"}, {2, "B"}};
}


REMOTE_CLASS(CTest): public ITest
{
    void REMOTE_METHOD_IMPL(Method1)() override
    {
        counter_++;
    }

    int REMOTE_METHOD_IMPL(Method2)(std::string& str, char c) override
    {
        str += c;

        return ++counter_;
    }

    int counter_ = 0;
};
