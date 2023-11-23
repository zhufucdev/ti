#include "socketcompat.h"

namespace compat {
namespace socket {
void send(SocketFd fd, const void *buf, size_t len, int flags) {
#ifdef _WIN32
::send(fd, (char *)buf, len, flags);
#else
::send(fd, buf, len, flags);
#endif
}
} // namespace socket
}
