#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <belt.pp/delegate.hpp>
#include <belt.pp/processor.hpp>
#include <belt.pp/packet.hpp>
#include <belt.pp/messages.hpp>
#include <belt.pp/socket.hpp>
#include <belt.pp/event.hpp>
#include <belt.pp/json.hpp>

template beltpp::void_unique_ptr beltpp::new_void_unique_ptr<beltpp::message_error>();
template beltpp::void_unique_ptr beltpp::new_void_unique_ptr<beltpp::message_join>();
template beltpp::void_unique_ptr beltpp::new_void_unique_ptr<beltpp::message_drop>();

beltpp::detail::pmsg_all message_list_load_own(
        beltpp::iterator_wrapper<char const>& iter_scan_begin,
        beltpp::iterator_wrapper<char const> const& iter_scan_end,
        beltpp::detail::session_special_data& ssd,
        void* p)

{
    return beltpp::message_list_load(iter_scan_begin, iter_scan_end, ssd, p);
}

using sf = beltpp::socket_family_t<
beltpp::message_error::rtt,
beltpp::message_join::rtt,
beltpp::message_drop::rtt,
&beltpp::new_void_unique_ptr<beltpp::message_error>,
&beltpp::new_void_unique_ptr<beltpp::message_join>,
&beltpp::new_void_unique_ptr<beltpp::message_drop>,
&beltpp::message_error::saver,
&beltpp::message_join::saver,
&beltpp::message_drop::saver,
&message_list_load_own
>;

#define VERSION 11

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
#elif (VERSION == 2)
        std::vector<std::string> strings_encode =
        {
            "ն, \n\t\x10ա",
            "",
            "\\"
        };
        for (auto const& item : strings_encode)
        {
            std::cout << "---\n";
            std::string encoded = beltpp::json::value_string::encode(item);
            std::cout << ":" << item << ":" << std::endl;
            std::cout << encoded;
            std::string decoded;
            if (beltpp::json::value_string::decode(encoded, decoded))
                std::cout << std::endl << ":" << decoded << ":" << std::endl;
            else
                std::cout << " - failed to decode" << std::endl;
        }
        std::cout << "===" << std::endl;
        std::cout << "===" << std::endl;
        std::vector<std::string> strings_decode =
        {
            "",
            "\\",
            "\"\"",
            "\"\\\"",
            "\"a\\rA\\\\\\f\"",
            "\"a\\nA\\\\\\t\x12\"",
            "\"\\\\",
            "\"\\x\"",
            "\"not closed bmp\\u1111bmp\\u0022a",
            "\"bmp\\u0576 bmp\\u0022\\u044Eb\"",
            "\"a bmp \\u0c01b\\u0055\\u007f\\u007e\"",
            "\"a low \\udc01b\"",
            "\"a low not closed \\\\\\udc01b",
            "\"a high\\ud801\\udc01low\"",
            "\"a high\\ud801\\u0576bmp\"",
            "\"a high high\\ud801\\ud802bmp\"",
            "\"a high\\ud801\"",
            "\"not closed high\\ud801",
            "\"high low high low\\ud801\\udc01\\udbff\\uDFFF\"",
        };
        for (auto const& item : strings_decode)
        {
            std::cout << "---\n";
            std::string decoded;
            std::cout << item;
            if (beltpp::json::value_string::decode(item, decoded))
            {
                std::cout << std::endl << ":" << decoded << ":" << std::endl;
                std::string encoded = beltpp::json::value_string::encode(decoded);
                std::cout << encoded << std::endl;
            }
            else
                std::cout << " - failed to decode" << std::endl;
        }

#elif (VERSION == 10)
        auto sv = beltpp::ip_address::e_type::ipv4;

        beltpp::event_handler eh;
        beltpp::socket sk = beltpp::getsocket<sf>(eh);
        if (argc == 1)
            sk.open({"127.0.0.1", 3333, "127.0.0.1", 4444, sv});
        else
            sk.open({"127.0.0.1", 4444, "127.0.0.1", 3333, sv});
        beltpp::packet msg;
        beltpp::socket::peer_id peer;
        beltpp::socket::packets message_list;
        beltpp::message_hello hello;

        while (true)
        {
            std::cout << "reading...\n";
            message_list = sk.receive(peer);
            for (auto const& msg : message_list)
            {
                if (msg.type() == beltpp::message_join::rtt)
                {
                    std::cout << "connected\n";
                    hello.m_message = sk.info(peer).local.address;
                    sk.send(peer, hello);
                }
                else if (msg.type() == beltpp::message_drop::rtt)
                {
                    std::cout << "disconnected\n";
                }
                else if (msg.type() == beltpp::message_hello::rtt)
                {
                    msg.get(hello);
                    std::cout << hello.m_message << std::endl;
                }
            }
        }
#else
        if (1 == argc) //server mode
        {
            //__debugbreak();

            beltpp::event_handler eh;
            beltpp::socket sk = beltpp::getsocket<sf>(eh);
            eh.add(sk);
            
            for (int i = 0; i < 10; ++i)
            {
                beltpp::ip_address listen_addr("127.0.0.1", short(3550 + i));
                listen_addr.ip_type = beltpp::ip_address::e_type::ipv4;
                sk.listen(listen_addr);
            }

            std::cout << sk.dump() << std::endl;

            beltpp::socket::peer_id channel_id;
            size_t index = 0;

            while (true)
            {
                std::unordered_set<beltpp::ievent_item const*> set_items;
                
                eh.wait(set_items);

                auto packets = sk.receive(channel_id);

                for (auto const& packet : packets)
                {
                    std::string str_type;
                    switch (packet.type())
                    {
                    case beltpp::message_join::rtt:
                        str_type = "joined";
                        break;
                    case beltpp::message_drop::rtt:
                        str_type = "dropped";
                        break;
                    default:
                        str_type = "unknown";
                    }

                    if (0 != ++index % 1000)
                        continue;

                    std::cout << str_type
                              << " [msg code - "
                              << static_cast<int>(packet.type())
                              << "]"
                              << " [channel_id - "
                              << channel_id
                              << "]"
                              << std::endl;

                    //std::cout << sk.dump() << std::endl;
                }
            }
        }
        else // client mode
        {
            //__debugbreak();

            beltpp::event_handler eh;
            beltpp::socket sk = beltpp::getsocket<sf>(eh);
            eh.add(sk);

            std::vector<beltpp::socket::peer_id> arr_channel_id;
            arr_channel_id.resize(10);

            for (size_t i = 0; i < 20000; ++i)
            {
                short index = short(i % arr_channel_id.size());

                beltpp::ip_address open_address("", 0, "127.0.0.1", 3550 + index, beltpp::ip_address::e_type::ipv4);
                sk.open(open_address);

                std::unordered_set<beltpp::ievent_item const*> set_items;
                while (true)
                {
                    beltpp::socket::peer_id channel_id;
                    
                    beltpp::isocket::packets pcs;
                    if (beltpp::ievent_handler::wait_result::event == eh.wait(set_items))
                        pcs = sk.receive(channel_id);

                    if (channel_id.empty())
                        continue;

                    for (auto const& pc : pcs)
                    {
                        if (pc.type() == beltpp::message_join::rtt)
                        {
                            std::cout << i << std::endl;
                            if (i >= arr_channel_id.size())
                                sk.send(arr_channel_id[index], beltpp::message_drop());

                            arr_channel_id[index] = channel_id;
                        }
                    }
                    break;
                }

                //std::cout << sk.dump() << std::endl;
            }
        }
#endif
    }
    catch(std::exception const& ex)
    {
        std::cout << std::endl << "exception: " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "well done\n";
    }

    return 0;
}
