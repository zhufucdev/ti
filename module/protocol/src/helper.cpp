#include "helper.h"
#include <sha3.h>
#include <timecompat.h>
#include <vector>

char *ti::helper::write_len_header(size_t len) {
    char *tsize = (char *)calloc(BYTES_LEN_HEADER, sizeof(char));
    int n = BYTES_LEN_HEADER - sizeof(len);
    while (n < BYTES_LEN_HEADER) {
        tsize[n] = (len >> (sizeof(len) - n - 1) * 8);
        n++;
    }
    return tsize;
}

size_t ti::helper::read_len_header(const char *tsize) {
    int n = 0;
    size_t msize = 0;
    while (n < BYTES_LEN_HEADER) {
        msize <<= sizeof(char);
        msize |= tsize[n++];
    }
    return msize;
}

std::string ti::helper::to_iso_time(const std::time_t &time) {
    char buf[sizeof "0000-00-00T00:00:00Z"]; // i hate magic numbers
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&time));
    return buf;
}

std::time_t ti::helper::parse_iso_time(const std::string &str) {
    std::tm tm{};
    compat::time::strptime(str.c_str(), "%FT%TZ", &tm);
    return compat::time::timegm(&tm);
}

std::vector<std::string>
ti::helper::read_message_body(const char *data, size_t len, char separator) {
    int i = 0, j = 0;
    std::vector<std::string> heap;
    while (i < len) {
        if (data[i] == separator) {
            std::string substr(data + j, i - j);
            heap.push_back(substr);
            j = i + 1;
        }
        i++;
    }
    if (j != i) {
        std::string substr(data + j, len - j);
        heap.push_back(substr);
    }
    return heap;
}

size_t hex2bin(const std::string &hex, char **buf) {
    *buf = (char *)calloc(hex.size() / 2, sizeof(char));
    for (size_t i = 0, j = 0; i < hex.size(); i++) {
        unsigned char high = hex[i] >= 'a' ? hex[i] - 'a' + 10 : hex[i] - '0';
        i++;
        unsigned char low = hex[i] >= 'a' ? hex[i] - 'a' + 10 : hex[i] - '0';
        (*buf)[j++] = high * 16 + low;
    }
    return hex.size() / 2;
}

size_t ti::helper::next_sync_hash(char *curr, size_t curr_len,
                                  const std::string &addition, char **dst) {
    SHA3 sha3;
    sha3.add(curr, curr_len);
    sha3.add(addition.c_str(), addition.length());
    auto hex = sha3.getHash();
    auto len = hex2bin(hex, dst);
    return len;
}
