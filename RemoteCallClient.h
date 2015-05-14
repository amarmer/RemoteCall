// Here are implemlementation of RemoteCall Client

#pragma once

#include "RemoteCallSerializer.h"

namespace RemoteCall 
{
    namespace Client
    {
        class Param
        {
        public:
            void Write(SerializeWriter& writer)
            {
                (*write_)(writer, p_);
            }

            void Read(SerializeReader& reader)
            {
                if (read_) {
                    (*read_)(reader, p_);
                }
            }

        protected:
            template <typename T>
            void Init(T& t, decltype(read_) read)
            {
                p_ = (void*)&t;
                write_ = [](SerializeWriter& writer, const void* p) { writer << *(T*)p; };
                read_ = read;
            }

        private:
            void* p_;
            void(*write_)(SerializeWriter&, const void*);
            void(*read_)(SerializeReader&, void*);
        };

        template <bool out>
        struct ParamType : public Param
        {
            template <typename T>
            ParamType(T& t)
            {
                Init(t, [](SerializeReader& reader, void* p) { reader >> *(T*)p; });
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


        template <typename TRet>
        struct CallInfo
        {
            CallInfo(const std::string& fName) : fName_(fName) {}

            std::string fName_;
            std::vector<Param> vPar_;
        };


        // CallProcessor
        template <typename ...DeclArgs> struct CallProcessor;

        template <typename DeclArg, typename ...DeclArgs>
        struct CallProcessor < DeclArg, DeclArgs... >
        {
            template <typename CallArg, typename ...CallArgs>
            static void CollectParam(std::vector<Param>& vPar, CallArg& callArg, CallArgs&...callArgs)
            {
                static_assert(!is_rvalue_reference<DeclArg>::value, "Parameter cannot be rvalue");
                static_assert(!is_pointer<remove_reference<DeclArg>::type>::value, "Parameter cannot be pointer");
                static_assert(is_convertible<CallArg, DeclArg>::value, "Caller parameter is not convertible to declared parameter");

                const bool isDeclOut = is_lvalue_reference<DeclArg>::value  &&  !is_const<remove_reference<DeclArg>::type>::value;

                static_assert(!isDeclOut || is_lvalue_reference<CallArg>::value, "Caller type is const, but declaration type is reference");

                vPar.push_back(ParamType<isDeclOut>(callArg));

                CallProcessor<DeclArgs...>::CollectParam<CallArgs...>(vPar, callArgs...);
            }
        };


        template <> struct CallProcessor < >
        {
            template <typename ...CallArgs>
            static void CollectParam(std::vector<Param>&) {}
        };


        template <typename Ret, typename ...CallArgs, typename ...DeclArgs>
        static CallInfo<Ret> GetCallInfo(CallArgs&...callArgs, const std::string& fName, Ret(*)(DeclArgs...))
        {
            CallInfo<Ret> info(fName);

            CallProcessor<DeclArgs...>::CollectParam<CallArgs...>(info.vPar_, callArgs...);

            return info;
        }

        template <typename Ret>
        static CallInfo<Ret> GetCallInfo(const std::string& fName, Ret(*)())
        {
            return CallInfo<Ret>(fName);
        }
    }
}


