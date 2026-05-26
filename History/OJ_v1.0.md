# OJ 在线判题系统 v1.0

## 版本概述

**版本号**: v1.0  
**发布日期**: 2026年4月  
**技术栈**: C++17 / MySQL / CMake 3.10+ / OpenSSL / Python3 / LangChain / Docker  

v1.0 是首个功能完整的正式版本。在 v0.3 基础上实现了 Docker 容器化评测引擎、容器池管理、完整的提交 → 评测 → 结果展示流程、submissions 数据库持久化，以及评测错误与 AI 助手的联动机制。

---

## 版本变更日志

### 新增功能

| 功能模块        | 具体功能          | 说明                                      |
| --------------- | ----------------- | ----------------------------------------- |
| **评测引擎**    | Docker 容器化评测 | SandboxContainer + ContainerPool 完整实现 |
| **评测引擎**    | 逐点测试与比对    | 加载 data/N/ 下所有 .in/.out 文件对       |
| **评测引擎**    | PIMPL 模式        | JudgeCore 隐藏实现细节                    |
| **评测结果**    | 详细评测报告      | 每个测试点状态、时间、内存                |
| **评测结果**    | 彩色终端输出      | AC 绿色 / WA 红色 / TLE 黄色 等           |
| **数据库集成**  | submissions 写入  | 评测完成后自动入库                        |
| **数据库集成**  | 用户统计更新      | submission_count / solved_count 自动更新  |
| **数据库集成**  | 提交历史查询      | 查看最近 20 条提交记录                    |
| **数据库集成**  | escape_string()   | SQL 字符串转义，防注入                    |
| **AI 联动**     | 错误上下文传递    | 评测失败信息自动传递给 AI 助手            |
| **AI 助手**     | 题目推荐          | AI 自主判断需求，按需加载题库             |
| **安全隔离**    | 多层防护          | 禁网/只读/非特权/进程限制/内存限制        |
| **测试数据**    | 8 道题标准用例    | data/1-8/ 每题 4-5 个测试点               |
| **Docker 镜像** | judge-sandbox     | Dockerfile 定义评测沙箱环境               |

### 改进优化

| 改进项       | v0.3         | v1.0                       |
| ------------ | ------------ | -------------------------- |
| 评测核心     | 仅接口定义   | Docker 容器化完整实现      |
| 代码提交     | 待实现占位   | 完整评测 + 入库            |
| 查看提交     | 待实现占位   | 查询 submissions 表        |
| AI 助手      | 无错误上下文 | 自动传递评测错误信息       |
| 容器文件写入 | docker cp    | docker exec + stdin 管道   |
| SQL 安全     | 字符串拼接   | escape_string() 转义       |
| 容器 tmpfs   | 默认权限     | mode=1777 支持非 root 写入 |

---

## 系统架构

### 整体架构

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
└──────────────────────────┬────────────────────────────────────────┘
                           │
               ┌───────────┴───────────┐
               ▼                       ▼
┌──────────────────────┐    ┌──────────────────────┐
│     AdminView        │    │      UserView        │
│    (管理员界面)       │    │     (用户界面)        │
└──────────┬───────────┘    └──────────┬───────────┘
           │                           │
           ▼                           ▼
┌──────────────────────┐    ┌──────────────────────┐
│     Admin            │    │      User            │
│    (业务逻辑)         │    │     (业务逻辑)        │
└──────────┬───────────┘    └──────────┬───────────┘
           │                           │
           │     ┌─────────────────────┼────────────────────┐
           │     ▼                     ▼                    ▼
           │  ┌──────────┐     ┌────────────┐      ┌────────────┐
           │  │Database  │     │  AIClient  │      │ JudgeCore  │
           │  │Manager   │     │            │      │  (PIMPL)   │
           │  └──────────┘     └──────┬─────┘      └──────┬─────┘
           │                          │                    │
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

### 新增组件

| 组件             | 文件路径                      | 职责                          |
| ---------------- | ----------------------------- | ----------------------------- |
| SandboxContainer | `include/sandbox_container.h` | 单个 Docker 容器生命周期管理  |
| ContainerPool    | `include/container_pool.h`    | 容器池调度（常驻 + 按需临时） |
| JudgeCore        | `include/judge_core.h`        | 评测引擎（PIMPL 模式）        |
| docker_manager   | `include/docker_manager.h`    | 聚合头文件                    |
| Dockerfile       | `judge-sandbox/Dockerfile`    | 评测沙箱镜像定义              |
| 测试数据         | `data/1-8/`                   | 8 道题标准测试用例            |

---

## 评测引擎详解

### 评测流程

```
用户选择"提交代码"
    │
    ▼
读取 workspace/solution.cpp
    │
    ▼
从 problems 表获取 time_limit / memory_limit / test_data_path
    │
    ▼
JudgeCore.judge()
    ├── ContainerPool.acquire()  → 获取可用容器
    ├── writeSourceCode()        → docker exec 写入 /sandbox/main.cpp
    ├── compile()                → g++ -O2 -std=c++17
    ├── loadTestCases()          → 读取 data/N/1.in .. N.in
    ├── 逐点 run() + compareOutput()
    │     ├── 通过 → 继续下一个测试点
    │     └── 失败 → 记录详情，停止
    └── release() / destroyTemporary()  → 归还或销毁容器
    │
    ▼
INSERT INTO submissions（最终状态，一步到位）
UPDATE users（submission_count++，首次 AC 时 solved_count++）
    │
    ▼
缓存错误上下文 → 供 AI 助手分析
    │
    ▼
显示评测结果（彩色终端输出 + 每个测试点详情）
```

### 容器池策略

| 策略       | 说明                                 |
| ---------- | ------------------------------------ |
| 常驻容器   | 启动时预创建 1 个，评测后 reset 复用 |
| 临时容器   | 常驻忙时按需创建，评测后立即销毁     |
| 最大并发   | 默认 4 个容器                        |
| 惰性初始化 | 第一次评测时才启动容器池             |
| 健康检查   | 检测失联容器并自动重建               |

### 安全隔离机制

| 防护措施           | 效果                     |
| ------------------ | ------------------------ |
| `--network none`   | 禁止网络访问             |
| `--memory=256m`    | 内存上限                 |
| `--pids-limit=64`  | 进程数限制，防 fork 炸弹 |
| `--cap-drop=ALL`   | 移除所有 Linux 特权      |
| `--read-only`      | 容器根文件系统只读       |
| `--tmpfs /sandbox` | 仅 /sandbox 可写 (tmpfs) |
| `USER runner`      | 非特权用户运行           |

---

## 核心类接口详解

### 1. JudgeCore 类（PIMPL 模式）

**头文件**: `include/judge_core.h`  
**源文件**: `src/judge_core.cpp`

#### 关键结构体

```cpp
enum class JudgeResult {
    PENDING, COMPILING, RUNNING,
    ACCEPTED, WRONG_ANSWER,
    TIME_LIMIT_EXCEEDED, MEMORY_LIMIT_EXCEEDED,
    RUNTIME_ERROR, COMPILE_ERROR, SYSTEM_ERROR
};

struct JudgeConfig {
    int time_limit_ms;
    int memory_limit_mb;
    int output_limit_mb;
    std::string language;
};

struct TestCaseResult {
    int case_id;
    JudgeResult result;
    int time_ms;
    int memory_mb;
    std::string output_diff;
};

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

#### 类接口

```cpp
class JudgeCore {
public:
    JudgeCore();
    ~JudgeCore();

    void setConfig(const JudgeConfig& config);
    void setSourceCode(const std::string& src);
    void setTestDataPath(const std::string& path);
    void setWorkDirectory(const std::string& dir);
    void setSecurityConfig(const SecurityConfig& s);

    JudgeReport judge();
    JudgeReport getLastReport() const;
    void saveResult(const JudgeReport& report, int submission_id);
    void cleanup();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
```

---

### 2. SandboxContainer 类

**头文件**: `include/sandbox_container.h`  
**源文件**: `src/sandbox_container.cpp`

| 方法                | 功能                             |
| ------------------- | -------------------------------- |
| `start(image)`      | docker run -d ... sleep infinity |
| `destroy()`         | docker rm -f                     |
| `isAlive()`         | docker inspect 检查运行状态      |
| `writeSourceCode()` | docker exec + stdin 写入源代码   |
| `compile()`         | docker exec g++ 编译             |
| `run()`             | docker exec + timeout 运行程序   |
| `reset()`           | 清理 /sandbox/ 供复用            |

---

### 3. ContainerPool 类

**头文件**: `include/container_pool.h`  
**源文件**: `src/container_pool.cpp`

| 方法                 | 功能                               |
| -------------------- | ---------------------------------- |
| `initialize()`       | 预创建常驻容器                     |
| `acquire()`          | 获取可用容器（优先常驻，否则临时） |
| `release()`          | 归还常驻容器（reset + IDLE）       |
| `destroyTemporary()` | 销毁临时容器                       |
| `healthCheck()`      | 健康检查，重建失联容器             |

---

### 4. User 类（完整实现）

**头文件**: `include/user.h`  
**源文件**: `src/user.cpp`

| 方法                    | 状态            | 功能                          |
| ----------------------- | --------------- | ----------------------------- |
| `login()`               | ✅ 已实现        | 数据库验证 + SHA256           |
| `register_user()`       | ✅ 已实现        | 账号唯一性检查 + 入库         |
| `change_my_password()`  | ✅ 已实现        | 旧密码验证 + 更新             |
| `list_problems()`       | ✅ 已实现        | 带分类的题目列表              |
| `view_problem()`        | ✅ 已实现        | 含知识点的题目详情            |
| `submit_code()`         | ✅ **v1.0 实现** | 评测 + 入库 + 统计 + 错误缓存 |
| `view_my_submissions()` | ✅ **v1.0 实现** | 查询最近 20 条提交记录        |
| `getLastErrorContext()` | ✅ **v1.0 新增** | 获取评测错误上下文供 AI 使用  |
| `getLastReport()`       | ✅ **v1.0 新增** | 获取最近评测报告              |

---

### 5. DatabaseManager 类（增强）

| 方法              | 版本     | 功能                          |
| ----------------- | -------- | ----------------------------- |
| `run_sql()`       | v0.1     | 执行 SQL（静默，失败提示）    |
| `query()`         | v0.1     | 执行查询返回结果集            |
| `escape_string()` | **v1.0** | MySQL 字符串转义，防 SQL 注入 |

---

## 数据库表结构

### submissions 表（完整使用）

```sql
CREATE TABLE submissions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    problem_id INT NOT NULL,
    code TEXT,
    status ENUM('Pending','AC','WA','TLE','MLE','RE','CE') DEFAULT 'Pending',
    submit_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (problem_id) REFERENCES problems(id)
);
```

### problems 表（路径更新）

`test_data_path` 字段指向本机测试数据绝对路径，如 `/home/.../OJ/data/1`。

---

## 测试数据

```
data/
├── 1/   A+B Problem       5 组（正常、零、负数、大数、双负）
├── 2/   爬楼梯            5 组（n=1,2,5,10,45）
├── 3/   买卖股票          4 组（最优、无利、单天、普通）
├── 4/   迷宫探索          4 组（可达、不可达、1x1、绕路）
├── 5/   素数判断          5 组（2、4、997、10^9+7、大合数）
├── 6/   回文串判断        4 组（奇数、非回文、单字符、长回文）
├── 7/   有效的括号        4 组（混合有效、交叉无效、嵌套、错序）
└── 8/   最短路径          4 组（可达、不可达、直连、多路径）
```

每个测试点格式：`N.in`（输入）+ `N.out`（期望输出）

---

## AI 助手增强

### 错误联动机制

评测失败时自动缓存错误上下文，进入 AI 助手后自动附加：

```
用户提交代码 → WA
    │
    ▼
last_error_context_ = "【上次评测结果】
状态: 答案错误 (WA)
通过: 3/5 测试点
第 4 个测试点:
期望: 5
实际: 4"
    │
    ▼
AI 助手收到 problem_info + error_context → 针对性分析
```

### 题目推荐（两阶段调用）

```
用户: "推荐一些题目"
    │
    ▼
AI 返回 [NEED_PROBLEMS] 信号
    │
    ▼
C++ 检测到 → 查询 problems 表 → 携带题库重新调用 AI
    │
    ▼
AI 根据题库推荐具体题目
```

---

## 文件清单

```
OJ/
├── CMakeLists.txt                 # CMake 构建配置
├── init.sql                       # 数据库初始化
├── README.md                      # 项目说明
├── ai/                            # AI 模块
│   ├── ai_service.py              # Python AI 服务
│   ├── requirements.txt           # Python 依赖
│   └── .env                       # API 密钥配置
├── data/                          # ★ 测试数据
│   ├── 1/ ~ 8/                    # 8 道题的测试用例
├── docs/                          # 设计文档
│   ├── code_submission_design.md
│   └── judge_implementation_plan.md
├── judge-sandbox/                 # ★ Docker 镜像
│   └── Dockerfile                 # 评测沙箱定义
├── include/
│   ├── admin.h / admin_view.h
│   ├── ai_client.h                # AI 客户端
│   ├── color_codes.h              # 颜色常量
│   ├── db_manager.h               # 数据库（+escape_string）
│   ├── judge_core.h               # ★ 评测核心（PIMPL）
│   ├── sandbox_container.h        # ★ 沙箱容器
│   ├── container_pool.h           # ★ 容器池
│   ├── docker_manager.h           # ★ 聚合头文件
│   ├── user.h                     # 用户（+评测缓存）
│   ├── user_view.h
│   └── view_manager.h
├── src/
│   ├── admin.cpp / admin_view.cpp
│   ├── ai_client.cpp
│   ├── db_manager.cpp             # +escape_string 实现
│   ├── main.cpp
│   ├── judge_core.cpp             # ★ 评测引擎实现
│   ├── sandbox_container.cpp      # ★ Docker 容器操作
│   ├── container_pool.cpp         # ★ 容器池管理
│   ├── user.cpp                   # ★ submit_code 完整实现
│   ├── user_view.cpp              # ★ 错误上下文传 AI
│   └── view_manager.cpp
├── workspace/
│   └── solution.cpp               # 用户代码工作区
└── History/
    ├── OJ_v0.1.md
    ├── OJ_v0.2.md
    ├── OJ_v0.3.md
    └── OJ_v1.0.md                 # ★ 本文档
```

标注 ★ 的为 v1.0 新增或重大变更文件。

---

## 功能清单

### v1.0 已实现功能

- [x] **Docker 容器化评测引擎**
  - [x] judge-sandbox Docker 镜像（Dockerfile）
  - [x] SandboxContainer 容器生命周期管理
  - [x] ContainerPool 容器池（常驻 1 + 按需临时，最大 4）
  - [x] JudgeCore PIMPL 模式评测主流程
  - [x] 编译 → 逐点测试 → 输出比对
  - [x] 安全隔离（禁网/只读/非特权/资源限制）

- [x] **评测结果展示**
  - [x] AC / WA / TLE / MLE / RE / CE / SE 状态
  - [x] 每个测试点时间和内存详情
  - [x] 彩色终端输出

- [x] **数据库集成**
  - [x] submissions 表自动写入
  - [x] 用户 submission_count / solved_count 统计
  - [x] 提交历史查询（最近 20 条）
  - [x] escape_string() SQL 防注入

- [x] **AI 错误联动**
  - [x] 评测错误自动缓存
  - [x] AI 助手自动获取错误上下文
  - [x] 题目推荐（AI 自主判断 + 按需加载）

- [x] **标准测试数据**
  - [x] 8 道题 35 个测试点
  - [x] 覆盖边界情况

### 可优化方向（v1.1+）

- [ ] Seccomp 系统调用白名单
- [ ] `--cpus` CPU 限制
- [ ] 并发评测（多用户同时提交）
- [ ] 消息队列异步评测
- [ ] 代码下载功能
- [ ] 排行榜

---

## 常用操作速查

### 首次部署

```bash
# 1. 初始化数据库
mysql -u root -p < init.sql

# 2. 构建 Docker 评测镜像（一次性）
cd judge-sandbox && docker build -t judge-sandbox:latest .

# 3. 初始化 AI 环境
cd ai && python3 -m venv venv && source venv/bin/activate
pip install -r requirements.txt
# 配置 .env 中的 API 密钥

# 4. 构建项目
cd build && cmake .. && make

# 5. 运行
./oj_app
```

### 日常使用

```bash
cd build && ./oj_app
```

### 测试账号

- 管理员数据库用户：oj_admin / 090800
- 普通平台用户：自行注册或 test_user / 123456

---

## 交互流程示例

### 提交代码 → 评测 → 查看结果

```
👤 用户模式 - test_user
请输入选项: 2
请输入题目 ID (0 返回): 1

============================ 题目详情 ============================
【题号】     1
【标题】     A+B Problem
【知识点】   模拟
【时间限制】 1000 ms
【内存限制】 128 MB

---------- 操作选项 ----------
 1. 提交代码
 2. AI 助手
 0. 返回用户模式

请输入选项: 1
✅ 已读取 workspace/solution.cpp（140 字节）

⏳ 正在提交评测，请稍候...
[ContainerPool] 常驻容器已就绪: 8ba91b363576

========== 评测结果 ==========
✅ Accepted (AC)
通过测试点: 5 / 5
时间: 12 ms  |  内存: 3 MB

测试点详情:
  #1 AC  8ms  3MB
  #2 AC  7ms  3MB
  #3 AC  9ms  3MB
  #4 AC  12ms  3MB
  #5 AC  8ms  3MB
==============================

按回车键返回...
```

### 评测失败 → AI 助手分析

```
========== 评测结果 ==========
❌ Wrong Answer (WA)
通过测试点: 3 / 5
时间: 10 ms  |  内存: 3 MB

测试点详情:
  #1 AC  8ms  3MB
  #2 AC  7ms  3MB
  #3 AC  9ms  3MB
  #4 WA  10ms  3MB
     期望: 5
     实际: 4
==============================

请输入选项: 2  (进入 AI 助手)

========== AI 助手 ==========
你> 我的代码哪里错了
正在思考...
AI> 根据评测结果，你的代码在第 4 个测试点输出了 4 而期望是 5...
```

---

## 已知限制与注意事项

| 项目        | 说明                          |
| ----------- | ----------------------------- |
| Docker 依赖 | 宿主机必须安装并运行 Docker   |
| 单语言      | 仅支持 C++ 评测               |
| 同步评测    | 提交代码时阻塞等待评测完成    |
| Ctrl+C 残留 | 强制退出可能残留 Docker 容器  |
| 路径绑定    | test_data_path 为本机绝对路径 |

---

*文档版本: v1.0*  
*最后更新: 2026年4月*
