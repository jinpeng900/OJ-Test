#include "../include/app_context.h"
#include <cstdlib>

using namespace std;

static string getDbHost()
{
    const char *host = getenv("OJ_DB_HOST");
    return host ? string(host) : "localhost";
}

unique_ptr<DatabaseManager> AppContext::createAdminDB()
{
    return make_unique<DatabaseManager>(
        getDbHost(), "oj_admin", "090800", "OJ");
}

unique_ptr<DatabaseManager> AppContext::createUserDB()
{
    return make_unique<DatabaseManager>(
        getDbHost(), "oj_user", "user123", "OJ");
}
