#include "p2psocket.hpp"

//#include <scope_helper.hpp>
//#include <queue.hpp>
#include <processor.hpp>
#include <message.hpp>

//#include <exception>
//#include <unordered_map>
//#include <unordered_set>
#include <cassert>
#include <utility>
//#include <mutex>
//#include <memory>
//#include <list>
//
using string = std::string;
using peer_ids = beltpp::p2psocket::peer_ids;
using messages = beltpp::p2psocket::messages;

namespace beltpp
{

/*
 * p2psocket_internals
 */
namespace detail
{

class p2psocket_internals
{
public:
    p2psocket_internals(){}

    beltpp::socket m_introducer_sk;
};

}
/*
 * p2psocket
 */
p2psocket::p2psocket()
    : m_pimpl(new detail::p2psocket_internals())
{
}

p2psocket::~p2psocket()
{
}

peer_ids p2psocket::open(address const& introducer_address)
{
    peer_ids lst_peers = m_pimpl->m_introducer_sk.open({"", 0},
                                                       introducer_address);

    if (false == lst_peers.empty())
    {
        messages msgs = m_pimpl->m_introducer_sk.read(lst_peers.front());
        if (msgs.size())
        {

        }
    }
    return peer_ids();
}

messages p2psocket::read(peer_id& peer)
{
    return messages();
}

void p2psocket::write(peer_id const& peer,
                      message const& msg)
{
}

string p2psocket::dump() const
{
    return string();
}

namespace detail
{
}// end detail

}// end beltpp


