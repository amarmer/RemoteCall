#pragma once

#include "RemoteCall.h"

struct ABC
{
   ABC(const std::string& s = "", char c = 0): s_(s), c_(c) {}

   std::string s_;
   char c_;
};


inline RemoteCall::SerializeWriter& operator << (RemoteCall::SerializeWriter& writer, const ABC& abc)
{
   writer << abc.s_ << abc.c_;

   return writer;
}

inline RemoteCall::SerializeReader& operator >> (RemoteCall::SerializeReader& reader, ABC& abc)
{
   reader >> abc.s_ >> abc.c_;

   return reader;
}


void DECLARE_REMOTE(f0)();
void DECLARE_REMOTE(f1)(std::string& s, char c, int& n);
std::tuple<int, std::string> DECLARE_REMOTE(f2)(const std::string& s, std::vector<ABC>& vABC);
