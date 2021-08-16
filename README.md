# belt.pp
## highlights
### networking essentials
+ socket wrapper on top of BSD Sockets
+ epoll/kqueue/completion ports
### parser
+ generic flexible parser
  - JSON implementation
  - IDL implementation
### code generation to support serialization/deserialization using JSON
+ IDL is used to define model specification which is turned to a set of cpp classes by the code generator

## details on supported/unsupported features/technologies
+ *dependencies?* just the system - windows/linux/macos
+ *UDP?* not yet
+ *asynchronous interface?* yes, please!
+ *multithreading necessary?* nope
+ *SSL/TLS?* no. use a proxy server
+ ipv6 vs ipv4? code is generic, but a bug exists on windows leaving ipv4 option only
+ multiple connections served by single socket object!
+ *portable?* yes! it's a goal. clang, gcc, msvc working.
+ *build system?* cmake. refer to cmake project generator options for msvc, xcode, etc... project generation.
+ *static linking vs dynamic linking?* both supported, refer to BUILD_SHARED_LIBS cmake option
+ *32 bit build?* never tried to fix build errors/warnings
+ *header only?* no.
+ c++11
+ *unicode support?* JSON encode/decode follows the rules to handle the string cases, the rest of the code assumes that std::string is utf8 encoded.

## belt.pp is analogue to ...?
protobuf, I guess.

## why?
why not?

## how does the IDL look like?
```cpp
module TheProtocol
{
    class Sum
    {
        Float64 first
        Float64 second
    }

    class SumResult
    {
        Float64 value
    }
}
```

### how does the cpp code look like?
```cpp
#include "message.hpp" // TheProtocol is defined here by code generator

#include <belt.pp/packet.hpp>
#include <belt.pp/socket.hpp>
#include <belt.pp/event.hpp>

...
using beltpp::ip_address;
using beltpp::packet;
using packets = beltpp::socket::packets;
using peer_id = beltpp::socket::peer_id;
using peer_ids = beltpp::socket::peer_ids;

using namespace TheProtocol;
using sf = beltpp::socket_family_t<&message_list_load>;
...

...

beltpp::event_handler eh;
beltpp::socket sk = beltpp::getsocket<sf>(eh);

ip_address addr("127.0.0.1", 5555);
addr.ip_type = ip_address::e_type::ipv4;

//  intended to use, when need to wait for events from
//  more than one beltpp::socket object
unordered_set<beltpp::ievent_item const*> set;

if (server_mode)
{
    sk.listen(addr);
    while (true)
    {
        eh.wait(set);
        peer_id peer;
        packets received_packets = sk.receive(peer);

        for (auto const& received_packet : received_packets)
        {
            switch (received_packet.type())
            {
            case beltpp::isocket_join::rtt:
            {
                cout << "peer " << peer << " joined" << endl;
                break;
            }
            case beltpp::isocket_drop::rtt:
            {
                cout << "peer " << peer << " dropped" << endl;
                break;
            }
            case beltpp::isocket_protocol_error::rtt:
            {
                beltpp::isocket_protocol_error msg;
                received_packet.get(msg);
                cout << "error from peer " << peer << endl;
                cout << msg.buffer << endl;
                break;
            }
            case Sum::rtt:
            {
                Sum msg_sum;
                received_packet.get(msg_sum);
                SumResult msg_result;
                msg_result.value = msg_sum.first + msg_sum.second;

                cout << msg_sum.first << " + " << msg_sum.second << endl;
                sk.send(peer, msg_result);
                break;
            }
            default:
                break;
            }
        }
    }
}
else  //  client mode
{
    sk.open(addr);
    peer_id connected_peer;
    while (true)
    {
        eh.wait(set);
        peer_id peer;
        packets received_packets = sk.receive(peer);

        if (!peer.empty())
            connected_peer = peer;
        if (connected_peer.empty())
            return 0;

        if (received_packets.empty())
            continue;

        if (received_packets.front().type() == SumResult::rtt)
        {
            SumResult msg_sum;
            received_packets.front().get(msg_sum);

            cout << msg_sum.value << endl;
        }
        else {} // socket join, drop are handled implicitly here

        double a, b;
        cin >> a >> b;
        Sum msg_sum;
        msg_sum.first = a;
        msg_sum.second = b;
        sk.send(peer, msg_sum);
    };
}
...

```

## how to build belt.pp?
```console
user@pc:~$ mkdir projects
user@pc:~$ cd projects
user@pc:~/projects$ git clone https://github.com/tigrangh/belt.pp
user@pc:~/projects$ mkdir belt.pp.build
user@pc:~/projects$ cd belt.pp.build
user@pc:~/projects/belt.pp.build$ cmake -DCMAKE_INSTALL_PREFIX=./install ../belt.pp
user@pc:~/projects/belt.pp.build$ cmake --build . --target install
```

## how to link to belt.pp?
##### option 1
`add_subdirectory(src/3rdparty/belt.pp)`
then use targets `socket`, `packet`, `utility` as dependencies
#### option 2
`find_package(belt.pp)` then use targets `belt::socket`, `belt::packet`, `belt::utility` as dependencies

## where is the magic code generation tool?
it's the idl executable

##### thanks for reaching here. feel free to contribute if there are issues of any kind. this is a work in progress.

