# OJ 在线判题系统 —— C++ 编程教程

> 一个基于 C++17 实现的完整在线评测系统，涵盖 Docker 容器化沙箱、AI 辅助调试、依赖注入架构设计等现代工程实践。

---

## Docker 快速启动

> **唯一前提**：安装 [Docker Desktop](https://www.docker.com/products/docker-desktop/) 或 Docker Engine

### 第一步：获取项目

```bash
git clone <仓库地址>
cd OJ
```

### 第二步：配置 AI 密鑰（可选）

```bash
# 没有 API Key 也可以运行，跳过此步骤，AI 功能不可用，其他功能正常

cp ai/.env.example ai/.env
# 编辑 ai/.env，将 your_api_key_here 替换为真实的 DeepSeek API Key
# 获取地址： https://platform.deepseek.com/api_keys
```

### 第三步：启动系统

```bash
# 启动数据库（后台运行，自动初始化表结构和示例数据）
docker compose up -d oj-db

# 启动 OJ 主程序（交互式终端）
docker compose run --rm oj-app
```

**首次启动将自动完成：**
- 构建 C++ 应用镜像（编译源码）
- 初始化 MySQL 数据库（建表、创建用户、插入 8 道示例题目）
- 构建评测沙箱 Docker 镜像
- 启动交互式终端

### 常用命令

```bash
# 再次进入系统
docker compose run --rm oj-app

# 查看数据库日志
docker compose logs oj-db

# 停止所有服务
docker compose down

# 停止并删除数据库数据（清空重置）
docker compose down -v

# 重新构建应用镜像（修改源码后）
docker compose build oj-app
```

### 修改代码再提交

```bash
# 1. 将代码写入宿主机 workspace/solution.cpp
vim workspace/solution.cpp

# 2. 启动 OJ 系统，工作区已通过 volume 挂载，座位共享
docker compose run --rm oj-app
```

> **注意事项**：`oj-app` 容器需要 `--privileged` 权限以在容器内调用 Docker（评测沙箱所需）。这是此类应用的常用设计，不影响评测沙箱本身的安全隔离。

---

## 目录

- [OJ 在线判题系统 —— C++ 编程教程](#oj-在线判题系统--c-编程教程)
  - [Docker 快速启动](#docker-快速启动)
    - [第一步：获取项目](#第一步获取项目)
    - [第二步：配置 AI 密鑰（可选）](#第二步配置-ai-密鑰可选)
    - [第三步：启动系统](#第三步启动系统)
    - [常用命令](#常用命令)
    - [修改代码再提交](#修改代码再提交)
  - [目录](#目录)
  - [1. 项目概述](#1-项目概述)
    - [核心功能](#核心功能)
  - [2. 技术栈说明](#2-技术栈说明)
    - [C++17](#c17)
    - [MySQL C API（libmysqlclient）](#mysql-c-apilibmysqlclient)
    - [OpenSSL（EVP 接口）](#opensslevp-接口)
    - [Docker](#docker)
    - [DeepSeek API + Python LangChain](#deepseek-api--python-langchain)
    - [CMake 构建系统](#cmake-构建系统)
  - [3. 项目架构](#3-项目架构)
    - [整体分层结构](#整体分层结构)
    - [关键架构决策：依赖注入](#关键架构决策依赖注入)
  - [4. 核心模块详解](#4-核心模块详解)
    - [4.1 DatabaseManager — 数据库封装](#41-databasemanager--数据库封装)
    - [4.2 AppContext — 依赖注入工厂](#42-appcontext--依赖注入工厂)
    - [4.3 JudgeCore — 评测引擎（PIMPL）](#43-judgecore--评测引擎pimpl)
      - [PIMPL 模式的使用](#pimpl-模式的使用)
      - [核心数据结构](#核心数据结构)
      - [评测结果枚举](#评测结果枚举)
    - [4.4 ContainerPool — Docker 容器池](#44-containerpool--docker-容器池)
      - [调度策略](#调度策略)
    - [4.5 AIClient — AI 助手集成](#45-aiclient--ai-助手集成)
      - [接口设计](#接口设计)
      - [两阶段题目推荐机制](#两阶段题目推荐机制)
  - [5. 关键流程代码详解](#5-关键流程代码详解)
    - [5.1 用户认证流程](#51-用户认证流程)
    - [5.2 代码提交与评测流程](#52-代码提交与评测流程)
    - [5.3 Docker 沙箱容器管理](#53-docker-沙箱容器管理)
    - [5.4 AI 助手上下文交互](#54-ai-助手上下文交互)
  - [6. 项目特色](#6-项目特色)
    - [依赖注入与关注点分离](#依赖注入与关注点分离)
    - [PIMPL 封装实现细节](#pimpl-封装实现细节)
    - [零 SDK 的 Docker 集成](#零-sdk-的-docker-集成)
    - [错误驱动的 AI 联动](#错误驱动的-ai-联动)
    - [最小权限原则](#最小权限原则)
  - [7. 使用说明](#7-使用说明)
    - [环境依赖](#环境依赖)
    - [首次部署步骤](#首次部署步骤)
    - [日常开发](#日常开发)
    - [测试账号](#测试账号)
    - [提交代码](#提交代码)
  - [8. 已知限制](#8-已知限制)
    - [可优化方向（v1.1+）](#可优化方向v11)

---

## 1. 项目概述

OJ（Online Judge）在线判题系统是一个允许用户提交代码、自动评测并返回结果的平台。本项目是一个从零开始用 **C++17** 实现的完整 OJ 系统，适合作为学习现代 C++ 工程实践的参考项目。

### 核心功能

| 功能模块        | 说明                                                               |
| --------------- | ------------------------------------------------------------------ |
| 双角色系统      | 管理员（题目管理、用户管理）与普通用户（做题、提交）分权限独立运行 |
| Docker 沙箱评测 | 每次评测在隔离的 Docker 容器内执行，防止恶意代码危害宿主机         |
| 容器池调度      | 预热常驻容器 + 按需临时容器，复用容器降低冷启动延迟                |
| AI 辅助调试     | 集成 DeepSeek 大模型，自动携带题目和错误信息帮助用户分析代码       |
| 完整评测流程    | 编译 → 逐测试点运行 → 输出比对 → 入库 → 结果展示                   |
| 安全多层隔离    | 禁网、只读文件系统、非特权用户、进程数限制、内存限制               |

---

## 2. 技术栈说明

### C++17

项目全面使用 C++17 特性：

- **`std::filesystem`**：检查测试数据路径是否存在
- **`std::unique_ptr` / `std::make_unique`**：RAII 自动管理对象生命周期，杜绝内存泄漏
- **`std::shared_ptr`**：容器池中多处共享同一容器对象
- **`enum class`**：强类型枚举，避免隐式整数转换（`JudgeResult`、`ContainerState`）
- **PIMPL 模式**：`JudgeCore` 使用 `class Impl` + `unique_ptr<Impl>` 彻底隐藏实现细节

### MySQL C API（libmysqlclient）

通过 `mysql/mysql.h` 直接调用 MySQL 原生 C 接口，封装为 `DatabaseManager` 类。相比 ORM，直接使用 C API 能更清晰地理解数据库连接、查询、转义等底层操作。

### OpenSSL（EVP 接口）

用户密码使用 **SHA256** 哈希后存入数据库，绝不明文存储。使用 OpenSSL 的现代 `EVP_DigestInit_ex` 接口（而非已废弃的 `SHA256()` 直接调用）：

```cpp
// src/user.cpp — SHA256 哈希实现
static string sha256(const string &password)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, password.c_str(), password.length());

    unsigned char hash[32];
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);  // RAII 手动管理（C API 无法使用 unique_ptr）

    string result;
    for (unsigned int i = 0; i < len; i++) {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        result += buf;
    }
    return result;
}
```

### Docker

评测引擎不依赖任何 Docker SDK，而是直接通过 `popen()` 调用宿主机的 `docker` 命令行工具。这种方式零依赖、易理解：

```bash
# 启动常驻容器
docker run -d --network none --memory=256m --pids-limit=64 \
  --cap-drop=ALL --read-only --tmpfs /sandbox:mode=1777 \
  judge-sandbox:latest sleep infinity

# 在容器内编译代码
docker exec <container_id> g++ -O2 -std=c++17 /sandbox/main.cpp -o /sandbox/main

# 在容器内运行程序（带超时）
echo "<input>" | docker exec -i <container_id> timeout <limit>s /sandbox/main
```

### DeepSeek API + Python LangChain

AI 功能通过 C++ 启动子进程调用 `ai/ai_service.py`，Python 脚本使用 LangChain 框架与 DeepSeek API 通信。C++/Python 之间通过**命令行参数传参、标准输出传回结果**，无需复杂的进程间通信。

### CMake 构建系统

```cmake
# CMakeLists.txt 核心配置
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # 生成 compile_commands.json，支持 clangd

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL mysqlclient REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB SOURCES "src/*.cpp")  # 自动收集所有源文件
add_executable(oj_app ${SOURCES})
target_link_libraries(oj_app PRIVATE ${MYSQL_LIBRARIES} OpenSSL::Crypto)
```

---

## 3. 项目架构

### 整体分层结构

```
┌─────────────────────────────────────────────┐
│                  main.cpp                   │  程序入口，创建 ViewManager
└──────────────────────┬──────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────┐
│               ViewManager                  │  主控制器，角色切换
└──────────┬──────────────────────┬───────────┘
           │                      │
           ▼                      ▼
┌──────────────────┐   ┌──────────────────────┐
│    AdminView     │   │      UserView        │  视图层（输入/输出/菜单）
└────────┬─────────┘   └──────────┬───────────┘
         │                        │
         ▼                        ▼
┌──────────────────┐   ┌──────────────────────┐
│     Admin        │   │       User           │  业务逻辑层
└────────┬─────────┘   └──┬───────┬───────────┘
         │                │       │
         ▼                ▼       ▼
  ┌─────────────┐  ┌──────────┐  ┌──────────────┐
  │  Database   │  │ AIClient │  │  JudgeCore   │  基础设施层
  │  Manager   │  └────┬─────┘  └──────┬───────┘
  └─────────────┘      │               │
                  ┌────┴────┐    ┌─────┴──────────┐
                  │DeepSeek │    │ ContainerPool  │
                  │  API    │    │ SandboxContainer│
                  └─────────┘    └────────────────┘
```

### 关键架构决策：依赖注入

**问题**：最初视图层在构造函数中直接 `new DatabaseManager()`，导致数据库连接参数散落在多处，视图层承担了不该有的职责。

**解决方案**：引入 `AppContext` 作为数据库连接工厂，`ViewManager` 负责创建连接并通过**构造函数注入**传给视图层：

```
反模式（改造前）：
  UserView::UserView() {
      db = new DatabaseManager("localhost", "oj_user", "...", "OJ");  // 视图层直接管理连接
  }

正确做法（改造后）：
  ViewManager::start_login_menu() {
      user_view = make_unique<UserView>(AppContext::createUserDB());  // 工厂创建，注入视图
  }
```

这样，视图层只关心「如何展示」，连接参数的维护收敛到 `AppContext` 一处。

---

## 4. 核心模块详解

### 4.1 DatabaseManager — 数据库封装

**文件**：`include/db_manager.h` / `src/db_manager.cpp`

`DatabaseManager` 封装了 MySQL C API 的三个核心操作：

```cpp
class DatabaseManager
{
public:
    // 构造时即建立连接，析构时自动释放
    DatabaseManager(const string &host, const string &user,
                    const string &password, const string &dbname = "");
    ~DatabaseManager();

    // 执行不返回结果集的 SQL（INSERT / UPDATE / DELETE）
    bool run_sql(const string &sql);

    // 执行查询，返回 vector<map<列名, 值>>
    vector<map<string, string>> query(const string &sql);

    // MySQL 字符串转义，防止 SQL 注入
    string escape_string(const string &s) const;

private:
    MYSQL *conn;  // MySQL 连接句柄
};
```

**防注入示例**：所有用户输入在拼入 SQL 前必须经过转义：

```cpp
// src/user.cpp — 登录时的安全处理
string escaped_account = db_manager->escape_string(account);
string sql = "SELECT id, password_hash FROM users WHERE account = '" 
             + escaped_account + "'";
```

**查询结果的使用**：`query()` 返回 `vector<map<string,string>>`，每行是一个字段名到字符串值的映射：

```cpp
auto results = db_manager->query(sql);
if (!results.empty()) {
    string id   = results[0]["id"];
    string name = results[0]["account"];
}
```

---

### 4.2 AppContext — 依赖注入工厂

**文件**：`include/app_context.h` / `src/app_context.cpp`

`AppContext` 是一个纯静态工厂类（构造函数 `= delete`，不可实例化），统一管理数据库连接参数：

```cpp
// include/app_context.h
class AppContext
{
public:
    static unique_ptr<DatabaseManager> createAdminDB();  // 全权限连接
    static unique_ptr<DatabaseManager> createUserDB();   // 受限连接
private:
    AppContext() = delete;  // 禁止实例化
};
```

```cpp
// src/app_context.cpp
unique_ptr<DatabaseManager> AppContext::createAdminDB()
{
    return make_unique<DatabaseManager>("localhost", "oj_admin", "090800", "OJ");
}

unique_ptr<DatabaseManager> AppContext::createUserDB()
{
    return make_unique<DatabaseManager>("localhost", "oj_user", "user123", "OJ");
}
```

**调用方（ViewManager）**：每次进入角色时创建连接，退出时自动销毁（`unique_ptr` 析构），无连接泄漏：

```cpp
// src/view_manager.cpp
case 1:
    admin_view = make_unique<AdminView>(AppContext::createAdminDB());
    admin_view->start();
    admin_view.reset();  // 退出管理员模式，连接立即释放
    break;
case 2:
    user_view = make_unique<UserView>(AppContext::createUserDB());
    user_view->start();
    user_view.reset();
    break;
```

> **设计要点**：`oj_admin` 与 `oj_user` 是 MySQL 中权限不同的两个账号。管理员账号拥有对 `problems`/`users` 表的写权限；用户账号仅能查询题目、写入自己的提交记录。权限隔离在数据库层强制执行。

---

### 4.3 JudgeCore — 评测引擎（PIMPL）

**文件**：`include/judge_core.h` / `src/judge_core.cpp`

#### PIMPL 模式的使用

PIMPL（Pointer to IMPLementation）是 C++ 中隐藏实现细节的经典手法：头文件只暴露公共接口，所有实现数据和私有方法藏在 `Impl` 类中，外部代码的编译不依赖实现细节。

```cpp
// include/judge_core.h — 头文件只有公共接口，无实现细节
class JudgeCore
{
public:
    JudgeCore();
    ~JudgeCore();
    void setConfig(const JudgeConfig &config);
    void setSourceCode(const string &source_code);
    void setTestDataPath(const string &test_data_path);
    JudgeReport judge();
private:
    class Impl;                 // 前向声明，不暴露 Impl 的成员
    unique_ptr<Impl> impl_;     // 通过指针持有实现
    JudgeCore(const JudgeCore &) = delete;  // 禁止拷贝
};
```

```cpp
// src/judge_core.cpp — Impl 的定义只在 .cpp 中
class JudgeCore::Impl
{
public:
    JudgeConfig config_;
    string source_code_;
    string test_data_path_;
    ContainerPool pool_;  // 容器池，外部完全不可见
    // ... 所有私有方法
};
```

#### 核心数据结构

```cpp
// 评测配置
struct JudgeConfig {
    int time_limit_ms;    // 时间限制（毫秒）
    int memory_limit_mb;  // 内存限制（MB）
};

// 单个测试点结果
struct TestCaseResult {
    int case_id;
    JudgeResult result;
    int time_ms;
    int memory_mb;
    string output_diff;  // WA 时存储期望输出与实际输出的差异
};

// 完整评测报告
struct JudgeReport {
    JudgeResult result;              // 总体结果（AC/WA/TLE/...）
    int time_used_ms;
    int memory_used_mb;
    string error_message;            // CE/RE 时的错误信息
    int passed_test_cases;
    int total_test_cases;
    vector<TestCaseResult> details;  // 每个测试点详情
};
```

#### 评测结果枚举

```cpp
enum class JudgeResult {
    COMPILING,             // 编译中
    ACCEPTED,              // 通过 (AC)
    WRONG_ANSWER,          // 答案错误 (WA)
    TIME_LIMIT_EXCEEDED,   // 超时 (TLE)
    MEMORY_LIMIT_EXCEEDED, // 超内存 (MLE)
    RUNTIME_ERROR,         // 运行时错误 (RE)
    COMPILE_ERROR,         // 编译错误 (CE)
    SYSTEM_ERROR           // 系统错误 (SE)
};
```

---

### 4.4 ContainerPool — Docker 容器池

**文件**：`include/container_pool.h` / `src/container_pool.cpp`

容器池解决了 Docker 容器冷启动慢（约 1-2 秒）的问题，通过预热常驻容器实现评测快速响应。

#### 调度策略

```
请求容器
    │
    ├─ 有空闲常驻容器 → 标记 BUSY，直接返回（零延迟）
    │
    └─ 无空闲常驻容器
            │
            ├─ 当前总容器数 < max_size → 创建临时容器（约 1s 延迟）
            │
            └─ 已达上限 → 返回 nullptr（评测失败，需排队）
```

```cpp
class ContainerPool
{
public:
    // 预创建 min_size 个常驻容器
    bool initialize(int min_size = 1, int max_size = 4,
                    const string &image = "judge-sandbox:latest");

    // 获取可用容器；is_temporary=true 表示这是临时容器
    shared_ptr<SandboxContainer> acquire(bool &is_temporary);

    // 归还常驻容器：reset() 清理文件后置回 IDLE
    void release(shared_ptr<SandboxContainer> container);

    // 销毁临时容器
    void destroyTemporary(shared_ptr<SandboxContainer> container);

private:
    vector<shared_ptr<SandboxContainer>> resident_;  // 常驻容器列表
    mutable mutex mutex_;  // 保护并发访问
    int max_size_ = 4;
    int temporary_active_ = 0;
};
```

> **惰性初始化**：容器池在第一次调用 `judge()` 时才启动，避免程序启动时等待 Docker。

---

### 4.5 AIClient — AI 助手集成

**文件**：`include/ai_client.h` / `src/ai_client.cpp`

AI 功能通过 **C++ 启动 Python 子进程** 实现，无需在 C++ 中引入任何 HTTP 库：

```
C++ AIClient::ask()
    │
    └─ popen("python ai_service.py --session xxx --message \"...\" --code \"...\"", "r")
           │
           └─ Python LangChain → DeepSeek API → 返回文本
                    │
                    └─ C++ 读取 stdout → 返回给调用方
```

#### 接口设计

```cpp
class AIClient
{
public:
    // 注入数据库管理器，供 AI 查询题目信息
    void setDatabaseManager(DatabaseManager *db);

    // 基础问答接口
    string ask(const string &message,
               const string &codeContext = "",
               const string &problemContext = "");

    // 自动查询题目上下文的高层接口
    string askWithProblemContext(const string &message,
                                 const string &codeContext,
                                 int problemId,
                                 const string &errorContext = "");

private:
    DatabaseManager *db_manager_ = nullptr;
    string queryProblemInfo(int problemId);  // 查单题详情
    string queryProblemList();               // 查全部题库（按需）
};
```

#### 两阶段题目推荐机制

AI 不在每次请求都加载全部题库（浪费 token），而是在需要时才按需拉取：

```cpp
string AIClient::askWithProblemContext(...)
{
    // 第一阶段：携带当前题目信息调用 AI
    string problemContext = queryProblemInfo(problemId);
    string response = ask(message, codeContext, problemContext);

    // AI 返回 [NEED_PROBLEMS] 信号表示需要题库列表
    if (response.find("[NEED_PROBLEMS]") != string::npos) {
        // 第二阶段：补充题库列表后重新调用
        string contextWithList = problemContext + "\n\n" + queryProblemList();
        response = ask(message, codeContext, contextWithList);
    }
    return response;
}
```

---

## 5. 关键流程代码详解

### 5.1 用户认证流程

登录和注册都通过 SHA256 哈希保护密码：

```cpp
// src/user.cpp — 登录验证
bool User::login(const string &account, const string &password)
{
    // 1. 转义输入，防 SQL 注入
    string escaped_account = db_manager->escape_string(account);
    string sql = "SELECT id, account, password_hash FROM users "
                 "WHERE account = '" + escaped_account + "'";
    auto results = db_manager->query(sql);

    if (results.empty()) {
        cout << "账号不存在" << endl;
        return false;
    }

    // 2. 对比哈希，而非明文密码
    string stored_hash = results[0]["password_hash"];
    string input_hash  = sha256(password);
    if (input_hash != stored_hash) {
        cout << "密码错误" << endl;
        return false;
    }

    // 3. 登录成功，记录状态并更新最后登录时间
    current_user_id = stoi(results[0]["id"]);
    current_account = results[0]["account"];
    logged_in = true;
    db_manager->run_sql("UPDATE users SET last_login = NOW() WHERE id = "
                        + to_string(current_user_id));
    return true;
}
```

---

### 5.2 代码提交与评测流程

`User::submit_code()` 是系统最核心的方法，完整流程如下：

```cpp
void User::submit_code(int problem_id, const string &code)
{
    // ---- 1. 从数据库获取题目配置 ----
    string prob_sql = "SELECT time_limit, memory_limit, test_data_path "
                      "FROM problems WHERE id = " + to_string(problem_id);
    auto prob = db_manager->query(prob_sql);

    int time_limit_ms   = stoi(prob[0]["time_limit"]);
    int memory_limit_mb = stoi(prob[0]["memory_limit"]);
    string test_data_path = prob[0]["test_data_path"];

    // ---- 2. 配置并调用评测引擎 ----
    JudgeCore judge_engine;
    JudgeConfig config;
    config.time_limit_ms   = time_limit_ms;
    config.memory_limit_mb = memory_limit_mb;
    judge_engine.setConfig(config);
    judge_engine.setSourceCode(code);
    judge_engine.setTestDataPath(test_data_path);

    JudgeReport report = judge_engine.judge();  // 阻塞，直到评测完成

    // ---- 3. 构建错误上下文，供 AI 助手使用 ----
    if (report.result != JudgeResult::ACCEPTED) {
        last_error_context_ = buildErrorContext(report);  // 记录 WA/TLE/CE 详情
    }

    // ---- 4. 判断是否首次 AC ----
    bool is_first_ac = false;
    if (report.result == JudgeResult::ACCEPTED) {
        string chk = "SELECT 1 FROM submissions WHERE user_id = "
                     + to_string(current_user_id) + " AND problem_id = "
                     + to_string(problem_id) + " AND status = 'AC' LIMIT 1";
        is_first_ac = db_manager->query(chk).empty();
    }

    // ---- 5. 写入 submissions 表 ----
    string escaped_code = db_manager->escape_string(code);
    string insert_sql = "INSERT INTO submissions (user_id, problem_id, code, status) "
                        "VALUES (" + to_string(current_user_id) + ", "
                        + to_string(problem_id) + ", '" + escaped_code
                        + "', '" + status_str + "')";
    db_manager->run_sql(insert_sql);

    // ---- 6. 更新用户统计 ----
    db_manager->run_sql("UPDATE users SET submission_count = submission_count + 1 "
                        "WHERE id = " + to_string(current_user_id));
    if (is_first_ac) {
        db_manager->run_sql("UPDATE users SET solved_count = solved_count + 1 "
                            "WHERE id = " + to_string(current_user_id));
    }
}
```

---

### 5.3 Docker 沙箱容器管理

`SandboxContainer` 用 `popen()` 调用 docker 命令行完成全部容器操作，核心方法流程：

**启动容器（常驻模式）**
```bash
# start() 执行的命令
docker run -d \
  --network none          # 禁止网络访问
  --memory=256m           # 内存上限 256MB
  --pids-limit=64         # 进程数限制，防 fork 炸弹
  --cap-drop=ALL          # 移除所有 Linux capabilities
  --read-only             # 容器根文件系统只读
  --tmpfs /sandbox:mode=1777  # 仅 /sandbox 可写（内存文件系统）
  judge-sandbox:latest \
  sleep infinity          # 常驻模式：保持存活等待 exec
```

**写入源代码（stdin 管道）**
```bash
# writeSourceCode() 执行的命令
# 通过 docker exec -i 将代码通过 stdin 管道写入，避免临时文件权限问题
echo "<源代码内容>" | docker exec -i <container_id> \
  sh -c 'cat > /sandbox/main.cpp'
```

**编译（在容器内）**
```bash
# compile() 执行的命令
docker exec <container_id> \
  g++ -O2 -std=c++17 -o /sandbox/main /sandbox/main.cpp
```

**运行并计时（逐测试点）**
```bash
# run() 执行的命令
echo "<测试输入>" | docker exec -i <container_id> \
  /usr/bin/time -v timeout <时间限制>s /sandbox/main
# 通过解析 /usr/bin/time -v 的输出获取实际内存使用量
```

**Dockerfile 沙箱定义**
```dockerfile
FROM ubuntu:22.04
RUN apt-get install -y g++ gcc make time

# 创建非特权用户
RUN useradd -m -s /bin/bash runner
WORKDIR /sandbox
USER runner  # 评测代码在非特权用户下运行

CMD ["sleep", "infinity"]  # 常驻模式
```

---

### 5.4 AI 助手上下文交互

评测失败后，错误上下文会自动随用户的提问一起发送给 AI：

```cpp
// src/user_view.cpp — AI 助手调用处
string code  = read_file("workspace/solution.cpp");  // 读取当前代码
string error = user_obj->getLastErrorContext();       // 获取上次评测错误

// askWithProblemContext 会自动：
// 1. 查询题目信息（标题、描述、时限）
// 2. 附加评测错误上下文
// 3. 按需拉取题库列表（AI 返回 [NEED_PROBLEMS] 时）
string response = ai_client->askWithProblemContext(
    question, code, problem_id, error
);
```

AI 收到的完整上下文示例：
```
【题目信息】
题号: 1
标题: A+B Problem
描述: 给定两个整数 A 和 B，输出 A+B 的值
时间限制: 1000 ms
内存限制: 128 MB

【上次评测结果】
状态: 答案错误 (WA)
通过: 3/5 测试点
第 4 个测试点:
期望: 5
实际: 4

【用户代码】
#include <iostream>
...

【用户问题】
我的代码哪里错了？
```

---

## 6. 项目特色

### 依赖注入与关注点分离

项目通过 `AppContext` 工厂 + 构造函数注入，实现视图层与数据库层的彻底解耦。每一层只做自己的事：视图层处理输入输出，业务层处理逻辑，基础设施层处理 DB/Docker/AI。

### PIMPL 封装实现细节

`JudgeCore` 对外只暴露三个 setter 和一个 `judge()`，内部容器池、Docker 调用、文件 IO 全部隐藏在 `Impl` 类中。修改评测实现不会引起头文件变化，不会导致级联重编译。

### 零 SDK 的 Docker 集成

不引入任何 Docker REST SDK，直接用 `popen()` 调用 CLI，实现简单、依赖少、易于理解和调试。

### 错误驱动的 AI 联动

评测失败时自动缓存结构化错误信息（`last_error_context_`），用户进入 AI 助手时无需手动描述错误，AI 自动获得完整上下文，大幅提升辅助调试的准确性。

### 最小权限原则

- 数据库层：管理员用 `oj_admin`，用户用 `oj_user`，MySQL 层强制权限
- 容器层：`--cap-drop=ALL`、非特权用户 `runner`、只读文件系统
- 密码层：SHA256 哈希存储，绝不明文

---

## 7. 使用说明

### 环境依赖

| 依赖               | 版本要求          | 用途                   |
| ------------------ | ----------------- | ---------------------- |
| CMake              | >= 3.10           | 构建系统               |
| g++                | >= 9 (支持 C++17) | 编译项目               |
| MySQL Server       | >= 5.7            | 数据存储               |
| libmysqlclient-dev | —                 | MySQL C API 头文件和库 |
| libssl-dev         | —                 | OpenSSL SHA256         |
| Docker             | >= 20.10          | 评测沙箱               |
| Python3            | >= 3.8            | AI 服务                |

### 首次部署步骤

**第一步：初始化数据库**

```bash
mysql -u root -p < init.sql
```

`init.sql` 会创建 `OJ` 数据库、`users`/`problems`/`submissions` 三张表、`oj_admin` 和 `oj_user` 两个 MySQL 账号，并插入 8 道示例题目。

**第二步：构建 Docker 评测镜像（一次性）**

```bash
cd judge-sandbox
docker build -t judge-sandbox:latest .
```

**第三步：配置并初始化 AI 环境**

```bash
cd ai
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

编辑 `ai/.env`，填入 DeepSeek API Key：

```
DEEPSEEK_API_KEY=your_api_key_here
```

**第四步：编译项目**

```bash
mkdir -p build && cd build
cmake ..
make -j4
```

**第五步：运行**

```bash
# 在 build 目录下运行（相对路径依赖此位置）
./oj_app
```

### 日常开发

```bash
# 重新编译（在 build 目录）
make -j4

# 运行
./oj_app
```

### 测试账号

- 管理员 MySQL 用户：`oj_admin` / `090800`
- 平台用户：自行注册，或使用 `init.sql` 中的示例用户

### 提交代码

将代码写入 `workspace/solution.cpp`，进入用户模式选择题目后点击「提交代码」，系统会自动读取该文件进行评测。

---

## 8. 已知限制

| 限制项             | 说明                                                                       |
| ------------------ | -------------------------------------------------------------------------- |
| Docker 依赖        | 宿主机必须安装并运行 Docker，容器池初始化失败会导致评测不可用              |
| 单语言支持         | 当前仅支持 C++ 评测（编译命令硬编码为 `g++ -O2 -std=c++17`）               |
| 同步阻塞评测       | 提交代码后主线程阻塞等待，不支持并发多用户同时评测                         |
| 连接参数硬编码     | 数据库密码写在 `app_context.cpp` 中，生产环境应改为读取环境变量或配置文件  |
| Ctrl+C 容器残留    | 强制退出程序可能导致 Docker 容器未被销毁，需手动 `docker rm -f` 清理       |
| 测试数据路径绝对化 | `problems.test_data_path` 存储的是本机绝对路径，迁移部署时需更新数据库记录 |
| AI 依赖外网        | DeepSeek API 需要网络连接，断网环境下 AI 功能不可用（不影响评测）          |

### 可优化方向（v1.1+）

- [ ] Seccomp 系统调用白名单，进一步限制容器内权限
- [ ] 多语言支持（Python、Java）
- [ ] 异步评测队列，支持并发提交
- [ ] 数据库连接池，避免每次进入角色都重新建立连接
- [ ] 配置文件化，将连接参数、路径等移出代码
- [ ] 排行榜功能

---

*文档版本: v1.0 | 最后更新: 2026年4月*
