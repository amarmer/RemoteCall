#pragma once

#include <map>
#include <cctype>
#include <thread>   
#include <atomic>   
#include <sstream> 
#include <mutex> 

#include "RemoteCallUtils.h"
#include "RemoteCallSerializer.h"
#include "RemoteCallInterface.h"
#include "RemoteCallException.h"

namespace RemoteCall
{
    namespace Server
    {
        template <typename Ret, typename ...DeclArgs> struct CallProcessor;

        template <typename Ret, typename DeclArg, typename ...DeclArgs>
        struct CallProcessor<Ret, DeclArg, DeclArgs...>
        {
            template <typename Caller, typename ...CallArgs>
            static void Call(Caller* pCaller, Serializer& writer, Serializer& reader, CallArgs&...callArgs)
            {
                auto t = remove_reference<DeclArg>::type();

                reader >> t;

                CallProcessor<Ret, DeclArgs...>::Call(pCaller, writer, reader, callArgs..., t);

                // If out parameter
                if (is_lvalue_reference<DeclArg>::value  &&  !is_const<remove_reference<DeclArg>::type>::value) 
                {
                    writer << t;
                }
            }
        };


        template <typename Ret> struct CallProcessor<Ret>
        {
            template <typename Caller, typename ...CallArgs>
            static void Call(Caller* pCaller, Serializer& writer, Serializer& reader, CallArgs&...args)
            {
                writer << pCaller->Call<Ret>(args...);
            }
        };

        template <> struct CallProcessor<void>
        {
            template <typename Caller, typename ...CallArgs>
            static void Call(Caller* pCaller, Serializer& writer, Serializer& reader, CallArgs&...args)
            {
                pCaller->Call<void>(args...);
            }
        };

        struct Caller
        {
            virtual void Call(Serializer& writer, Serializer& reader) = 0;
        };


        template <typename F>
        struct TFunctionCaller: public Caller
        {
            TFunctionCaller(F f)
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
                CallProcessor<Ret, Args...>::Call(this, writer, reader);
            }

        private:
            F f_;
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


        inline Callers* FunctionCallers()
        {
            static Callers s_functionCallers; return &s_functionCallers;
        }

        template <typename F>
        inline bool RegisterFunc(const std::string& name, F f)
        {
            FunctionCallers()->AddCaller(name, new TFunctionCaller<F>(f));

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

        // Instead of implementation bellow, GUID should be created (platform specfic)
        inline std::string CreateInstanceId()
        {
            static std::atomic<unsigned int> s_counter = 0;
            
            return ToString() << std::this_thread::get_id() << ':' << ++s_counter;
        }

        template <typename T>
        RemoteInterface* ClassCreator()
        {
            auto pT = new T;

            pT->instanceId_ = CreateInstanceId();

            return pT;
        }

        using TClassCreator = decltype(&ClassCreator<void>);

        struct ClassFactory
        {
            void AddClassCreator(const std::string& name, TClassCreator classCreator)
            {
                mapNameClassCreator_.insert(make_pair(name, classCreator));
            }

            TClassCreator GetClassCreator(const std::string& name)
            {
                auto it = mapNameClassCreator_.find(name);
                if (it == mapNameClassCreator_.end())
                    return nullptr;

                return it->second;
            }

        private:
            std::map<std::string, TClassCreator> mapNameClassCreator_;
        };

        struct ClassInstances
        {
            void AddClassInfo(RemoteInterface* pRemoteInterface)
            {
                std::lock_guard<std::recursive_mutex> lock(locker_);

                mapInstancedIdInterface_.insert(make_pair(pRemoteInterface->instanceId_, pRemoteInterface));
            }

            RemoteInterface* GetInterface(const std::string& instanceId)
            {
                std::lock_guard<std::recursive_mutex> lock(locker_);

                auto itInterface = mapInstancedIdInterface_.find(instanceId);
                if (itInterface == mapInstancedIdInterface_.end())
                    return nullptr;

                return itInterface->second;
            }

            std::string GetClassName(const std::string& instanceId)
            {
                auto pInterface = GetInterface(instanceId);
                if (!pInterface)
                    return std::string();

                return pInterface->className_;
            }

            Callers* GetMethodCallers(const std::string& instanceId)
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

            RemoteInterface* RemoveInterface(const std::string& instanceId)
            {
                std::lock_guard<std::recursive_mutex> lock(locker_);

                auto itInterface = mapInstancedIdInterface_.find(instanceId);
                if (itInterface == mapInstancedIdInterface_.end())
                    return nullptr;

                auto pInterface = itInterface->second;

                mapInterfaceMethodCallers_.erase(mapInterfaceMethodCallers_.find(pInterface));
                mapInstancedIdInterface_.erase(itInterface);

                return pInterface;
            }

        private:
            std::map<RemoteInterface*, Callers> mapInterfaceMethodCallers_;
            std::map<std::string, RemoteInterface*> mapInstancedIdInterface_;
            std::recursive_mutex locker_;
        };

        inline ClassFactory* GetClassFactory() 
        { 
            static ClassFactory s_classFactory; return &s_classFactory; 
        }

        inline ClassInstances* GetClassInstances() 
        { 
            static ClassInstances s_classInstances; return &s_classInstances; 
        }

        template <typename T>
        inline bool RegisterClass(const std::string& name)
        {
            GetClassFactory()->AddClassCreator(name, ClassCreator<T>);

            return true;
        }

        template <typename C, typename M>
        struct TMethodCaller : public Caller
        {
            TMethodCaller(C* pC, M m)
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
                CallProcessor<Ret, Args...>::Call(this, writer, reader);
            }

        private:
            C* pC_;
            M m_;
        };

        template <typename C, typename M>
        inline bool RegisterMethod(C* pC, const std::string& method, M m)
        {
            GetClassInstances()->AddMethodCaller(pC, method, new TMethodCaller<C, M>(pC, m));

            return true;
        }

        inline RemoteInterface* CreateClass(const std::string& className)
        {
            auto classCreator = GetClassFactory()->GetClassCreator(className);
            if (classCreator)
            {
                auto pRemoteInterface = classCreator();
        
                GetClassInstances()->AddClassInfo(pRemoteInterface);

                return pRemoteInterface;
            }

            return nullptr;
        }

        inline void ProcessClassCall(std::vector<char>& vChar)
        {
            Serializer reader(vChar);

            Serializer writer;

            if (0 == reader.GetCurrent())
            {   
                // Class name is second element
                std::string className; 
                reader >> className >> className; 

                auto pRemoteInterface = CreateClass(className);

                if (pRemoteInterface)
                { 
                    writer << NoException() << pRemoteInterface->instanceId_ << pRemoteInterface->GetInterfaceName();
                }
                else
                {
                    writer << Exception(Exception::InvalidClass, ToString() << "Class " << className << " is not found.");
                }

                vChar = writer;
            }
            else 
            {
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
                        auto pMethodCallers = GetClassInstances()->GetMethodCallers(instanceId);
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
                            writer << "Method " << GetClassInstances()->GetClassName(instanceId) << "::" << method << " is not implemented.";
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

                            writer << Exception(Exception::ServerError, 
                                ToString() << "Exception in " << GetClassInstances()->GetClassName(instanceId) << "::" << method << " \"" << e.what() << "\".");
                        }
                    } while (false);
                }

                vChar = writer;
            }
        }


        inline void CallFromClient(std::vector<char>& vChar)
        {
            // If first element is empty (create class) or start with digit (class instanceId)
            if (0 == vChar[0]  ||  std::isdigit(vChar[0]))
            {
                ProcessClassCall(vChar);
            }
            else
            {
                ProcessFunctionCall(vChar);
            }
        }
    }
}
