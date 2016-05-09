// Implementation of Serializer for basic C++ types and std::vector, std::string, const char*, std::tuple

#pragma once

#include <string>
#include <vector>
#include <map>

#include "RemoteCallUtils.h"

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
            static_assert(!is_pointer<T>::value, "Parameter cannot be pointer");
            static_assert(!is_class<T>::value, "Parameter cannot be class");

            union { T value; char data[sizeof T]; } u;

            for (size_t i = 0; i < sizeof T; i++) 
            {
                u.data[i] = v_[readPos_ + i];
            }

            t = u.value;

            readPos_ += sizeof T;
        }


        template <typename T>
        void Write(const T& t)
        {
            static_assert(!is_pointer<T>::value, "Parameter cannot be pointer");
            static_assert(!is_class<T>::value, "Parameter cannot be class");

            union { T value; char data[sizeof T]; } u;
            u.value = t;

            for (auto& el : u.data) 
            {
                v_.push_back(el);
            }
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


    // tuple << and >>
    template<int n>
    struct Tuple
    {
        template <typename ...T>
        static void Write(Serializer& writer, const std::tuple<T...>& tpl)
        {
            Tuple<n - 1>::Write(writer, tpl);

            writer << get<n - 1>(tpl);
        }

        template <typename ...T>
        static void Read(Serializer& reader, std::tuple<T...>& tpl)
        {
            Tuple<n - 1>::Read(reader, tpl);

            reader >> get<n - 1>(tpl);
        }
    };

    template <>
    struct Tuple <0>
    {
        template <typename ...T>
        static void Write(Serializer&, const std::tuple<T...>&) {}

        template <typename ...T>
        static void Read(Serializer&, std::tuple<T...>&) {}
    };


    template<typename ...T>
    Serializer& operator << (Serializer& writer, const std::tuple<T...>& tpl)
    {
        Tuple<sizeof...(T)>::Write(writer, tpl);

        return writer;
    }

    template<typename ...T>
    Serializer& operator >> (Serializer& reader, std::tuple<T...>& tpl)
    {
        Tuple<sizeof...(T)>::Read(reader, tpl);

        return reader;
    }
}
