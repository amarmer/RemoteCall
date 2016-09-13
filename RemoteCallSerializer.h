// Implementation of Serializer for basic C++ types and std::vector, std::string, const char*, std::tuple

#pragma once

#include "RemoteCallUtils.h"
#include "RemoteCallInterface.h"

#include <vector>
#include <map>
#include <memory.h>
#include <algorithm>
#include <iterator>

namespace RemoteCall
{
    // Serializer
    struct Serializer
    {
        Serializer() {}

        Serializer(const std::vector<char>& v)
            : readPos_(0), v_(v)
        {}

        void operator = (const std::vector<char>& v)
        {
            readPos_ = 0;
            v_ = v;
        }

        template <typename T>
        void Read(T& t)
        {
            static_assert(!std::is_pointer<T>::value, "Parameter cannot be pointer");
            static_assert(!std::is_class<T>::value, "Parameter cannot be class");

	    char data[sizeof(T)];
            for (size_t i = 0; i < sizeof(T); i++) 
            {
                data[i] = v_[readPos_ + i];
            }

	    memcpy((void*)&t, data, sizeof(T));

            readPos_ += sizeof(T);
        }

        template <typename T>
        void Write(const T& t)
        {
            static_assert(!std::is_pointer<T>::value, "Parameter cannot be pointer");
            static_assert(!std::is_class<T>::value, "Parameter cannot be class");

	    std::copy((char*)&t, (char*)&t + sizeof(T), back_inserter(v_));
        }

        operator std::vector<char>()
        {
            return v_;
        }

        void clear()
        {
            v_.clear();
        }

        char GetCurrent()
        {
            return v_[readPos_];
        }
    private:
        int readPos_ = 0;
        std::vector<char> v_;
    };


    // Built-in types
    template <typename T>
    Serializer& operator << (Serializer& writer, const T& t)
    {
        writer.Write(t);

        return writer;
    }

    template <typename T>
    Serializer& operator >> (Serializer& reader, T& t)
    {
        reader.Read(t);

        return reader;
    }


    // RemoteInterface*
    template <typename T>
    Serializer& operator << (Serializer& writer, T* pT)
    {
        RemoteInterface* pInterface = pT;

        if (pInterface)
        {
            writer << pInterface->instanceId_;
        }
        else
        {
            writer << "";
        }

        return writer;
    }

    template <typename T>
    Serializer& operator >> (Serializer& reader, T*& pT)
    {
	// Better compiler error than check is_base
	RemoteInterface* pInterface = pT; 
	pInterface = nullptr;

        std::string instanceId;

        reader >> instanceId;

        if (instanceId.empty())
        {
            pT = nullptr;
        }
        else
        {
            pT = (T*)new RemoteInterface;

            pT->instanceId_ = instanceId;
        }

        return reader;
    }


    // string
    inline Serializer& operator << (Serializer& writer, const std::string& str)
    {
        for (auto& c : str) 
        {
            writer.Write(c);
        }
        writer.Write('\0');

        return writer;
    }

    inline Serializer& operator >> (Serializer& reader, std::string& str)
    {
        str.clear();

        char c;
        while (true) 
        {
            reader.Read(c);
            if (!c)
                break;

            str.push_back(c);
        }

        return reader;
    }


    // vector
    template<typename T>
    Serializer& operator << (Serializer& writer, const std::vector<T>& v)
    {
        writer << v.size();

        for (auto& t : v) 
        {
            writer << t;
        }

        return writer;
    }

    template<typename T>
    Serializer& operator >> (Serializer& reader, std::vector<T>& v)
    {
        v.clear();

        size_t size;
        reader >> size;

        for (size_t i = 0; i < size; i++) 
        {
            T t;
            reader >> t;

            v.push_back(t);
        }

        return reader;
    }


    // map
    template<typename TKey, typename TValue>
    Serializer& operator << (Serializer& writer, const std::map<TKey, TValue>& m)
    {
        writer << m.size();

        for (auto& t : m) 
        {
            writer << t.first << t.second;
        }

        return writer;
    }

    template<typename TKey, typename TValue>
    Serializer& operator >> (Serializer& reader, std::map<TKey, TValue>& m)
    {
        m.clear();

        size_t size;
        reader >> size;

        for (size_t i = 0; i < size; i++) 
        {
            TKey key;
            reader >> key;

            TValue value;
            reader >> value;

            m.insert(std::make_pair(key, value));
        }

        return reader;
    }


    // const char* << 
    inline Serializer& operator << (Serializer& writer, const char* s)
    {
        if (!s)
            return writer;

        while (*s) 
        {
            writer.Write(*s);
            s++;
        }
        writer.Write('\0');

        return writer;
    }


    // tuple
    template<int n>
    struct Tuple
    {
        template <typename ...Args>
        static void Write(Serializer& writer, const std::tuple<Args...>& tpl)
        {
            Tuple<n - 1>::Write(writer, tpl);

            writer << std::get<n - 1>(tpl);
        }

        template <typename ...Args>
        static void Read(Serializer& reader, std::tuple<Args...>& tpl)
        {
            Tuple<n - 1>::Read(reader, tpl);

            reader >> std::get<n - 1>(tpl);
        }
    };

    template <>
    struct Tuple <0>
    {
        template <typename ...Args>
        static void Write(Serializer&, const std::tuple<Args...>&) {}

        template <typename ...Args>
        static void Read(Serializer&, std::tuple<Args...>&) {}
    };


    template<typename ...Args>
    Serializer& operator << (Serializer& writer, const std::tuple<Args...>& tpl)
    {
        Tuple<sizeof...(Args)>::Write(writer, tpl);

        return writer;
    }

    template<typename ...Args>
    Serializer& operator >> (Serializer& reader, std::tuple<Args...>& tpl)
    {
        Tuple<sizeof...(Args)>::Read(reader, tpl);

        return reader;
    }
}
