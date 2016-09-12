#pragma once

#include "RemoteCallUtils.h"
#include "RemoteCallSerializer.h"
#include "RemoteCallException.h"

namespace RemoteCall 
{
    class Param
    {
    public:
        void Write(Serializer& writer) const
        {
            (*write_)(writer, p_);
        }

        void Read(Serializer& reader) const
        {
            if (read_) 
            {
                (*read_)(reader, p_);
            }
        }

    protected:
		typedef void(*TRead)(Serializer&, void*);

        template <typename T>
        void Init(T& t, TRead read)
        {
            p_ = (void*)&t;

            write_ = [](Serializer& writer, const void* p) {  writer << *(T*)p; };
            read_ = read;
        }

    private:
        void* p_;
        void(*write_)(Serializer&, const void*);
        void(*read_)(Serializer&, void*);
    };

    template <bool out>
    struct ParamType : public Param
    {
        template <typename T>
        ParamType(T& t)
        {
            Init(t, [](Serializer& reader, void* p) { reader >> *(T*)p; });
        }
    };

    template <>
    struct ParamType<false> : public Param
    {
        template <typename T>
        ParamType(T& t)
        {
            Init(t, nullptr);
        }
    };


    // AllArgsIn
    template <typename ...DeclArgs> struct AllArgsIn;

    template <typename DeclArg, typename ...DeclArgs>
    struct AllArgsIn<DeclArg, DeclArgs...>
    {
        constexpr static bool Check()
        {
            return AllArgsIn<DeclArgs...>::Check()  &&  !(std::is_lvalue_reference<DeclArg>::value  &&  !std::is_const<typename std::remove_reference<DeclArg>::type>::value);
        }
    };

    template <> struct AllArgsIn <>
    {
        constexpr static bool Check() { return true; }
    };

		
    // Check if return type is not 'void' or at least one parameter is non-const reference, then should be used transport 'SendReceive' (otherwise can be used 'Send')
    template <typename Ret, typename ...DeclArgs>
    inline constexpr bool UseSendReceive()
    {
        return !std::is_same<void, Ret>::value  ||  !AllArgsIn<DeclArgs...>::Check();
    }


    template <typename T>
    inline constexpr void CheckRemoteInterfacePointer()
    {
        static_assert(!std::is_pointer<T>::value  ||  std::is_base_of<RemoteInterface, typename std::remove_pointer<T>::type>::value, 
            "Pointer can only be used if it's type is declared via REMOTE_INTERFACE");
    }


    // ClientCallProcessor
    template <typename ...DeclArgs> struct ClientCallProcessor;

    template <typename DeclArg, typename ...DeclArgs>
    struct ClientCallProcessor<DeclArg, DeclArgs...>
    {
        template <typename CallArg, typename ...CallArgs>
        static void CollectParam(std::vector<Param>& vPar, CallArg& callArg, CallArgs&...callArgs)
        {
            static_assert(!std::is_rvalue_reference<DeclArg>::value, "Parameter cannot be rvalue");

            using DeclArgNoReference = typename std::remove_reference<DeclArg>::type;

            CheckRemoteInterfacePointer<DeclArgNoReference>();

            const bool isDeclOut = std::is_lvalue_reference<DeclArg>::value  &&  !std::is_const<DeclArgNoReference>::value;

            static_assert(!std::is_pointer<DeclArgNoReference>::value  || !isDeclOut, "Parameter cannot be reference to pointer");

			static_assert(std::is_convertible<CallArg, DeclArg>::value, "Caller parameter is not convertible to declared parameter");

            vPar.push_back(ParamType<isDeclOut>(callArg));

            ClientCallProcessor<DeclArgs...>::template CollectParam<CallArgs...>(vPar, callArgs...);
        }
    };

    template <> struct ClientCallProcessor <>
    {
        template <typename ...CallArgs>
        static void CollectParam(std::vector<Param>&) {}
    };


    // CallInfo
    template <bool useSendReceive, typename Ret>
    struct CallInfo
    {
        CallInfo(const std::string callName, const std::vector<Param>& vPar)
            : callName_(callName), vPar_(vPar)
        {}

        virtual void Serialize(Serializer& writer) const = 0;

        std::string callName_;
        std::vector<Param> vPar_;
    };


    // FunctionInfo
    template <bool useSendReceive, typename Ret>
    struct FunctionInfo: public CallInfo<useSendReceive, Ret>
    {
		FunctionInfo(const std::string callName, const std::vector<Param>& vPar)
			: CallInfo<useSendReceive, Ret>(callName, vPar)
		{}

        void Serialize(Serializer& writer) const override
        {
            writer << this->callName_;
        }
    };

    template <typename ...CallArgs, typename Ret, typename ...DeclArgs>
    inline auto GetFunctionInfo(const std::string& callName, Ret(*)(DeclArgs...), CallArgs&...callArgs)
    {
        CheckRemoteInterfacePointer<Ret>();

        std::vector<Param> vPar;
        ClientCallProcessor<DeclArgs...>::template CollectParam<CallArgs...>(vPar, callArgs...);

        return FunctionInfo<UseSendReceive<Ret, DeclArgs...>(), Ret>(callName, vPar);
    }

    // MethodInfo
    template <bool useSendReceive, typename Ret>
    struct MethodInfo: public CallInfo<useSendReceive, Ret>
    {
        MethodInfo(const std::string& instanceId, std::string callName, const std::vector<Param>& vPar = std::vector<Param>())
            : CallInfo<useSendReceive, Ret>(callName, vPar), instanceId_(instanceId)
        {}

        void Serialize(Serializer& writer) const override
        {
            writer << instanceId_ << this->callName_;
        }

    private:
        std::string instanceId_;
    };

    template <typename Ret, typename C, typename ...CallArgs, typename ...DeclArgs>
    inline MethodInfo<UseSendReceive<Ret, DeclArgs...>(), Ret> 
        GetMethodInfo(const std::string& instanceId, const std::string& callName, Ret(C::*)(DeclArgs...), CallArgs&...callArgs)
    {
        CheckRemoteInterfacePointer<Ret>();

        std::vector<Param> vPar;
		ClientCallProcessor<DeclArgs...>::template CollectParam<CallArgs...>(vPar, callArgs...);

        return MethodInfo<UseSendReceive<Ret, DeclArgs...>(), Ret>(instanceId, callName, vPar);
    }


    template <typename Ret>
    inline Ret Return(Serializer& reader, const std::vector<Param>& vParam)
    {
        Ret ret;
        reader >> ret;

        Return<void>(reader, vParam);

        return ret;
    }

    template <>
    inline void Return<void>(Serializer& reader, const std::vector<Param>& vParam)
    {
        for (size_t i = vParam.size(); i--;) 
        {
            vParam[i].Read(reader);
        }
    }

    struct SFINAE
    {
        template <typename T, T> struct TypeValue {};

        template<typename> static void SendReceive(...);
        template<typename C> static char SendReceive(TypeValue<bool(C::*)(const std::vector<char>&, std::vector<char>&), &C::SendReceive>*);

        template<typename> static void Send(...);
        template<typename C> static char Send(TypeValue<bool(C::*)(const std::vector<char>&), &C::Send>*);
    };

	template< typename T> inline constexpr bool HasSendReceive() { return std::is_same<decltype(SFINAE::template SendReceive<T>(0)), char>::value; }
	template< typename T> inline constexpr bool HasSend()        { return std::is_same<decltype(SFINAE::template Send<T>(0)), char>::value; }

	template <typename T, bool yes>
	struct ResolveSendFunctions
	{
        static void SendReceiveOrSend(T* pT, Serializer& reader, const std::vector<char>& vIn)
        {
            static_assert(HasSendReceive<T>(), "'bool SendReceive(const std::vector<char>&, std::vector<char>&)' is not implemented");

            std::vector<char> vOut;
            if (!pT->SendReceive(vIn, vOut))
                throw Exception(Exception::TransportError);

            if (vOut.empty())
                return;

            reader = vOut;

            if (reader.GetCurrent())
            { 
                Exception e;
                reader >> e;

                throw e;
            }

            // Skip "no exception" empty string
			std::string NoException;
            reader >> NoException;
        };

        static void SendReceiveAndSend(T* pT, Serializer& reader, const std::vector<char>& vIn)
        {
            ResolveSendFunctions<T, true>::SendReceiveOrSend(pT, reader, vIn);
        }
	};


	template <typename T>
	struct ResolveSendFunctions<T, false>
	{
        static void SendReceiveOrSend(T* pT, Serializer& reader, const std::vector<char>& vIn)
        {
            static_assert(HasSendReceive<T>() | HasSend<T>(), "'SendReceive' and 'Send' are not implemented");

            ResolveSendFunctions<T, HasSendReceive<T>()>::SendReceiveAndSend(pT, reader, vIn);
        };

        static void SendReceiveAndSend(T* pT, Serializer& reader, const std::vector<char>& vIn)
        {
            static_assert(HasSend<T>(), "'bool Send(const std::vector<char>&)' is not implemented");

            if (!pT->Send(vIn))
                throw Exception(Exception::TransportError);
        }
	};


    // Transport
    template <typename T>
    struct Transport
    {
        virtual ~Transport() {};

        template <bool useSendReceive, typename Ret>
        Ret operator()(const CallInfo<useSendReceive, Ret>& callInfo) const
        {
            Serializer writer;

	    writer << (std::is_base_of<RemoteInterface, typename std::remove_pointer<Ret>::type>::value? ClientId() : std::string());

            callInfo.Serialize(writer);

            for (auto& el : callInfo.vPar_) 
            {
                el.Write(writer);
            }

            std::vector<char> vChar = writer;

            Serializer reader;
            ResolveSendFunctions<T, useSendReceive>::SendReceiveOrSend((T*)this, reader, vChar);

            return Return<Ret>(reader, callInfo.vPar_);
        }

		virtual std::string ClientId() const 
		{ 
			return std::string(); 
		}
    };
}

