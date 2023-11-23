#include <ctime>

namespace compat {
namespace time {
char* strptime(const char *time, const char *format, std::tm *tm);
std::time_t timegm(std::tm *tm);
}
}
