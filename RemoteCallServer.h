#pragma once

#include "RemoteCallUtils.h"
#include "RemoteCallSerializer.h"
#include "RemoteCallInterface.h"
#include "RemoteCallException.h"

#include <map>
#include <cctype>
#include <mutex> 

namespace RemoteCall
{
	struct Caller
	{
		virtual void Call(Serializer& writer, Serializer& reader) = 0;
	};

	struct Callers
	{
		Caller* GetCaller(const std::string& name)
		{
			auto it = mapNameCaller_.find(name);
			if (it == mapNameCaller_.end())
				return nullptr;

			return it->second;
		}

		void AddCaller(const std::string& name, Caller* pCaller)
		{
			mapNameCaller_.insert(make_pair(name, pCaller));
		}

	private:
		std::map<std::string, Caller*> mapNameCaller_;
	};


	// ClassInstances
	struct ClassInstances
	{
		void AddInterface(RemoteInterface* pRemoteInterface)
		{
			std::lock_guard<std::recursive_mutex> lock(locker_);

			mapInstancedIdInterface_.insert(make_pair(pRemoteInterface->instanceId_, pRemoteInterface));
		}

		Callers* GeMethodCallers(const std::string& instanceId)
		{
			std::lock_guard<std::recursive_mutex> lock(locker_);

			auto itInterface = mapInstancedIdInterface_.find(instanceId);
			if (itInterface == mapInstancedIdInterface_.end())
				return nullptr;
                
			auto itMethodCallers = mapInterfaceMethodCallers_.find(itInterface->second);
			if (itMethodCallers == mapInterfaceMethodCallers_.end())
				return nullptr;

			return &itMethodCallers->second;
		}

		void AddMethodCaller(RemoteInterface* pInterface, const std::string& method, Caller* pCaller)
		{
			std::lock_guard<std::recursive_mutex> lock(locker_);

			auto it = mapInterfaceMethodCallers_.find(pInterface);
			if (it == mapInterfaceMethodCallers_.end())
			{ 
				it = mapInterfaceMethodCallers_.insert(std::make_pair(pInterface, Callers())).first;
			}

			it->second.AddCaller(method, pCaller);
		}

		void RemoveInterface(RemoteInterface* pInterface)
		{
			std::lock_guard<std::recursive_mutex> lock(locker_);

			for (auto it = mapInstancedIdInterface_.begin(); it != mapInstancedIdInterface_.end(); ++it)
			{
				if (it->second == pInterface)
				{
					mapInstancedIdInterface_.erase(it);
					break;
				}
			}

			auto it = mapInterfaceMethodCallers_.find(pInterface);
			if (it != mapInterfaceMethodCallers_.end())
			{
				mapInterfaceMethodCallers_.erase(it);
			}
		}

		RemoteInterface* RemoveInterface(const std::string& instanceId)
		{
			std::lock_guard<std::recursive_mutex> lock(locker_);

			auto it = mapInstancedIdInterface_.find(instanceId);
			if (it == mapInstancedIdInterface_.end())
				return nullptr;

			auto pInterface = it->second;

			mapInterfaceMethodCallers_.erase(mapInterfaceMethodCallers_.find(pInterface));
			mapInstancedIdInterface_.erase(it);

			return pInterface;
		}

	private:
		std::map<RemoteInterface*, Callers> mapInterfaceMethodCallers_;
		std::map<std::string, RemoteInterface*> mapInstancedIdInterface_;
		std::recursive_mutex locker_;
	};

	inline ClassInstances* GetClassInstances() 
	{ 
		static ClassInstances s_classInstances; return &s_classInstances; 
	}


	// ServerCallProcessor
    template <typename Ret, typename ...DeclArgs> struct ServerCallProcessor;

    template <typename Ret, typename DeclArg, typename ...DeclArgs>
    struct ServerCallProcessor<Ret, DeclArg, DeclArgs...>
    {
        template <typename Caller, typename ...CallArgs>
        static void Call(Caller* pCaller, Serializer& writer, Serializer& reader, CallArgs&...callArgs)
        {
            auto t = remove_reference<DeclArg>::type();

            reader >> t;

            ServerCallProcessor<Ret, DeclArgs...>::Call(pCaller, writer, reader, callArgs..., t);

            // If out parameter
            if (is_lvalue_reference<DeclArg>::value  &&  !is_const<remove_reference<DeclArg>::type>::value) 
            {
                writer << t;
            }
        }
    };


    template <typename Ret> struct ServerCallProcessor<Ret>
    {
        template <typename Caller, typename ...CallArgs>
        static void Call(Caller* pCaller, Serializer& writer, Serializer& reader, CallArgs&...args)
        {
            writer << pCaller->Call<Ret>(args...);
        }
    };

    template <> struct ServerCallProcessor<void>
    {
        template <typename Caller, typename ...CallArgs>
        static void Call(Caller* pCaller, Serializer& writer, Serializer& reader, CallArgs&...args)
        {
            pCaller->Call<void>(args...);
        }
    };


	// FunctionCaller
    template <typename F>
    struct FunctionCaller: public Caller
    {
        FunctionCaller(F f)
            :f_(f)
        {}

        template <typename Ret, typename ...Args>
        Ret Call(Args&...args)
        {
            return (*f_)(args...);
        }

        void Call(Serializer& writer, Serializer& reader) override
        {
            Call(f_, writer, reader);
        }

        template <typename Ret, typename ...Args>
        void Call(Ret(*f)(Args...), Serializer& writer, Serializer& reader)
        {
            ServerCallProcessor<Ret, Args...>::Call(this, writer, reader);
        }

    private:
        F f_;
    };

    inline Callers* FunctionCallers()
    {
        static Callers s_functionCallers; return &s_functionCallers;
    }

    template <typename F>
    bool RegisterFunc(const std::string& name, F f)
    {
        FunctionCallers()->AddCaller(name, new FunctionCaller<F>(f));

        return true;
    }

    inline void ProcessFunctionCall(std::vector<char>& vChar)
    {
        Serializer reader(vChar);

        std::string func;
        reader >> func;

        Serializer writer;

        auto pFunctionCaller = FunctionCallers()->GetCaller(func);

        if (pFunctionCaller)
        {
            try 
            {
                writer << NoException();

                pFunctionCaller->Call(writer, reader);
            }
            catch (const std::exception& e) 
            {
                writer.clear();

                writer << Exception(Exception::ServerError, ToString() << "Server exception in " << func << " \"" << e.what() << "\".");
            }
        }
        else
        {
            writer << Exception(Exception::InvalidFunction, ToString() << "Function " << func << " is not implemented.");
        }

        vChar = writer;
    }


	// MethodCaller
    template <typename C, typename M>
    struct MethodCaller : public Caller
    {
        MethodCaller(C* pC, M m)
            :pC_(pC), m_(m)
        {};

        template <typename Ret, typename ...Args>
        Ret Call(Args&...args)
        {
            return ((*pC_).*(m_))(args...);
        }

        void Call(Serializer& writer, Serializer& reader) override
        {
            Call(pC_, m_, writer, reader);
        };

        template <typename C, typename Ret, typename ...Args>
        void Call(C* pC, Ret(C::*)(Args...), Serializer& writer, Serializer& reader)
        {
            ServerCallProcessor<Ret, Args...>::Call(this, writer, reader);
        }

    private:
        C* pC_;
        M m_;
    };

    template <typename C, typename M>
    bool RegisterMethod(C* pC, const std::string& method, M m)
    {
        GetClassInstances()->AddMethodCaller(pC, method, new MethodCaller<C, M>(pC, m));

        return true;
    }


    inline void ProcessClassCall(std::vector<char>& vChar)
    {
        Serializer reader(vChar);

        Serializer writer;

        std::string instanceId;
        reader >> instanceId;

        if ('~' == reader.GetCurrent())
        {
            auto pInterface = GetClassInstances()->RemoveInterface(instanceId);
            if (pInterface)
            {
				writer << NoException();

                delete pInterface;
            }
			else
			{
				writer << Exception(Exception::InvalidClassInstance, ToString() << "Invalid class instance " << instanceId << '.');
			}
        }
        else
        {
            do
            {
                auto pMethodCallers = GetClassInstances()->GeMethodCallers(instanceId);
                if (!pMethodCallers)
                {
                    writer << Exception(Exception::InvalidClassInstance, ToString() << "Invalid class instance " << instanceId << '.');
                    break;
                }

                std::string method;
                reader >> method;

                auto pMethodCaller = pMethodCallers->GetCaller(method);
                if (!pMethodCaller)
                {
                    writer << "Method " << method << " is not implemented.";
                    break;
                }

                try 
                {
                    writer << NoException();

                    pMethodCaller->Call(writer, reader);
                }
                catch (const std::exception& e) 
                {
                    writer.clear();

                    writer << Exception(Exception::ServerError, ToString() << "Exception in " << method << " \"" << e.what() << "\".");
                }
            } while (false);
        }

        vChar = writer;
    }

    inline void ProcessCall(const std::vector<char>& vIn, std::vector<char>& vOut = std::vector<char>())
    {
        vOut = vIn;

        if (std::isdigit(vOut[0]))
        {
            ProcessClassCall(vOut);
        }
        else
        {
            ProcessFunctionCall(vOut);
        }
    }


	inline void AddInterface(RemoteInterface* pInterface)
	{
		GetClassInstances()->AddInterface(pInterface);
	}

	inline void RemoveInterface(RemoteInterface* pInterface)
	{
		GetClassInstances()->RemoveInterface(pInterface);
	}
}
