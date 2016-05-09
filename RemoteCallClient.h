#pragma once

#include "RemoteCallUtils.h"
#include "RemoteCallSerializer.h"

namespace RemoteCall 
{
    namespace Client
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
                write_ = [](Serializer& writer, const void* p) { writer << *(T*)p; };
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


        // CallProcessor
        template <typename ...DeclArgs> struct CallProcessor;

        template <typename DeclArg, typename ...DeclArgs>
        struct CallProcessor<DeclArg, DeclArgs...>
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

        template <> struct CallProcessor <>
        {
            template <typename ...CallArgs>
            static void CollectParam(std::vector<Param>&) {}
        };


        template <typename Ret>
        struct CallInfo
        {
            CallInfo(const std::string callName, const std::vector<Param>& vPar)
                : callName_(callName), vPar_(vPar)
            {}

            virtual void Serialize(Serializer& writer) = 0;

            std::string callName_;
            std::vector<Param> vPar_;
        };

        template <typename Ret>
        struct FunctionInfo: public CallInfo<Ret>
        {
            using CallInfo::CallInfo;

            void Serialize(Serializer& writer) override
            {
                writer << callName_;
            }
        };

        template <typename Ret, typename ...CallArgs, typename ...DeclArgs>
        static FunctionInfo<Ret> GetFunctionInfo(const std::string& fName, Ret(*)(DeclArgs...), CallArgs&...callArgs)
        {
            std::vector<Param> vPar;
            CallProcessor<DeclArgs...>::CollectParam<CallArgs...>(vPar, callArgs...);

            return FunctionInfo<Ret>(fName, vPar);
        }


        template <typename Ret>
        struct MethodInfo: public CallInfo<Ret>
        {
            MethodInfo(const std::string& className, const std::string& instanceId, std::string callName, const std::vector<Param>& vPar)
                : CallInfo(callName, vPar), className_(className), instanceId_(instanceId)
            {}

            void Serialize(Serializer& writer) override
            {
                writer << instanceId_ << callName_;
            }

        private:
            std::string className_;
            std::string instanceId_;
        };

        template <typename Ret, typename C, typename ...CallArgs, typename ...DeclArgs>
        static MethodInfo<Ret> GetMethodInfo(const std::string& className, const std::string& instanceId, const std::string& fName, Ret(C::*)(DeclArgs...), CallArgs&...callArgs)
        {
            std::vector<Param> vPar;
            CallProcessor<DeclArgs...>::CollectParam<CallArgs...>(vPar, callArgs...);

            return MethodInfo<Ret>(className, instanceId, fName, vPar);
        }
    }
}
