#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <delegate.hpp>
#include <processor.hpp>
#include <type_traits>
#include <socket.hpp>
#include <message.hpp>
#include <queue.hpp>

#define VERSION 1

int main(int argc, char** argv)
{
    try
    {
#if (VERSION == 1)
        beltpp::socket sk;

        sk.listen("localhost", 3558, beltpp::socket::socketv::any);
        //sk.open("", 45544, "", 3558, beltpp::socket::socketv::any);
        //sk.listen("::1", 3557, beltpp::socket::socketv::ipv6);
        //sk.open("192.168.0.18", 3030, "", 3557, beltpp::socket::socketv::ipv4);
        //sk.open("", 3030, "", 3558, beltpp::socket::socketv::ipv6);
        //sk.open("", 3030, "", 3558, beltpp::socket::socketv::ipv4);
        //sk.open("", 3033, "", 3033, beltpp::socket::socketv::any);

        std::cout << sk.dump() << std::endl;
#else
        else if (1 == argc)
        {
            beltpp::socket sk;
            beltpp::isocket& isk = sk;
            //
            for (int i = 0; i < 10; ++i)
                sk.listen("", 3550 + i, beltpp::socket::socketv::ipv4);

            //sk.listen("::1", 3557, beltpp::socket::socketv::ipv6);
            //sk.open("192.168.0.18", 3030, "", 3557, beltpp::socket::socketv::ipv4);
            //sk.open("", 3030, "", 3557, beltpp::socket::socketv::ipv6);
            //sk.open("", 3030, "", 3030, beltpp::socket::socketv::ipv4);
            std::cout << sk.dump() << std::endl;

            beltpp::message msg;
            beltpp::imessage& imsg = msg;
            beltpp::socket::peer_id channel_id;
            size_t index = 0;

            while (true)
            {
                ++index;
                isk.read(channel_id, imsg);

                std::string str_type;
                switch (imsg.get_type())
                {
                case beltpp::imessage::message_type::joined:
                    str_type = "joined";
                    break;
                case beltpp::imessage::message_type::dropped:
                    str_type = "dropped";
                    break;
                default:
                    str_type = "unknown";
                }

                if (0 != index % 1000)
                    continue;

                std::cout << str_type
                          << " [msg code - "
                          << static_cast<int>(imsg.get_type())
                          << "]"
                          << " [channel_id - "
                          << channel_id
                          << "]"
                          << std::endl;
                //std::cout << sk.dump() << std::endl;
            }
        }
        else
        {
            beltpp::socket sk;
            std::vector<beltpp::socket::peer_id> arr_channel_id;
            arr_channel_id.resize(1);
            for (size_t i = 0; i < arr_channel_id.size(); ++i)
            {
                arr_channel_id[i] = sk.open("", 0, "127.0.0.1", 3557, beltpp::socket::socketv::ipv4);
            }

            size_t index = 0;
            beltpp::message msg;
            msg.set(beltpp::message::message_type::drop);
            for (size_t i = 0; i < 100000; ++i)
            {
                index = index % arr_channel_id.size();

                sk.write(arr_channel_id[index], msg);

                //while (true)
                {
                    //try
                    //{
                        arr_channel_id[index] = sk.open("", 0, "127.0.0.1", 3550 + i % 10, beltpp::socket::socketv::ipv4);
                    //    break;
                    //}
                    //catch(...){}
                }

                ++index;

                //std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
#endif
    }
    catch(std::exception const& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "well done\n";
    }

    return 0;
}
