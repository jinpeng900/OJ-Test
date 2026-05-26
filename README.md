# OJ 在线判题系统 v2.0 —— C++ 图形化编程教程

> 一个基于 C++17 实现的桌面版在线评测系统，涵盖 ImGui 图形界面、Docker 容器化沙箱、MySQL 数据持久化、AI 辅助调试、容器池调度等工程实践。

---

## Docker 快速启动

> v2.0 推荐使用 Docker Compose 启动：`oj-db` 提供 MySQL，`oj-app` 运行 ImGui 桌面程序，并通过宿主机 Docker Socket 创建评测沙箱容器。

### 第一步：准备 Docker 和图形显示

需要安装：

- Docker Engine 或 Docker Desktop
- Docker Compose v2
- Linux 桌面环境或可用的 X11 显示服务

Linux 桌面环境下，启动前允许本机容器访问 X11：

```bash
xhost +local:docker
```

> Windows/macOS 用户需要额外准备 X Server，或使用支持图形转发的 Linux/WSL2 环境。当前程序是 ImGui 桌面窗口，不是 Web 服务。

### 第二步：配置 AI 密钥（可选）

没有 API Key 也可以运行系统，只是 AI 助手不可用。

```bash
cp ai/.env.example ai/.env 2>/dev/null || touch ai/.env
```

在 `ai/.env` 中写入：

```text
DEEPSEEK_API_KEY=your_api_key_here
```

### 第三步：启动系统

```bash
docker compose up -d oj-db
docker compose run --rm oj-app
```

首次启动会完成：

- 拉起 MySQL 8.0 数据库容器
- 执行 `init.sql` 初始化数据库、表和示例数据
- 构建 `oj-app` 主程序镜像
- 检查并构建 `judge-sandbox:latest` 评测沙箱镜像
- 启动 ImGui 桌面窗口

> 如果已经用旧版 `init.sql` 初始化失败过，请执行 `docker compose down -v` 清空旧数据卷后再重新启动。

### 常用命令

```bash
# 再次进入系统
docker compose run --rm oj-app

# 查看数据库日志
docker compose logs oj-db

# 停止服务
docker compose down

# 停止并清空数据库数据
docker compose down -v

# 修改 C++ 源码后重建应用镜像
docker compose build oj-app
```

### 本地开发构建（可选）

如果不使用 Docker 运行 GUI，也可以在宿主机直接构建：

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j
./oj_app
```

### 测试账号

| 类型 | 账号 | 密码 |
| ---- | ---- | ---- |
| 普通用户 | `test_user` | `123456` |
| 图形界面管理员 | `admin` | `admin123` |
| 管理员数据库用户 | `oj_admin` | `090800` |
| 普通数据库用户 | `oj_user` | `user123` |

---

## 目录

- [OJ 在线判题系统 v2.0 —— C++ 图形化编程教程](#oj-在线判题系统-v20--c-图形化编程教程)
  - [Docker 快速启动](#docker-快速启动)
  - [1. 项目概述](#1-项目概述)
  - [2. 技术栈说明](#2-技术栈说明)
  - [3. 项目架构](#3-项目架构)
  - [4. 核心模块详解](#4-核心模块详解)
  - [5. 关键流程代码详解](#5-关键流程代码详解)
  - [6. 项目特色](#6-项目特色)
  - [7. 使用说明](#7-使用说明)
  - [8. 已知限制](#8-已知限制)

---

## 1. 项目概述

OJ（Online Judge）在线判题系统是一个允许用户提交代码、自动评测并返回结果的平台。本项目使用 **C++17** 从零实现一个桌面版 OJ，适合作为学习 C++ 工程化、数据库封装、Docker 沙箱、GUI 状态机和 AI 集成的参考项目。

v2.0 在 v1.0 的 Docker 评测核心基础上，将终端菜单升级为 **ImGui 图形界面**，用户可以在窗口中完成登录、浏览题目、编辑代码、提交评测、查看结果和询问 AI。

### 核心功能

| 功能模块 | 说明 |
| -------- | ---- |
| 双角色系统 | 管理员与普通用户分入口运行 |
| ImGui 图形界面 | GLFW + OpenGL + Dear ImGui 实现桌面应用 |
| 用户工作区 | 每个用户拥有独立 `workspace/<user_id>/solution.cpp` |
| 题目系统 | 题目列表、题目详情、搜索、知识点展示、已完成绿色方块标记 |
| 管理员题目管理 | 管理员可通过图形化表单添加新题目 |
| Docker 沙箱评测 | 用户代码在隔离容器内编译运行 |
| 容器池调度 | 常驻容器 + 按需临时容器，降低冷启动成本 |
| 完整评测流程 | 编译 → 逐测试点运行 → 输出比对 → 入库 → 图形化展示 |
| AI 辅助调试 | 集成 DeepSeek，携带题目、代码、评测错误上下文 |
| 提交详情 | 可查看提交状态、错误原因说明和提交代码 |
| 数据持久化 | MySQL 保存用户、题目、提交代码、提交记录和统计数据 |

---

## 2. 技术栈说明

### C++17

项目使用 C++17 作为主语言：

- `std::unique_ptr` / `std::make_unique`：管理数据库、视图、AI 客户端等对象生命周期
- `std::shared_ptr`：容器池中共享 `SandboxContainer`
- `enum class`：强类型枚举，如 `JudgeResult`、`ContainerState`、`GUIState`
- PIMPL 模式：`JudgeCore` 隐藏评测实现细节
- STL 容器：`vector`、`map`、`set` 用于界面数据和查询结果

### ImGui + GLFW + OpenGL

v2.0 的主要变化是引入图形界面：

- `main.cpp` 初始化 GLFW、OpenGL 和 ImGui
- `ViewManager` 每帧渲染当前页面
- `UserView` / `AdminView` 负责具体页面
- `AppState` 保存页面共享状态

界面支持中文字体加载，并根据窗口高度动态调整 `UI_SCALE`。

### MySQL C API（libmysqlclient）

项目通过 `mysql/mysql.h` 调用 MySQL C API，并封装为 `DatabaseManager`：

```cpp
class DatabaseManager
{
public:
    DatabaseManager(const std::string &host,
                    const std::string &user,
                    const std::string &password,
                    const std::string &dbname = "");
    ~DatabaseManager();

    bool run_sql(const std::string &sql);
    std::vector<std::map<std::string, std::string>> query(const std::string &sql);
    std::string escape_string(const std::string &s) const;
};
```

### OpenSSL（EVP 接口）

用户密码使用 SHA256 哈希后存入数据库，不明文保存：

```cpp
static string sha256(const string &password)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, password.c_str(), password.length());

    unsigned char hash[32];
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);

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

Docker 在项目中有两层用途：

- Docker Compose 启动 `oj-db` 和 `oj-app`
- `oj-app` 内部通过 Docker CLI 创建 `judge-sandbox:latest` 评测容器

评测引擎不使用 Docker SDK，而是通过 `popen()` 调用 Docker CLI：

```bash
docker run -d --network none --memory=256m --pids-limit=64 \
  --cap-drop=ALL --read-only \
  --tmpfs /sandbox:exec,size=128m,mode=1777 \
  judge-sandbox:latest sleep infinity
```

容器内通过 `g++` 编译，通过 `timeout` 和 `/usr/bin/time` 运行程序并采集资源使用。

> `oj-app` 会挂载 `/var/run/docker.sock`，因此它可以调用宿主机 Docker 来创建评测沙箱。评测沙箱本身仍使用禁网、只读根文件系统、非特权用户等隔离策略。

### DeepSeek API + Python LangChain

AI 功能通过 C++ 调用 `ai/ai_service.py` 实现：

```
C++ AIClient
    │
    └── popen("python ai_service.py ...")
            │
            └── LangChain + DeepSeek API
                    │
                    └── stdout 返回 AI 文本
```

### CMake 构建系统

```cmake
cmake_minimum_required(VERSION 3.10)
project(OJ_Project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL mysqlclient REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
```

---

## 3. 项目架构

### 整体分层结构

```
┌─────────────────────────────────────────────┐
│                  main.cpp                   │  GLFW/OpenGL/ImGui 初始化
└──────────────────────┬──────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────┐
│               ViewManager                  │  GUI 状态机，页面切换
└──────────┬──────────────────────┬───────────┘
           │                      │
           ▼                      ▼
┌──────────────────┐   ┌──────────────────────┐
│    AdminView     │   │      UserView        │  视图层
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
                  │  API    │    │SandboxContainer│
                  └─────────┘    └────────────────┘
```

### 关键架构决策：GUI 状态机

v1.0 主要通过终端菜单函数推进流程。v2.0 改为 GUI 程序后，所有页面必须在每一帧根据状态渲染，因此引入 `GUIState`：

```cpp
enum class GUIState
{
    MAIN_MENU,
    USER_LOGIN,
    USER_REGISTER,
    USER_MENU,
    PROBLEM_LIST,
    PROBLEM_DETAIL,
    SUBMIT_RESULT,
    SUBMISSIONS_LIST,
    SUBMISSION_DETAIL,
    CHANGE_PASSWORD,
    AI_ASSISTANT,
    ADMIN_LOGIN,
    ADMIN_MENU,
    ADMIN_ADD_PROBLEM,
};
```

`AppState` 保存页面之间共享的数据，例如登录输入、题目列表、当前题目、代码缓冲区、评测报告和 AI 聊天历史。

---

## 4. 核心模块详解

### 4.1 ViewManager — GUI 主控制器

**文件**：`include/view_manager.h` / `src/view_manager.cpp`

`ViewManager` 是 v2.0 图形界面的入口控制器。它持有：

- `AppState st`
- `unique_ptr<UserView>`
- `unique_ptr<AdminView>`

每一帧调用 `render()`，根据 `st.state` 分发到对应页面：

```cpp
void ViewManager::Impl::render()
{
    switch (st.state)
    {
    case GUIState::MAIN_MENU:
        draw_main_menu();
        break;
    case GUIState::USER_LOGIN:
        user_view->draw_login(st);
        break;
    case GUIState::PROBLEM_LIST:
        user_view->draw_problem_list(st);
        break;
    case GUIState::ADMIN_MENU:
        admin_view->draw_menu(st);
        break;
    }
}
```

### 4.2 UserView — 用户图形界面

**文件**：`include/user_view.h` / `src/user_view.cpp`

`UserView` 负责普通用户的所有页面：

| 方法 | 功能 |
| ---- | ---- |
| `draw_login()` | 用户登录 |
| `draw_register()` | 用户注册 |
| `draw_menu()` | 用户菜单 |
| `draw_problem_list()` | 题目列表、搜索、AI 题库入口 |
| `draw_problem_detail()` | 题目详情、代码编辑、保存、提交 |
| `draw_submit_result()` | 评测结果 |
| `draw_submissions_list()` | 提交记录列表 |
| `draw_submission_detail()` | 提交详情、状态说明、提交代码 |
| `draw_change_password()` | 修改密码 |
| `draw_ai_assistant()` | AI 聊天助手 |

v2.0 的代码编辑器位于题目详情页：

```cpp
ImGui::InputTextMultiline("##code",
                          st.code_buf,
                          sizeof(st.code_buf),
                          ImVec2(0, 200 * UI_SCALE),
                          ImGuiInputTextFlags_AllowTabInput);
```

提交记录支持点击查看详情。详情页会展示提交 ID、题目、提交时间、状态、状态原因说明和提交代码。当前数据库保存的是提交状态与代码，因此错误原因说明基于 `AC/WA/TLE/MLE/RE/CE/SE` 状态生成；如果要显示逐测试点 diff 或编译错误全文，需要扩展 `submissions` 表保存完整评测报告。

已完成题目在题目列表中使用 ImGui 绘制绿色小方块标记，不依赖字体中的勾号字符，避免缺字时显示成问号。

### 4.3 AdminView — 管理员图形界面

**文件**：`include/admin_view.h` / `src/admin_view.cpp`

管理员界面提供登录、查看题目和添加题目能力。添加题目不再要求直接手写 SQL，而是使用表单输入：

| 字段 | 说明 |
| ---- | ---- |
| 标题 | 题目名称 |
| 知识点 | 分类或标签 |
| 时间限制(ms) | 评测时间限制 |
| 内存限制(MB) | 评测内存限制 |
| 测试数据路径 | 例如 `data/9` |
| 题目描述 | 题面正文 |

### 4.4 DatabaseManager — 数据库封装

**文件**：`include/db_manager.h` / `src/db_manager.cpp`

封装 MySQL 连接、执行 SQL、查询结果和字符串转义：

```cpp
auto rows = db_->query(
    "SELECT id, title, category, time_limit, memory_limit "
    "FROM problems ORDER BY id");
```

`query()` 的返回值是：

```cpp
vector<map<string, string>>
```

每一行用 `map<列名, 值>` 表示，便于 GUI 表格直接读取。

### 4.5 JudgeCore — 评测引擎（PIMPL）

**文件**：`include/judge_core.h` / `src/judge_core.cpp`

`JudgeCore` 是评测主入口：

```cpp
JudgeCore jc;
JudgeConfig cfg{tl, ml, 64, "C++"};
jc.setConfig(cfg);
jc.setSourceCode(code);
jc.setTestDataPath(tp);

JudgeReport report = jc.judge();
```

核心数据结构：

```cpp
struct JudgeReport {
    JudgeResult result;
    int time_used_ms;
    int memory_used_mb;
    std::string error_message;
    int passed_test_cases;
    int total_test_cases;
    std::vector<TestCaseResult> details;
};
```

### 4.6 ContainerPool — Docker 容器池

**文件**：`include/container_pool.h` / `src/container_pool.cpp`

调度策略：

```
请求容器
    │
    ├─ 有空闲常驻容器 → 标记 BUSY，直接返回
    │
    └─ 无空闲常驻容器
            │
            ├─ 未达最大数量 → 创建临时容器
            │
            └─ 已达上限 → 返回 nullptr
```

### 4.7 AIClient — AI 助手集成

**文件**：`include/ai_client.h` / `src/ai_client.cpp`

接口：

```cpp
std::string ask(const std::string &message,
                const std::string &codeContext = "",
                const std::string &problemContext = "");
```

v2.0 中 AI 有两种模式：

- 题目详情页：携带当前代码、题目信息和上次评测错误
- 题目列表页：携带完整题库列表，用于题目推荐

---

## 5. 关键流程代码详解

### 5.1 用户认证流程

用户登录时根据账号查询数据库，使用 SHA256 对比密码哈希：

```cpp
bool User::login(const string &account, const string &password)
{
    string escaped_account = db_manager->escape_string(account);
    string sql = "SELECT id, account, password_hash FROM users WHERE account = '"
                 + escaped_account + "'";
    auto results = db_manager->query(sql);

    if (results.empty())
        return false;

    string stored_hash = results[0]["password_hash"];
    string input_hash = sha256(password);

    if (input_hash != stored_hash)
        return false;

    current_user_id = stoi(results[0]["id"]);
    current_account = results[0]["account"];
    logged_in = true;

    string update_sql = "UPDATE users SET last_login = NOW() WHERE id = "
                        + to_string(current_user_id);
    db_manager->run_sql(update_sql);

    ensure_workspace(current_user_id);
    return true;
}
```

> 安全提示：登录和注册流程已经对账号使用 `escape_string()`，但项目整体仍未全面切换到 prepared statement。后续生产化应统一使用参数化查询。

### 5.2 图形化代码提交与评测流程

```
用户在题目详情页编辑代码
    │
    ▼
点击“保存代码”
    │
    ▼
写入 workspace/<user_id>/solution.cpp
    │
    ▼
点击“提交代码”
    │
    ▼
读取 workspace/<user_id>/solution.cpp
    │
    ▼
JudgeCore.judge()
    │
    ▼
写入 submissions 表
    │
    ▼
更新 users.submission_count / solved_count
    │
    ▼
切换到 SUBMIT_RESULT 页面
```

GUI 提交路径复用 `User::submit_code()`，避免界面层和业务层各自维护一套提交逻辑。这样提交入库、用户统计、最近评测报告和 AI 错误上下文都由同一处维护。

```cpp
user_->submit_code(pid, code, "C++");
st.last_report = user_->getLastReport();
st.state = GUIState::SUBMIT_RESULT;
```

### 5.3 提交记录与详情查看

```
用户菜单
    │
    ▼
查看我的提交
    │
    ▼
提交记录表格：ID / 题目ID / 题目 / 状态 / 时间
    │
    ▼
点击某条提交记录
    │
    ▼
提交详情页：
    - 提交 ID
    - 题目编号和标题
    - 提交时间
    - 状态
    - 错误原因 / 状态说明
    - 提交代码
```

### 5.4 Docker 沙箱容器管理

**启动容器**

```bash
docker run -d \
  --network none \
  --memory=256m \
  --pids-limit=64 \
  --cap-drop=ALL \
  --read-only \
  --tmpfs /sandbox:exec,size=128m,mode=1777 \
  --tmpfs /tmp:exec,size=64m,mode=1777 \
  judge-sandbox:latest sleep infinity
```

**编译**

```bash
docker exec <container_id> sh -c \
  "g++ -O2 -std=c++17 /sandbox/main.cpp -o /sandbox/program 2>&1"
```

**运行**

```bash
docker exec <container_id> sh -c \
  "timeout <time>s /usr/bin/time -f '%e %M' /sandbox/program < /sandbox/input.txt"
```

### 5.5 AI 助手上下文交互

题目详情页中，AI 会收到：

```text
【题目信息】
题号、标题、描述、时间限制、内存限制

【我的代码】
workspace/<user_id>/solution.cpp 当前内容

【上次评测结果】
WA / TLE / MLE / RE / CE 的关键上下文

【问题】
用户输入的问题
```

题目列表页中，AI 会收到：

```text
【题库列表】
题号 | 标题 | 类别 | 描述摘要
```

---

## 6. 项目特色

### 从终端菜单升级到桌面 GUI

v2.0 最大变化是使用 ImGui 替代终端菜单。用户不再通过输入数字操作，而是在窗口中完成登录、选题、写代码、提交和查看结果。

### 评测核心与界面解耦

`JudgeCore` 不依赖 ImGui，界面层只负责收集代码和展示 `JudgeReport`。评测核心仍然可以被 CLI、GUI 或未来服务端复用。

### PIMPL 封装实现细节

`JudgeCore` 对外只暴露配置和 `judge()`，内部的容器池、测试数据读取、输出比对都隐藏在 `Impl` 中。

### 零 SDK 的 Docker 集成

直接调用 Docker CLI，依赖少，便于学习和调试。对于课程项目和原型系统，这种实现方式直观清晰。

### 错误驱动的 AI 联动

评测失败后自动缓存错误上下文，用户询问 AI 时不必手动复制错误信息。

### 最小权限思路

- 数据库：普通用户和管理员使用不同 MySQL 账号
- 容器：禁网、非特权用户、只读根文件系统、cap drop
- 密码：SHA256 哈希存储

---

## 7. 使用说明

### 环境依赖

Docker 方式只需要 Docker、Docker Compose 和可用的 X11 显示服务。下面依赖表主要用于宿主机本地构建。

| 依赖 | 版本要求 | 用途 |
| ---- | -------- | ---- |
| CMake | >= 3.10 | 构建系统 |
| g++ | 支持 C++17 | 编译项目 |
| MySQL Server | >= 5.7 | 数据存储 |
| libmysqlclient-dev | - | MySQL C API |
| libssl-dev | - | OpenSSL SHA256 |
| GLFW3 | - | GUI 窗口 |
| OpenGL | - | GUI 渲染 |
| Docker | >= 20.10 | 评测沙箱 |
| Python3 | >= 3.8 | AI 服务 |

### 首次部署步骤

```bash
# 1. 允许容器访问本机 X11 显示
xhost +local:docker

# 2. 启动数据库
docker compose up -d oj-db

# 3. 启动图形界面 OJ
docker compose run --rm oj-app
```

本地构建方式：

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j
./oj_app
```

### 日常开发

```bash
# Docker 方式
docker compose build oj-app
docker compose run --rm oj-app

# 本地方式
cd build && cmake --build . -j && ./oj_app
```

### 提交代码

v2.0 支持两种方式：

1. 在 GUI 的题目详情页直接编辑代码，点击“保存代码”。
2. 在外部编辑器中修改 `workspace/<user_id>/solution.cpp`，再回到 GUI 点击“提交代码”。

### 查看提交详情

进入“查看我的提交”后，点击提交记录表格中的任意一行，可以查看该次提交的具体信息：

- 提交 ID
- 题目 ID 和标题
- 提交时间
- 状态
- 错误原因 / 状态说明
- 提交代码

状态说明基于提交状态生成：

| 状态 | 含义 |
| ---- | ---- |
| `AC` | 所有测试点通过 |
| `WA` | 输出与标准答案不一致 |
| `TLE` | 超过时间限制 |
| `MLE` | 超过内存限制 |
| `RE` | 运行时异常退出 |
| `CE` | 编译失败 |
| `SE` | 评测环境或系统错误 |

### 管理员添加题目

管理员登录后进入“添加题目”，填写标题、知识点、时间限制、内存限制、测试数据路径和题目描述即可添加题目。测试数据路径需要对应实际目录，例如 `data/9`，并包含 `1.in/1.out` 等测试点文件。

### 题目测试数据格式

```text
data/<problem_id>/
├── 1.in
├── 1.out
├── 2.in
├── 2.out
└── ...
```

`JudgeCore` 会从 `1.in/1.out` 开始顺序读取，直到遇到缺失文件。

---

## 8. 已知限制

| 限制项 | 说明 |
| ------ | ---- |
| 图形显示依赖 | Docker 运行 GUI 依赖 X11，远程服务器或 Windows/macOS 需要额外配置显示服务 |
| 单语言支持 | 当前仅支持 C++，编译命令硬编码为 `g++ -O2 -std=c++17` |
| 同步阻塞评测 | 提交代码后 GUI 会等待评测完成 |
| SQL 安全 | 当前通过 `escape_string()` 降低输入拼接风险，但尚未全面使用 prepared statement |
| 命令执行安全 | Docker 和 Python 调用基于 shell 字符串，需要进一步加固 |
| Docker Socket 权限 | `oj-app` 挂载宿主机 Docker Socket，适合本地实验，不应直接暴露给不可信用户 |
| AI 依赖外网 | DeepSeek API 需要网络和 API Key |
| 远程访问 | 当前是桌面 GUI，不是 Web 或 TCP 多客户端服务 |

### 可优化方向（v2.1+）

- [ ] 全面使用 prepared statement
- [ ] 多语言支持（Python、Java）
- [ ] 提交详情保存逐测试点 diff、编译错误全文和资源使用详情
- [ ] 排行榜
- [ ] 远程服务端或 Web 前端

---

*文档版本: v2.0 | 最后更新: 2026年5月*
