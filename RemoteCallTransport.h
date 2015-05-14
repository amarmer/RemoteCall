#pragma once

#include <vector>
#include "RemoteCallSerializer.h"
#include "RemoteCallClient.h"

namespace RemoteCall
{
    //  Client needs to create a class which is derived from Transport and implements SendReceive
    struct Transport
    {
        virtual bool SendReceive(std::vector<char>& str) = 0;

        virtual ~Transport() {};

        // ResolveVoidReturn
        template <typename TRet>
        struct ResolveVoidReturn
        {
            static TRet Return(SerializeReader& reader, std::vector<Client::Param>& vParam)
            {
                TRet ret;
                reader >> ret;

                ResolveVoidReturn<void>::Return(reader, vParam);

                return ret;
            }
        };

        template <>
        struct ResolveVoidReturn<void>
        {
            static void Return(SerializeReader& reader, std::vector<Client::Param>& vParam)
            {
                for (size_t i = vParam.size(); i--;) {
                    vParam[i].Read(reader);
                }
            }
        };


        // Aux
        struct Aux
        {
            Aux(Transport* pTransport)
                : pTransport_(pTransport)
            {}

            template <typename TRet>
            TRet operator = (Client::CallInfo<TRet>& t)
            {
                SerializeWriter writer;
                writer << t.fName_;

                for (auto& el : t.vPar_) {
                    el.Write(writer);
                }

                auto vChar = writer.CopyData();

                if (!pTransport_->SendReceive(vChar))
                    throw Exception(t.fName_, Exception::RPC_ERROR);

                SerializeReader reader(vChar);

                string fName;
                reader >> fName;

                // If Server exception
                if (fName.size()) {
                    Exception::ERROR err;
                    reader >> err;

                    std::string description;
                    reader >> description;

                    throw Exception(t.fName_, err, description);
                }

                return ResolveVoidReturn<TRet>::Return(reader, t.vPar_);
            }

            Transport* pTransport_;
        };

        Aux GetAux()
        {
            return Aux(this);
        };
    };
}
