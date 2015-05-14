// Here are implementations of SerializeWriter and SerializeReader for basic C++ types and std::vector, std::string, const char*, std::tuple

#pragma once

#include <vector>
#include <string>

namespace RemoteCall
{
    // SerializeWriter
    struct SerializeWriter
    {
        template <typename T>
        void Write(const T& t)
        {
            static_assert(!is_pointer<T>::value, "Parameter cannot be pointer");
            static_assert(!is_class<T>::value, "Parameter cannot be class");

            union { T value; char data[sizeof T]; } u;
            u.value = t;

            for (auto& el : u.data) {
                v_.push_back(el);
            }
        }

        std::vector<char> CopyData()
        {
            return v_;
        }

        void clear()
        {
            v_.clear();
        }

    private:
        std::vector<char> v_;
    };


    // SerializeReader
    struct SerializeReader
    {
        SerializeReader(const std::vector<char>& v)
            : readPos_(0), v_(v)
        {}

        template <typename T>
        void Read(T& t)
        {
            static_assert(!is_pointer<T>::value, "Parameter cannot be pointer");
            static_assert(!is_class<T>::value, "Parameter cannot be class");

            union { T value; char data[sizeof T]; } u;

            for (size_t i = 0; i < sizeof T; i++) {
                u.data[i] = v_[readPos_ + i];
            }

            t = u.value;

            readPos_ += sizeof T;
        }

    private:
        int readPos_;
        std::vector<char> v_;
    };


    // Primitive type << and >>
    template <typename T>
    SerializeWriter& operator << (SerializeWriter& writer, const T& t)
    {
        writer.Write(t);

        return writer;
    }

    template <typename T>
    SerializeReader& operator >> (SerializeReader& reader, T& t)
    {
        reader.Read(t);

        return reader;
    }


    // string << and >>
    inline SerializeWriter& operator << (SerializeWriter& writer, const std::string& str)
    {
        for (auto& c : str) {
            writer.Write(c);
        }
        writer.Write('\0');

        return writer;
    }

    inline SerializeReader& operator >> (SerializeReader& reader, std::string& str)
    {
        str.clear();

        char c;
        while (true) {
            reader.Read(c);
            if (!c)
                break;

            str.push_back(c);
        }

        return reader;
    }


    // vector<T> << and >>
    template<typename T>
    SerializeWriter& operator << (SerializeWriter& writer, const std::vector<T>& vT)
    {
        writer << vT.size();

        for (auto& t : vT) {
            writer << t;
        }

        return writer;
    }

    template<typename T>
    SerializeReader& operator >> (SerializeReader& reader, std::vector<T>& vT)
    {
        vT.clear();

        size_t size;
        reader >> size;

        for (size_t i = 0; i < size; i++) {
            T t;
            reader >> t;

            vT.push_back(t);
        }

        return reader;
    }


    // const char* << 
    inline SerializeWriter& operator << (SerializeWriter& writer, const char* s)
    {
        if (!s)
            return writer;

        while (*s) {
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
        static void Write(SerializeWriter& writer, const std::tuple<T...>& tpl)
        {
            Tuple<n - 1>::Write(writer, tpl);

            writer << get<n - 1>(tpl);
        }

        template <typename ...T>
        static void Read(SerializeReader& reader, std::tuple<T...>& tpl)
        {
            Tuple<n - 1>::Read(reader, tpl);

            reader >> get<n - 1>(tpl);
        }
    };

    template <>
    struct Tuple <0>
    {
        template <typename ...T>
        static void Write(SerializeWriter&, const std::tuple<T...>&) {}

        template <typename ...T>
        static void Read(SerializeReader&, std::tuple<T...>&) {}
    };


    template<typename ...T>
    SerializeWriter& operator << (SerializeWriter& writer, const std::tuple<T...>& tpl)
    {
        Tuple<sizeof...(T)>::Write(writer, tpl);

        return writer;
    }

    template<typename ...T>
    SerializeReader& operator >> (SerializeReader& reader, std::tuple<T...>& tpl)
    {
        Tuple<sizeof...(T)>::Read(reader, tpl);

        return reader;
    }
}
