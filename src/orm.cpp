#include "orm.h"

#define BUFFER_SIZE 3

using namespace ti::orm;

typedef struct {
    Table *table;
    int cap;
} TableProcessor;

SqlHelper::SqlHelper(const char *dbfile) { sqlite3_open(dbfile, &dbhandle); }

SqlHelper::~SqlHelper() { sqlite3_close(dbhandle); }

int callback(void *t, int argc, char **argv, char **azColName) {
    auto *tp = (TableProcessor *)t;
    auto tt = tp->table;
    if (tt->width <= 0) {
        tt->width = argc;
    }
    if (tt->columns == nullptr) {
        tt->columns = (std::string *)malloc(sizeof(std::string *) * argc);
        for (int i = 0; i < argc; ++i) {
            tt->columns[i] = std::string(azColName[i]);
        }
    }

    if (tp->cap <= tt->height) {
        if (tt->rows == nullptr) {
            tt->rows = (std::string **)calloc(tp->cap, sizeof(std::string *));
        } else {
            tt->rows = (std::string **)realloc(tt->rows,
                                               sizeof(std::string *) * tp->cap);
            if (tt->rows == nullptr) {
                throw std::runtime_error("realloc() failed");
            }
        }
        tp->cap += BUFFER_SIZE;
    }
    tt->rows[tt->height] = (std::string *)malloc(sizeof(std::string *));
    for (int i = 0; i < argc; ++i) {
        tt->rows[tt->height][i] = std::string(argv[i]);
    }
    tt->height++;
    return 0;
}
Table *SqlHelper::exec_sql(char *expr) {
    auto *tt = (Table *)malloc(sizeof(Table));
    tt->height = 0;
    tt->width = 0;
    tt->rows = nullptr;
    tt->columns = nullptr;
    auto *tp = (TableProcessor *)malloc(sizeof(TableProcessor));
    tp->table = tt;
    tp->cap = 0;
    char *err = nullptr;
    int n = sqlite3_exec(dbhandle, expr, callback, tp, &err);
    if (n == SQLITE_OK) {
        return tt;
    } else {
        auto m = std::string(err);
        sqlite3_free(err);
        throw std::runtime_error(m);
    }
}
void SqlHelper::initialize() {
    sqlite3_initialize();
}
void SqlHelper::shutdown() {
    sqlite3_shutdown();
}