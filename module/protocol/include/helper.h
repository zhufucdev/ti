#include <cstddef>
#include <string>

#define BYTES_LEN_HEADER 8

namespace ti {
namespace helper {
char *write_len_header(size_t len);
size_t read_len_header(const char *tsize);
std::string to_iso_time(const std::time_t &time);
std::time_t parse_iso_time(const std::string &str);
std::vector<std::string> read_message_body(const char *data, size_t len,
                                           char separator = '\0');
template <class InputIterator>
InputIterator get_entity_in(InputIterator first, InputIterator last,
                            const std::string &id) {
    for (; first != last; first++) {
        if (*first != nullptr && (*first)->get_id() == id) {
            return first;
        }
    }
    return last;
}
size_t next_sync_hash(char *curr, size_t curr_len, const std::string &addition,
                      char **dst);
} // namespace helper
} // namespace ti
