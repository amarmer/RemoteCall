#pragma once

#include "RemoteCallUtils.h"
#include "RemoteCallSerializer.h"

namespace RemoteCall 
{
    class Param
    {
    public:
        void Write(Serializer& writer)
        {
            (*write_)(writer, p_);
        }

        void Read(Serializer& reader)
        {
            if (read_) 
            {
                (*read_)(reader, p_);
            }
        }

    protected:
        template <typename T>
        void Init(T& t, decltype(read_) read)
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
            return AllArgsIn<DeclArgs...>::Check()  &&  !(is_lvalue_reference<DeclArg>::value  &&  !is_const<remove_reference<DeclArg>::type>::value);
        }
    };

    template <> struct AllArgsIn <>
    {
        constexpr static bool Check() { return true; }
    };


    // Check if return type is not 'void' or at least one parameter is non-const reference, then should be used transport 'SendReceive' (otherwise can be used 'Send')
    template <typename Ret, typename ...DeclArgs>
    constexpr bool UseSendReceive()
    {
        return !is_same<void, Ret>::value  ||  !AllArgsIn<DeclArgs...>::Check();
    }


    template <typename T>
    static void constexpr CheckRemoteInterfacePointer()
    {
        static_assert(!is_pointer<T>::value  ||  is_base_of<RemoteInterface, remove_pointer<T>::type>::value, 
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
            static_assert(!is_rvalue_reference<DeclArg>::value, "Parameter cannot be rvalue");

            using DeclArgNoReference = remove_reference<DeclArg>::type;

	    CheckRemoteInterfacePointer<DeclArgNoReference>();

            const bool isDeclOut = is_lvalue_reference<DeclArg>::value  &&  !is_const<DeclArgNoReference>::value;

            static_assert(!is_pointer<DeclArgNoReference>::value  || !isDeclOut, "Parameter cannot be reference to pointer");

            static_assert(is_convertible<CallArg, DeclArg>::value, "Caller parameter is not convertible to declared parameter");

            static_assert(!isDeclOut || is_lvalue_reference<CallArg>::value, "Caller type is const, but declaration type is reference");

            vPar.push_back(ParamType<isDeclOut>(callArg));

            ClientCallProcessor<DeclArgs...>::CollectParam<CallArgs...>(vPar, callArgs...);
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

	virtual void Serialize(Serializer& writer) = 0;

        std::string callName_;
        std::vector<Param> vPar_;
    };


	// FunctionInfo
    template <bool useSendReceive, typename Ret>
    struct FunctionInfo: public CallInfo<useSendReceive, Ret>
    {
        using CallInfo::CallInfo;

        void Serialize(Serializer& writer) override
        {
            writer << callName_;
        }
    };

    template <typename Ret, typename ...CallArgs, typename ...DeclArgs>
    static FunctionInfo<UseSendReceive<Ret, DeclArgs...>(), Ret> 
        GetFunctionInfo(const std::string& callName, Ret(*)(DeclArgs...), CallArgs&...callArgs)
    {
	CheckRemoteInterfacePointer<Ret>();

        std::vector<Param> vPar;
        ClientCallProcessor<DeclArgs...>::CollectParam<CallArgs...>(vPar, callArgs...);

        return FunctionInfo<UseSendReceive<Ret, DeclArgs...>(), Ret>(callName, vPar);
    }


	// MethodInfo
    template <bool useSendReceive, typename Ret>
    struct MethodInfo: public CallInfo<useSendReceive, Ret>
    {
        MethodInfo(const std::string& instanceId, std::string callName, const std::vector<Param>& vPar)
            : CallInfo(callName, vPar), instanceId_(instanceId)
        {}

        void Serialize(Serializer& writer) override
        {
            writer << instanceId_ << callName_;
        }

    private:
        std::string instanceId_;
    };

    template <typename Ret, typename C, typename ...CallArgs, typename ...DeclArgs>
    static MethodInfo<UseSendReceive<Ret, DeclArgs...>(), Ret> 
        GetMethodInfo(const std::string& instanceId, const std::string& callName, Ret(C::*)(DeclArgs...), CallArgs&...callArgs)
    {
	CheckRemoteInterfacePointer<Ret>();

        std::vector<Param> vPar;
        ClientCallProcessor<DeclArgs...>::CollectParam<CallArgs...>(vPar, callArgs...);

	return MethodInfo<UseSendReceive<Ret, DeclArgs...>(), Ret>(instanceId, callName, vPar);
    }


	// Transport
    template <typename T>
    struct Transport
    {
        virtual ~Transport() {};

		struct SFINAE
		{
			template <typename T, T> struct TypeValue {};

			template<typename> static void SendReceive(...);
			template<typename C> static char SendReceive(TypeValue<bool(C::*)(const std::vector<char>&, std::vector<char>&), &C::SendReceive>*);

			template<typename> static void Send(...);
			template<typename C> static char Send(TypeValue<bool(C::*)(const std::vector<char>&), &C::Send>*);
		};

		static constexpr bool HasSendReceive() 	{ return is_same<decltype(SFINAE::SendReceive<T>(0)), char>::value; }
		static constexpr bool HasSend() 		{ return is_same<decltype(SFINAE::Send<T>(0)), char>::value; }

		// If return type is not 'void' or at least one parameter is non-const reference, SendReceive should be used
        template <bool useSendReceive>
		static void SendReceiveOrSend(T* pT, Serializer& reader, const std::vector<char>& vIn)
		{
            static_assert(HasSendReceive(), "'bool SendReceive(const std::vector<char>&, std::vector<char>&)' is not implemented");

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
            reader >> std::string();
		};


		// SendReceived doesn't have to be used since return type is 'void' and there are no non-const reference parameters,
		// but if SendReceive is implemented then it is used since it provides more exception info than Send,
		// otherwise (if SendReceive is not implemented) Send should be implemented.
        template <>
		static void SendReceiveOrSend<false>(T* pT, Serializer& reader, const std::vector<char>& vIn)
		{
            static_assert(HasSendReceive() | HasSend(), "'SendReceive' and 'Send' are not implemented");

			SendReceiveAndSend<HasSendReceive()>(pT, reader, vIn);
		};


		// Has SendReceive, use it
		template <bool hasSendReceive>
		static void SendReceiveAndSend(T* pT, Serializer& reader, const std::vector<char>& vIn)
		{
			SendReceiveOrSend<true>(pT, reader, vIn);
		}

		// Doesn't have SendReceive, use Send
		template <>
		static void SendReceiveAndSend<false>(T* pT, Serializer& reader, const std::vector<char>& vIn)
		{
			static_assert(HasSend(), "'bool Send(const std::vector<char>&)' is not implemented");

			if (!pT->Send(vIn))
				throw Exception(Exception::TransportError);
		}

        template <typename Ret>
        static Ret Return(Serializer& reader, std::vector<Param>& vParam)
        {
            Ret ret;
            reader >> ret;

            Return<void>(reader, vParam);

            return ret;
        }

        template <>
        static void Return<void>(Serializer& reader, std::vector<Param>& vParam)
        {
            for (size_t i = vParam.size(); i--;) 
            {
                vParam[i].Read(reader);
            }
        }


		void Delete(RemoteInterface* pInterface) const
		{
			auto instanceId = pInterface->instanceId_;

			delete pInterface;

			SendReceiveOrSend<false>((T*)this, Serializer(), std::vector<char>(Serializer() << instanceId << "~" ));
		}

        template <bool useSendReceive, typename Ret>
		Ret Call(CallInfo<useSendReceive, Ret>& callInfo) const
		{
            Serializer writer;
            callInfo.Serialize(writer);

            for (auto& el : callInfo.vPar_) 
            {
				el.Write(writer);
            }

            std::vector<char> vChar = writer;

            Serializer reader;
            SendReceiveOrSend<useSendReceive>((T*)this, reader, vChar);

            return Return<Ret>(reader, callInfo.vPar_);
		}
    };

	template <typename Transport>
	struct Deleter
	{
		const Transport& transport_;
		
		Deleter(const Transport& transport)
			: transport_(transport)
		{}

		void operator () (RemoteInterface* pInterface)
		{
			transport_.Delete(pInterface);
		}
	};
}

template <typename T>
RemoteCall::Deleter<RemoteCall::Transport<T>> Delete(const RemoteCall::Transport<T>& t)
{
	return RemoteCall::Deleter<RemoteCall::Transport<T>>(t);
}

