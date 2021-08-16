#include <belt.pp/json.hpp>
#include <belt.pp/math_expression.hpp>

#ifdef B_OS_LINUX
#include <sys/resource.h>
#endif

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <iterator>
#include <fstream>
#include <chrono>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::pair;
using std::make_pair;

inline void load_file(string const& path,
                      std::ifstream& fl,
                      std::istreambuf_iterator<char>& begin,
                      std::istreambuf_iterator<char>& end)
{
    fl.open(path, std::ios_base::binary);

    if (!fl)
    {
        end = std::istreambuf_iterator<char>();
        begin = std::istreambuf_iterator<char>();
    }
    else
    {
        end = std::istreambuf_iterator<char>();
        begin = std::istreambuf_iterator<char>(fl);
    }
}

// ::beltpp::e_three_state_result json_parse(string const& buf)
// {
//     auto iter_scan_begin(buf.cbegin());
//     auto iter_scan_end(buf.cend());

//     ::beltpp::json::expression_tree expression;

//     auto code = ::beltpp::json::parse_stream(expression,
//                                              iter_scan_begin,
//                                              iter_scan_end,
//                                              1024*1024,
//                                              50);

//     if (::beltpp::e_three_state_result::success == code &&
//         expression.empty())
//     {
//         assert(false);
//         code = ::beltpp::e_three_state_result::error;
//     }

//     return code;
// }

class math_parser_types
{
public:
    using T_rtt_type = uint8_t;
    using T_priority_type = uint8_t;
    using T_size_type = uint32_t;
    using T_property_type = uint8_t;
};

// template <typename T_lexers>
// pair<bool, ::beltpp::expression_tree<T_lexers>>
// flexible_parse(string const& expression)
// {
//     ::beltpp::expression_tree<T_lexers> expression_tree;

//     auto it_begin = expression.begin();
//     auto it_end = expression.end();
//     auto it_begin_keep = it_begin;
//     while (beltpp::e_three_state_result::success ==
//            beltpp::parse(expression_tree,
//                          it_begin,
//                          it_end,
//                          100, 100, 100))
//     {
//         if (it_begin == it_begin_keep)
//             break;
//         else
//             it_begin_keep = it_begin;
//     }

//     bool is_complete = expression_tree.complete();
//     expression_tree.node_address.resize(1);

//     if (false == is_complete)
//         return std::make_pair(false, std::move(expression_tree));
//         //throw runtime_error("missing expression, apparently");

//     if (it_begin != it_end)
//         return std::make_pair(false, std::move(expression_tree));
//         //throw runtime_error("syntax error, maybe: " + string(it_begin, it_end));

//     if (expression_tree.depth() > 30)
//         return std::make_pair(false, std::move(expression_tree));
//         //throw runtime_error("expected tree max depth 30 is exceeded");

//     return std::make_pair(true, std::move(expression_tree));
// }

pair<bool, ::beltpp::expression_tree<beltpp::math_expression::lexers<math_parser_types>>>
math_parse(string const& expression)
{
    ::beltpp::expression_tree<beltpp::math_expression::lexers<math_parser_types>> expression_tree;

    return std::make_pair(beltpp::parse_full<size_t(-1),
                                             size_t(-1),
                                             size_t(-1)>
                                                (expression_tree,
                                                 expression.begin(),
                                                 expression.end())
                                             ,
                          std::move(expression_tree));
}

pair<bool, ::beltpp::expression_tree<beltpp::json::lexers<beltpp::json::json_parser_types>>>
json_parse(string const& expression)
{
    ::beltpp::expression_tree<beltpp::json::lexers<beltpp::json::json_parser_types>> expression_tree;
    return std::make_pair(beltpp::parse_full<30,            // depth
                                             size_t(-1),    // size
                                             size_t(-1)>    // buffer size
                                                (expression_tree,
                                                 expression.begin(),
                                                 expression.end())
                                             ,
                          std::move(expression_tree));
}

pair<::beltpp::e_three_state_result, ::beltpp::expression_tree<beltpp::json::lexers<beltpp::json::json_parser_types>>>
json_parse_stream(string const& expression)
{
    ::beltpp::expression_tree<beltpp::json::lexers<beltpp::json::json_parser_types>> expression_tree;
    auto it_begin = expression.begin();
    auto it_begin_copy = it_begin;
    bool code = beltpp::parse_stream<1024,          // not yet parsed
                                     30,            // depth
                                     size_t(-1),    // size
                                     size_t(-1)     // buffer size
                                     >(expression_tree,
                                       it_begin_copy,
                                       expression.end());
    
    ::beltpp::e_three_state_result result;
    if (false == code)
        result = ::beltpp::e_three_state_result::error;
    else if (it_begin == it_begin_copy || false == expression_tree.complete())
        result = ::beltpp::e_three_state_result::attempt;
    else
        result = ::beltpp::e_three_state_result::success;
    
    return std::make_pair(result, std::move(expression_tree));
}

class parse_test
{
public:
    bool pass;
    string expression;
    string dump;
};

int main()
{
#ifdef B_OS_LINUX
    struct rlimit rl{1<<15, 1l<<16}; 
    if (setrlimit(RLIMIT_STACK, &rl))
        cout << "Cannot set stack size! (errno = " << errno << ")\n";
    else
        cout << "Stack size: " << rl.rlim_cur/(1<<10) << "KiB up to " << rl.rlim_max/(1<<10) << "KiB\n";
#endif

    bool pass = true;

    vector<string> numbers
    {
        "1",
        "0.",
        ".0",
        "-1",
        "-01",
        "+0.",
        "-.0",
        "0xf",
        "0xa.",
        "0x1.e",
        "0x1.2eE1",
        "0x.c",
        "-0xf",
        "+0xA.",
        "-0x1.e",
        "+0x.c",
        "1e1",
        "1E-1",
        "0.e2",
        ".0e-01",
        "-1e-11",
        "+0.E+1",
        "-.000E0",
        "-",
        "-.E3",
        ".",
        "++1",
        "1+",
        "0..1",
        "+0A",
        "1.2.3",
        "",
        "1E+1A",
        "1e-",
        "1e-1.1",
        "0x.",
        "aa",
        ".e1",
        ".1e",
        ".1e-1-",
        "- 1",
        "0x1e-1"
    };
    auto is_number1 = [](string const& str)
    {
        char* e;
        strtod(str.c_str(), &e);
        return e == &*str.end();
    };
    auto parse_number_get_length1 = [](string const& str)
    {
        char* e;
        strtod(str.c_str(), &e);
        return e - &*str.begin();
    };
    auto is_number2 = [](string const& str)
    {
        auto it_begin = str.begin();
        auto it_end = str.end();
        bool res = beltpp::check_number<false>(it_begin, it_end);

        return it_begin == str.end() && res;
    };
    auto parse_number_get_length2 = [](string const& str)
    {
        auto it_begin = str.begin();
        auto it_end = str.end();
        beltpp::check_number<false>(it_begin, it_end);

        return it_begin - str.begin();
    };
    if (pass)
    for (string const& number : numbers)
    {
        if (parse_number_get_length1(number) != parse_number_get_length2(number) ||
            is_number1(number) != is_number2(number))
        {
            pass = false;
            break;
        }
    }

    using state = ::beltpp::e_three_state_result;
    vector< pair<state, string> > vectJson;
    vectJson.push_back(make_pair(state::error, "1 2 "));
    vectJson.push_back(make_pair(state::success, "{{\"Status\":\"The JSON string into {{}}\"}}"));
    vectJson.push_back(make_pair(state::success, "{\"Status\": \"Inside string \\\"exist double\\\" quotes\"}"));
    vectJson.push_back(make_pair(state::error, "{\"Status\": \"Inside string \"exist double\" quotes\"}"));
    vectJson.push_back(make_pair(state::error, "{\"escape_character\":\"Escape sequences 1\t2\n3\"}"));
    vectJson.push_back(make_pair(state::success, "{\"type\":\"double\", \"value\": 1e-4}"));
    vectJson.push_back(make_pair(state::success, "{\"person\": [\"ID\": \"1\",\"Name\": \"Tom\"]}"));
    vectJson.push_back(make_pair(state::success, "{\"cols\": [{\"id\":1, \"label:Artikelname\", \"type:string\"}] }"));
    vectJson.push_back(make_pair(state::success, "{\"val\":10., \"type\":\"double\"}"));
    vectJson.push_back(make_pair(state::success, "{\"type\":\"double\", \"value\":0.3, \"type\":\"integer\", \"value\":3, \"Status\":\"Duplicate key\"}"));
    vectJson.push_back(make_pair(state::success, "{\"type\":\"double\", \"value\": 5,5}"));
    vectJson.push_back(make_pair(state::success, "{\"list\":[{\"Summary\":{\"Id\":95,\"Name\":\"Martin\"},{\"Id\":96,\"Name\":\"Dennis\"}}], \"status\":\"expecting_strging\"}"));
    vectJson.push_back(make_pair(state::error, "{\"hAxis\":{\"baselineColor\":\"transparent\n\", \"gridlines\":{\"color\":\"transparent\"}},\"vAxis\":{\"viewWindow\":{\"min\":0}},\"colors\":[\"#00bbde\",\"#fe6672\",\"#eeb058\"]}"));
    vectJson.push_back(make_pair(state::success, "{\"fields\": [{\"type\": \"string\", \"name\": \"cname\"}, {\"type\": \"int\", \"name\": \"id\"}] }"));
    vectJson.push_back(make_pair(state::success, "{\"url\":\"charting/Country.php?country=2&Type=0\", \"dataType\":\"json\", \"async\":false}"));
    vectJson.push_back(make_pair(state::success, u8"{\"face\": \"😂\" }"));
    vectJson.push_back(make_pair(state::success, u8"{\"testing\":\"ab\u263Ade\"}"));
//    vectJson.push_back(make_pair(state::success, "{\"cols\":[ {\"id\":\"\",\"label\":\"Land\",\"type\":\"string\"}, {\"id\":\"\",\"label\":\"Bewertung\",\"type\":\"number\"}, {\"type\":\"string\",\"role\":\"style\"}], \"rows\": [{\"c\":[ {\"v\":\"Neuseeland\",\"f\":null}, {\"v\":0.694,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Kanada\",\"f\":null}, {\"v\":0.6779,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Norwegen\",\"f\":null}, {\"v\":0.6533,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Australien\",\"f\":null}, {\"v\":0.6483,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Belgien\",\"f\":null}, {\"v\":0.5923,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Grossbritanien\",\"f\":null}, {\"v\":0.5427,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Schweden\",\"f\":null}, {\"v\":0.4668,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Frankreich\",\"f\":null}, {\"v\":0.3631,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Brasilien\",\"f\":null}, {\"v\":0.2624,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Irland\",\"f\":null}, {\"v\":0.2479,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Finnland\",\"f\":null}, {\"v\":0.2396,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Israel\",\"f\":null}, {\"v\":0.2105,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"DÃ¤nemark\",\"f\":null}, {\"v\":0.2022,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Niederlande\",\"f\":null}, {\"v\":0.1762,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Spanien\",\"f\":null}, {\"v\":0.1728,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Ã–sterreich\",\"f\":null}, {\"v\":0.1577,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"TÃ¼rkei\",\"f\":null}, {\"v\":0.153,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"SÃ¼d Afrika\",\"f\":null}, {\"v\":0.1274,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Luxemburg\",\"f\":null}, {\"v\":0.1106,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Mexiko\",\"f\":null}, {\"v\":0.0737,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Schweiz\",\"f\":null}, {\"v\":0.0587,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"USA\",\"f\":null}, {\"v\":0.0533,\"f\":null},{\"v\":\"#0F2047\",\"f\":null}]},{\"c\":[ {\"v\":\"Russland\",\"f\":null}, {\"v\":0.0128,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"China\",\"f\":null}, {\"v\":0.0104,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Italien\",\"f\":null}, {\"v\":0.0099,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Island\",\"f\":null}, {\"v\":-0.0252,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Tschechische Republik\",\"f\":null}, {\"v\":-0.0336,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Deutschland\",\"f\":null}, {\"v\":-0.0452,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Estland\",\"f\":null}, {\"v\":-0.0735,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Slowakei\",\"f\":null}, {\"v\":-0.0907,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Litauen\",\"f\":null}, {\"v\":-0.1032,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Lettland\",\"f\":null}, {\"v\":-0.1178,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Portugal\",\"f\":null}, {\"v\":-0.1207,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Korea\",\"f\":null}, {\"v\":-0.1477,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Indonesien\",\"f\":null}, {\"v\":-0.1529,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Slowenien\",\"f\":null}, {\"v\":-0.1538,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Ungarn\",\"f\":null}, {\"v\":-0.1594,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Griechenland\",\"f\":null}, {\"v\":-0.1931,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]},{\"c\":[ {\"v\":\"Japan\",\"f\":null}, {\"v\":-0.2535,\"f\":null},{\"v\":\"#A9B6D4\",\"f\":null}]}]}"));
    vectJson.push_back(make_pair(state::success, "{\"Cartoon Foxes\": [{\"Name\": \"Fox Tall\",\"Job\": \"Bein' tall\"},{\"Name\": \"Fox Small\",\"Job\": \"Bein' small\"}]}"));
    vectJson.push_back(make_pair(state::success, u8"{\"π\": \"Math.PI\",\"ø\": [],\"Ø\": null,\"e\": 2.7182818284590452353602874713527,\"root2\": 2.7182818284590452353602874713527,\"α\": 2.5029,\"δ\": 4.6692,\"ζ\": 1.2020569,\"φ\": 1.61803398874,\"γ\": 1.30357,\"K\": 2.6854520010,\"oo\": 9999999999e9,\"A\": 1.2824271291,\"C10\": 0.12345678910111213141516171819202122232425252728293031323334353637,\"c\": 299792458}"));
    vectJson.push_back(make_pair(state::success, u8"{\"Emoji_string\":\"⛲ ⛳ ⛺ ⛽ ✅ ✉ ✌ 〽\",\"moji_array\":[\"️1️⃣\",\"️2️⃣\",\"️3️⃣\",\"️4️⃣\",\"️5️⃣\",\"️6️⃣\",\"️7️⃣\",\"️8️⃣\"],\"moji_index\":{\"⛲\":\"fountain\",\"⛳\":\"golf\",\"⛺\":\"tent\",\"⛽\":\"fuelpump\"}}"));
    vectJson.push_back(make_pair(state::success, u8"{\"age\":18,\"address\": {\"streetAddr\": \"国定东路\", \"city\": \"上海\"},\"phones\": [110,12306],\"date\":0}"));
    vectJson.push_back(make_pair(state::success, "{\"date\":\"Tue Oct 03 2017 07:20:02 GMT+0000 (UTC)\",\"process\":{\"pid\":27006,\"uid\":1000,\"gid\":1000,\"cwd\":\"/home/folder/project\",\"execPath\":\"/usr/bin/nodejs\",\"version\":\"v6.11.2\",\"argv\":[\"/usr/bin/nodejs\",\"/home/folder/project/server.js\"],\"memoryUsage\":{\"rss\":100712448,\"heapTotal\":58740736,\"heapUsed\":49848408,\"external\":878295}},\"os\":{\"loadavg\":[3.359375,3.21337890625,2.24609375],\"uptime\":3418282},\"trace\":[{\"column\":null,\"file\":null,\"function\":\"Object.parse\",\"line\":null,\"method\":\"parse\",\"native\":true},{\"column\":22,\"file\":\"/home/folder/project/node_modules/@google/maps/lib/internal/make-url-request.js\",\"function\":null,\"line\":87,\"method\":null,\"native\":false},{\"column\":20,\"file\":\"events.js\",\"function\":\"emitNone\",\"line\":91,\"method\":null,\"native\":false},{\"column\":7,\"file\":\"events.js\",\"function\":\"IncomingMessage.emit\",\"line\":185,\"method\":\"emit\",\"native\":false},{\"column\":12,\"file\":\"_stream_readable.js\",\"function\":\"endReadableNT\",\"line\":974,\"method\":null,\"native\":false},{\"column\":11,\"file\":\"internal/process/next_tick.js\",\"function\":\"_combinedTickCallback\",\"line\":80,\"method\":null,\"native\":false},{\"column\":9,\"file\":\"internal/process/next_tick.js\",\"function\":\"process._tickCallback\",\"line\":104,\"method\":\"_tickCallback\",\"native\":false}],\"stack\":[\"SyntaxError: Unexpected end of JSON input\",\"    at Object.parse (native)\",\"    at IncomingMessage.<anonymous> (/home/folder/project/node_modules/@google/maps/lib/internal/make-url-request.js:87:22)\",\"    at emitNone (events.js:91:20)\",\"    at IncomingMessage.emit (events.js:185:7)\",\"    at endReadableNT (_stream_readable.js:974:12)\",\"    at _combinedTickCallback (internal/process/next_tick.js:80:11)\",\"    at process._tickCallback (internal/process/next_tick.js:104:9)\"],\"level\":\"error\",\"message\":\"uncaughtException: Unexpected end of JSON input\",\"timestamp\":\"2017-10-03T07:20:02.331Z\"}"));
    vectJson.push_back(make_pair(state::success, "{\"timestamp\":1483674960219000,\"max_pause\":33.120,\"total_time\":146.346,\"zones_collected\":2,\"total_zones\":8,\"total_compartments\":448,\"minor_gcs\":12,\"store_buffer_overflows\":0,\"mmu_20ms\":0,\"mmu_50ms\":33,\"scc_sweep_total\":5.246,\"scc_sweep_max_pause\":5.246,\"nonincremental_reason\":\"None\",\"allocated\":36,\"added_chunks\":0,\"removed_chunks\":0,\"slices\":[{\"slice\":0,\"pause\":11.026,\"when\":0.000,\"reason\":\"CC_WAITING\",\"initial_state\":\"NotActive\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":52,\"start_timestamp\":11598.700,\"end_timestamp\":11598.711,\"times\":{\"wait_background_thread\":0.017,\"mark_discard_code\":1.037,\"purge\":0.070,\"mark\":5.610,\"mark_roots\":3.758,\"buffer_gray_roots\":2.011,\"mark_cross_compartment_wrappers\":0.461,\"mark_c_and_js_stacks\":0.001,\"mark_embedding\":0.103,\"mark_compartments\":0.954,\"unmark\":0.792,\"minor_gcs_to_evict_nursery\":2.349,\"mark_roots\":0.227,\"mark_c_and_js_stacks\":0.002}},{\"slice\":1,\"pause\":10.083,\"when\":49.295,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":0,\"start_timestamp\":11598.750,\"end_timestamp\":11598.760,\"times\":{\"wait_background_thread\":0.000,\"mark\":9.091,\"minor_gcs_to_evict_nursery\":0.971}},{\"slice\":2,\"pause\":10.038,\"when\":73.961,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":3,\"start_timestamp\":11598.774,\"end_timestamp\":11598.784,\"times\":{\"wait_background_thread\":0.000,\"mark\":9.775,\"minor_gcs_to_evict_nursery\":0.228}},{\"slice\":3,\"pause\":10.029,\"when\":98.869,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":1,\"start_timestamp\":11598.799,\"end_timestamp\":11598.809,\"times\":{\"wait_background_thread\":0.001,\"mark\":9.951,\"minor_gcs_to_evict_nursery\":0.056}},{\"slice\":4,\"pause\":10.031,\"when\":122.848,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":6,\"start_timestamp\":11598.823,\"end_timestamp\":11598.833,\"times\":{\"wait_background_thread\":0.000,\"mark\":9.975,\"minor_gcs_to_evict_nursery\":0.025}},{\"slice\":5,\"pause\":10.024,\"when\":146.961,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":0,\"start_timestamp\":11598.847,\"end_timestamp\":11598.857,\"times\":{\"wait_background_thread\":0.000,\"mark\":9.978,\"minor_gcs_to_evict_nursery\":0.027}},{\"slice\":6,\"pause\":10.117,\"when\":170.167,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":0,\"start_timestamp\":11598.871,\"end_timestamp\":11598.881,\"times\":{\"wait_background_thread\":0.001,\"mark\":10.073,\"minor_gcs_to_evict_nursery\":0.024}},{\"slice\":7,\"pause\":10.015,\"when\":192.776,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"10ms\",\"page_faults\":0,\"start_timestamp\":11598.893,\"end_timestamp\":11598.903,\"times\":{\"wait_background_thread\":0.001,\"mark\":9.968,\"minor_gcs_to_evict_nursery\":0.027}},{\"slice\":8,\"pause\":33.120,\"when\":303.537,\"reason\":\"INTER_SLICE_GC\",\"initial_state\":\"Mark\",\"final_state\":\"Mark\",\"budget\":\"40ms\",\"page_faults\":0,\"start_timestamp\":11599.004,\"end_timestamp\":11599.037,\"times\":{\"wait_background_thread\":0.000,\"mark\":33.078,\"minor_gcs_to_evict_nursery\":0.024}},{\"slice\":9,\"pause\":31.729,\"when\":436.949,\"reason\":\"INTER_SLICE_GC\",\"initial_state\":\"Mark\",\"final_state\":\"Finalize\",\"budget\":\"40ms\",\"page_faults\":696,\"start_timestamp\":11599.137,\"end_timestamp\":11599.169,\"times\":{\"wait_background_thread\":0.001,\"mark\":0.050,\"mark_delayed\":0.007,\"sweep\":30.220,\"mark_during_sweeping\":5.186,\"mark_incoming_black_pointers\":0.011,\"mark_weak\":0.539,\"mark_incoming_gray_pointers\":0.005,\"mark_gray\":4.333,\"mark_gray_and_weak\":0.293,\"finalize_start_callbacks\":0.373,\"per_slice_weak_callback\":0.079,\"per_compartment_weak_callback\":0.292,\"sweep_compartments\":18.194,\"sweep_discard_code\":0.609,\"sweep_cross_compartment_wrappers\":1.944,\"sweep_type_objects\":0.145,\"sweep_breakpoints\":1.063,\"sweep_miscellaneous\":0.719,\"sweep_type_information\":13.501,\"sweep_type_tables_and_compilations\":0.001,\"free_type_arena\":0.007,\"sweep_object\":0.545,\"sweep_string\":0.075,\"sweep_script\":0.402,\"sweep_scope\":0.000,\"sweep_shape\":0.946,\"sweep_jit_code\":0.057,\"finalize_end_callback\":0.378,\"deallocate\":1.191,\"minor_gcs_to_evict_nursery\":0.524,\"mark_roots\":0.203,\"mark_c_and_js_stacks\":0.001}},{\"slice\":10,\"pause\":0.076,\"when\":536.336,\"reason\":\"REFRESH_FRAME\",\"initial_state\":\"Finalize\",\"final_state\":\"Decommit\",\"budget\":\"10ms\",\"page_faults\":1,\"start_timestamp\":11599.237,\"end_timestamp\":11599.237,\"times\":{\"wait_background_thread\":0.001,\"sweep\":0.017,\"deallocate\":0.017,\"minor_gcs_to_evict_nursery\":0.025}},{\"slice\":11,\"pause\":0.058,\"when\":637.091,\"reason\":\"INTER_SLICE_GC\",\"initial_state\":\"Decommit\",\"final_state\":\"NotActive\",\"budget\":\"40ms\",\"page_faults\":4,\"start_timestamp\":11599.338,\"end_timestamp\":11599.338,\"times\":{\"wait_background_thread\":0.001,\"minor_gcs_to_evict_nursery\":0.022}}],\"totals\":{\"wait_background_thread\":0.025,\"mark_discard_code\":1.037,\"purge\":0.070,\"mark\":107.551,\"mark_roots\":3.758,\"buffer_gray_roots\":2.011,\"mark_cross_compartment_wrappers\":0.461,\"mark_c_and_js_stacks\":0.001,\"mark_embedding\":0.103,\"mark_compartments\":0.954,\"unmark\":0.792,\"mark_delayed\":0.007,\"sweep\":30.236,\"mark_during_sweeping\":5.186,\"mark_incoming_black_pointers\":0.011,\"mark_weak\":0.539,\"mark_incoming_gray_pointers\":0.005,\"mark_gray\":4.333,\"mark_gray_and_weak\":0.293,\"finalize_start_callbacks\":0.373,\"per_slice_weak_callback\":0.079,\"per_compartment_weak_callback\":0.292,\"sweep_compartments\":18.194,\"sweep_discard_code\":0.609,\"sweep_cross_compartment_wrappers\":1.944,\"sweep_type_objects\":0.145,\"sweep_breakpoints\":1.063,\"sweep_miscellaneous\":0.719,\"sweep_type_information\":13.501,\"sweep_type_tables_and_compilations\":0.001,\"free_type_arena\":0.007,\"sweep_object\":0.545,\"sweep_string\":0.075,\"sweep_script\":0.402,\"sweep_scope\":0.000,\"sweep_shape\":0.946,\"sweep_jit_code\":0.057,\"finalize_end_callback\":0.378,\"deallocate\":1.207,\"minor_gcs_to_evict_nursery\":4.302,\"mark_roots\":0.430,\"mark_c_and_js_stacks\":0.004,\"barriers\":4.904,\"unmark_gray\":4.584}}"));
    vectJson.push_back(make_pair(state::success, "{\"web-app\": {\"servlet\": [{\"servlet-name\": \"cofaxCDS\",\"servlet-class\": \"org.cofax.cds.CDSServlet\",\"init-param\": {\"configGlossary:installationAt\": \"Philadelphia, PA\",\"configGlossary:adminEmail\": \"ksm@pobox.com\",\"configGlossary:poweredBy\": \"Cofax\",\"configGlossary:poweredByIcon\": \"/images/cofax.gif\",\"configGlossary:staticPath\": \"/content/static\",\"templateProcessorClass\": \"org.cofax.WysiwygTemplate\",\"templateLoaderClass\": \"org.cofax.FilesTemplateLoader\",\"templatePath\": \"templates\",\"templateOverridePath\": \"\",\"defaultListTemplate\": \"listTemplate.htm\",\"defaultFileTemplate\": \"articleTemplate.htm\",\"useJSP\": false,\"jspListTemplate\": \"listTemplate.jsp\",\"jspFileTemplate\": \"articleTemplate.jsp\",\"cachePackageTagsTrack\": 200,\"cachePackageTagsStore\": 200,\"cachePackageTagsRefresh\": 60,\"cacheTemplatesTrack\": 100,\"cacheTemplatesStore\": 50,\"cacheTemplatesRefresh\": 15,\"cachePagesTrack\": 200,\"cachePagesStore\": 100,\"cachePagesRefresh\": 10,\"cachePagesDirtyRead\": 10,\"searchEngineListTemplate\": \"forSearchEnginesList.htm\",\"searchEngineFileTemplate\": \"forSearchEngines.htm\",\"searchEngineRobotsDb\": \"WEB-INF/robots.db\",\"useDataStore\": true,\"dataStoreClass\": \"org.cofax.SqlDataStore\",\"redirectionClass\": \"org.cofax.SqlRedirection\",\"dataStoreName\": \"cofax\",\"dataStoreDriver\": \"com.microsoft.jdbc.sqlserver.SQLServerDriver\",\"dataStoreUrl\": \"jdbc:microsoft:sqlserver://LOCALHOST:1433;DatabaseName=goon\",\"dataStoreUser\": \"sa\",\"dataStorePassword\": \"dataStoreTestQuery\",\"dataStoreTestQuery\": \"SET NOCOUNT ON;select test='test';\",\"dataStoreLogFile\": \"/usr/local/tomcat/logs/datastore.log\",\"dataStoreInitConns\": 10,\"dataStoreMaxConns\": 100,\"dataStoreConnUsageLimit\": 100,\"dataStoreLogLevel\": \"debug\",\"maxUrlLength\": 500}},{\"servlet-name\": \"cofaxEmail\",\"servlet-class\": \"org.cofax.cds.EmailServlet\",\"init-param\": {\"mailHost\": \"mail1\",\"mailHostOverride\": \"mail2\"}},{\"servlet-name\": \"cofaxAdmin\",\"servlet-class\": \"org.cofax.cds.AdminServlet\"},{\"servlet-name\": \"fileServlet\",\"servlet-class\": \"org.cofax.cds.FileServlet\"},{\"servlet-name\": \"cofaxTools\",\"servlet-class\": \"org.cofax.cms.CofaxToolsServlet\",\"init-param\": {\"templatePath\": \"toolstemplates/\",\"log\": 1,\"logLocation\": \"/usr/local/tomcat/logs/CofaxTools.log\",\"logMaxSize\": \"\",\"dataLog\": 1,\"dataLogLocation\": \"/usr/local/tomcat/logs/dataLog.log\",\"dataLogMaxSize\": \"\",\"removePageCache\": \"/content/admin/remove?cache=pages&id=\",\"removeTemplateCache\": \"/content/admin/remove?cache=templates&id=\",\"fileTransferFolder\": \"/usr/local/tomcat/webapps/content/fileTransferFolder\",\"lookInContext\": 1,\"adminGroupID\": 4,\"betaServer\": true}}],\"servlet-mapping\": {\"cofaxCDS\": \"/\",\"cofaxEmail\": \"/cofaxutil/aemail/*\",\"cofaxAdmin\": \"/admin/*\",\"fileServlet\": \"/static/*\",\"cofaxTools\": \"/tools/*\"},\"taglib\": {\"taglib-uri\": \"cofax.tld\",\"taglib-location\": \"/WEB-INF/tlds/cofax.tld\"}}}"));
    vectJson.push_back(make_pair(state::success, u8"{\"Emoji in JSON\": \"I \u2661 JSON\"}"));
    vectJson.push_back(make_pair(state::success, "{\"journal\":[],\"timezone\":[{\"standard\":[{\"tzoffsetfrom\":[{\"value\":\"+0200\",\"name\":\"TZOFFSETFROM\",\"params\":[]}] }] }] }"));
    vectJson.push_back(make_pair(state::error, "[\"{\"Status\":\"Json format is invalid. Please check JSON format\"}\"]"));
    vectJson.push_back(make_pair(state::error, u8"{\"error\":\"Excepci\u00f3n - String could not be parsed as XML\",\"stacktrace\":\"* line 479 of /lib/googleapi.php: Exception thrown\n* line 479 of /lib/googleapi.php: call to SimpleXMLElement->__construct()\n* line 440 of /lib/googleapi.php: call to google_picasa->get_albums()\n* line 87 of /repository/picasa/lib.php: call to google_picasa->get_file_list()\n* line 140 of /repository/repository_ajax.php: call to repository_picasa->get_listing()\n\",\"debuginfo\":null,\"reproductionlink\":\"http://www.myweb.com/\"}"));
    vectJson.push_back(make_pair(state::success, "{\"name\": \"GEOM\",\"description\": \"\",\"defaultValue\": \"<lt>?xml<space>version=<quote>1.0<quote><space>encoding=     <quote>US_ASCII<quote><space>standalone=<quote>no<quote><space>?<gt><lt>geometry<gt>  <lt>polygon<gt><lt>line<gt><lt>coord<space>x=<quote>-124<quote><space>y=<quote>48<quote><solidus><gt><lt>coord<space>x=<quote>-124<quote><space>y=<quote>49.399999999999999<quote><solidus><gt><lt><solidus>line<gt><lt><solidus>polygon<gt><lt><solidus>geometry<gt>\",\"optionsType\": \"SINGLECHOICE_CONFIG\",\"type\": \"GEOMETRY\",\"options\": {\"option\": {\"value\": \"GEOM\"}}}"));
    vectJson.push_back(make_pair(state::success, "{\"enpoint\":\"\",\"addresss\":\"bangalore\",\"id\":\"12345\"}"));
    vectJson.push_back(make_pair(state::success, "{\"api_key\":\"156462asd\",\"data\":{\"item\":[{\"value\":\"State of Sao Paulo\",\"label\":\"Region\",\"color\":\"60b8ec\"},{\"value\":\"State of Minas Gerais\",\"label\":\"Region\",\"color\":\"60b8ec\"}]}}"));
    vectJson.push_back(make_pair(state::success, u8"{\"π\": \"Math.PI\",\"var\": 4,\"foo bar\": 5,\"\": 7}"));
    vectJson.push_back(make_pair(state::success, "{\"keyword\":\"Hello guy! \xF0\x9F\x98\x84\"}"));
    vectJson.push_back(make_pair(state::success, u8"[{\"keyword\":\"Hey! \\uD83D\\uDE01\"}]"));
    vectJson.push_back(make_pair(state::error, u8"{\"message\":\"jjasdajdasjdj laslla aasdasd ssdfdsf!!! \\u{1F3FD}\", \"updated_time\":\"2015-04-14T22:37:13+0000\", \"id\":\"145193995506_148030368559\"}"));
    vectJson.push_back(make_pair(state::success, R"f({"status" : "BAD_REQUEST","message" : "Invalid data","error" : "javax.validation.ConstraintViolationException","validationErrors" : [ {"type" : "Pattern","path" : "companyBic","arguments" : {"flags" : [ ],"regexp" : "\\A[0-9]{7}[\\-]?[0-9]\\z"}}, {"type" : "NotBlank","path" : "templateTag","message" : "may not be empty","arguments" : { }} ],"timestamp" : "2018-07-06T19:18:19.231Z"})f"));
    vectJson.push_back(make_pair(state::success, R"f({"regex" : "/^([\\w\\+\\-\\.\\/]+)\\s+(\\w+\\s)*($fileext\\s)/i"})f"));
    vectJson.push_back(make_pair(state::success, "{\"id_num\":09}"));

    vectJson.push_back(make_pair(state::success, u8"{\"name\": \" Z͑ͫ̓ͪ̂ͫ̽͏̴̙̤̞͉͚̯̞̠͍A̴̵̜̰͔ͫ͗͢L̠ͨͧͩ͘G̴̻͈͍͔̹̑͗̎̅͛́Ǫ̵̹̻̝̳͂̌̌͘!͖̬̰̙̗̿̋ͥͥ̂ͣ̐́́͜͞ \"}"));
    vectJson.push_back(make_pair(state::error, "{\"id_nu\\:09}"));
    vectJson.push_back(make_pair(state::attempt, "{\"id_nu"));
    vectJson.push_back(make_pair(state::attempt, "{-1:-"));

    if (pass && false)
    for (auto const& item : vectJson)
    {
        if(item.first != json_parse_stream(item.second).first)
        {
            pass = false;
            break;
        }
    }

    vector<parse_test> json_test_suite = {

            // parse_test{
            //     false,
            //     "{-1:-",
            //     "1|8 !!!! \n.3|2 !!!! \n..-1\n"
            // },
            // parse_test{
            //     false,
            //     "{1:2:3}",
            //     "7|8 !!!! \n.8|1 !!!! \n"
            // }
    };

    if (pass && false)
    for (auto const& test : json_test_suite)
    {
        auto result = json_parse(test.expression);
        
        string dump = result.second.dump();
        if (result.first != test.pass || test.dump != dump)
        {
            pass = false;
            cout << "succeeded to parse: " << result.first << endl;
            cout << test.expression << endl;
            cout << dump << endl;
            cout << test.dump;
            break;
        }
    }

    std::ifstream fl;
    std::istreambuf_iterator<char> begin, end;
    load_file("/storage/media/data/prog.projects/publiq.pp/src/belt.pp/src/test_parser/block.31353", fl, begin, end);
    
    string json_data(begin, end);

        int a;
        //std::cin >> a;
        auto tp_start = std::chrono::steady_clock::now();
    for (int i = 0; i < 100; ++i)
    {
        json_parse(json_data);
    }
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tp_start);
        cout << duration.count() << "\n";
        //std::cin >> a;
        B_UNUSED(a);
    
    if (pass && false)
    {
        size_t stack_size_check = 10000;
        parse_test stack_test{true, "", ""};
        for (size_t index = 0; index != stack_size_check; ++index)
        {
            stack_test.expression += std::to_string(index);
            if (index != stack_size_check - 1)
                stack_test.expression += " + (";

            if (index != stack_size_check - 1)
            {
                stack_test.dump += string(2 * index, '.') + "1|1\n";
                stack_test.dump += string(2 * index, '.') + "." + std::to_string(index) + "\n";
                stack_test.dump += string(2 * index, '.') + ".7|2\n";
            }
            else
                stack_test.dump += string(2 * index, '.') + std::to_string(index) + "\n";
        }
        for (size_t index = 0; index != stack_size_check - 1; ++index)
        {
            stack_test.expression += ")";
        }

        vector<parse_test> test_suite = 
        {
            
            parse_test{
                true,
                "1 2 ",
                "4|1\n.1\n.2\n"
            },

            parse_test{
                true,
                "1+2*3",
                "1|1\n.1\n.4|1\n..2\n..3\n"
            },
            parse_test{
                true,
                "(1)",
                "7|2\n.1\n"
            },

            parse_test{
                true,
                "1 + 2",
                "1|1\n.1\n.2\n"
            },

            parse_test{
                true,
                "1 + 2 + 3",
                "1|1\n.1\n.2\n.3\n"
            },

            parse_test{
                true,
                "1 ^ 2 ^ 3",
                "6|1\n.1\n.2\n.3\n"
            },

            parse_test{
                true,
                "1 + 2 * 3",
                "1|1\n.1\n.4|1\n..2\n..3\n"
            },

            parse_test{
                true,
                "1 * 2 / 3 * 4",
R"A(4|1
.5|1
..4|1
...1
...2
..3
.4
)A"
            },

            parse_test{
                true,
                "1 + 2 - 3 + 4",
R"A(1|1
.2|1
..1|1
...1
...2
..3
.4
)A"
            },

            parse_test{
                true,
                "1 - + 2",
                "2|1\n.1\n.8|1\n..2\n"
            },
            
            parse_test{
                true,
                "1 2",
                "4|1\n.1\n.2\n"
            },

            parse_test{
                true,
                "1 2 3",
                "4|1\n.1\n.2\n.3\n"
            },

            parse_test{
                true,
                "(1 2 3 * 4)",
                "7|2\n.4|1\n..1\n..2\n..3\n..4\n"
            },

            parse_test{
                true,
                "1 - 2 / 3 4 5",
                "2|1\n.1\n.4|1\n..5|1\n...2\n...3\n..4\n..5\n"
            },
            parse_test{
                true,
                "1 - 2 * 3 / 4 5",
                "2|1\n.1\n.4|1\n..5|1\n...4|1\n....2\n....3\n...4\n..5\n"
            },
            parse_test{
                true,
                "1 + 2 3",
                "1|1\n.1\n.4|1\n..2\n..3\n"
            },

            parse_test{
                false,
                "*",
                " !!!! \n"
            },

            parse_test{
                false,
                "+",
                "8|1 !!!! \n"
            },

            parse_test{
                false,
                "(+)",
                "7|4 !!!! \n.8|1 !!!! \n"
            },

            parse_test{
                false,
                ")",
                " !!!! \n"
            },

            parse_test{
                false,
                "(",
                "7|4 !!!! \n"
            },

            parse_test{
                false,
                "1)",
                "7|8 !!!! \n.1\n"
            },

            parse_test{
                true,
                "()",
                "7|2\n"
            },

            parse_test{
                true,
                "+1",
                "+1\n"
            },

            parse_test{
                true,
                "++1",
                "8|1\n.+1\n"
            },

            parse_test{
                true,
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
                "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
                "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
                "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
                "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
                "241 + 242 + 243 + 244 + 245 + 246 + 247 + 248 + 249 + 250 +"
                "251 + 252 + 253 + 254 + 255 + 256",
                "1|1\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.241\n.242\n.243\n.244\n.245\n.246\n.247\n.248\n.249\n"
                ".250\n.251\n.252\n.253\n.254\n.255\n.256\n"
            },

            // // the below test is tricky
            // // it has too many additions, so the last "+1" fails as binary addition
            // // so it becomes a multiplication by "+1"
            // parse_test{
            //     true,
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + "
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109 + 110 +"
            //     "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
            //     "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
            //     "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
            //     "201 + 202 + 203 + 204 + 205 + 206 + 207 + 208 + 209 + 210 +"
            //     "241 + 242 + 243 + 244 + 245 + 246 + 247 + 248 + 249 + 250 +"
            //     "251 + 252 + 253 + 254 + 255 + 256 +1",
            //     "1|1\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.1\n.2\n.3\n.4\n.5\n.6\n.7\n.8\n.9\n.10\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.101\n.102\n.103\n.104\n.105\n.106\n.107\n.108\n.109\n.110\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.201\n.202\n.203\n.204\n.205\n.206\n.207\n.208\n.209\n.210\n.241\n.242\n.243\n.244\n.245\n.246\n.247\n.248\n.249\n"
            //     ".250\n.251\n.252\n.253\n.254\n.255\n.4|1\n..256\n..8|1\n...1\n"
            // },

            parse_test{
                true,
                "1 + (2 + (2 + (2 + (2 + (2 + (2 + 1 + (2 + (2 + (2 + (2 + (2 + (2 + 1 + (2 + (2 + (2 + (2 + (2 + (2 + 1 + (2 + (2 + (2 + (2 + (2 + (2 + 1 + (2 + (2 + (2 + (2 + (2 + (2 + 1 + (2 + (2 + (2 + (2 + (2 + (2 + 1 + (2 + (2 + (2 + (2 + (2 + (2 + 1 + (2 + (2 + (2 + (2 + (2 + (2 + 1))))))))))))))))))))))))))))))))))))))))))))))))",
                "1|1\n.1\n.7|2\n..1|1\n...2\n...7|2\n....1|1\n.....2\n.....7|2\n......1|1\n.......2\n.......7|2\n........1|1\n.........2\n.........7|2\n..........1|1\n...........2\n...........7|2\n............1|1\n.............2\n.............1\n.............7|2\n..............1|1\n...............2\n...............7|2\n................1|1\n.................2\n.................7|2\n..................1|1\n...................2\n...................7|2\n....................1|1\n.....................2\n.....................7|2\n......................1|1\n.......................2\n.......................7|2\n........................1|1\n.........................2\n.........................1\n.........................7|2\n..........................1|1\n...........................2\n...........................7|2\n............................1|1\n.............................2\n.............................7|2\n..............................1|1\n...............................2\n...............................7|2\n................................1|1\n.................................2\n.................................7|2\n..................................1|1\n...................................2\n...................................7|2\n....................................1|1\n.....................................2\n.....................................1\n.....................................7|2\n......................................1|1\n.......................................2\n.......................................7|2\n........................................1|1\n.........................................2\n.........................................7|2\n..........................................1|1\n...........................................2\n...........................................7|2\n............................................1|1\n.............................................2\n.............................................7|2\n..............................................1|1\n...............................................2\n...............................................7|2\n................................................1|1\n.................................................2\n.................................................1\n.................................................7|2\n..................................................1|1\n...................................................2\n...................................................7|2\n....................................................1|1\n.....................................................2\n.....................................................7|2\n......................................................1|1\n.......................................................2\n.......................................................7|2\n........................................................1|1\n.........................................................2\n.........................................................7|2\n..........................................................1|1\n...........................................................2\n...........................................................7|2\n............................................................1|1\n.............................................................2\n.............................................................1\n.............................................................7|2\n..............................................................1|1\n...............................................................2\n...............................................................7|2\n................................................................1|1\n.................................................................2\n.................................................................7|2\n..................................................................1|1\n...................................................................2\n...................................................................7|2\n....................................................................1|1\n.....................................................................2\n.....................................................................7|2\n......................................................................1|1\n.......................................................................2\n.......................................................................7|2\n........................................................................1|1\n.........................................................................2\n.........................................................................1\n.........................................................................7|2\n..........................................................................1|1\n...........................................................................2\n...........................................................................7|2\n............................................................................1|1\n.............................................................................2\n.............................................................................7|2\n..............................................................................1|1\n...............................................................................2\n...............................................................................7|2\n................................................................................1|1\n.................................................................................2\n.................................................................................7|2\n..................................................................................1|1\n...................................................................................2\n...................................................................................7|2\n....................................................................................1|1\n.....................................................................................2\n.....................................................................................1\n.....................................................................................7|2\n......................................................................................1|1\n.......................................................................................2\n.......................................................................................7|2\n........................................................................................1|1\n.........................................................................................2\n.........................................................................................7|2\n..........................................................................................1|1\n...........................................................................................2\n...........................................................................................7|2\n............................................................................................1|1\n.............................................................................................2\n.............................................................................................7|2\n..............................................................................................1|1\n...............................................................................................2\n...............................................................................................7|2\n................................................................................................1|1\n.................................................................................................2\n.................................................................................................1\n"
                //"2|1\n.1\n.7|2\n..2|1\n...2\n...7|2\n....2|1\n.....2\n.....7|2\n......2|1\n.......2\n.......7|2\n........2|1\n.........2\n.........7|2\n..........2|1\n...........2\n...........7|2\n............2|1\n.............2\n.............1\n.............7|2\n..............2|1\n...............2\n...............7|2\n................2|1\n.................2\n.................7|2\n..................2|1\n...................2\n...................7|2\n....................2|1\n.....................2\n.....................7|2\n......................2|1\n.......................2\n.......................7|2\n........................2|1\n.........................2\n.........................1\n.........................7|2\n..........................2|1\n...........................2\n...........................7|2\n............................2|1\n.............................2\n.............................7|2\n..............................2|1\n...............................2\n...............................7|2\n................................2|1\n.................................2\n.................................7|2\n..................................2|1\n...................................2\n...................................7|2\n....................................2|1\n.....................................2\n.....................................1\n.....................................7|2\n......................................2|1\n.......................................2\n.......................................7|2\n........................................2|1\n.........................................2\n.........................................7|2\n..........................................2|1\n...........................................2\n...........................................7|2\n............................................2|1\n.............................................2\n.............................................7|2\n..............................................2|1\n...............................................2\n...............................................7|2\n................................................2|1\n.................................................2\n.................................................1\n.................................................7|2\n..................................................2|1\n...................................................2\n...................................................7|2\n....................................................2|1\n.....................................................2\n.....................................................7|2\n......................................................2|1\n.......................................................2\n.......................................................7|2\n........................................................2|1\n.........................................................2\n.........................................................7|2\n..........................................................2|1\n...........................................................2\n...........................................................7|2\n............................................................2|1\n.............................................................2\n.............................................................1\n.............................................................7|2\n..............................................................2|1\n...............................................................2\n...............................................................7|2\n................................................................2|1\n.................................................................2\n.................................................................7|2\n..................................................................2|1\n...................................................................2\n...................................................................7|2\n....................................................................2|1\n.....................................................................2\n.....................................................................7|2\n......................................................................2|1\n.......................................................................2\n.......................................................................7|2\n........................................................................2|1\n.........................................................................2\n.........................................................................1\n.........................................................................7|2\n..........................................................................2|1\n...........................................................................2\n...........................................................................7|2\n............................................................................2|1\n.............................................................................2\n.............................................................................7|2\n..............................................................................2|1\n...............................................................................2\n...............................................................................7|2\n................................................................................2|1\n.................................................................................2\n.................................................................................7|2\n..................................................................................2|1\n...................................................................................2\n...................................................................................7|2\n....................................................................................2|1\n.....................................................................................2\n.....................................................................................1\n.....................................................................................7|2\n......................................................................................2|1\n.......................................................................................2\n.......................................................................................7|2\n........................................................................................2|1\n.........................................................................................2\n.........................................................................................7|2\n..........................................................................................2|1\n...........................................................................................2\n...........................................................................................7|2\n............................................................................................2|1\n.............................................................................................2\n.............................................................................................7|2\n..............................................................................................2|1\n...............................................................................................2\n...............................................................................................7|2\n................................................................................................2|1\n.................................................................................................2\n.................................................................................................1\n"
                //"+\n.1\n.()\n..+\n...2\n...()\n....+\n.....2\n.....()\n......+\n.......2\n.......()\n........+\n.........2\n.........()\n..........+\n...........2\n...........()\n............++\n.............2\n.............1\n.............()\n..............+\n...............2\n...............()\n................+\n.................2\n.................()\n..................+\n...................2\n...................()\n....................+\n.....................2\n.....................()\n......................+\n.......................2\n.......................()\n........................++\n.........................2\n.........................1\n.........................()\n..........................+\n...........................2\n...........................()\n............................+\n.............................2\n.............................()\n..............................+\n...............................2\n...............................()\n................................+\n.................................2\n.................................()\n..................................+\n...................................2\n...................................()\n....................................++\n.....................................2\n.....................................1\n.....................................()\n......................................+\n.......................................2\n.......................................()\n........................................+\n.........................................2\n.........................................()\n..........................................+\n...........................................2\n...........................................()\n............................................+\n.............................................2\n.............................................()\n..............................................+\n...............................................2\n...............................................()\n................................................++\n.................................................2\n.................................................1\n.................................................()\n..................................................+\n...................................................2\n...................................................()\n....................................................+\n.....................................................2\n.....................................................()\n......................................................+\n.......................................................2\n.......................................................()\n........................................................+\n.........................................................2\n.........................................................()\n..........................................................+\n...........................................................2\n...........................................................()\n............................................................++\n.............................................................2\n.............................................................1\n.............................................................()\n..............................................................+\n...............................................................2\n...............................................................()\n................................................................+\n.................................................................2\n.................................................................()\n..................................................................+\n...................................................................2\n...................................................................()\n....................................................................+\n.....................................................................2\n.....................................................................()\n......................................................................+\n.......................................................................2\n.......................................................................()\n........................................................................++\n.........................................................................2\n.........................................................................1\n.........................................................................()\n..........................................................................+\n...........................................................................2\n...........................................................................()\n............................................................................+\n.............................................................................2\n.............................................................................()\n..............................................................................+\n...............................................................................2\n...............................................................................()\n................................................................................+\n.................................................................................2\n.................................................................................()\n..................................................................................+\n...................................................................................2\n...................................................................................()\n....................................................................................++\n.....................................................................................2\n.....................................................................................1\n.....................................................................................()\n......................................................................................+\n.......................................................................................2\n.......................................................................................()\n........................................................................................+\n.........................................................................................2\n.........................................................................................()\n..........................................................................................+\n...........................................................................................2\n...........................................................................................()\n............................................................................................+\n.............................................................................................2\n.............................................................................................()\n..............................................................................................+\n...............................................................................................2\n...............................................................................................()\n................................................................................................+\n.................................................................................................2\n.................................................................................................1\n"
            },

            stack_test
        };

        if (pass && false)
        for (auto& test : test_suite)
        {
            auto result = math_parse(test.expression);
            
            string dump = result.second.dump();
            if (result.first != test.pass || test.dump != dump)
            {
                pass = false;
                cout << "succeeded to parse: " << result.first << endl;
                cout << test.expression << endl;
                cout << dump << endl;
                cout << test.dump;
                break;
            }
        }
    }

    if(pass)
        cout << u8"The test is passed 😁 " << endl;
    else
        cout << "The test is failed!!!" << endl;

    //string tempStr = R"FF({"some-dbl": "10."})FF";

    return 0;
}
