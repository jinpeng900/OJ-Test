#include "../include/user.h"
#include "../include/color_codes.h"
#include "../include/judge_core.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <openssl/evp.h> // 用于 SHA256 哈希

using namespace std;
using namespace Color;

User::User(DatabaseManager *db) : db_manager(db), current_user_id(-1), logged_in(false) {}

// SHA256 哈希函数 - 用于密码哈希
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

    string escaped_account = db_manager->escape_string(account);
    string sql = "SELECT id, account, password_hash FROM users WHERE account = '" + escaped_account + "'";
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "账号不存在" << endl;
        return false;
    }

    string stored_hash = results[0]["password_hash"];
    string input_hash = sha256(password);

    if (input_hash != stored_hash)
    {
        cout << "密码错误" << endl;
        return false;
    }

    current_user_id = stoi(results[0]["id"]);
    current_account = results[0]["account"];
    logged_in = true;

    string update_sql = "UPDATE users SET last_login = NOW() WHERE id = " + to_string(current_user_id);
    db_manager->run_sql(update_sql);

    cout << " 登录成功，欢迎 " << current_account << endl;
    return true;
}

bool User::register_user(const string &account, const string &password)
{
    if (!db_manager)
        return false;

    string escaped_account = db_manager->escape_string(account);
    string check_sql = "SELECT 1 FROM users WHERE account = '" + escaped_account + "'";
    auto results = db_manager->query(check_sql);

    if (!results.empty())
    {
        cout << "账号已存在" << endl;
        return false;
    }

    string password_hash = sha256(password);
    string escaped_password_hash = db_manager->escape_string(password_hash);
    string insert_sql = "INSERT INTO users (account, password_hash) VALUES ('" +
                        escaped_account + "', '" + escaped_password_hash + "')";

    if (db_manager->run_sql(insert_sql))
    {
        cout << "注册成功" << endl;
        return true;
    }

    return false;
}

bool User::change_my_password(const string &old_password, const string &new_password)
{
    if (!logged_in)
    {
        cout << "请先登录" << endl;
        return false;
    }

    string sql = "SELECT password_hash FROM users WHERE id = " + to_string(current_user_id);
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "用户数据异常" << endl;
        return false;
    }

    string stored_hash = results[0]["password_hash"];
    string old_hash = sha256(old_password);

    if (old_hash != stored_hash)
    {
        cout << "旧密码错误" << endl;
        return false;
    }

    string new_hash = sha256(new_password);
    string escaped_new_hash = db_manager->escape_string(new_hash);
    string update_sql = "UPDATE users SET password_hash = '" + escaped_new_hash +
                        "' WHERE id = " + to_string(current_user_id);

    if (db_manager->run_sql(update_sql))
    {
        cout << "密码修改成功" << endl;
        return true;
    }

    return false;
}

void User::list_problems()
{
    if (!db_manager)
        return;

    string sql = "SELECT id, title, category, time_limit, memory_limit FROM problems ORDER BY id";
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "暂无题目" << endl;
        return;
    }

    cout << "\n"
         << GREEN << "============================== 题目列表 ================================" << RESET << endl;
    cout << "ID  标题                                知识点" << endl;
    cout << string(70, '-') << endl;

    for (const auto &row : results)
    {
        string category = row.at("category");
        if (category.empty())
            category = "-";

        string title = row.at("title");
        string id = row.at("id");

        // --- 核心修改：遍历字符串，计算显示宽度，并安全截断防止中文乱码 ---
        int display_width = 0;
        string formatted_title = "";

        for (size_t i = 0; i < title.length();)
        {
            int bytes = 1; // 当前字符占用的字节数
            int width = 1; // 当前字符在终端的显示宽度

            // 判断 UTF-8 字符边界
            if ((title[i] & 0x80) == 0)
            {
                bytes = 1;
                width = 1;
            } // ASCII 英文/数字
            else if ((title[i] & 0xE0) == 0xE0)
            {
                bytes = 3;
                width = 2;
            } // 常用中文 (3字节，显示宽度2)
            else if ((title[i] & 0xE0) == 0xC0)
            {
                bytes = 2;
                width = 1;
            } // 拉丁文等 (2字节)
            else
            {
                bytes = 4;
                width = 2;
            } // Emoji 或生僻字

            // 限制标题长度：如果加上当前字符超过 33 个显示宽度，且还没读完，则截断
            if (display_width + width > 33 && i + bytes < title.length())
            {
                formatted_title += "...";
                display_width += 3;
                break; // 跳出，不再拼接后面的字符
            }

            formatted_title += title.substr(i, bytes);
            display_width += width;
            i += bytes;
        }

        // 填充空格补齐到 36 个显示宽度
        if (display_width < 36)
        {
            formatted_title += string(36 - display_width, ' ');
        }

        // --- 输出环节 ---
        cout << id;

        // ID 补齐 4 个宽度 (假设 ID 都是纯数字/ASCII)
        int id_spaces = 4 - id.length();
        if (id_spaces > 0)
        {
            cout << string(id_spaces, ' ');
        }

        // 输出对齐后的标题和知识点
        cout << formatted_title << category << endl;
    }

    cout << GREEN << "======================================================================" << RESET << "\n"
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
        cout << "题目不存在 (ID: " << id << ")" << endl;
        return;
    }

    const auto &p = results[0];

    cout << "\n"
         << GREEN << "============================ 题目详情 ============================" << RESET << endl;
    cout << "【题号】     " << p.at("id") << endl;
    cout << "【标题】     " << p.at("title") << endl;
    cout << "【知识点】   " << (p.at("category").empty() ? "-" : p.at("category")) << endl;
    cout << "【时间限制】 " << p.at("time_limit") << " ms" << endl;
    cout << "【内存限制】 " << p.at("memory_limit") << " MB" << endl;
    cout << GREEN << "---------------------------- 题目描述 ----------------------------" << RESET << endl;
    cout << p.at("description") << endl;
    cout << GREEN << "================================================================" << RESET << "\n"
         << endl;
}

void User::submit_code(int problem_id, const string &code)
{
    if (!logged_in)
    {
        cout << "请先登录" << endl;
        return;
    }
    if (!db_manager)
        return;

    // ---- 1. 获取题目配置 ----
    string prob_sql = "SELECT time_limit, memory_limit, test_data_path FROM problems WHERE id = " +
                      to_string(problem_id);
    auto prob = db_manager->query(prob_sql);
    if (prob.empty())
    {
        cout << RED << "题目不存在 (ID: " << problem_id << ")" << RESET << endl;
        return;
    }

    int time_limit_ms = 1000, memory_limit_mb = 128;
    try
    {
        time_limit_ms = stoi(prob[0]["time_limit"]);
        memory_limit_mb = stoi(prob[0]["memory_limit"]);
    }
    catch (...)
    {
    }

    string test_data_path = prob[0]["test_data_path"];
    if (test_data_path.empty() || !filesystem::exists(test_data_path))
    {
        cout << RED << "测试数据路径不存在: " << test_data_path << RESET << endl;
        return;
    }

    // ---- 2. 配置并执行评测 ----
    cout << GREEN << "\n 正在提交评测，请稍候..." << RESET << endl;

    JudgeCore judge_engine; // 评测核心
    JudgeConfig config;     // 配置评测参数
    config.time_limit_ms = time_limit_ms;
    config.memory_limit_mb = memory_limit_mb;
    judge_engine.setConfig(config);
    judge_engine.setSourceCode(code);
    judge_engine.setTestDataPath(test_data_path);

    JudgeReport report = judge_engine.judge();
    // ---- 3. 构建错误上下文（供 AI 使用）----
    last_error_context_ = "";
    if (report.result != JudgeResult::ACCEPTED)
    {
        string ctx = "【上次评测结果】\n";
        switch (report.result)
        {
        case JudgeResult::COMPILE_ERROR:
            ctx += "状态: 编译错误 (CE)\n编译错误信息:\n" + report.error_message;
            break;
        case JudgeResult::WRONG_ANSWER:
            ctx += "状态: 答案错误 (WA)\n";
            ctx += "通过: " + to_string(report.passed_test_cases) + "/" +
                   to_string(report.total_test_cases) + " 测试点\n";
            for (const auto &d : report.details)
                if (d.result == JudgeResult::WRONG_ANSWER)
                {
                    ctx += "第 " + to_string(d.case_id) + " 个测试点:\n" +
                           d.output_diff + "\n";
                    break;
                }
            break;
        case JudgeResult::TIME_LIMIT_EXCEEDED:
            ctx += "状态: 超时 (TLE)\n";
            ctx += "通过: " + to_string(report.passed_test_cases) + "/" +
                   to_string(report.total_test_cases) + " 测试点\n";
            ctx += "最大耗时: " + to_string(report.time_used_ms) + " ms\n";
            break;
        case JudgeResult::MEMORY_LIMIT_EXCEEDED:
            ctx += "状态: 超内存 (MLE)\n内存: " +
                   to_string(report.memory_used_mb) + " MB\n";
            break;
        case JudgeResult::RUNTIME_ERROR:
            ctx += "状态: 运行时错误 (RE)\n";
            ctx += "通过: " + to_string(report.passed_test_cases) + "/" +
                   to_string(report.total_test_cases) + " 测试点\n";
            break;
        default:
            ctx += "状态: " + report.error_message;
            break;
        }
        last_error_context_ = ctx;
    }

    // ---- 4. 检查是否首次 AC（在插入前查询）----
    bool is_first_ac = false;
    if (report.result == JudgeResult::ACCEPTED)
    {
        string chk = "SELECT 1 FROM submissions WHERE user_id = " +
                     to_string(current_user_id) + " AND problem_id = " +
                     to_string(problem_id) + " AND status = 'AC' LIMIT 1";
        is_first_ac = db_manager->query(chk).empty();
    }

    // ---- 5. 写入 submissions 表 ----
    string status_str;
    switch (report.result)
    {
    case JudgeResult::ACCEPTED:
        status_str = "AC";
        break;
    case JudgeResult::WRONG_ANSWER:
        status_str = "WA";
        break;
    case JudgeResult::TIME_LIMIT_EXCEEDED:
        status_str = "TLE";
        break;
    case JudgeResult::MEMORY_LIMIT_EXCEEDED:
        status_str = "MLE";
        break;
    case JudgeResult::RUNTIME_ERROR:
        status_str = "RE";
        break;
    case JudgeResult::COMPILE_ERROR:
        status_str = "CE";
        break;
    default:
        status_str = "Pending";
        break;
    }
    string escaped_code = db_manager->escape_string(code);
    string escaped_status = db_manager->escape_string(status_str);
    string ins_sql = "INSERT INTO submissions (user_id, problem_id, code, status) VALUES (" +
                     to_string(current_user_id) + ", " + to_string(problem_id) + ", '" +
                     escaped_code + "', '" + escaped_status + "')";
    db_manager->run_sql(ins_sql);

    // ---- 6. 更新用户提交统计 ----
    string upd_sql = "UPDATE users SET submission_count = submission_count + 1";
    if (is_first_ac)
        upd_sql += ", solved_count = solved_count + 1";
    upd_sql += " WHERE id = " + to_string(current_user_id);
    db_manager->run_sql(upd_sql);

    // ---- 7. 显示评测结果 ----
    cout << "\n"
         << GREEN << "========== 评测结果 ==========" << RESET << endl;
    switch (report.result)
    {
    case JudgeResult::ACCEPTED:
        cout << GREEN << "Accepted (AC)" << RESET << endl;
        break;
    case JudgeResult::WRONG_ANSWER:
        cout << RED << "Wrong Answer (WA)" << RESET << endl;
        break;
    case JudgeResult::TIME_LIMIT_EXCEEDED:
        cout << YELLOW << "Time Limit Exceeded (TLE)" << RESET << endl;
        break;
    case JudgeResult::MEMORY_LIMIT_EXCEEDED:
        cout << YELLOW << "Memory Limit Exceeded (MLE)" << RESET << endl;
        break;
    case JudgeResult::RUNTIME_ERROR:
        cout << RED << "Runtime Error (RE)" << RESET << endl;
        break;
    case JudgeResult::COMPILE_ERROR:
        cout << YELLOW << "Compile Error (CE)" << RESET << endl;
        cout << RED << "\n编译错误信息:\n"
             << report.error_message << RESET << endl;
        cout << GREEN << "==============================" << RESET << endl;
        return;
    case JudgeResult::SYSTEM_ERROR:
        cout << RED << "System Error\n详情: " << report.error_message << RESET << endl;
        cout << GREEN << "==============================" << RESET << endl;
        return;
    default:
        cout << "状态: " << report.error_message << endl;
        cout << GREEN << "==============================" << RESET << endl;
        return;
    }

    cout << "通过测试点: " << report.passed_test_cases << " / " << report.total_test_cases << endl;
    cout << "时间: " << report.time_used_ms << " ms  |  内存: " << report.memory_used_mb << " MB" << endl;

    cout << GREEN << "==============================" << RESET << endl;
}

void User::view_my_submissions()
{
    if (!logged_in)
    {
        cout << "请先登录" << endl;
        return;
    }

    string sql =
        "SELECT s.id, p.title, s.status, s.submit_time "
        "FROM submissions s JOIN problems p ON s.problem_id = p.id "
        "WHERE s.user_id = " +
        to_string(current_user_id) +
        " ORDER BY s.submit_time DESC LIMIT 20";
    auto results = db_manager->query(sql);

    if (results.empty())
    {
        cout << "暂无提交记录" << endl;
        return;
    }

    cout << "\n"
         << GREEN
         << "===== 我的提交记录（最近20条） =====" << RESET << endl;
    cout << "――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――" << endl;
    cout << left << setw(6) << "ID"
         << setw(26) << "题目"
         << setw(14) << "状态"
         << setw(20) << "提交时间" << endl;
    cout << "――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――" << endl;

    for (const auto &row : results)
    {
        string status = row.at("status");
        string colored;
        if (status == "AC")
            colored = GREEN + status + RESET;
        else
            colored = RED + status + RESET;

        // 题目标题截断至 22 字符
        string title = row.at("title");
        if (title.length() > 22)
            title = title.substr(0, 19) + "...";

        cout << left << setw(6) << row.at("id")
             << setw(26) << title
             << colored;

        // 手动补齐（ANSI 代码影响 setw）
        int pad = 14 - static_cast<int>(status.length());
        if (pad > 0)
            cout << string(pad, ' ');

        cout << row.at("submit_time") << endl;
    }
    cout << GREEN
         << "======================================================================" << RESET << endl;
}
