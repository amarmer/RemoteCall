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
			:counter_(1)
        {
            AddInterface(this);
        }

        virtual ~RemoteInterface() 
        {
            RemoveInterface(this);
        }


		void DeleteWhenNoClient()
		{
			deleteWhenNoClient_ = true;
		}

        std::string instanceId_ = CreateInstanceId();

		bool deleteWhenNoClient_ = false;

		void IncCounter()
		{
			counter_++;
		}

		void DecCounter()
		{
			counter_--;
			if (!counter_)
			{
				if (deleteWhenNoClient_)
				{
					delete this;
				}
			}
		}
    private:
        // Instead of implementation bellow, GUID should be created (platform specfic)
        static std::string CreateInstanceId()
        {
			static std::atomic<unsigned int> s_counter{0};
            
            return ToString() << std::this_thread::get_id() << ':' << ++s_counter;
        }
	private:
		std::atomic<unsigned int> counter_;
    };


    template <typename I>
    struct TRemoteInterface: public RemoteInterface
    {
        typedef I Interface;

        static constexpr int CheckNoDataMembers()
        {
            static_assert(sizeof(I) == sizeof(TRemoteInterface), "REMOTE_INTERFACE cannot have data memebers");

            static_assert(std::is_abstract<I>::value, "REMOTE_INTERFACE should have pure virtual function declared via REMOTE_METHOD_DECL");

            return 0;
        }

        const int checkNoDataMembers = CheckNoDataMembers();
    };
}