#ifndef MESSAGE_GLOBAL_HPP
#define MESSAGE_GLOBAL_HPP

#include "global.hpp"
#include <belt.pp/iterator_wrapper.hpp>
#include <vector>

namespace beltpp
{
namespace detail
{
using fptr_deleter = void(*)(void*&);
using ptr_msg = std::unique_ptr<void, fptr_deleter>;
using fptr_creator = ptr_msg(*)();
//enum class e_scan_result {success, attempt, error};
using e_scan_result = e_three_state_result;
using scan_result = std::pair<e_scan_result, size_t>;
using fptr_scanner = scan_result(*)(void*,
                                    beltpp::iterator_wrapper<char const> const&,
                                    beltpp::iterator_wrapper<char const> const&);
using fptr_saver = std::vector<char>(*)(void*);

class pmsg_all
{
public:
    pmsg_all(size_t a_rtt,
             ptr_msg a_pmsg,
             fptr_saver a_fsaver)
        : rtt(a_rtt)
        , pmsg(std::move(a_pmsg))
        , fsaver(a_fsaver)
    {}

    size_t rtt;
    ptr_msg pmsg;
    fptr_saver fsaver;
};

using fptr_message_loader = detail::pmsg_all (*)(
        beltpp::iterator_wrapper<char const> const&,
        beltpp::iterator_wrapper<char const> const&,
        size_t&);
}
}

#endif// MESSAGE_GLOBAL_HPP
