#pragma once

#include "message_global.hpp"
#include "parser.hpp"
#include "scope_helper.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <iterator>
#include <utility>

namespace beltpp
{
namespace http
{
using std::vector;
using std::string;
using std::unordered_map;
using std::pair;

class request
{
    static
    vector<string> split(string const& value,
                         string const& separator,
                         bool skip_empty,
                         size_t limit,
                         bool till_the_end = false)
    {
        vector<string> parts;

        size_t find_index = 0;

        while (find_index < value.size() &&
               parts.size() < limit)
        {
            size_t next_index = value.find(separator, find_index);
            if (next_index == string::npos ||
                (
                    parts.size() == limit - 1 &&
                    till_the_end
                ))
                next_index = value.size();

            string part = value.substr(find_index, next_index - find_index);

            if (false == part.empty() || false == skip_empty)
                parts.push_back(part);

            find_index = next_index + separator.size();
        }

        return parts;
    }

    static size_t from_hex(string const& value)
    {
        size_t result = 0;

        for (auto const& item : value)
        {
            if (item >= '0' && item <= '9')
            {
                result *= 16;
                result += size_t(item - '0');
            }
            else if (item >= 'a' && item <= 'f')
            {
                result *= 16;
                result += size_t(item - 'a' + 10);
            }
            else if (item >= 'A' && item <= 'F')
            {
                result *= 16;
                result += size_t(item - 'A' + 10);
            }
            else
                return size_t(-1);
        }

        return result;
    }

    static
    string percent_decode(string const& value)
    {
        string result;

        size_t find_index = 0;

        while (find_index < value.size())
        {
            size_t next_index = value.find("%", find_index);
            if (next_index == string::npos)
                next_index = value.size();

            if (next_index + 2 >= value.size() ||
                size_t(-1) == from_hex(value.substr(next_index + 1, 2)))
                result += value.substr(find_index, value.size() - find_index);
            else
            {
                result += value.substr(find_index, next_index - find_index);
                result += char(from_hex(value.substr(next_index + 1, 2)));
            }

            find_index = next_index + 3;
        }

        return result;
    }
public:
    vector<string> path;
    unordered_map<string, string> arguments;
    unordered_map<string, string> properties;

    bool set(string const& url)
    {
        bool code = true;

        vector<string> parts = split(url, "?", true, 2);
        string part_path, part_query, part_hash;

        if (parts.empty() ||
            parts.size() > 2)
            code = false;
        if (parts.size() == 1)
            part_path = parts.front();
        else
        {
            part_path = parts.front();

            string query_and_hash = parts.back();

            parts = split(query_and_hash, "#", true, 2);

            if (parts.empty() ||
                parts.size() > 2)
                code = false;
            else if (parts.size() == 1)
                part_query = parts.front();
            else
            {
                part_query = parts.front();
                part_hash = parts.back();
            }
        }

        path = split(part_path, "/", true, size_t(-1));

        for (auto& path_item : path)
            path_item = percent_decode(path_item);

        parts = split(part_query, "&", true, size_t(-1));
        for (auto const& part_item : parts)
        {
            vector<string> subparts = split(part_item, "=", true, 2);
            if (subparts.empty() ||
                subparts.size() > 2)
                code = false;
            else if (subparts.size() == 1)
                arguments[percent_decode(subparts.front())] = string();
            else
                arguments[percent_decode(subparts.front())] =
                        percent_decode(subparts.back());
        }

        return code;
    }

    bool add_property(string const& value)
    {
        vector<string> parts = split(value, ": ", false, 2, true);

        if (parts.empty() ||
            parts.size() > 2)
            return false;
        else if (parts.size() == 1)
            properties[parts.front()] = string();
        else
            properties[parts.front()] = parts.back();

        return true;
    }
};

namespace detail
{
class scan_status : public beltpp::detail::iscan_status
{
public:
    enum e_status {clean, http_request_progress, http_properties_progress, http_done};
    enum e_type {get, post, put, del, options};
    scan_status()
        : status(clean)
        , type(post)
        , http_header_scanning(0)
    {}
    ~scan_status() override
    {}
    e_status status;
    e_type type;
    size_t http_header_scanning;
    beltpp::http::request resource;
};
}// end of namespace detail

inline
string http_response(beltpp::detail::session_special_data& ssd,
                     string const& buffer)
{
    ssd.session_specal_handler = nullptr;

    string str_result;
    str_result += "HTTP/1.1 200 OK\r\n";
    str_result += "Content-Type: application/json\r\n";
    str_result += "Access-Control-Allow-Origin: *\r\n";
    str_result += "Content-Length: ";
    str_result += std::to_string(buffer.size());
    str_result += "\r\n\r\n";
    str_result += buffer;

    return str_result;
}

inline
string http_not_found(beltpp::detail::session_special_data& ssd,
                      string const& buffer)
{
    ssd.session_specal_handler = nullptr;

    string str_result;
    str_result += "HTTP/1.1 404 Not Found\r\n";
    str_result += "Content-Type: text/plain\r\n";
    str_result += "Access-Control-Allow-Origin: *\r\n";
    str_result += "Content-Length: ";
    str_result += std::to_string(buffer.size());
    str_result += "\r\n\r\n";
    str_result += buffer;

    return str_result;
}

inline
string http_internal_server_error(beltpp::detail::session_special_data& ssd,
                                  string const& buffer)
{
    ssd.session_specal_handler = nullptr;

    string str_result;
    str_result += "HTTP/1.1 500 Internal Server Error\r\n";
    str_result += "Content-Type: text/plain\r\n";
    str_result += "Access-Control-Allow-Origin: *\r\n";
    str_result += "Content-Length: ";
    str_result += std::to_string(buffer.size());
    str_result += "\r\n\r\n";
    str_result += buffer;

    return str_result;
}

inline pair<beltpp::e_three_state_result, detail::scan_status>
protocol(beltpp::detail::session_special_data& ssd,
         string::const_iterator& iter_scan_begin,
         string::const_iterator const& iter_scan_end,
         string::const_iterator& iter_fallback,
         size_t enough_length,
         size_t header_max_size,
         size_t content_max_size,
         string& body)
{
    string const value_post = "POST ";
    string const value_get = "GET ";
    string const value_put = "PUT ";
    string const value_delete = "DELETE ";
    string const value_options = "OPTIONS ";

    bool long_enough_message = (size_t(std::distance(iter_scan_begin, iter_scan_end)) >
                                enough_length);

    if (ssd.lst_ptr_data.empty() ||
        nullptr == dynamic_cast<detail::scan_status*>(ssd.lst_ptr_data.back().get()))
        ssd.lst_ptr_data.push_back(beltpp::new_dc_unique_ptr<beltpp::detail::iscan_status, detail::scan_status>());

    detail::scan_status& ss = dynamic_cast<detail::scan_status&>(*ssd.lst_ptr_data.back().get());

    beltpp::on_failure guard([&ssd]
    {
        ssd.lst_ptr_data.pop_back();
    });
    bool guard_force = false;

    if (detail::scan_status::clean == ss.status)
    {
        auto iter_scan = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_post);
        if (iter_scan_begin == iter_scan)
            iter_scan = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_get);
        if (iter_scan_begin == iter_scan)
            iter_scan = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_put);
        if (iter_scan_begin == iter_scan)
            iter_scan = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_delete);
        if (iter_scan_begin == iter_scan)
            iter_scan = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_options);

        if (iter_scan_begin != iter_scan)
        {   //  even if "P", "G", "D" or "O" occured switch to http mode
            string temp(iter_scan_begin, iter_scan);
            ss.status = detail::scan_status::http_request_progress;
        }
    }

    if (detail::scan_status::http_request_progress == ss.status)
    {
        string const value_ending = " HTTP/1.1\r\n";
        auto iter_scan_check_begin = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_post);
        if (iter_scan_begin == iter_scan_check_begin)
            iter_scan_check_begin = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_get);
        if (iter_scan_begin == iter_scan_check_begin)
            iter_scan_check_begin = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_put);
        if (iter_scan_begin == iter_scan_check_begin)
            iter_scan_check_begin = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_delete);
        if (iter_scan_begin == iter_scan_check_begin)
            iter_scan_check_begin = beltpp::check_begin(iter_scan_begin, iter_scan_end, value_options);

        auto iter_scan_check_end = beltpp::check_end(iter_scan_begin, iter_scan_end, value_ending);

        string scanned_begin(iter_scan_begin, iter_scan_check_begin);
        string scanned_ending(iter_scan_check_end.first, iter_scan_check_end.second);

        if (scanned_begin.empty() ||
            (scanned_ending.empty() && long_enough_message))
            return std::make_pair(beltpp::e_three_state_result::error, ss);
        else if ((scanned_begin != value_post &&
                  scanned_begin != value_get &&
                  scanned_begin != value_put &&
                  scanned_begin != value_delete &&
                  scanned_begin != value_options) ||
                 scanned_ending != value_ending)
        {
            //  that's ok, wait for more data
        }
        else
        {
            string scanned_line(iter_scan_begin, iter_scan_check_end.second);

            if (scanned_begin == value_get)
                ss.type = detail::scan_status::get;
            else if (scanned_begin == value_post)
                ss.type = detail::scan_status::post;
            else if (scanned_begin == value_put)
                ss.type = detail::scan_status::put;
            else if (scanned_begin == value_delete)
                ss.type = detail::scan_status::del;
            else// if (scanned_begin == value_options)
                ss.type = detail::scan_status::options;

            ss.http_header_scanning += scanned_line.length();
            iter_scan_begin = iter_scan_check_end.second;
            iter_fallback = iter_scan_begin;
            long_enough_message = false;

            ss.status = detail::scan_status::http_properties_progress;

            if (false == ss.resource.set(string(iter_scan_check_begin,
                                                iter_scan_check_end.first)))
                return std::make_pair(beltpp::e_three_state_result::error, ss);
        }
    }

    while (detail::scan_status::http_properties_progress == ss.status &&
           ss.http_header_scanning < header_max_size)
    {
        string const value_ending = "\r\n";
        auto iter_scan_check_end = beltpp::check_end(iter_scan_begin, iter_scan_end, value_ending);

        string scanned_begin(iter_scan_begin, iter_scan_check_end.first);
        string scanned_ending(iter_scan_check_end.first, iter_scan_check_end.second);

        if (scanned_ending.empty() && long_enough_message)
            return std::make_pair(beltpp::e_three_state_result::error, ss);
        else if (scanned_ending != value_ending)
        {
            //  that's ok, wait for more data
            break;
        }
        else
        {
            string scanned_line(iter_scan_begin, iter_scan_check_end.second);
            ss.http_header_scanning += scanned_line.length();

            iter_scan_begin = iter_scan_check_end.second;
            iter_fallback = iter_scan_begin;
            long_enough_message = false;

            if (scanned_begin.empty())
                ss.status = detail::scan_status::http_done;
            else if(false == ss.resource.add_property(scanned_begin))
                return std::make_pair(beltpp::e_three_state_result::error, ss);
        }
    }

    if (ss.http_header_scanning >= header_max_size)
        return std::make_pair(beltpp::e_three_state_result::error, ss);

    if (detail::scan_status::http_properties_progress == ss.status ||
        detail::scan_status::http_request_progress == ss.status)
    {
        //  that's ok, wait for more data
    }
    else if (ss.status == detail::scan_status::http_done)
    {
        iter_scan_begin = iter_fallback;
        std::string line(iter_scan_begin, iter_scan_end);
        size_t line_length = line.length();
        B_UNUSED(line_length);

        if (ss.type == detail::scan_status::options)
        {
            ssd.session_specal_handler = nullptr;

            ssd.autoreply += "HTTP/1.1 200 OK\r\n";
            ssd.autoreply += "Access-Control-Allow-Origin: *\r\n";
            ssd.autoreply += "Access-Control-Allow-Methods: POST, GET, PUT, DELETE, OPTIONS\r\n";
            ssd.autoreply += "Access-Control-Allow-Headers: X-CUSTOM-HEADER, Content-Type\r\n";
            ssd.autoreply += "Access-Control-Max-Age: 86400\r\n";
            //ssd.autoreply += "Vary: Accept-Encoding, Origin\r\n";
            ssd.autoreply += "Content-Length: 0\r\n";
            ssd.autoreply += "\r\n";

            guard_force = true;
        }
        else if (ss.type == detail::scan_status::get ||
                 ss.type == detail::scan_status::del)
        {
            return std::make_pair(beltpp::e_three_state_result::success, ss);
        }
        else if (ss.type == detail::scan_status::post ||
                 ss.type == detail::scan_status::put)
        {
            string cl = ss.resource.properties["Content-Length"];
            size_t pos;
            uint64_t content_length = beltpp::stoui64(cl, pos);

            if (cl.empty() ||
                pos != cl.length() ||
                content_length > content_max_size)
                return std::make_pair(beltpp::e_three_state_result::error, ss);
            else if (size_t(std::distance(iter_scan_begin, iter_scan_end)) < content_length)
            {
                //  that's ok, wait for more data
            }
            else
            {
                for (size_t index = 0; index < content_length; ++index)
                {
                    assert(iter_scan_begin != iter_scan_end);
                    ++iter_scan_begin;
                }

                body.assign(iter_fallback, iter_scan_begin);
                return std::make_pair(beltpp::e_three_state_result::success, ss);
            }
        }
    }

    if (ss.status == detail::scan_status::clean)
        return std::make_pair(beltpp::e_three_state_result::error, ss);

    if (false == guard_force)
        guard.dismiss();

    return std::make_pair(beltpp::e_three_state_result::attempt, ss);
}

}// end of namespace http
}// end of namespace beltpp
