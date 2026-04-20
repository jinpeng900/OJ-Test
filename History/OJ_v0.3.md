# OJ 在线判题系统 v0.3

## 版本概述

**版本号**: v0.3  
**发布日期**: 2025年  
**技术栈**: C++17 / MySQL / CMake 3.10+ / OpenSSL / Python3 / LangChain / Docker (规划中)  

v0.3 版本在 v0.2 基础上实现了 AI 助手功能集成、题目分类系统、评测核心接口定义，以及代码提交机制的完整设计方案。

---

## 版本变更日志

### 新增功能

| 功能模块     | 具体功能          | 说明                            |
| ------------ | ----------------- | ------------------------------- |
| **AI 助手**  | DeepSeek API 集成 | C++ 调用 Python 脚本实现        |
| **AI 助手**  | 多轮对话交互      | 输入 0 或 /quit 退出            |
| **AI 助手**  | 工作区文件读取    | 自动读取 workspace/solution.cpp |
| **AI 助手**  | 题目信息感知      | 查询数据库获取题目详情          |
| **题目分类** | 知识点标签        | 动态规划、贪心、搜索等 8 类     |
| **题目列表** | 中文描述题目      | 8 道示例题目                    |
| **评测核心** | 接口定义          | JudgeCore 类接口设计            |
| **代码提交** | 设计方案          | 工作区文件提交机制              |
| **历史下载** | 设计方案          | 提交记录下载功能                |

### 改进优化

| 改进项   | v0.2           | v0.3           |
| -------- | -------------- | -------------- |
| 题目列表 | 仅显示基本信息 | 显示知识点分类 |
| 题目详情 | 仅显示描述     | 显示知识点标签 |
| AI 功能  | 无             | 完整集成       |
| 评测核心 | 无             | 接口定义完成   |

---

## 系统架构

### 3.1 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        main.cpp                                 │
│                   (程序入口点)                                   │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                      ViewManager                                │
│              (主控制器 - 负责模式切换)                            │
└─────────────────────┬───────────────────────────────────────────┘
                      │
          ┌───────────┴───────────┐
          ▼                       ▼
┌──────────────────┐    ┌──────────────────┐
│   AdminView      │    │    UserView      │
│   (管理员界面)    │    │   (用户界面)      │
└────────┬─────────┘    └────────┬─────────┘
         │                       │
         ▼                       ▼
┌──────────────────┐    ┌──────────────────┐
│   Admin          │    │   User           │
│   (业务逻辑)      │    │   (业务逻辑)      │
└────────┬─────────┘    └────────┬─────────┘
         │                       │
         │         ┌─────────────┼─────────────┐
         │         │             │             │
         │         ▼             ▼             ▼
         │    ┌─────────┐   ┌─────────┐  ┌─────────┐
         │    │Database │   │ AIClient│  │JudgeCore│
         │    │Manager  │   │         │  │(接口)   │
         │    └─────────┘   └────┬────┘  └─────────┘
         │                       │
         │              ┌────────┴────────┐
         │              ▼                 ▼
         │        ┌──────────┐      ┌──────────┐
         │        │ai_service│      │  DeepSeek│
         │        │  .py     │      │   API    │
         │        └──────────┘      └──────────┘
         │
         └───────────────────────┬───────────────────────────┘
                                 │
                                 ▼
                    ┌─────────────────────────┐
                    │      MySQL Database     │
                    │  - users                │
                    │  - problems (含category)│
                    │  - submissions          │
                    └─────────────────────────┘
```

### 3.2 新增组件

| 组件       | 文件路径                 | 职责               |
| ---------- | ------------------------ | ------------------ |
| AIClient   | `include/ai_client.h`    | C++ 端 AI 调用接口 |
| ai_service | `ai/ai_service.py`       | Python AI 服务端   |
| JudgeCore  | `include/judge_core.h`   | 评测核心接口定义   |
| Workspace  | `workspace/solution.cpp` | 用户代码工作区     |

---

## 核心类接口详解

### 1. AIClient 类

**头文件**: `include/ai_client.h`  
**源文件**: `src/ai_client.cpp`

#### 接口定义

```cpp
class AIClient {
public:
    AIClient();
    ~AIClient();
    
    // 检查 AI 服务是否可用
    bool isAvailable() const;
    
    // 单轮提问（简化接口）
    std::string ask(const std::string& question);
    
    // 带上下文的提问（完整接口）
    std::string ask(const std::string& question, 
                    const std::string& code,
                    const std::string& problem_info);
    
    // 设置会话 ID
    void setSessionId(const std::string& session_id);

private:
    std::string pythonPath_;       // Python 解释器路径
    std::string pythonScriptPath_; // Python 脚本路径
    std::string sessionId_;        // 会话 ID
};
```

#### 使用示例

```cpp
AIClient ai_client;
if (ai_client.isAvailable()) {
    std::string response = ai_client.ask("帮我检查代码", 
                                          user_code, 
                                          problem_details);
}
```

---

### 2. JudgeCore 类（接口定义）

**头文件**: `include/judge_core.h`

#### 枚举与结构体

```cpp
// 评测结果枚举
enum class JudgeResult {
    PENDING,               // 等待评测
    COMPILING,             // 编译中
    RUNNING,               // 运行中
    ACCEPTED,              // 通过 (AC)
    WRONG_ANSWER,          // 答案错误 (WA)
    TIME_LIMIT_EXCEEDED,   // 超时 (TLE)
    MEMORY_LIMIT_EXCEEDED, // 超内存 (MLE)
    RUNTIME_ERROR,         // 运行时错误 (RE)
    COMPILE_ERROR,         // 编译错误 (CE)
    SYSTEM_ERROR           // 系统错误
};

// 评测配置
struct JudgeConfig {
    int time_limit_ms;      // 时间限制 (毫秒)
    int memory_limit_mb;    // 内存限制 (MB)
    int output_limit_mb;    // 输出限制 (MB)
    int stack_limit_mb;     // 栈空间限制 (MB)
    std::string language;   // 编程语言
};

// 评测报告
struct JudgeReport {
    JudgeResult result;         // 评测结果
    int time_used_ms;           // 实际使用时间
    int memory_used_mb;         // 实际使用内存
    std::string error_message;  // 错误信息
    int passed_test_cases;      // 通过测试点数量
    int total_test_cases;       // 总测试点数量
};
```

#### 类接口

```cpp
class JudgeCore {
public:
    JudgeCore();
    ~JudgeCore();
    
    void setConfig(const JudgeConfig& config);
    void setSourceCode(const std::string& source_code);
    void setTestDataPath(const std::string& test_data_path);
    void setWorkDirectory(const std::string& work_dir);
    
    JudgeReport judge();              // 执行评测
    JudgeReport getLastReport() const; // 获取结果
    void cleanup();                    // 清理临时文件

private:
    JudgeConfig config_;
    std::string source_code_;
    std::string test_data_path_;
    std::string work_dir_;
    JudgeReport last_report_;
};
```

---

### 3. User 类（增强）

**新增方法**:

| 方法            | 签名                        | 功能                   | 状态       |
| --------------- | --------------------------- | ---------------------- | ---------- |
| `list_problems` | `void list_problems()`      | 显示带分类的题目列表   | ✅ 已实现   |
| `view_problem`  | `void view_problem(int id)` | 显示含知识点的题目详情 | ✅ 已实现   |
| `submit_code`   | `void submit_code(...)`     | 提交代码（待实现）     | ⏳ 接口预留 |

---

### 4. UserView 类（增强）

**新增方法**:

| 方法                         | 签名                                       | 功能            | 状态     |
| ---------------------------- | ------------------------------------------ | --------------- | -------- |
| `handle_ai_assistant`        | `void handle_ai_assistant(int problem_id)` | AI 助手多轮对话 | ✅ 已实现 |
| `handle_submit_code_with_id` | `void handle_submit_code_with_id(int id)`  | 题目详情页提交  | ✅ 已实现 |

---

## 数据库表结构

### 4.1 problems 表（更新）

```sql
CREATE TABLE problems (
    id INT PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(255) NOT NULL,
    description TEXT,
    time_limit INT DEFAULT 1000,      -- 毫秒
    memory_limit INT DEFAULT 128,     -- MB
    test_data_path VARCHAR(255),
    category VARCHAR(50),             -- 新增：知识点分类
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### 4.2 submissions 表（预留）

```sql
CREATE TABLE submissions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    problem_id INT NOT NULL,
    code TEXT,                        -- 提交的代码
    language VARCHAR(20) DEFAULT 'cpp',
    status VARCHAR(20),               -- Pending/AC/WA/TLE/MLE/RE/CE
    time_used INT,                    -- 实际用时 (ms)
    memory_used INT,                  -- 实际内存 (MB)
    error_message TEXT,               -- 错误信息
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    judged_at TIMESTAMP NULL,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (problem_id) REFERENCES problems(id)
);
```

---

## 功能清单

### v0.3 已实现功能

- [x] **AI 助手模块**
  - [x] DeepSeek API 集成
  - [x] C++ 调用 Python 脚本
  - [x] 多轮对话交互
  - [x] 工作区文件读取
  - [x] 题目信息感知
  - [x] 虚拟环境隔离

- [x] **题目分类系统**
  - [x] 8 种知识点分类（动态规划、贪心、搜索、数学、字符串、数据结构、图论、模拟）
  - [x] 题目列表显示分类
  - [x] 题目详情显示分类

- [x] **评测核心接口**
  - [x] JudgeCore 类接口定义
  - [x] JudgeResult 枚举
  - [x] JudgeConfig / JudgeReport 结构体

- [x] **工作区机制**
  - [x] workspace/solution.cpp 文件
  - [x] AI 助手自动读取
  - [x] .gitignore 排除

- [x] **开发环境**
  - [x] Git 仓库初始化
  - [x] .gitignore 配置
  - [x] Python 虚拟环境

### 待实现功能（v0.4+）

- [ ] **评测核心实现**
  - [ ] Docker 容器化运行环境
  - [ ] 容器池管理系统
  - [ ] 资源监控与限制
  - [ ] 安全沙箱机制

- [ ] **代码提交功能**
  - [ ] 工作区文件提交到数据库
  - [ ] 提交历史记录
  - [ ] 代码下载功能

- [ ] **评测结果展示**
  - [ ] 实时评测状态
  - [ ] 详细评测报告
  - [ ] 测试点详情

- [ ] **系统优化**
  - [ ] 并发评测支持
  - [ ] 缓存机制
  - [ ] 性能监控

---

## 安全风险与注意事项

### 当前安全风险

| 风险项       | 等级 | 说明                     | 缓解措施             |
| ------------ | ---- | ------------------------ | -------------------- |
| SQL 注入     | 中   | 部分 SQL 使用字符串拼接  | 使用参数化查询       |
| 代码执行     | 高   | 评测功能未实现，暂无沙箱 | 必须实现 Docker 隔离 |
| AI API 密钥  | 中   | .env 文件存储密钥        | 已添加 .gitignore    |
| 文件路径遍历 | 低   | 测试数据路径未验证       | 路径白名单验证       |

### 安全建议

1. **评测沙箱**: 必须使用 Docker 容器隔离用户代码执行
2. **资源限制**: 严格限制 CPU、内存、时间、磁盘使用
3. **网络隔离**: 评测容器禁止网络访问
4. **文件权限**: 评测目录只读，临时目录大小限制
5. **输入验证**: 所有用户输入必须验证和转义

---

## 文件清单

```
OJ/
├── CMakeLists.txt              # CMake 构建配置
├── init.sql                    # 数据库初始化（含分类字段）
├── .gitignore                  # Git 忽略配置
├── README.md                   # 项目说明
├── ai.md                       # AI 功能设计文档
├── ai/                         # AI 模块
│   ├── ai_service.py           # Python AI 服务
│   ├── requirements.txt        # Python 依赖
│   ├── .env.example            # 环境变量示例
│   └── venv/                   # Python 虚拟环境
├── docs/                       # 设计文档
│   ├── code_submission_design.md   # 代码提交方案
│   └── judge_implementation_plan.md # 评测机实现方案
├── include/
│   ├── admin.h
│   ├── admin_view.h
│   ├── ai_client.h             # 新增：AI 客户端
│   ├── color_codes.h
│   ├── db_manager.h
│   ├── judge_core.h            # 新增：评测核心接口
│   ├── user.h
│   ├── user_view.h
│   └── view_manager.h
├── src/
│   ├── admin.cpp
│   ├── admin_view.cpp
│   ├── ai_client.cpp           # 新增：AI 客户端实现
│   ├── db_manager.cpp
│   ├── main.cpp
│   ├── user.cpp                # 更新：题目分类显示
│   ├── user_view.cpp           # 更新：AI 助手集成
│   └── view_manager.cpp
├── workspace/                  # 新增：用户工作区
│   └── solution.cpp            # 用户代码文件
└── History/                    # 版本文档
    ├── OJ_v0.1.md
    ├── OJ_v0.2.md
    └── OJ_v0.3.md              # 本文档
```

---

## 常用操作速查

### 初始化 AI 环境

```bash
cd ai
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
# 复制 .env.example 为 .env 并配置 API 密钥
```

### 初始化/更新数据库

```bash
mysql -u root -p < init.sql
```

### 构建项目

```bash
cd build
rm -rf CMakeCache.txt CMakeFiles
cmake ..
make
```

### 运行程序

```bash
./oj_app
```

---

## 交互流程示例

### AI 助手使用流程

```
👤 用户模式 - zhangsan
----------------------------------------
 1. 查看题目列表
 2. 查看题目详情
 3. 查看我的提交
 4. 修改密码
 0. 退出登录
----------------------------------------
请输入选项: 2
请输入题目 ID (0 返回): 1

[清屏]

============================ 题目详情 ============================
【题号】     1
【标题】     A+B Problem
【知识点】   模拟
【时间限制】 1000 ms
【内存限制】 128 MB
---------------------------- 题目描述 ----------------------------
计算两个整数的和

输入：两个整数 a, b
输出：a + b 的结果
------------------------------------------------------------------
 1. 提交代码
 2. AI 助手
 0. 返回用户模式
------------------------------------------------------------------
请输入选项: 2

[清屏]

========== AI 助手 ==========
题目 ID: 1
输入问题进行对话，输入 0 或 /quit 返回
=============================

你> 帮我检查这段代码有什么错误
正在思考...
AI> 我注意到你的代码中...

你> 时间复杂度是多少
正在思考...
AI> 这段代码的时间复杂度是 O(1)...

你> 0
🔙 返回...
```

---

*文档版本: v0.3*  
*最后更新: 2025年*
