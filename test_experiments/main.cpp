#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <delegate.hpp>
#include <processor.hpp>
#include <message.hpp>
#include <messagecodes.hpp>
#include <p2psocket.hpp>
#include <type_traits>
#include <socket.hpp>
#include <message.hpp>
#include <queue.hpp>

using sf = beltpp::socket_family_t<beltpp::message_code_join::rtt,
beltpp::message_code_drop::rtt,
&beltpp::message_code_join::creator,
&beltpp::message_code_drop::creator,
&beltpp::message_code_join::saver,
&beltpp::message_code_drop::saver,
&beltpp::message_list_load
>;

#define VERSION 2

int main(int argc, char** argv)
{
    try
    {
#if (VERSION == 1)
        beltpp::socket sk;

        sk.listen({"", 3558});
        sk.listen({"", 3559});
        sk.listen({"", 3559});
        sk.open({"", 3559}, {"", 3559});
        //sk.listen({"::1", 3557}, beltpp::socket::socketv::ipv6);
        //sk.open({"192.168.0.18", 3030}, {"", 3557}, beltpp::socket::socketv::ipv4);
        //sk.open({"", 3030}, {"", 3558}, beltpp::socket::socketv::ipv6);
        //sk.open({"", 3030}, {"", 3558}, beltpp::socket::socketv::ipv4);
        //sk.open({"", 3033}, {"", 3033}, beltpp::socket::socketv::any);

        std::cout << sk.dump() << std::endl;
#elif (VERSION == 10)
        auto sv = beltpp::socket::socketv::ipv4;

        beltpp::socket sk = beltpp::getsocket<sf>();
        if (argc == 1)
            sk.open({"127.0.0.1", 3333}, {"127.0.0.1", 4444}, sv);
        else
            sk.open({"127.0.0.1", 4444}, {"127.0.0.1", 3333}, sv);
        beltpp::message msg, msg2;
        beltpp::socket::peer_id peer;
        beltpp::socket::messages message_list;
        beltpp::message_code_hello hello;

        while (true)
        {
            std::cout << "reading...\n";
            message_list = sk.read(peer);
            for (auto const& msg : message_list)
            {
                if (msg.type() == beltpp::message_code_join::rtt)
                {
                    std::cout << "connected\n";
                    hello.m_message = sk.info(peer).local.m_address;
                    msg2.set(hello);
                    sk.write(peer, msg2);
                }
                else if (msg.type() == beltpp::message_code_drop::rtt)
                {
                    std::cout << "disconnected\n";
                }
                else if (msg.type() == beltpp::message_code_hello::rtt)
                {
                    msg.get(hello);
                    std::cout << hello.m_message << std::endl;
                }
            }
        }
#elif (VERSION == 2)
        auto sv = beltpp::socket::socketv::ipv4;

        if (1 == argc)
        {
            std::map<std::string, beltpp::address> addresses;
            beltpp::socket sk = beltpp::getsocket<sf>();
            sk.listen({"127.0.0.1", 3450}, sv);

            beltpp::message msg;
            beltpp::p2psocket::peer_id peer;
            while (true)
            {
                std::cout << "server reading...\n";
                beltpp::socket::messages msgs = sk.read(peer);
                for (beltpp::message const& msg : msgs)
                {
                    switch (msg.type())
                    {
                    case beltpp::message_code_join::rtt:
                    {
                        std::cout << "connected\n";
                        beltpp::message msgpi;
                        beltpp::message_code_peer_info mpi;
                        mpi.m_address = sk.info(peer).remote.m_address;
                        mpi.m_port = sk.info(peer).remote.m_port;
                        mpi.m_online = true;
                        msgpi.set(mpi);

                        for (auto const& item : addresses)
                            sk.write(item.first, msgpi);
                        for (auto const& item : addresses)
                        {
                            mpi.m_address = item.second.m_address;
                            mpi.m_port = item.second.m_port;
                            mpi.m_online = true;
                            msgpi.set(mpi);
                            sk.write(peer, msgpi);
                        }

                        addresses.insert(std::make_pair(peer, sk.info(peer).remote));
                    }
                        break;
                    case beltpp::message_code_drop::rtt:
                    {
                        std::cout << "disconnected\n";
                        auto it = addresses.find(peer);
                        assert(it != addresses.end());

                        beltpp::message msgpi;
                        beltpp::message_code_peer_info mpi;
                        mpi.m_address = it->second.m_address;
                        mpi.m_port = it->second.m_port;
                        mpi.m_online = false;
                        msgpi.set(mpi);

                        addresses.erase(it);

                        for (auto const& item : addresses)
                            sk.write(item.first, msgpi);
                    }
                        break;
                    case beltpp::message_code_hello::rtt:
                    {
                        auto pi = sk.info(peer);
                        std::cout << pi.local.m_address << ' '
                                  << pi.local.m_port << " <=> "
                                  << pi.remote.m_address << ' '
                                  << pi.remote.m_port << std::endl;

                        beltpp::message_code_hello msg_code;
                        msg.get(msg_code);
                        std::cout << msg_code.m_message << std::endl;
                    }
                    break;
                    case beltpp::message_code_error::rtt:
                        throw std::runtime_error("error");
                    break;
                    default:
                        break;
                    }
                }
            }
        }
        else if (2 == argc)
        {
            beltpp::socket sk = beltpp::getsocket<sf>();
            sk.open({"", 0}, {"127.0.0.1", 3450}, sv);
            beltpp::message msg;
            beltpp::socket::peer_id peer;
            beltpp::socket::messages message_list;

            while (true)
            {
                std::cout << "client reading...\n";
                message_list = sk.read(peer);
                for (auto const& msg : message_list)
                {
                    if (msg.type() == beltpp::message_code_peer_info::rtt)
                    {
                        beltpp::message_code_peer_info msg_peer_info;
                        msg.get(msg_peer_info);

                        std::cout << msg_peer_info.m_address << ' '
                                  << msg_peer_info.m_port << ' '
                                  << msg_peer_info.m_online << std::endl;

                        if (msg_peer_info.m_online)
                        {
                            std::cout << "local is "
                                      << sk.info(peer).local.m_address << ' '
                                      << sk.info(peer).local.m_port
                                      << std::endl;
                            std::cout << "remote is "
                                      << msg_peer_info.m_address << ' '
                                      << msg_peer_info.m_port
                                      << std::endl;
                            //sk.listen(sk.info(peer).local, sv);

                            sk.open(sk.info(peer).local,
                                    {msg_peer_info.m_address,
                                     msg_peer_info.m_port}, sv, 100);
                            //sk.open({"", 0}, {"google.com", 80}, sv);
                        }
                    }
                    else if (msg.type() == beltpp::message_code_hello::rtt)
                    {
                        beltpp::message_code_hello msg_hello;
                        msg.get(msg_hello);

                        std::cout << msg_hello.m_message << std::endl;

                    }
                    else if (msg.type() == beltpp::message_code_join::rtt)
                    {
                        std::cout << "connected\n";
                    }
                    else if (msg.type() == beltpp::message_code_drop::rtt)
                    {
                        std::cout << "disconnected\n";
                    }
                }
            }
        }
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
