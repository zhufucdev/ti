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
template <class EntityIterator>
inline std::vector<std::string> get_ids(EntityIterator first, EntityIterator last) {
    std::vector<std::string> result;
    std::transform(first, last, std::back_inserter(result),
                   [&](auto e) { return e->get_id(); });
    return result;
}
size_t next_sync_hash(char *curr, size_t curr_len, const std::string &addition,
                      char **dst);
template <class T, typename Iterator = typename std::vector<T>::iterator>
class Diff {
  public:
    std::vector<T> plus, minus;
    Diff(Iterator a_first, Iterator a_last, Iterator b_first, Iterator b_last)
        : plus(), minus() {
        Iterator ai, bi;
        for (ai = a_first; ai != a_last; ai++) {
            if (std::find(b_first, b_last, *ai) == b_last) {
                plus.push_back(*ai);
            }
        }
        for (bi = b_first; bi != b_last; bi++) {
            if (std::find(a_first, a_last, *bi) == a_last) {
                minus.push_back(*bi);
            }
        }
    }
};
} // namespace helper
} // namespace ti
