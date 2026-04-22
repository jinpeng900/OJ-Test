#include "../include/app_context.h"

using namespace std;

unique_ptr<DatabaseManager> AppContext::createAdminDB()
{
    return make_unique<DatabaseManager>(
        "localhost", "oj_admin", "090800", "OJ");
}

unique_ptr<DatabaseManager> AppContext::createUserDB()
{
    return make_unique<DatabaseManager>(
        "localhost", "oj_user", "user123", "OJ");
}
