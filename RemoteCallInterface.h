#pragma once

#include <string>
#include <thread>
#include <atomic>

#include <iostream>

namespace RemoteCall
{
	struct RemoteInterface;
    void AddInterface(RemoteInterface* pInterface);
    void RemoveInterface(RemoteInterface* pInterface);

    struct RemoteInterface
    {
        RemoteInterface()
        {
            AddInterface(this);
        }

        virtual ~RemoteInterface() 
        {
            RemoveInterface(this);
        }

        std::string instanceId_ = CreateInstanceId();

    private:
        // Instead of implementation bellow, GUID should be created (platform specfic)
        static std::string CreateInstanceId()
        {
            static std::atomic<unsigned int> s_counter = 0;
            
            return ToString() << std::this_thread::get_id() << ':' << ++s_counter;
        }
    };


    template <typename I>
    struct TRemoteInterface: public RemoteInterface
    {
        typedef I Interface;

		static constexpr int CheckNoDataMembers()
		{
			static_assert(sizeof(I) == sizeof(TRemoteInterface), "REMOTE_INTERFACE cannot have data memebers");

			static_assert(is_abstract<I>::value, "REMOTE_INTERFACE should have pure virtual function declared via REMOTE_METHOD_DECL");

			return 0;
		}

		const int checkNoDataMembers = CheckNoDataMembers();
    };
}