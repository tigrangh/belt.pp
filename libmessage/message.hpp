#ifndef BELT_MESSAGE_H
#define BELT_MESSAGE_H

#include "global.hpp"
#include <imessage.hpp>

#include <memory>

namespace beltpp
{

namespace detail
{
    class message_internals;
}

class MESSAGESHARED_EXPORT message : public beltpp::imessage
{
public:
    using message_type = beltpp::imessage::message_type;
    message();
    virtual ~message();

    void clean() override;
    message_type get_type() const override;
    void get_buffer(char const* &p_buffer,
                    size_t& size) const override;
    void set(message::message_type type,
             char const* buffer = nullptr,
             size_t size = 0) override;
    size_t scan(std::vector<char const*, size_t> const& arr_buffer) const override;

private:
    std::unique_ptr<detail::message_internals> m_pimpl;
};

}

#endif // BELT_MESSAGE_H
