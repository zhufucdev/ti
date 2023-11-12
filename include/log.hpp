#include <iostream>
#ifdef _DEBUG
#define logD(x, ...) printf(x "\n", ##__VA_ARGS__)
#else
#define logD(x, ...)
#endif