#include <sqlite3.h>
#include <iostream>

namespace ti {
namespace orm {

typedef struct {
    int height, width;
    std::string *columns;
    std::string **rows;
} Table;

class SqlHelper {
    sqlite3 *dbhandle;

  protected:
    Table *exec_sql(char *expr);

  public:
    explicit SqlHelper(const char *dbfile);
    ~SqlHelper();
    static void initialize();
    static void shutdown();
};
} // namespace orm
} // namespace ti
