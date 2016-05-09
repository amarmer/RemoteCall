#pragma once

#include <vector>

#include "RemoteCallInterface.h"
#include "RemoteCallUtils.h"
#include "RemoteCallSerializer.h"
#include "RemoteCallClient.h"
#include "RemoteCallException.h"

namespace RemoteCall
{
    //  Client needs to create a class which is derived from RemoteCall::Transport and implements SendReceive
    struct Transport
    {
        virtual bool SendReceive(std::vector<char>& str) = 0;

        virtual ~Transport() {};

        void SendReceive(Serializer& reader, std::vector<char>& vChar, const CallLocation& callLocation)
        {
            if (!SendReceive(vChar))
            {
                Exception e(Exception::TransportError);
                e.AddCallLocation(callLocation);

                throw e;
            }

            reader = vChar;

            if (reader.GetCurrent())
            { 
                Exception e;
                reader >> e;

                e.AddCallLocation(callLocation);

                throw e;
            }

            // Skip "no exception" empty string
            reader >> std::string();
        }

        void DeleteInterface(const CallLocation& callLocation, RemoteInterface* pInterface)
        {
            Serializer reader;
            SendReceive(reader, std::vector<char>(Serializer() << pInterface->instanceId_ << "~" ), callLocation);

            delete pInterface;
        }

        // ResolveVoidReturn
        template <typename Ret>
        struct ResolveVoidReturn
        {
            static Ret Return(Serializer& reader, std::vector<Client::Param>& vParam)
            {
                Ret ret;
                reader >> ret;

                ResolveVoidReturn<void>::Return(reader, vParam);

                return ret;
            }
        };

        template <>
        struct ResolveVoidReturn<void>
        {
            static void Return(Serializer& reader, std::vector<Client::Param>& vParam)
            {
                for (size_t i = vParam.size(); i--;) 
                {
                    vParam[i].Read(reader);
                }
            }
        };

        // CallAux
        struct CallAux
        {
            CallAux(const CallLocation& callLocation, Transport* pTransport)
                : callLocation_(callLocation), pTransport_(pTransport)
            {}

            template <typename Ret>
            Ret operator = (Client::CallInfo<Ret>& functionInfo)
            {
                Serializer writer;
                functionInfo.Serialize(writer);

                for (auto& el : functionInfo.vPar_) 
                {
                    el.Write(writer);
                }

                std::vector<char> vChar = writer;

                Serializer reader;
                pTransport_->SendReceive(reader, vChar, callLocation_);

                return ResolveVoidReturn<Ret>::Return(reader, functionInfo.vPar_);
            }

        private:
            CallLocation callLocation_;
            Transport* pTransport_;
        };

        CallAux GetCallAux(const CallLocation& callLocation)
        {
            return CallAux(callLocation, this);
        };

        struct ClassAux
        {
            ClassAux(const std::string& className, const std::string& instanceId, const std::string& interfaceName)
                : className_(className), instanceId_(instanceId), interfaceName_(interfaceName)
            {}

            template <typename T>
            operator T*()
            {
                // Chec if T is derived from RemoteInterface
                RemoteInterface* pCheckDerived = (T*)nullptr;

                auto interfaceName = T::InterfaceName();

                if (interfaceName != interfaceName_)
                    throw Exception(Exception::InvalidInterface, "Client interface != server class interface");

                auto pInterface = new RemoteInterface;
                pInterface->className_ = className_;
                pInterface->instanceId_ = instanceId_;

                return (T*)pInterface;
            }

        private:
            std::string className_;
            std::string instanceId_;
            std::string interfaceName_; 
        };

        ClassAux GetClassAux(const CallLocation& callLocation, const std::string& className)
        {
            std::vector<char> vChar = Serializer() << "" << className;

            Serializer reader;
            SendReceive(reader, vChar, callLocation);

            std::string instanceId;
            reader >> instanceId;

            std::string interfaceName;
            reader >> interfaceName;

            return ClassAux(className, instanceId, interfaceName);
        };
    };
}
