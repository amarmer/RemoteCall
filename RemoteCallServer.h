#pragma once

#include <map>
#include "RemoteCallSerializer.h"
#include "RemoteCallException.h"

namespace RemoteCall
{
    namespace Server
    {
        // CallProcessor
        template <typename Ret, typename ...DeclArgs> struct CallProcessor;

        template <typename Ret, typename DeclArg, typename ...DeclArgs>
        struct CallProcessor<Ret, DeclArg, DeclArgs...>
        {
            template <typename TF, typename ...CallArgs>
            static void Call(TF f, SerializeWriter& writer, SerializeReader& reader, CallArgs&...callArgs)
            {
                auto t = remove_reference<DeclArg>::type();

                reader >> t;

                CallProcessor<Ret, DeclArgs...>::Call(f, writer, reader, callArgs..., t);

                // If out parameter
                if (is_lvalue_reference<DeclArg>::value  &&  !is_const<remove_reference<DeclArg>::type>::value) {
                    writer << t;
                }
            }
        };


        template <typename Ret> struct CallProcessor<Ret>
        {
            template <typename F, typename ...CallArgs>
            static void Call(F f, SerializeWriter& writer, SerializeReader& reader, CallArgs&...args)
            {
                writer << (*f)(args...);
            }
        };

        template <> struct CallProcessor<void>
        {
            template <typename F, typename ...CallArgs>
            static void Call(F f, SerializeWriter& writer, SerializeReader& reader, CallArgs&...args)
            {
                (*f)(args...);
            }
        };


        template <typename Ret, typename ...Args>
        static void Call(Ret(*f)(Args...), SerializeWriter& writer, SerializeReader& reader)
        {
            CallProcessor<Ret, Args...>::Call(f, writer, reader);
        }


        // Func
        class Func
        {
        public:
            template <typename F>
            Func(F f)
                : f_(f)
            {
                call_ = [](void* f, SerializeWriter& writer, SerializeReader& reader) { Server::Call((F)f, writer, reader); };
            }

            void Call(SerializeWriter& writer, SerializeReader& reader)
            {
                (*call_)(f_, writer, reader);
            }

            void* f_;
            void(*call_)(void*, SerializeWriter&, SerializeReader&);
        };


        inline std::map<std::string, Func>& MapNameFunc()
        {
            static std::map<std::string, Func> s_mapNameFunc;
            return s_mapNameFunc;
        }


        // NameFunc
        struct NameFunc
        {
            NameFunc(const std::string& name, const Func& func)
            {
                MapNameFunc().insert(std::make_pair(name, func));
            }
        };

        inline void CallFromClient(std::vector<char>& vChar)
        {
            SerializeReader reader(vChar);

            std::string f;
            reader >> f;

            SerializeWriter writer;

            auto it = MapNameFunc().find(f);
            if (it == MapNameFunc().end()) {
                writer << f;
                writer << Exception::NOT_IMPLEMENTED;
                writer << "";
            }
            else {
                Func& func = it->second;

                try {
                    // Means no exception
                    writer << "";

                    func.Call(writer, reader);
                }
                catch (const std::exception& e) {
                    writer.clear();
                    writer << f;
                    writer << Exception::SERVER_ERROR;
                    writer << e.what();
                }
            }

            vChar = writer.CopyData();
        }
    }
}
