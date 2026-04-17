#include "../include/user.h"
#include "../include/color_codes.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <openssl/evp.h>

using namespace std;
using namespace Color;

User::User(DatabaseManager *db) : db_manager(db), current_user_id(-1), logged_in(false) {}

// SHA256 哈希函数 - 简化版
static string sha256(const string &password)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == nullptr)
        return "";

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, password.c_str(), password.length());

    unsigned char hash[32];
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);

    string result;
    for (unsigned int i = 0; i < len; i++)
    {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        result += buf;
    }

    return result;
}

bool User::login(const string &account, const string &password)
{
    if (!db_manager)
        return false;

    string sql = "SELECT id, account, password_hash FROM users WHERE account = '" + account + "'";
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "❌ 账号不存在" << endl;
        return false;
    }

    string stored_hash = results[0]["password_hash"];
    string input_hash = sha256(password);

    if (input_hash != stored_hash)
    {
        cout << "❌ 密码错误" << endl;
        return false;
    }

    current_user_id = stoi(results[0]["id"]);
    current_account = results[0]["account"];
    logged_in = true;

    string update_sql = "UPDATE users SET last_login = NOW() WHERE id = " + to_string(current_user_id);
    db_manager->run_sql(update_sql);

    cout << "✅ 登录成功，欢迎 " << current_account << endl;
    return true;
}

bool User::register_user(const string &account, const string &password)
{
    if (!db_manager)
        return false;

    string check_sql = "SELECT 1 FROM users WHERE account = '" + account + "'";
    auto results = db_manager->query(check_sql);

    if (!results.empty())
    {
        cout << "❌ 账号已存在" << endl;
        return false;
    }

    string password_hash = sha256(password);
    string insert_sql = "INSERT INTO users (account, password_hash) VALUES ('" +
                        account + "', '" + password_hash + "')";

    if (db_manager->run_sql(insert_sql))
    {
        cout << "✅ 注册成功" << endl;
        return true;
    }

    return false;
}

bool User::change_my_password(const string &old_password, const string &new_password)
{
    if (!logged_in)
    {
        cout << "❌ 请先登录" << endl;
        return false;
    }

    string sql = "SELECT password_hash FROM users WHERE id = " + to_string(current_user_id);
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "❌ 用户数据异常" << endl;
        return false;
    }

    string stored_hash = results[0]["password_hash"];
    string old_hash = sha256(old_password);

    if (old_hash != stored_hash)
    {
        cout << "❌ 旧密码错误" << endl;
        return false;
    }

    string new_hash = sha256(new_password);
    string update_sql = "UPDATE users SET password_hash = '" + new_hash +
                        "' WHERE id = " + to_string(current_user_id);

    if (db_manager->run_sql(update_sql))
    {
        cout << "✅ 密码修改成功" << endl;
        return true;
    }

    return false;
}

void User::list_problems()
{
    if (!db_manager)
        return;

    string sql = "SELECT id, title, time_limit, memory_limit FROM problems ORDER BY id";
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "📭 暂无题目" << endl;
        return;
    }

    cout << "\n"
         << GREEN << "========== 题目列表 ==========" << RESET << endl;
    cout << left << setw(6) << "ID"
         << setw(30) << "标题"
         << setw(15) << "时间限制"
         << setw(15) << "内存限制" << endl;
    cout << string(66, '-') << endl;

    for (const auto &row : results)
    {
        cout << left << setw(6) << row.at("id")
             << setw(30) << row.at("title")
             << setw(15) << (row.at("time_limit") + " ms")
             << setw(15) << (row.at("memory_limit") + " MB") << endl;
    }

    cout << GREEN << "==============================" << RESET << "\n"
         << endl;
}

void User::view_problem(int id)
{
    if (!db_manager)
        return;

    string sql = "SELECT * FROM problems WHERE id = " + to_string(id);
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "❌ 题目不存在 (ID: " << id << ")" << endl;
        return;
    }

    const auto &p = results[0];

    cout << "\n"
         << GREEN << "========== 题目详情 ==========" << RESET << endl;
    cout << "【题号】 " << p.at("id") << endl;
    cout << "【标题】 " << p.at("title") << endl;
    cout << "【时间限制】 " << p.at("time_limit") << " ms" << endl;
    cout << "【内存限制】 " << p.at("memory_limit") << " MB" << endl;
    cout << GREEN << "---------- 题目描述 ----------" << RESET << endl;
    cout << p.at("description") << endl;
    cout << GREEN << "==============================" << RESET << "\n"
         << endl;
}

void User::submit_code(int problem_id, const string &code, const string &language)
{
    if (!logged_in)
    {
        cout << "❌ 请先登录" << endl;
        return;
    }

    cout << "[User] 提交代码 (Problem ID: " << problem_id
         << ", Language: " << language << ") - 待实现" << endl;
}

void User::view_my_submissions()
{
    if (!logged_in)
    {
        cout << "❌ 请先登录" << endl;
        return;
    }

    cout << "[User] 查看我的提交记录 - 待实现" << endl;
}
