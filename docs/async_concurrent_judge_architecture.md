# OJ 系统异步并发评测架构升级方案

> 版本: v2.0 设计草案
> 状态: 技术方案设计阶段

---

## 目录

1. [概述](#1-概述)
2. [系统架构与异步处理流程](#2-系统架构与异步处理流程)
3. [Redis 数据结构设计](#3-redis-数据结构设计)
4. [RESTful API 接口规范](#4-restful-api-接口规范)
5. [用户工作区管理策略](#5-用户工作区管理策略)
6. [AI 会话隔离实现方案](#6-ai-会话隔离实现方案)
7. [容器池与排队机制详细设计](#7-容器池与排队机制详细设计)
8. [并发控制与锁机制](#8-并发控制与锁机制)
9. [数据库事务处理策略](#9-数据库事务处理策略)
10. [性能优化建议](#10-性能优化建议)
11. [部署配置说明](#11-部署配置说明)
12. [安全考虑事项](#12-安全考虑事项)

---

## 1. 概述

### 1.1 当前架构的局限性

v1.0 架构采用**同步阻塞**的评测模式：用户提交代码后，`User::submit_code()` 直接调用 `JudgeCore::judge()`，主线程阻塞等待评测完成。这种模式存在以下问题：

| 问题         | 影响                                     |
| ------------ | ---------------------------------------- |
| 单线程阻塞   | 评测期间终端完全卡住，用户体验差         |
| 无并发支持   | 第二个用户提交时，第一个评测必须完成     |
| 无排队机制   | 容器池满载时直接失败，而非排队等待       |
| 无状态持久化 | 程序崩溃后，正在进行的评测结果丢失       |
| 单点终端     | 必须与服务器在同一终端交互，无法远程使用 |

### 1.2 升级目标

本次升级将 v1.0 从**同步终端交互式**演进为**异步 HTTP 服务化**架构，核心目标：

1. **解耦交互与评测**：终端只负责展示，评测引擎独立运行
2. **支持高并发**：多用户同时提交，评测任务异步排队执行
3. **状态可恢复**：评测中间状态持久化到 Redis，崩溃后可恢复
4. **远程可用**：通过 RESTful API + WebSocket 支持任意客户端接入
5. **资源可控**：容器池满载时优雅排队，而非直接拒绝

### 1.3 技术选型

| 组件          | 选型                   | 版本  | 职责                        |
| ------------- | ---------------------- | ----- | --------------------------- |
| HTTP 服务框架 | Crow (C++)             | 1.0+  | 轻量级 RESTful API 服务器   |
| 消息队列      | Redis                  | 6.0+  | 评测任务队列 + 状态缓存     |
| JSON 序列化   | nlohmann/json          | 3.11+ | API 请求/响应的 JSON 编解码 |
| WebSocket     | Crow 内置              | —     | 实时推送评测进度            |
| 工作区隔离    | std::filesystem        | C++17 | 用户目录隔离                |
| 线程池        | std::thread + 条件变量 | C++17 | 评测工作线程池              |

---

## 2. 系统架构与异步处理流程

### 2.1 整体架构

```
┌──────────────────────────────────────────────────────────────────────┐
│                        客户端（任意终端/浏览器）                        │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                      │
│  │  Web 终端   │  │  CLI 工具   │  │  VS Code  │                      │
│  └──────┬─────┘  └──────┬─────┘  └──────┬─────┘                      │
└─────────┼──────────────┼──────────────┼──────────────────────────────┘
          │              │              │
          └──────────────┼──────────────┘
                         │  HTTP / WebSocket
                         ▼
┌──────────────────────────────────────────────────────────────────────┐
│                      OJ API 服务进程 (oj_app)                         │
│                                                                      │
│  ┌────────────────────┐    ┌────────────────────┐                    │
│  │   HTTP Router      │    │   WebSocket Hub    │                    │
│  │   (Crow)           │    │   (实时推送)        │                    │
│  └────────┬───────────┘    └────────┬───────────┘                    │
│           │                         │                                │
│           ▼                         ▼                                │
│  ┌────────────────────────────────────────────────────┐             │
│  │           API Controller Layer                      │             │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐           │             │
│  │  │ AuthCtrl │ │ProblemCtrl│ │JudgeCtrl │           │             │
│  │  └──────────┘ └──────────┘ └──────────┘           │             │
│  └──────────────────────────┬─────────────────────────┘             │
│                             │                                        │
│                             ▼                                        │
│  ┌────────────────────────────────────────────────────┐             │
│  │           Service Layer (复用 v1.0 业务逻辑)          │             │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐           │             │
│  │  │  Admin   │ │   User   │ │ ViewMgr  │           │             │
│  │  └──────────┘ └──────────┘ └──────────┘           │             │
│  └──────────────────────────┬─────────────────────────┘             │
│                             │                                        │
│                             ▼                                        │
│  ┌────────────────────────────────────────────────────┐             │
│  │         Async Judge Engine (v2.0 新增)              │             │
│  │  ┌──────────────┐  ┌──────────────┐                │             │
│  │  │ Task Enqueue │  │Worker Thread │ × N             │             │
│  │  │   (Redis)    │  │   Pool       │                │             │
│  │  └──────────────┘  └──────────────┘                │             │
│  └────────────────────────────────────────────────────┘             │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
          │                    │                    │
          ▼                    ▼                    ▼
   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
   │   MySQL     │     │   Redis     │     │   Docker    │
   │  (持久化)    │     │ (队列+缓存)  │     │ (沙箱容器)   │
   └─────────────┘     └─────────────┘     └─────────────┘
```

### 2.2 异步评测处理流程

```
用户提交代码
    │
    ▼
┌──────────────────┐
│ POST /api/judge  │  ← 立即返回 submission_id + 状态 "QUEUED"
└────────┬─────────┘
         │
         ▼
┌──────────────────────────────┐
│ 1. 生成唯一 submission_id    │
│ 2. 写入 submissions 表       │  status = 'Pending'
│ 3. 保存代码到用户工作区      │  workspace/{user_id}/{submission_id}/main.cpp
│ 4. 将任务推入 Redis 队列     │  LPUSH judge:queue:pending
│ 5. 返回 {id, status}         │
└──────────────────────────────┘
         │
         │  ← 用户收到响应，可继续操作或轮询状态
         │
         ▼
┌──────────────────────────────────────────────┐
│              评测工作线程池                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐      │
│  │ Worker1 │  │ Worker2 │  │ Worker3 │ ...  │
│  └────┬────┘  └────┬────┘  └────┬────┘      │
│       │            │            │            │
│       └────────────┴────────────┘            │
│                    │                         │
│            BRPOP judge:queue:pending          │  ← 阻塞等待任务
│                    │                         │
│         获取到任务 → 调用 JudgeCore::judge()   │
│                    │                         │
│         更新 Redis 状态 → 写入 MySQL           │
│                    │                         │
│         WebSocket 推送实时进度                 │
└──────────────────────────────────────────────┘
```

### 2.3 状态流转图

```
[QUEUED] ──→ [PENDING] ──→ [COMPILING] ──→ [RUNNING] ──→ [FINISHED]
    │            │                              │               │
    │            └────── 异常分支 ──────────────┘               │
    │            │                              │               │
    │            ▼                              ▼               ▼
    │      [COMPILE_ERROR]              [TIME_LIMIT_EXCEEDED]  [ACCEPTED]
    │      [SYSTEM_ERROR]               [MEMORY_LIMIT_EXCEEDED][WRONG_ANSWER]
    │                                    [RUNTIME_ERROR]       ...
    │
    └──── 排队中，等待容器资源
```

### 2.4 关键技术决策

**为什么用 Crow 而不是其他 C++ Web 框架？**

| 框架        | 优势                                   | 劣势                   | 选择     |
| ----------- | -------------------------------------- | ---------------------- | -------- |
| Crow        | 头文件库、零依赖、轻量、支持 WebSocket | 社区较小               | **选中** |
| Pistache    | RESTful 友好                           | 编译复杂、维护不活跃   | —        |
| Drogon      | 高性能、功能全                         | 依赖多、学习成本高     | —        |
| cpp-httplib | 极简                                   | 无 WebSocket、功能有限 | —        |

Crow 的 **头文件单文件**特性最适合本项目：不引入额外的链接依赖，与现有 CMake 构建系统无缝集成。

---

## 3. Redis 数据结构设计

Redis 在本方案中承担三个角色：**消息队列**、**状态缓存**、**会话存储**。

### 3.1 评测任务队列

```
Key: judge:queue:pending
Type: List
Direction: RPUSH (入队) / BRPOP (出队)
Element: JSON 字符串

示例元素:
{
    "submission_id": 1001,
    "user_id": 42,
    "problem_id": 3,
    "code_path": "workspace/42/1001/main.cpp",
    "language": "cpp",
    "enqueue_time": 1713686400,
    "priority": 0          // 0=普通, 1=VIP, 数字越小优先级越高
}
```

**为什么用 List 而不是 Stream？**

- List 的 `BRPOP` 支持**阻塞等待**，工作线程无需轮询，CPU 零开销
- 实现简单，配合优先级队列（多个 List）即可满足需求
- Stream 更适合事件溯源，本场景只需要简单的 FIFO 调度

### 3.2 评测状态缓存

```
Key: judge:status:{submission_id}
Type: Hash
TTL: 3600 秒（1小时后自动清理，已持久化到 MySQL）

字段:
  status        → "QUEUED" | "PENDING" | "COMPILING" | "RUNNING" | "FINISHED"
  result        → "AC" | "WA" | "TLE" | "MLE" | "RE" | "CE" | "SE"
  passed_cases  → "3"
  total_cases   → "5"
  time_ms       → "120"
  memory_mb     → "8"
  error_msg     → "编译错误: ..."
  progress      → "60"       // 百分比，用于实时进度推送
  updated_at    → "1713686420"
```

**状态更新策略**：

- 评测引擎每完成一个测试点，更新 `progress` 和 `passed_cases`
- 评测完成后写入完整结果，TTL 开始计时
- 用户查询时优先读 Redis（纳秒级），miss 则回源 MySQL

### 3.3 用户会话缓存

```
Key: session:{session_token}
Type: Hash
TTL: 7200 秒（2小时，每次请求刷新）

字段:
  user_id       → "42"
  account       → "test_user"
  role          → "user" | "admin"
  login_time    → "1713686400"
  last_active   → "1713687000"
```

**Token 生成**：`SHA256(user_id + timestamp + random_salt)`，128 位十六进制字符串。

### 3.4 限流计数器

```
Key: rate_limit:{user_id}:{minute_bucket}
Type: String (INCR)
TTL: 120 秒

用途: 限制单个用户每分钟提交次数（默认 10 次/分钟）
```

### 3.5 AI 会话隔离存储

```
Key: ai:session:{user_id}:{terminal_id}
Type: Hash
TTL: 3600 秒

字段:
  session_id    → "default"           // Python AI 服务的会话标识
  history_len   → "12"                // 当前对话轮数
  created_at    → "1713686400"
  last_active   → "1713687000"
```

详见第 6 章 AI 会话隔离方案。

### 3.6 容器池状态缓存

```
Key: pool:containers
Type: Hash

字段:
  idle_count        → "2"     // 当前空闲容器数
  busy_count        → "1"     // 当前忙碌容器数
  max_size          → "4"     // 最大容量
  queue_length      → "3"     // 等待队列长度
```

---

## 4. RESTful API 接口规范

### 4.1 基础约定

| 项目         | 约定                                         |
| ------------ | -------------------------------------------- |
| 基础地址     | `http://localhost:8080`                      |
| 内容类型     | `Content-Type: application/json`             |
| 认证方式     | `Authorization: Bearer {token}`              |
| 统一响应格式 | `{ "code": 0, "message": "ok", "data": {} }` |
| 错误码       | `code != 0` 表示错误，`message` 携带错误描述 |

### 4.2 认证接口

#### POST /api/auth/register

用户注册。

**请求体：**
```json
{
    "account": "new_user",
    "password": "123456"
}
```

**响应：**
```json
{
    "code": 0,
    "message": "注册成功",
    "data": {
        "user_id": 43,
        "account": "new_user"
    }
}
```

#### POST /api/auth/login

用户登录，返回 JWT Token。

**请求体：**
```json
{
    "account": "test_user",
    "password": "123456"
}
```

**响应：**
```json
{
    "code": 0,
    "message": "登录成功",
    "data": {
        "token": "a1b2c3d4...f128",
        "user_id": 42,
        "account": "test_user",
        "role": "user"
    }
}
```

### 4.3 题目接口

#### GET /api/problems

获取题目列表。

**请求头：** `Authorization: Bearer {token}`

**查询参数：**
- `category` (可选): 筛选分类，如 "动态规划"
- `page` (可选): 页码，默认 1
- `limit` (可选): 每页数量，默认 20

**响应：**
```json
{
    "code": 0,
    "data": {
        "total": 8,
        "page": 1,
        "limit": 20,
        "problems": [
            {
                "id": 1,
                "title": "A+B Problem",
                "category": "模拟",
                "time_limit": 1000,
                "memory_limit": 128
            }
        ]
    }
}
```

#### GET /api/problems/{id}

获取单题详情。

**响应：**
```json
{
    "code": 0,
    "data": {
        "id": 1,
        "title": "A+B Problem",
        "description": "Calculate the sum of two integers.",
        "time_limit": 1000,
        "memory_limit": 128,
        "category": "模拟"
    }
}
```

### 4.4 评测接口（核心）

#### POST /api/judge/submit

提交代码评测。

**请求头：** `Authorization: Bearer {token}`

**请求体：**
```json
{
    "problem_id": 1,
    "code": "#include <iostream>\nusing namespace std;\nint main() { int a,b; cin>>a>>b; cout<<a+b; }",
    "language": "cpp"
}
```

**响应（立即返回）：**
```json
{
    "code": 0,
    "message": "提交成功，评测已排队",
    "data": {
        "submission_id": 1001,
        "status": "QUEUED",
        "queue_position": 3,
        "estimated_wait_seconds": 15
    }
}
```

> 注意：`code` 字段直接以字符串传入，服务端保存到用户工作区文件。

#### GET /api/judge/status/{submission_id}

查询评测状态。

**响应（评测中）：**
```json
{
    "code": 0,
    "data": {
        "submission_id": 1001,
        "status": "RUNNING",
        "progress": 60,
        "passed_cases": 3,
        "total_cases": 5,
        "current_case": 4
    }
}
```

**响应（已完成）：**
```json
{
    "code": 0,
    "data": {
        "submission_id": 1001,
        "status": "FINISHED",
        "result": "ACCEPTED",
        "time_used_ms": 120,
        "memory_used_mb": 8,
        "passed_test_cases": 5,
        "total_test_cases": 5,
        "details": [
            {"case_id": 1, "result": "AC", "time_ms": 12, "memory_mb": 3},
            {"case_id": 2, "result": "AC", "time_ms": 15, "memory_mb": 3},
            {"case_id": 3, "result": "AC", "time_ms": 11, "memory_mb": 3},
            {"case_id": 4, "result": "AC", "time_ms": 13, "memory_mb": 3},
            {"case_id": 5, "result": "AC", "time_ms": 14, "memory_mb": 3}
        ]
    }
}
```

#### GET /api/judge/history

获取当前用户的提交历史。

**查询参数：**
- `page`: 页码，默认 1
- `limit`: 每页数量，默认 20

**响应：**
```json
{
    "code": 0,
    "data": {
        "total": 15,
        "submissions": [
            {
                "id": 1001,
                "problem_id": 1,
                "problem_title": "A+B Problem",
                "status": "AC",
                "submit_time": "2025-04-21T10:30:00Z"
            }
        ]
    }
}
```

### 4.5 WebSocket 实时推送

#### 连接地址

```
ws://localhost:8080/ws/judge?token={jwt_token}
```

#### 推送消息格式

```json
// 评测状态更新
{
    "type": "judge_progress",
    "submission_id": 1001,
    "status": "RUNNING",
    "progress": 60,
    "current_case": 4,
    "result": null
}

// 评测完成
{
    "type": "judge_complete",
    "submission_id": 1001,
    "status": "FINISHED",
    "result": "ACCEPTED",
    "time_used_ms": 120,
    "memory_used_mb": 8
}

// 系统通知（排队位置变化）
{
    "type": "queue_update",
    "submission_id": 1001,
    "queue_position": 1,
    "estimated_wait_seconds": 5
}
```

### 4.6 AI 助手接口

#### POST /api/ai/ask

向 AI 助手提问。

**请求体：**
```json
{
    "message": "我的代码哪里错了？",
    "problem_id": 1,
    "terminal_id": "web_001"    // 终端标识，用于会话隔离
}
```

**响应：**
```json
{
    "code": 0,
    "data": {
        "response": "根据评测结果，你的代码在第 4 个测试点...",
        "session_id": "user42_web_001"
    }
}
```

### 4.7 错误码定义

| 错误码 | 含义                         | HTTP 状态码 |
| ------ | ---------------------------- | ----------- |
| 0      | 成功                         | 200         |
| 1001   | 参数错误                     | 400         |
| 1002   | 认证失败（Token 无效或过期） | 401         |
| 1003   | 权限不足                     | 403         |
| 1004   | 资源不存在                   | 404         |
| 2001   | 账号已存在                   | 409         |
| 2002   | 账号或密码错误               | 401         |
| 3001   | 评测队列已满                 | 503         |
| 3002   | 提交频率超限                 | 429         |
| 3003   | 代码为空或过长               | 400         |
| 4001   | AI 服务不可用                | 503         |
| 5001   | 系统内部错误                 | 500         |

---

## 5. 用户工作区管理策略

### 5.1 目录结构重构

v1.0 的 `workspace/solution.cpp` 是**全局单一文件**，所有用户共享，存在严重的安全和并发问题。v2.0 重构为**用户隔离的多级目录结构**：

```
workspace/
├── 42/                          # 用户 ID = 42 的工作区
│   ├── 1001/                    # submission_id = 1001
│   │   └── main.cpp             # 本次提交的代码
│   ├── 1002/
│   │   └── main.cpp
│   └── solution.cpp             # 用户的默认/草稿代码（保留兼容）
├── 43/                          # 用户 ID = 43 的工作区
│   ├── 1003/
│   │   └── main.cpp
│   └── solution.cpp
└── .gitkeep
```

### 5.2 目录创建与权限控制

```cpp
// workspace_manager.h
class WorkspaceManager
{
public:
    // 获取用户工作区根路径
    static string getUserWorkspace(int user_id);

    // 为单次提交创建独立目录
    static string createSubmissionDir(int user_id, int submission_id);

    // 保存代码到工作区
    static bool saveCode(int user_id, int submission_id, const string &code);

    // 读取代码
    static string readCode(int user_id, int submission_id);

    // 清理过期工作区（定时任务）
    static void cleanup(int max_age_days = 7);

private:
    static constexpr const char *BASE_DIR = "workspace";
};
```

```cpp
// workspace_manager.cpp
string WorkspaceManager::getUserWorkspace(int user_id)
{
    return BASE_DIR + "/" + to_string(user_id);
}

string WorkspaceManager::createSubmissionDir(int user_id, int submission_id)
{
    string dir = getUserWorkspace(user_id) + "/" + to_string(submission_id);
    filesystem::create_directories(dir);
    return dir;
}

bool WorkspaceManager::saveCode(int user_id, int submission_id, const string &code)
{
    string dir = createSubmissionDir(user_id, submission_id);
    string path = dir + "/main.cpp";

    ofstream ofs(path);
    if (!ofs.is_open()) return false;
    ofs << code;
    return ofs.good();
}
```

### 5.3 安全隔离原则

| 约束         | 实现                                                         |
| ------------ | ------------------------------------------------------------ |
| 路径遍历防护 | 所有路径拼接前校验 `user_id` 为纯数字，禁止 `../` 等特殊字符 |
| 目录权限     | 使用 `filesystem::perms` 设置 `owner_read                    | owner_write`，禁止其他用户访问 |
| 磁盘配额     | 单用户代码文件总大小限制为 10MB，超限拒绝写入                |
| 自动清理     | 后台定时任务每天清理超过 7 天的提交目录，保留最近 20 次提交  |

### 5.4 与评测引擎的衔接

```cpp
// 提交时
string code_path = WorkspaceManager::saveCode(user_id, submission_id, code);
// code_path = "workspace/42/1001/main.cpp"

// 评测时
judge_engine.setSourceCodePath(code_path);
// JudgeCore 读取此路径的代码，而非硬编码的 workspace/solution.cpp
```

### 5.5 向后兼容

v2.0 保留 `workspace/{user_id}/solution.cpp` 作为**默认草稿文件**，CLI 客户端可以沿用 v1.0 的习惯直接编辑此文件后提交。API 提交时若不带代码内容，服务端自动读取此文件。

---

## 6. AI 会话隔离实现方案

### 6.1 问题背景

v1.0 的 AI 服务使用单一 `sessionId_ = "default"`，所有用户共享同一个 Python 会话对象。v2.0 面临以下场景：

| 场景                    | v1.0 问题                | v2.0 需求               |
| ----------------------- | ------------------------ | ----------------------- |
| 用户 A 在 Web 端提问    | 与终端端的对话混在一起   | Web 和终端会话独立      |
| 用户 A 同时登录两个终端 | 两个终端看到同一对话历史 | 每个终端有独立上下文    |
| 用户 B 提问             | 能看到用户 A 的历史      | 完全隔离，互不可见      |
| 会话恢复                | 重启后历史丢失           | 可从 Redis 恢复最近对话 |

### 6.2 会话标识设计

采用**复合键**标识会话：

```
session_key = "{user_id}:{terminal_id}"
```

- `user_id`: 平台用户唯一标识
- `terminal_id`: 终端实例标识，由客户端生成（如 `web_001`、`cli_abc123`）

**terminal_id 生成策略**：

```cpp
// 客户端生成（如 Web 前端）
const terminalId = 'web_' + Math.random().toString(36).substr(2, 8);

// CLI 客户端生成
terminal_id = "cli_" + get_machine_id() + "_" + to_string(getpid());
```

### 6.3 AIClient 改造

```cpp
// ai_client.h (v2.0 改造)
class AIClient
{
public:
    // 为特定会话生成/获取 session_id
    string getOrCreateSession(int user_id, const string &terminal_id);

    // 带会话隔离的提问接口
    string askWithSession(const string &session_key,
                          const string &message,
                          const string &code_context,
                          int problem_id,
                          const string &error_context = "");

    // 清除指定会话的历史（用户主动重置）
    bool clearSession(const string &session_key);

private:
    // Redis 中存储的会话信息
    struct AISession {
        string python_session_id;   // 传给 Python 脚本的 session 参数
        int message_count;          // 当前对话轮数
        time_t created_at;
        time_t last_active;
    };

    AISession loadSession(const string &session_key);
    void saveSession(const string &session_key, const AISession &session);

    RedisClient *redis_;  // Redis 连接
};
```

### 6.4 会话生命周期

```
用户首次提问
    │
    ▼
┌─────────────────────────────────────┐
│ 1. 检查 Redis: ai:session:{key}      │
│    不存在 → 生成新的 python_session_id│
│    存在   → 复用已有的 session_id    │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 2. 调用 Python AI 服务               │
│    python ai_service.py --session {id}│
│    传递 session_id 保持上下文连续性   │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ 3. 更新 Redis 会话信息               │
│    message_count++, last_active=now │
│    刷新 TTL                         │
└─────────────────────────────────────┘
```

### 6.5 Redis 存储结构

```
Key: ai:session:{user_id}:{terminal_id}
Type: Hash
TTL: 3600 秒（1小时无活动自动清理）

字段:
  python_session_id  → "sess_a7b3c9d2"    // Python 层的 session 标识
  message_count      → "5"                  // 当前对话轮数
  created_at         → "1713686400"
  last_active        → "1713687000"
```

### 6.6 Python 服务端适配

```python
# ai_service.py (适配 v2.0)
# 现有代码已支持 --session 参数，无需修改
# session_memories 字典天然支持多会话隔离

def main():
    parser.add_argument("--session", type=str, default="default")
    # ... 其余参数不变

    # session_memories 已经是按 session_id 隔离的字典
    # 不同 user_id + terminal_id 组合传入不同的 session 参数即可
```

### 6.7 会话上限与清理

```cpp
// 防止单个用户创建过多会话（DDOS 风险）
static constexpr int MAX_SESSIONS_PER_USER = 5;

bool AIClient::getOrCreateSession(int user_id, const string &terminal_id)
{
    // 检查该用户已有会话数
    string pattern = "ai:session:" + to_string(user_id) + ":*";
    auto keys = redis_->keys(pattern);

    if (keys.size() >= MAX_SESSIONS_PER_USER) {
        // 删除最久未使用的会话（LRU）
        string lru_key = findLRUSession(keys);
        redis_->del(lru_key);
    }

    // ... 创建或复用会话
}
```

---

## 7. 容器池与排队机制详细设计

### 7.1 当前 v1.0 容器池的局限

v1.0 的 `ContainerPool` 在容器满载时直接返回 `nullptr`，评测失败。v2.0 需要实现**满载排队**机制：

```
v1.0 行为:
  请求容器 → 满载 → return nullptr → 评测失败

v2.0 行为:
  请求容器 → 满载 → 加入 Redis 队列 → 等待容器释放 → 自动获取 → 开始评测
```

### 7.2 排队架构

```
评测请求
    │
    ▼
┌─────────────────────────────┐
│ ContainerPool::acquire()    │
│                             │
│  if (有空闲容器)             │
│      → 立即返回容器          │
│  else if (可创建临时容器)     │
│      → 创建并返回            │
│  else                        │
│      → 阻塞等待（条件变量）   │
│      → 或返回 QUEUED 状态    │
└─────────────────────────────┘
```

### 7.3 Redis 驱动的任务队列

工作线程不再直接调用 `ContainerPool::acquire()`，而是从 Redis 队列**消费任务**：

```cpp
// worker_thread.cpp
void JudgeWorker::run()
{
    while (running_) {
        // 阻塞等待任务（最多等待 5 秒）
        auto task_opt = redis_->brpop("judge:queue:pending", 5);
        if (!task_opt) continue;  // 超时，继续循环

        JudgeTask task = parseTask(*task_opt);

        // 获取容器（此处可能阻塞等待）
        bool is_temp = false;
        auto container = pool_->acquire(is_temp);

        // 执行评测
        JudgeReport report = executeJudge(task, container);

        // 释放容器
        if (is_temp) pool_->destroyTemporary(container);
        else pool_->release(container);

        // 通知等待队列的线程：容器已释放
        pool_->notifyOneIdle();
    }
}
```

### 7.4 容器池改造

```cpp
// container_pool.h (v2.0 改造)
class ContainerPool
{
public:
    // v1.0 原有接口
    bool initialize(int min_size = 1, int max_size = 4, const string &image = "judge-sandbox:latest");
    shared_ptr<SandboxContainer> acquire(bool &is_temporary);
    void release(shared_ptr<SandboxContainer> container);
    void destroyTemporary(shared_ptr<SandboxContainer> container);

    // v2.0 新增：排队通知机制
    void notifyOneIdle();                    // 通知一个等待的线程
    bool waitForIdle(chrono::seconds timeout); // 阻塞等待空闲容器
    int idleCount() const;                   // 当前空闲容器数
    int busyCount() const;                   // 当前忙碌容器数

private:
    vector<shared_ptr<SandboxContainer>> resident_;
    mutable mutex mutex_;
    condition_variable cv_idle_;             // 容器空闲通知
    int max_size_ = 4;
    int temporary_active_ = 0;
    int busy_count_ = 0;                     // 新增：当前忙碌容器计数

    shared_ptr<SandboxContainer> createContainer();
};
```

### 7.5 acquire() 的排队逻辑

```cpp
shared_ptr<SandboxContainer> ContainerPool::acquire(bool &is_temporary)
{
    unique_lock<mutex> lock(mutex_);

    // 第一优先级：空闲常驻容器
    for (auto &c : resident_) {
        if (c->getState() == ContainerState::IDLE) {
            c->setState(ContainerState::BUSY);
            busy_count_++;
            is_temporary = false;
            return c;
        }
    }

    // 第二优先级：创建临时容器
    int total = resident_.size() + temporary_active_;
    if (total < max_size_) {
        auto container = createContainer();
        if (container) {
            temporary_active_++;
            busy_count_++;
            is_temporary = true;
            return container;
        }
    }

    // 第三优先级：等待其他线程释放容器（带超时）
    // 调用方应在此之前已将任务入队，此处只是工作线程的竞争获取
    // 如果等待超时，返回 nullptr，由上层决定重试或标记失败
    if (cv_idle_.wait_for(lock, chrono::seconds(30), [this]() {
        // 重新检查是否有空闲容器
        for (auto &c : resident_) {
            if (c->getState() == ContainerState::IDLE) return true;
        }
        return false;
    })) {
        // 唤醒后重新获取（递归或循环）
        lock.unlock();
        return acquire(is_temporary);
    }

    // 超时，资源不足
    is_temporary = false;
    return nullptr;
}
```

### 7.6 release() 的通知机制

```cpp
void ContainerPool::release(shared_ptr<SandboxContainer> container)
{
    unique_lock<mutex> lock(mutex_);

    container->reset();
    container->setState(ContainerState::IDLE);
    busy_count_--;

    // 通知等待的线程：有容器空闲了
    cv_idle_.notify_one();
}

void ContainerPool::notifyOneIdle()
{
    cv_idle_.notify_one();
}
```

### 7.7 队列优先级设计

支持两种调度策略，通过 Redis **Sorted Set** 实现：

```
Key: judge:queue:priority
Type: Sorted Set
Score: priority_timestamp（优先级数值 + 时间戳，值越小越优先）

普通用户: score = enqueue_time              // FIFO
VIP 用户:   score = enqueue_time - 1000000  // 插队（时间戳减一个大数）
```

```cpp
// 入队时计算 score
double calculatePriorityScore(int priority, time_t enqueue_time)
{
    // priority: 0=普通, 1=VIP
    // VIP 任务获得更小的 score，优先出队
    return static_cast<double>(enqueue_time) - priority * 1000000.0;
}

// 入队
redis_->zadd("judge:queue:priority", score, task_json);

// 出队（阻塞式）
auto result = redis_->bzpopmin("judge:queue:priority", timeout);
```

### 7.8 排队位置与等待时间估算

```cpp
// JudgeService::submit()
JudgeResponse submit(const JudgeRequest &req)
{
    // ... 保存代码，创建 submission 记录

    // 计算队列位置
    long queue_position = redis_->zrank("judge:queue:priority", task_id);
    if (queue_position < 0) queue_position = 0;

    // 估算等待时间（基于历史平均评测时长）
    int avg_judge_time = redis_->get("stats:avg_judge_time");  // 秒
    int estimated_wait = queue_position * avg_judge_time;

    return {
        .submission_id = submission_id,
        .status = "QUEUED",
        .queue_position = static_cast<int>(queue_position),
        .estimated_wait_seconds = estimated_wait
    };
}
```

---

## 8. 并发控制与锁机制

### 8.1 并发场景分析

| 场景                | 并发风险        | 解决方案                        |
| ------------------- | --------------- | ------------------------------- |
| 多用户同时提交      | 数据库连接竞争  | 连接池 + 事务                   |
| 多工作线程获取容器  | 容器状态竞争    | `mutex_` + `condition_variable` |
| 同时读写 Redis 状态 | 状态不一致      | Redis 单线程原子操作            |
| AI 服务并发调用     | Python GIL 串行 | 进程池或队列串行化              |
| 文件系统并发写      | 文件损坏        | 用户目录隔离 + 文件锁           |

### 8.2 数据库连接池

v1.0 每个视图层创建独立的 `DatabaseManager` 连接，v2.0 改为**全局连接池**：

```cpp
// db_connection_pool.h
class DBConnectionPool
{
public:
    static DBConnectionPool &instance();

    bool initialize(int pool_size = 10);
    shared_ptr<DatabaseManager> acquire();
    void release(shared_ptr<DatabaseManager> conn);

private:
    vector<shared_ptr<DatabaseManager>> pool_;
    queue<shared_ptr<DatabaseManager>> available_;
    mutex mutex_;
    condition_variable cv_;
};
```

```cpp
// 使用示例
auto conn = DBConnectionPool::instance().acquire();
auto results = conn->query(sql);
DBConnectionPool::instance().release(conn);
```

### 8.3 评测引擎并发安全

`JudgeCore` 的 PIMPL 实现内部包含 `ContainerPool` 成员。每个评测任务使用**独立的 `JudgeCore` 实例**，不存在跨任务的状态共享：

```cpp
// 工作线程中，每个任务独立创建 JudgeCore
void JudgeWorker::executeJudge(const JudgeTask &task)
{
    JudgeCore engine;  // 栈上创建，线程私有
    engine.setConfig(task.config);
    engine.setSourceCodePath(task.code_path);
    engine.setTestDataPath(task.test_data_path);

    // ContainerPool::acquire() 内部有 mutex 保护
    JudgeReport report = engine.judge();
    // ...
}
```

### 8.4 Redis 并发安全

Redis 服务端是**单线程事件循环**，天然保证命令的原子性。客户端侧使用**连接池**：

```cpp
// redis_pool.h
class RedisPool
{
public:
    bool initialize(const string &host, int port, int pool_size = 10);
    shared_ptr<RedisClient> acquire();
    void release(shared_ptr<RedisClient> conn);
};
```

关键操作使用 **Redis 事务**（MULTI/EXEC）：

```cpp
// 原子扣减限流计数器
auto pipe = redis->pipeline();
pipe.incr("rate_limit:42:202504211030");
pipe.expire("rate_limit:42:202504211030", 120);
pipe.exec();
```

### 8.5 文件系统并发控制

用户工作区按 `user_id` 隔离后，同一用户的并发操作仍需保护：

```cpp
// 文件写入锁（进程内）
static mutex file_write_mutexes_[MAX_USERS];  // 简单方案：按 user_id 取模

bool WorkspaceManager::saveCode(int user_id, int submission_id, const string &code)
{
    // 同一用户的写入串行化
    lock_guard<mutex> lock(file_write_mutexes_[user_id % MAX_USERS]);

    string path = createSubmissionDir(user_id, submission_id) + "/main.cpp";
    ofstream ofs(path, ios::out | ios::trunc);
    if (!ofs) return false;
    ofs << code;
    return ofs.good();
}
```

### 8.6 AI 服务并发控制

Python 的 GIL（全局解释器锁）使得多线程并发调用 `ai_service.py` 实际上仍是串行执行。解决方案：

**方案 A：进程池（推荐）**

```cpp
// ai_process_pool.h
class AIProcessPool
{
public:
    bool initialize(int pool_size = 3);  // 3 个 Python 进程
    string execute(const string &args);   // 从池中取进程执行
private:
    vector<unique_ptr<AIWorkerProcess>> workers_;
    mutex mutex_;
    condition_variable cv_;
};
```

**方案 B：异步队列（简化版）**

```cpp
// 使用线程安全的任务队列
ThreadSafeQueue<AIRequest> ai_queue_;

// 单线程消费 AI 请求（利用 GIL 串行特性，避免多进程开销）
void AIWorkerThread::run()
{
    while (running_) {
        AIRequest req = ai_queue_.pop();
        string response = executePython(req.args);
        req.callback(response);
    }
}
```

### 8.7 HTTP 请求的线程安全

Crow 框架内部使用**线程池**处理 HTTP 请求，每个请求在独立线程中执行。Controller 层需要确保：

- 无全局可变状态（或加锁保护）
- 数据库连接从连接池获取，用完归还
- Redis 连接从连接池获取，用完归还

```cpp
// 典型的线程安全 Controller 方法
void JudgeController::submit(const Request &req, Response &res)
{
    // 解析请求（栈上，线程私有）
    auto body = json::parse(req.body);
    int user_id = getUserIdFromToken(req);

    // 获取数据库连接（线程安全）
    auto db = DBConnectionPool::instance().acquire();
    defer { DBConnectionPool::instance().release(db); };

    // 获取 Redis 连接（线程安全）
    auto redis = RedisPool::instance().acquire();
    defer { RedisPool::instance().release(redis); };

    // 执行业务逻辑
    auto result = JudgeService::submit(user_id, body, db, redis);

    // 返回响应
    res = makeJsonResponse(result);
}
```

---

## 9. 数据库事务处理策略

### 9.1 事务边界设计

| 操作         | 事务范围                          | 隔离级别        |
| ------------ | --------------------------------- | --------------- |
| 用户注册     | INSERT users                      | SERIALIZABLE    |
| 提交评测     | INSERT submissions                | READ COMMITTED  |
| 评测完成更新 | UPDATE submissions + UPDATE users | READ COMMITTED  |
| 首次 AC 检测 | SELECT + UPDATE users             | REPEATABLE READ |

### 9.2 评测完成的事务处理

```cpp
// 使用 DatabaseManager 封装事务
bool DatabaseManager::executeTransaction(
    const vector<string> &sqls)
{
    if (!conn) return false;

    mysql_query(conn, "START TRANSACTION");

    for (const auto &sql : sqls) {
        if (mysql_query(conn, sql.c_str()) != 0) {
            mysql_query(conn, "ROLLBACK");
            return false;
        }
    }

    mysql_query(conn, "COMMIT");
    return true;
}
```

```cpp
// 评测完成后更新数据库
void updateSubmissionResult(int submission_id, const JudgeReport &report)
{
    string status_str = judgeResultToString(report.result);

    vector<string> sqls;

    // 1. 更新提交记录
    sqls.push_back(
        "UPDATE submissions SET status = '" + status_str + "' " +
        "WHERE id = " + to_string(submission_id)
    );

    // 2. 更新用户统计（提交数 +1）
    sqls.push_back(
        "UPDATE users SET submission_count = submission_count + 1 " +
        "WHERE id = (SELECT user_id FROM submissions WHERE id = " +
        to_string(submission_id) + ")"
    );

    // 3. 如果是 AC 且首次解决，更新 solved_count
    if (report.result == JudgeResult::ACCEPTED) {
        sqls.push_back(
            "UPDATE users SET solved_count = solved_count + 1 " +
            "WHERE id = (SELECT user_id FROM submissions WHERE id = " +
            to_string(submission_id) + ") " +
            "AND NOT EXISTS (" +
            "  SELECT 1 FROM submissions s2 " +
            "  WHERE s2.user_id = submissions.user_id " +
            "  AND s2.problem_id = submissions.problem_id " +
            "  AND s2.status = 'AC' AND s2.id < " +
            to_string(submission_id) + ")"
        );
    }

    db->executeTransaction(sqls);
}
```

### 9.3 乐观锁防止重复提交

```cpp
// 使用 Redis 原子操作防止重复提交
bool preventDuplicateSubmit(int user_id, const string &code_hash)
{
    string key = "submit:dedup:" + to_string(user_id) + ":" + code_hash;

    // SETNX: 只有 key 不存在时才设置成功
    bool success = redis_->setnx(key, "1", 60);  // 60 秒过期
    return success;  // true = 首次提交，false = 重复提交
}
```

### 9.4 分布式事务与最终一致性

评测流程涉及多个存储系统，采用**最终一致性**策略：

```
用户提交
    │
    ▼
MySQL: INSERT submissions (status='Pending')  ← 主事务，必须成功
    │
    ▼
Redis: LPUSH judge:queue:pending               ← 如果失败，后台补偿任务重试
    │
    ▼
Redis: SET judge:status:{id} status='QUEUED'   ← 如果失败，不影响核心流程
```

**补偿机制**：定时任务扫描 `submissions` 表中 `status='Pending'` 超过 5 分钟但未完成记录，重新推入队列。

---

## 10. 性能优化建议

### 10.1 评测沙箱预热

```cpp
// 系统启动时预创建容器，避免冷启动延迟
class ContainerPool
{
public:
    bool initialize(int min_size = 2, int max_size = 8);  // min_size 从 1 提升到 2
    // ...
};
```

### 10.2 代码缓存

用户多次提交同一题目的相似代码时，缓存编译结果：

```
Redis Key: compile:cache:{code_sha256}
Value:    { "container_id": "abc123", "binary_path": "/sandbox/main" }
TTL:      300 秒
```

### 10.3 数据库查询优化

```sql
-- 为高频查询添加索引
CREATE INDEX idx_submissions_user_time ON submissions(user_id, submit_time DESC);
CREATE INDEX idx_submissions_status ON submissions(status);
```

### 10.4 连接池参数调优

| 连接池 | 建议大小 | 说明                      |
| ------ | -------- | ------------------------- |
| MySQL  | 20       | 并发工作线程数 + 预留     |
| Redis  | 30       | 高频读写，连接数需充足    |
| 容器池 | 8        | 根据宿主机 CPU 和内存调整 |

### 10.5 评测结果缓存

已完成评测的结果缓存到 Redis，避免重复评测相同代码：

```
Key: judge:result:{problem_id}:{code_sha256}
Value: { "status": "AC", "time_ms": 120, ... }
TTL: 86400 秒（24 小时）
```

### 10.6 WebSocket 连接复用

评测进度推送使用**长连接复用**，而非每个请求新建连接：

```cpp
// Crow WebSocket 处理器
CROW_ROUTE(app, "/ws/judge")
    .websocket()
    .onopen([&](crow::websocket::connection &conn) {
        // 注册连接，关联 user_id
        ws_hub_.register(conn, getUserId(conn));
    })
    .onclose([&](crow::websocket::connection &conn, const string &reason) {
        ws_hub_.unregister(conn);
    });
```

---

## 11. 部署配置说明

### 11.1 Docker Compose 配置

```yaml
# docker-compose.v2.yml
version: "3.8"

services:

  oj-api:
    build:
      context: .
      dockerfile: Dockerfile.v2
    ports:
      - "8080:8080"          # HTTP API 端口
    environment:
      OJ_DB_HOST: oj-db
      OJ_DB_PORT: 3306
      OJ_DB_USER: oj_admin
      OJ_DB_PASS: 090800
      OJ_REDIS_HOST: oj-redis
      OJ_REDIS_PORT: 6379
      OJ_WORKER_COUNT: 4     # 评测工作线程数
      OJ_POOL_MAX: 8         # 容器池最大容量
    volumes:
      - ./workspace:/app/workspace
      - ./data:/app/data:ro
      - /var/run/docker.sock:/var/run/docker.sock
    privileged: true
    depends_on:
      - oj-db
      - oj-redis

  oj-db:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: ojroot2024
      MYSQL_DATABASE: OJ
    volumes:
      - oj-db-data:/var/lib/mysql
      - ./init.sql:/docker-entrypoint-initdb.d/init.sql:ro
    ports:
      - "3306:3306"

  oj-redis:
    image: redis:7-alpine
    volumes:
      - oj-redis-data:/data
    ports:
      - "6379:6379"
    command: redis-server --appendonly yes --maxmemory 256mb --maxmemory-policy allkeys-lru

volumes:
  oj-db-data:
  oj-redis-data:
```

### 11.2 环境变量配置

| 变量              | 默认值    | 说明                     |
| ----------------- | --------- | ------------------------ |
| `OJ_HTTP_PORT`    | 8080      | HTTP 服务监听端口        |
| `OJ_DB_HOST`      | localhost | MySQL 主机地址           |
| `OJ_DB_PORT`      | 3306      | MySQL 端口               |
| `OJ_REDIS_HOST`   | localhost | Redis 主机地址           |
| `OJ_REDIS_PORT`   | 6379      | Redis 端口               |
| `OJ_WORKER_COUNT` | 4         | 评测工作线程数           |
| `OJ_POOL_MIN`     | 2         | 容器池常驻容器数         |
| `OJ_POOL_MAX`     | 8         | 容器池最大容量           |
| `OJ_RATE_LIMIT`   | 10        | 每分钟最大提交次数       |
| `OJ_AI_POOL_SIZE` | 3         | AI 进程池大小            |
| `OJ_WS_TIMEOUT`   | 3600      | WebSocket 连接超时（秒） |

### 11.3 生产环境部署建议

```bash
# 1. 使用 Release 模式编译
cmake -DCMAKE_BUILD_TYPE=Release .
make -j$(nproc)

# 2. 启用系统服务
sudo systemctl enable docker
sudo systemctl start docker

# 3. 限制资源（systemd）
# /etc/systemd/system/oj-app.service
[Service]
MemoryLimit=2G
CPUQuota=200%
TasksMax=100
```

---

## 12. 安全考虑事项

### 12.1 API 安全

| 威胁            | 防护措施                                         |
| --------------- | ------------------------------------------------ |
| SQL 注入        | 沿用 v1.0 `escape_string()`，v2.0 增加参数化查询 |
| 身份伪造        | JWT Token + Redis 会话校验，每次请求验证         |
| 重放攻击        | Token 包含时间戳，服务端校验有效期               |
| DDoS / 暴力提交 | Redis 限流计数器，超限返回 429                   |
| 代码注入        | 沙箱容器 `--read-only` + `--cap-drop=ALL`        |

### 12.2 JWT Token 设计

```cpp
// token 结构（HS256 签名）
// Header: { "alg": "HS256", "typ": "JWT" }
// Payload: { "user_id": 42, "role": "user", "iat": 1713686400, "exp": 1713690000 }
// Signature: HMACSHA256(base64(header) + "." + base64(payload), secret)
```

密钥存储在环境变量 `OJ_JWT_SECRET` 中，绝不硬编码。

### 12.3 代码执行安全

评测沙箱的安全配置在 v1.0 基础上保持不变：

```bash
docker run \
  --network none \           # 禁止网络
  --read-only \              # 根文件系统只读
  --cap-drop=ALL \           # 移除所有特权
  --security-opt no-new-privileges:true \  # 禁止提权
  --pids-limit=64 \          # 进程数限制
  --memory=256m \            # 内存限制
  --tmpfs /sandbox:mode=1777,size=100m,exec  # 仅 /sandbox 可写
```

### 12.4 文件系统安全

```cpp
// 路径遍历防护
bool isValidPath(const string &path)
{
    // 禁止包含 .. 或绝对路径
    if (path.find("..") != string::npos) return false;
    if (path[0] == '/') return false;
    return true;
}

// 用户只能访问自己的工作区
bool WorkspaceManager::authorizeAccess(int user_id, const string &path)
{
    string prefix = getUserWorkspace(user_id) + "/";
    return path.find(prefix) == 0;
}
```

### 12.5 AI 服务安全

- API Key 存储在 Redis（或环境变量），绝不写入代码或日志
- AI 请求记录审计日志（用户、时间、问题长度）
- 限制单次请求代码长度（最大 64KB），防止超大 payload 攻击

### 12.6 网络安全

```yaml
# 生产环境建议增加 Nginx 反向代理
services:
  nginx:
    image: nginx:alpine
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
      - ./ssl:/etc/nginx/ssl:ro
```

```nginx
# nginx.conf 安全头配置
server {
    listen 443 ssl http2;
    server_name oj.example.com;

    ssl_certificate /etc/nginx/ssl/cert.pem;
    ssl_certificate_key /etc/nginx/ssl/key.pem;

    # 安全头
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;

    location / {
        proxy_pass http://oj-api:8080;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

---

## 附录

### A. 新增文件清单

| 文件                                          | 类型 | 说明               |
| --------------------------------------------- | ---- | ------------------ |
| `src/http_server.cpp`                         | 新增 | Crow HTTP 服务启动 |
| `src/api_router.cpp`                          | 新增 | RESTful 路由注册   |
| `src/judge_worker.cpp`                        | 新增 | 评测工作线程       |
| `src/redis_client.cpp`                        | 新增 | Redis 连接封装     |
| `src/workspace_manager.cpp`                   | 新增 | 用户工作区管理     |
| `src/db_connection_pool.cpp`                  | 新增 | 数据库连接池       |
| `include/http_server.h`                       | 新增 | HTTP 服务头文件    |
| `include/api_router.h`                        | 新增 | 路由定义           |
| `include/judge_worker.h`                      | 新增 | 工作线程定义       |
| `include/redis_client.h`                      | 新增 | Redis 客户端       |
| `include/workspace_manager.h`                 | 新增 | 工作区管理器       |
| `include/db_connection_pool.h`                | 新增 | 连接池定义         |
| `docs/async_concurrent_judge_architecture.md` | 新增 | 本文档             |

### B. 修改文件清单

| 文件                       | 修改内容                               |
| -------------------------- | -------------------------------------- |
| `CMakeLists.txt`           | 添加 Crow、hiredis、nlohmann/json 依赖 |
| `src/main.cpp`             | 改为启动 HTTP 服务而非终端交互         |
| `src/ai_client.cpp`        | 增加多会话管理                         |
| `include/ai_client.h`      | 增加 session_key 参数                  |
| `src/container_pool.cpp`   | 增加排队通知机制                       |
| `include/container_pool.h` | 增加 condition_variable                |
| `src/user.cpp`             | 适配异步提交接口                       |
| `src/view_manager.cpp`     | 可选：保留终端模式作为 fallback        |

### C. 依赖库安装

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    libhiredis-dev \          # Redis C 客户端
    libcrow-dev \             # Crow HTTP 框架（或 header-only）
    nlohmann-json3-dev        # JSON 库

# Crow 为 header-only，也可直接从 GitHub 下载单头文件
wget https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h -O include/crow.h
```

---

*文档版本: v1.0*
*最后更新: 2026年4月*
