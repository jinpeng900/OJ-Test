# OJ 在线判题系统 v1.1

## 版本概述

**版本号**: v1.1
**发布日期**: 2026年4月
**技术栈**: C++17 / MySQL / CMake 3.10+ / OpenSSL / Python3 / LangChain / Docker

v1.1 是在 v1.0 基础上的架构优化与代码质量提升版本。核心改进包括：引入 AppContext 依赖注入模式实现视图层与数据库层解耦、AI 数据库查询逻辑封装迁移、全项目代码风格统一、静态代码分析清理冗余元素、以及完整的 Docker 一体化部署方案。

---

## 版本变更日志

### 新增功能

| 功能模块     | 具体功能                | 说明                                                           |
| ------------ | ----------------------- | -------------------------------------------------------------- |
| **架构设计** | AppContext 应用上下文   | 数据库连接工厂模式，统一管理连接参数                           |
| **架构设计** | 依赖注入                | 视图层通过构造函数接收 DatabaseManager，不直接创建连接         |
| **AI 集成**  | AIClient 数据库查询封装 | queryProblemInfo()、queryProblemList() 从视图层迁移至 AIClient |
| **AI 集成**  | askWithProblemContext() | 自动查询题目信息并调用 AI 的高层接口                           |
| **部署运维** | Docker 一体化部署       | docker-compose.yml + Dockerfile.app 多阶段构建                 |
| **部署运维** | 自动启动脚本            | docker-entrypoint.sh 自动检测数据库/沙箱镜像/AI 配置           |
| **部署运维** | AI 配置模板             | ai/.env.example 提供 API Key 配置示例                          |
| **项目文档** | 详细编程教程 README     | 800+ 行，涵盖架构、模块、代码示例、部署指南                    |
| **项目文档** | v2.0 架构设计文档       | docs/async_concurrent_judge_architecture.md                    |

### 改进优化

| 改进项            | v1.0                            | v1.1                           |
| ----------------- | ------------------------------- | ------------------------------ |
| 数据库连接管理    | 视图层直接创建连接              | AppContext 工厂 + 构造函数注入 |
| AI 助手数据库访问 | UserView 中直接写 SQL           | 封装进 AIClient，视图层无 SQL  |
| 命名空间风格      | std:: 前缀与 using 混用         | 全项目统一 using namespace std |
| 冗余代码          | 存在未使用函数/变量/头文件      | 静态分析后全面清理             |
| 项目部署          | 需手动配置数据库/C++环境/Python | Docker compose 一键启动        |
| 项目文档          | 仅 2 行 README                  | 完整编程教程 + 架构设计文档    |
| 版本控制          | build/ .vscode/ 可能被误提交    | .gitignore 完善排除规则        |

### 代码清理清单

| 删除项                         | 位置                       | 原因                    |
| ------------------------------ | -------------------------- | ----------------------- |
| `User::getLastReport()`        | `include/user.h:64`        | 无任何调用方            |
| `User::last_report_`           | `include/user.h:80`        | 只写不读                |
| `JudgeConfig::output_limit_mb` | `include/judge_core.h:29`  | 定义未使用              |
| `JudgeConfig::language`        | `include/judge_core.h:30`  | 定义未使用              |
| `submit_code() language 参数`  | `include/user.h:55`        | 仅赋值给未使用字段      |
| `#include <cstring>`           | `src/user.cpp:6`           | 无任何 C 字符串函数调用 |
| `Color::BLUE`                  | `include/color_codes.h:11` | 全项目无使用            |
| `Color::MAGENTA`               | `include/color_codes.h:12` | 全项目无使用            |
| `Color::WHITE`                 | `include/color_codes.h:14` | 全项目无使用            |

---

## 系统架构

### 整体架构（v1.1 改进版）

```
┌───────────────────────────────────────────────────────────────────┐
│                          main.cpp                                 │
│                       (程序入口点)                                 │
└──────────────────────────┬────────────────────────────────────────┘
                           │
                           ▼
┌───────────────────────────────────────────────────────────────────┐
│                        ViewManager                                │
│                  (主控制器 - 模式切换)                              │
└──────────┬──────────────────────┬───────────────────────────────────┘
           │                      │
           ▼                      ▼
┌──────────────────────┐    ┌──────────────────────┐
│     AdminView        │    │      UserView        │
│    (管理员界面)       │    │     (用户界面)        │
└────────┬─────────────┘    └──────────┬───────────┘
         │                              │
         │     ┌────────────────────────┤
         │     ▼                        ▼
         │  ┌──────────┐        ┌────────────┐
         │  │  Admin   │        │    User    │
         │  └──────────┘        └──────┬─────┘
         │                             │
         │     ┌───────────────────────┼────────────────────┐
         │     ▼                       ▼                    ▼
         │  ┌──────────┐     ┌────────────┐      ┌────────────┐
         │  │Database  │     │  AIClient  │      │ JudgeCore  │
         │  │Manager   │     │            │      │  (PIMPL)   │
         │  └──────────┘     └──────┬─────┘      └──────┬─────┘
         │                          │                   │
         │                ┌─────────┴──────┐    ┌────────┴──────────┐
         │                ▼                ▼    ▼                   ▼
         │          ┌──────────┐    ┌────────┐ ┌──────────────┐ ┌────────┐
         │          │ai_service│    │DeepSeek│ │ContainerPool │ │  data/ │
         │          │  .py     │    │  API   │ │  常驻+临时    │ │测试数据│
         │          └──────────┘    └────────┘ └──────┬───────┘ └────────┘
         │                                            │
         │                                    ┌───────┴────────┐
         │                                    ▼                ▼
         │                            ┌──────────────┐ ┌──────────────┐
         │                            │  Sandbox     │ │  Sandbox     │
         │                            │  Container   │ │  Container   │
         │                            │  (常驻)      │ │  (临时)      │
         │                            └──────────────┘ └──────────────┘
         │                                    │
         └────────────────────┬───────────────┘
                              ▼
                 ┌─────────────────────────┐
                 │     MySQL Database      │
                 │  - users                │
                 │  - problems             │
                 │  - submissions          │
                 └─────────────────────────┘
```

**v1.1 架构变化**：
- 新增 **AppContext** 组件，位于 ViewManager 与 DatabaseManager 之间
- ViewManager 通过 `AppContext::createAdminDB()` / `createUserDB()` 获取连接，注入视图层
- **AIClient** 新增数据库查询能力，内部调用 DatabaseManager，视图层完全不接触 SQL

---

## 核心变更详解

### 1. AppContext — 应用上下文与依赖注入

**新增文件**：
- `include/app_context.h`
- `src/app_context.cpp`

**设计目标**：解决 v1.0 中视图层直接管理数据库连接的问题，实现关注点分离。

```cpp
// include/app_context.h
class AppContext
{
public:
    static unique_ptr<DatabaseManager> createAdminDB();
    static unique_ptr<DatabaseManager> createUserDB();
private:
    AppContext() = delete;
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

**调用方改造**（ViewManager）：

```cpp
// src/view_manager.cpp
case 1:
    admin_view = make_unique<AdminView>(AppContext::createAdminDB());
    admin_view->start();
    admin_view.reset();  // 退出即释放连接
    break;
case 2:
    user_view = make_unique<UserView>(AppContext::createUserDB());
    user_view->start();
    user_view.reset();
    break;
```

**视图层改造**：构造函数接收 `unique_ptr<DatabaseManager>`，不再自行创建。

```cpp
// include/user_view.h (v1.1)
explicit UserView(unique_ptr<DatabaseManager> db);

// include/admin_view.h (v1.1)
explicit AdminView(unique_ptr<DatabaseManager> db);
```

---

### 2. AIClient 数据库查询封装

**改造目标**：v1.0 中 `UserView::handle_ai_assistant()` 直接调用 SQL 查询题目信息，v1.1 将这些逻辑封装进 `AIClient`。

**新增接口**（`include/ai_client.h`）：

```cpp
class AIClient
{
public:
    void setDatabaseManager(DatabaseManager *db);

    // 自动查询题目上下文并调用 AI
    string askWithProblemContext(const string &message,
                                 const string &codeContext,
                                 int problemId,
                                 const string &errorContext = "");
private:
    DatabaseManager *db_manager_ = nullptr;
    string queryProblemInfo(int problemId);
    string queryProblemList();
};
```

**封装实现**（`src/ai_client.cpp`）：

```cpp
string AIClient::queryProblemInfo(int problemId)
{
    if (!db_manager_) return "题目ID: " + to_string(problemId);
    string sql = "SELECT title, description, time_limit, memory_limit "
                 "FROM problems WHERE id = " + to_string(problemId);
    auto results = db_manager_->query(sql);
    // ... 组装题目信息字符串
}

string AIClient::queryProblemList()
{
    if (!db_manager_) return "";
    string sql = "SELECT id, title, category, description FROM problems ORDER BY id";
    auto results = db_manager_->query(sql);
    // ... 组装题库列表字符串
}

string AIClient::askWithProblemContext(...)
{
    string problemContext = queryProblemInfo(problemId);
    if (!errorContext.empty()) problemContext += "\n\n" + errorContext;
    string response = ask(message, codeContext, problemContext);
    if (response.find("[NEED_PROBLEMS]") != string::npos) {
        string contextWithList = problemContext + "\n\n" + queryProblemList();
        response = ask(message, codeContext, contextWithList);
    }
    return response;
}
```

**视图层简化**：`user_view.cpp` 中 AI 助手函数从约 50 行 SQL 查询逻辑缩减为 3 行调用。

```cpp
// src/user_view.cpp (v1.1)
ai_client->setDatabaseManager(db_manager.get());
string response = ai_client->askWithProblemContext(
    question, code, problem_id, error_ctx);
```

---

### 3. 代码风格统一 — using namespace std

**变更范围**：所有 `.h` 和 `.cpp` 文件。

**改造方式**：
1. 在每个头文件顶部添加 `using namespace std;`
2. 将所有 `std::string`、`std::vector`、`std::unique_ptr` 等替换为无前缀形式

**涉及文件**：
- `include/sandbox_container.h`
- `include/judge_core.h`
- `include/container_pool.h`
- `include/app_context.h`
- `include/ai_client.h`
- `include/admin_view.h`
- `include/user_view.h`
- `include/user.h`
- `include/admin.h`
- `include/view_manager.h`
- `src/app_context.cpp`
- `src/admin_view.cpp`
- `src/user_view.cpp`

---

### 4. 静态代码分析清理

**分析范围**：`src/*.cpp`、`include/*.h`、`ai/.env`、`setup.sh`

**清理项**：

| 类别       | 元素                           | 操作                    |
| ---------- | ------------------------------ | ----------------------- |
| 未使用函数 | `User::getLastReport()`        | 删除声明和定义          |
| 未使用变量 | `User::last_report_`           | 删除成员变量            |
| 未使用字段 | `JudgeConfig::output_limit_mb` | 从结构体删除            |
| 未使用字段 | `JudgeConfig::language`        | 从结构体删除            |
| 未使用参数 | `submit_code() language`       | 从函数签名删除          |
| 冗余头文件 | `#include <cstring>`           | 从 `src/user.cpp` 删除  |
| 未使用常量 | `Color::BLUE`                  | 从 `color_codes.h` 删除 |
| 未使用常量 | `Color::MAGENTA`               | 从 `color_codes.h` 删除 |
| 未使用常量 | `Color::WHITE`                 | 从 `color_codes.h` 删除 |

---

### 5. Docker 一体化部署

**新增文件**：

| 文件                   | 职责                                                  |
| ---------------------- | ----------------------------------------------------- |
| `docker-compose.yml`   | 编排 `oj-db`（MySQL）和 `oj-app`（主程序）服务        |
| `Dockerfile.app`       | 多阶段构建：Stage1 编译 C++，Stage2 运行时镜像        |
| `docker-entrypoint.sh` | 容器启动入口：等 MySQL 就绪 → 构建沙箱镜像 → 启动程序 |
| `.dockerignore`        | 排除 build/、ai/.env、.git/ 等无需进镜像的文件        |
| `ai/.env.example`      | API Key 配置模板                                      |

**启动方式**：

```bash
# 启动数据库（首次自动建表+插入 8 道示例题）
docker compose up -d oj-db

# 启动 OJ 主程序（交互式终端）
docker compose run --rm oj-app
```

**关键设计**：
- `oj-app` 容器挂载宿主机 `/var/run/docker.sock`，在容器内调用宿主机 Docker 创建评测沙箱
- `oj-db` 挂载 `init.sql` 到 `/docker-entrypoint-initdb.d/`，首次启动自动初始化
- `ai/.env` 通过 `env_file` 注入，**不进镜像**，保护 API Key 安全

---

### 6. 项目文档

**README.md**：从 2 行扩展为 800+ 行的详细编程教程，涵盖：
- 项目概述与技术栈说明
- 分层架构设计与依赖注入解析
- 五大核心模块详解（DatabaseManager、AppContext、JudgeCore、ContainerPool、AIClient）
- 关键流程代码示例（认证、评测、沙箱、AI 交互）
- 项目特色（PIMPL、零 SDK Docker 集成、错误驱动 AI 联动）
- 完整部署与使用指南
- 已知限制与可优化方向

**docs/async_concurrent_judge_architecture.md**：v2.0 异步并发评测架构设计文档，约 1,700 行，涵盖：
- 异步处理流程与状态流转
- Redis 数据结构设计（6 种数据结构）
- RESTful API 接口规范（含 WebSocket 推送）
- 用户工作区隔离方案
- AI 多会话隔离实现
- 容器池排队机制与优先级调度
- 并发控制与锁机制
- 数据库事务处理策略
- 性能优化建议
- Docker 部署配置
- 安全考虑事项

---

## 代码统计

| 类型                           | 文件数 | 行数      |
| ------------------------------ | ------ | --------- |
| C++ 源文件 (`src/`)            | 12     | 2,080     |
| C++ 头文件 (`include/`)        | 12     | 686       |
| Python (`ai/`)                 | 1      | 128       |
| Shell (`docker-entrypoint.sh`) | 1      | 91        |
| Dockerfile                     | 1      | 79        |
| **合计**                       | **27** | **3,064** |

---

## 文件清单变更

### 新增文件

```
include/app_context.h           # 应用上下文（数据库连接工厂）
src/app_context.cpp             # 工厂实现
ai/.env.example                 # API Key 配置模板
docker-compose.yml              # Docker 服务编排
Dockerfile.app                  # 多阶段构建镜像
docker-entrypoint.sh            # 容器启动入口脚本
.dockerignore                   # Docker 构建排除规则
docs/async_concurrent_judge_architecture.md  # v2.0 架构设计
```

### 修改文件

```
include/user.h                  # 删除 getLastReport/last_report_，改造 submit_code 签名
include/judge_core.h            # 删除 output_limit_mb / language 字段
include/color_codes.h           # 删除 BLUE/MAGENTA/WHITE
include/sandbox_container.h     # 统一 using namespace std
include/container_pool.h        # 统一 using namespace std
include/ai_client.h             # 新增数据库查询接口
include/admin_view.h            # 构造函数改为注入模式
include/user_view.h             # 构造函数改为注入模式
include/admin.h                 # 统一 using namespace std
include/view_manager.h          # 统一 using namespace std
src/user.cpp                    # 删除 language 参数、last_report_ 赋值、#include <cstring>
src/ai_client.cpp               # 新增 queryProblemInfo/queryProblemList/askWithProblemContext
src/view_manager.cpp            # 通过 AppContext 注入数据库连接
src/admin_view.cpp              # 适配注入构造函数
src/user_view.cpp               # 移除 AI 助手中的 SQL 查询
.gitignore                      # 新增 build/、src/build/、.vscode/、.DS_Store
README.md                       # 重写为详细编程教程
```

---

## 测试验证

| 验证项         | 方法                                                                   | 结果                  |
| -------------- | ---------------------------------------------------------------------- | --------------------- |
| 编译检查       | `cd build && make -j4`                                                 | 零警告零错误通过      |
| 命名空间一致性 | `grep -r "std::" src/ include/`                                        | 零匹配                |
| 未使用元素残留 | `grep -r "last_report_\|getLastReport\|output_limit_mb" src/ include/` | 零匹配                |
| Docker 构建    | `docker compose build oj-app`                                          | 多阶段构建成功        |
| 数据库初始化   | `docker compose up -d oj-db`                                           | init.sql 自动执行成功 |

---

## 可优化方向（v1.2+）

- [ ] 配置文件化：将数据库连接参数从 `app_context.cpp` 硬编码移出
- [ ] 引入单元测试框架（Catch2 / GoogleTest）
- [ ] 日志系统：替换 `cout` 为结构化日志（spdlog）
- [ ] 配置热重载：支持运行时修改部分配置
- [ ] 评测结果缓存：避免相同代码重复评测

---

*文档版本: v1.1*
*最后更新: 2026年4月*
