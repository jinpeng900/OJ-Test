# OJ 系统代码提交与历史管理设计方案

> **文档版本**: v1.0
> **创建日期**: 2025-04-20
> **状态**: 设计阶段（未实现）
> **依赖**: ai.md（AI 助手设计）

---

## 目录

1. [设计目标](#一设计目标)
2. [功能一：统一工作区文件机制](#二功能一统一工作区文件机制)
3. [功能二：AI 助手读取工作区代码](#三功能二ai-助手读取工作区代码)
4. [功能三：AI 助手获取数据库上下文](#四功能三ai-助手获取数据库上下文)
5. [功能四：提交记录查看与代码下载](#五功能四提交记录查看与代码下载)
6. [数据库变更](#六数据库变更)
7. [目录结构变更](#七目录结构变更)
8. [接口设计总览](#八接口设计总览)
9. [交互流程总览](#九交互流程总览)
10. [实现优先级与路线图](#十实现优先级与路线图)

---

## 一、设计目标

| 目标          | 说明                                              |
| ------------- | ------------------------------------------------- |
| 用户体验      | 用户始终在一个文件中编写代码，无需频繁切换        |
| 代码持久化    | 提交后的代码保存到数据库，可追溯历史版本          |
| AI 上下文感知 | AI 助手能读取用户当前代码与题目信息，提供精准帮助 |
| 历史管理      | 用户可查看提交列表，按 ID 下载任意历史代码到本地  |

---

## 二、功能一：统一工作区文件机制

### 2.1 核心思路

用户始终编辑单一文件 `workspace/solution.cpp`。提交时读取该文件内容，保存到数据库 `submissions` 表的 `code` 字段，文件本身不受影响。

### 2.2 工作区文件生命周期

```
系统启动
  │
  ├── 检查 workspace/solution.cpp 是否存在
  │     ├── 不存在 → 创建空文件
  │     └── 存在   → 保持不变（上次编辑内容仍在）
  │
  ├── 用户查看题目详情 → 进入题目子菜单
  │
  ├── 选择"提交代码"
  │     ├── 读取 workspace/solution.cpp 内容
  │     ├── 保存代码到数据库 submissions 表
  │     ├── 执行评测
  │     └── 文件内容不变
  │
  ├── 选择"加载上次代码"
  │     ├── 查询该题目的最近一次提交
  │     ├── 有记录 → 将代码写回 workspace/solution.cpp
  │     └── 无记录 → 提示"暂无提交记录"
  │
  └── 退出系统 → 工作区文件保留，下次继续
```

### 2.3 文件操作清单

| 操作                        | 说明                         | 涉及文件                 |
| --------------------------- | ---------------------------- | ------------------------ |
| `read_file(path)`           | 读取文件全部内容到 string    | `workspace/solution.cpp` |
| `write_file(path, content)` | 将 string 写入文件           | `workspace/solution.cpp` |
| `ensure_file_exists(path)`  | 检查文件存在，不存在则创建   | `workspace/solution.cpp` |
| `count_lines(content)`      | 统计代码行数（用于界面提示） | -                        |

### 2.4 题目详情子菜单变更

当前界面：

```
---------- 操作选项 ----------
 1. 提交代码
 2. AI 助手
 0. 返回用户模式
```

变更后：

```
---------- 操作选项 ----------
 1. 提交代码              （读取 workspace/solution.cpp 提交）
 2. 加载上次代码           （从数据库恢复到 workspace/solution.cpp）
 3. AI 助手               （自动携带工作区代码和题目信息）
 0. 返回用户模式
```

### 2.5 UserView 需新增的私有方法

```cpp
// 读取工作区文件内容
std::string read_workspace_file();

// 将代码写入工作区文件
void write_workspace_file(const std::string &code);

// 确保工作区文件存在
void ensure_workspace_exists();

// 处理"加载上次代码"
void handle_load_last_code(int problem_id);
```

### 2.6 关键实现细节

**提交代码流程（修改 `handle_submit_code_with_id`）**：

1. 调用 `read_workspace_file()` 获取代码
2. 如果代码为空，提示用户并返回
3. 调用 `user_obj->submit_code(problem_id, code, "C++")`
4. 代码保存到数据库，文件不受影响

**加载上次代码流程（新增 `handle_load_last_code`）**：

1. 调用 `user_obj->get_last_submission_code(problem_id)` 获取代码
2. 如果为空，提示"暂无提交记录"
3. 调用 `write_workspace_file(code)` 写入工作区
4. 提示"已加载上次提交代码到工作区"

---

## 三、功能二：AI 助手读取工作区代码

### 3.1 核心思路

当用户调用 AI 助手时，C++ 端自动读取 `workspace/solution.cpp` 的内容，作为 `codeContext` 参数传给 AI，使 AI 能看到用户当前写的代码。

### 3.2 数据流

```
workspace/solution.cpp
        │
        v
  C++ 读取文件内容
        │
        v
  AIClient::ask(message, codeContext, problemContext)
        │
        v
  Python ai_service.py 接收 --code 参数
        │
        v
  拼接到 Prompt: 【我的代码】
```
{code}
```

【问题】{message}
        │
        v
  DeepSeek API 调用
        │
        v
  AI 返回针对用户代码的建议
```

### 3.3 handle_ai_assistant 变更

当前实现：

```cpp
string response = ai_client->ask(question, "", "题目ID: " + to_string(problem_id));
//                         问题 ^^^^  代码为空^^^^  只有题目ID
```

变更后：

```cpp
// 读取用户当前代码
string code = read_workspace_file();

// 获取题目详情作为上下文
string problem_context = user_obj->get_problem_info(problem_id);

// 调用 AI，携带代码和题目信息
string response = ai_client->ask(question, code, problem_context);
```

### 3.4 AI 界面提示优化

```
========== AI 助手 ==========
题目 ID: 1
当前工作区代码: 15 行     ← 新增：显示代码行数
输入你的问题 (0 返回):
```

### 3.5 Python 端 SYSTEM_PROMPT 补充

在现有"严师"模式提示词基础上追加：

```
当用户提供代码时：
1. 分析代码的逻辑和潜在问题
2. 指出错误但不直接给出修复后的完整代码
3. 通过提问引导用户自己发现问题
4. 可以建议调试方法或测试用例

用户代码可能是不完整的，这是正常的，请基于现有代码给出建议。
```

---

## 四、功能三：AI 助手获取数据库上下文

### 4.1 核心思路

C++ 端从数据库查询题目详情和用户最近提交记录，拼成 `problemContext` 字符串传给 AI，让 AI 了解题目的完整信息和用户的历史表现。

### 4.2 上下文内容

AI 获取的上下文信息包括：

| 信息         | 来源                    | 示例                                 |
| ------------ | ----------------------- | ------------------------------------ |
| 题目标题     | `problems.title`        | "A+B Problem"                        |
| 题目描述     | `problems.description`  | "Calculate the sum of two integers." |
| 时间限制     | `problems.time_limit`   | "1000 ms"                            |
| 内存限制     | `problems.memory_limit` | "128 MB"                             |
| 最近提交状态 | `submissions.status`    | "WA"                                 |
| 提交次数     | COUNT(submissions)      | "3 次"                               |

### 4.3 上下文格式

传给 AI 的 `problemContext` 格式：

```
【题目信息】
题号: 1
标题: A+B Problem
描述: Calculate the sum of two integers.
时间限制: 1000 ms
内存限制: 128 MB

【用户提交情况】
已提交 3 次，最近状态: WA
```

### 4.4 User 类需新增的方法

```cpp
// 获取题目详情信息（返回格式化字符串，供 AI 使用）
std::string get_problem_info(int problem_id);

// 获取用户在某道题目的最近提交状态信息（返回格式化字符串）
std::string get_problem_submission_info(int problem_id);
```

### 4.5 SQL 查询

```sql
-- 获取题目详情
SELECT id, title, description, time_limit, memory_limit
FROM problems WHERE id = ?;

-- 获取用户在某题目的提交统计
SELECT COUNT(*) as total,
       (SELECT status FROM submissions
        WHERE user_id = ? AND problem_id = ?
        ORDER BY submit_time DESC LIMIT 1) as last_status
FROM submissions
WHERE user_id = ? AND problem_id = ?;
```

### 4.6 安全边界

- AI **只能读取**题目信息和提交统计，不能读取其他用户的代码
- 行级隔离由 `WHERE user_id = current_user_id` 保证
- AI **不能执行**任何数据库写操作
- 所有数据库查询由 C++ 端完成，Python 端不直接连接数据库

---

## 五、功能四：提交记录查看与代码下载

### 5.1 核心思路

用户在"查看我的提交"界面中，可查看所有提交记录列表，输入提交 ID 即可将该次提交的代码下载到本地历史文件夹。

### 5.2 目录结构

```
history/
└── user_{用户ID}/
    └── problem_{题目ID}/
        ├── submit_1_20250420_143022.cpp
        ├── submit_2_20250420_145511.cpp
        └── submit_3_20250421_090015.cpp
```

### 5.3 文件命名规则

```
submit_{提交序号}_{YYYYMMDD}_{HHMMSS}.cpp
```

- 提交序号：同一题目下的第几次提交（从 1 开始）
- 时间戳：提交的精确时间

### 5.4 交互界面

#### 5.4.1 提交记录列表

```
========== 我的提交 ==========
ID    题目ID   题目标题        状态    提交时间
────────────────────────────────────────────────
5     1        A+B Problem     AC      2025-04-20 14:30
4     1        A+B Problem     WA      2025-04-20 14:25
3     2        两数之和        TLE     2025-04-19 09:15
2     1        A+B Problem     RE      2025-04-18 16:40
1     1        A+B Problem     CE      2025-04-18 16:35
────────────────────────────────────────────────
 1. 下载指定提交代码
 2. 加载提交代码到工作区
 0. 返回
请输入选项:
```

#### 5.4.2 下载代码流程

```
选择"1. 下载指定提交代码"
  │
  ├── 提示: 请输入提交 ID (0 返回):
  │
  ├── 输入 ID (如 4)
  │     │
  │     ├── 查询数据库获取代码内容
  │     │
  │     ├── 生成文件名: submit_2_20250420_145511.cpp
  │     │
  │     ├── 创建目录: history/user_1/problem_1/
  │     │
  │     ├── 写入文件
  │     │
  │     └── 提示: ✅ 代码已下载到 history/user_1/problem_1/submit_2_20250420_145511.cpp
  │
  └── 输入 0 → 返回列表
```

#### 5.4.3 加载到工作区流程

```
选择"2. 加载提交代码到工作区"
  │
  ├── 提示: 请输入提交 ID (0 返回):
  │
  ├── 输入 ID (如 5)
  │     │
  │     ├── 查询数据库获取代码内容
  │     │
  │     ├── 写入 workspace/solution.cpp
  │     │
  │     └── 提示: ✅ 已加载到工作区，可在题目详情中继续编辑和提交
  │
  └── 输入 0 → 返回列表
```

### 5.5 User 类需新增的方法

```cpp
// 获取用户的提交列表（返回查询结果，供界面显示）
std::vector<std::map<std::string, std::string>> get_submission_list();

// 获取指定提交的代码内容
std::string get_submission_code(int submission_id);

// 获取用户某题目的最近一次提交代码
std::string get_last_submission_code(int problem_id);

// 下载指定提交代码到文件（返回文件路径）
std::string download_submission_code(int submission_id);

// 将指定提交代码加载到工作区
bool load_submission_to_workspace(int submission_id);
```

### 5.6 UserView 需修改的方法

```cpp
// 修改：从简单的调用 view_my_submissions() 改为完整的交互界面
void handle_view_submissions();

// 新增：处理下载代码
void handle_download_submission();

// 新增：处理加载到工作区
void handle_load_to_workspace();
```

### 5.7 目录创建逻辑

下载代码前需确保目录存在，按以下逻辑逐级创建：

```
history/                              ← 项目根目录下
  └── user_{id}/                      ← 按用户隔离
      └── problem_{id}/               ← 按题目隔离
          └── submit_{n}_{time}.cpp   ← 代码文件
```

C++ 实现使用 `mkdir -p` 等效逻辑：

```cpp
// 伪代码
string dir = "history/user_" + to_string(user_id) + "/problem_" + to_string(problem_id);
system(("mkdir -p " + dir).c_str());
```

---

## 六、数据库变更

### 6.1 现有表结构

`submissions` 表已包含 `code TEXT` 字段，无需修改表结构。

### 6.2 需新增的 SQL 查询

| 用途             | SQL                                                                                                                                                                                              |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 获取提交列表     | `SELECT s.id, s.problem_id, p.title, s.status, s.submit_time FROM submissions s JOIN problems p ON s.problem_id = p.id WHERE s.user_id = ? ORDER BY s.submit_time DESC`                          |
| 获取指定提交代码 | `SELECT code, problem_id FROM submissions WHERE id = ? AND user_id = ?`                                                                                                                          |
| 获取最近提交代码 | `SELECT code FROM submissions WHERE user_id = ? AND problem_id = ? ORDER BY submit_time DESC LIMIT 1`                                                                                            |
| 获取提交统计     | `SELECT COUNT(*) as total, (SELECT status FROM submissions WHERE user_id=? AND problem_id=? ORDER BY submit_time DESC LIMIT 1) as last_status FROM submissions WHERE user_id=? AND problem_id=?` |
| 获取题目信息     | `SELECT id, title, description, time_limit, memory_limit FROM problems WHERE id = ?`                                                                                                             |

### 6.3 数据库权限

`oj_user` 已拥有 `SELECT, INSERT ON OJ.submissions` 权限，足够支撑所有查询和提交操作，无需额外授权。

---

## 七、目录结构变更

### 7.1 新增目录/文件

```
OJ/
├── workspace/
│   └── solution.cpp           ← 新增：用户工作区文件（始终在此编辑）
├── history/                   ← 新增：历史代码下载目录
│   └── user_{id}/
│       └── problem_{id}/
│           └── *.cpp
├── src/                       ← 现有：需修改 ai_client.cpp, user_view.cpp, user.cpp
├── include/                   ← 现有：需修改 user.h, user_view.h
├── ai/                        ← 现有：需修改 ai_service.py
└── ...
```

### 7.2 .gitignore 建议

```
workspace/solution.cpp
history/
```

工作区文件和历史目录为运行时产物，不应纳入版本控制。

---

## 八、接口设计总览

### 8.1 User 类新增接口

| 方法                           | 参数                | 返回值                       | 说明                     |
| ------------------------------ | ------------------- | ---------------------------- | ------------------------ |
| `get_problem_info`             | `int problem_id`    | `string`                     | 获取题目详情格式化字符串 |
| `get_problem_submission_info`  | `int problem_id`    | `string`                     | 获取用户在某题的提交统计 |
| `get_submission_list`          | 无                  | `vector<map<string,string>>` | 获取用户全部提交列表     |
| `get_submission_code`          | `int submission_id` | `string`                     | 获取指定提交的代码       |
| `get_last_submission_code`     | `int problem_id`    | `string`                     | 获取某题最近提交代码     |
| `download_submission_code`     | `int submission_id` | `string`                     | 下载代码到文件，返回路径 |
| `load_submission_to_workspace` | `int submission_id` | `bool`                       | 加载代码到工作区         |

### 8.2 UserView 类新增/修改接口

| 方法                         | 参数                 | 返回值   | 说明                       |
| ---------------------------- | -------------------- | -------- | -------------------------- |
| `read_workspace_file`        | 无                   | `string` | 读取工作区文件             |
| `write_workspace_file`       | `const string &code` | `void`   | 写入工作区文件             |
| `ensure_workspace_exists`    | 无                   | `void`   | 确保工作区文件存在         |
| `handle_load_last_code`      | `int problem_id`     | `void`   | 加载上次代码到工作区       |
| `handle_view_submissions`    | 无                   | `void`   | **修改**：改为完整交互界面 |
| `handle_download_submission` | 无                   | `void`   | 处理下载提交代码           |
| `handle_load_to_workspace`   | 无                   | `void`   | 处理加载代码到工作区       |

### 8.3 AI 相关修改

| 文件            | 修改内容                                                |
| --------------- | ------------------------------------------------------- |
| `user_view.cpp` | `handle_ai_assistant` 中读取工作区代码和题目信息传给 AI |
| `ai_service.py` | SYSTEM_PROMPT 补充代码分析相关指引                      |

---

## 九、交互流程总览

### 9.1 用户主菜单（已登录）

```
👤 用户模式 - test_user
----------------------------------------
 1. 查看题目列表
 2. 查看题目详情
 3. 查看我的提交          ← 修改：改为完整交互界面
 4. 修改密码
 0. 退出登录
----------------------------------------
```

### 9.2 题目详情子菜单（修改后）

```
========== 题目详情 ==========
【题号】 1
【标题】 A+B Problem
...
==============================

---------- 操作选项 ----------
 1. 提交代码              （读取 workspace/solution.cpp）
 2. 加载上次代码           （从数据库恢复到工作区文件）
 3. AI 助手               （携带代码+题目信息）
 0. 返回用户模式
------------------------------
```

### 9.3 AI 助手界面（修改后）

```
========== AI 助手 ==========
题目: A+B Problem (ID: 1)
当前工作区代码: 15 行
已提交 3 次，最近状态: WA

输入你的问题 (0 返回):
> 我的代码为什么 WA 了？

正在思考...

========== AI 回答 ==========
你的代码在处理负数输入时可能存在问题。
你能检查一下当 a 和 b 都是负数时，程序会输出什么吗？
另外，题目要求输出的是 a+b 的结果，你的变量命名
是否有混淆？
==============================
按回车键返回...
```

### 9.4 提交记录界面（修改后）

```
========== 我的提交 ==========
ID    题目        状态    提交时间
────────────────────────────────────
5     A+B Problem AC      2025-04-20 14:30
4     A+B Problem WA      2025-04-20 14:25
3     两数之和    TLE     2025-04-19 09:15

 1. 下载指定提交代码到文件
 2. 加载提交代码到工作区
 0. 返回
请输入选项: 1

请输入提交 ID (0 返回): 4
✅ 代码已下载到 history/user_1/problem_1/submit_2_20250420_145511.cpp
```

---

## 十、实现优先级与路线图

### Phase 1：统一工作区文件（核心基础）

1. 创建 `workspace/` 目录和 `solution.cpp`
2. 实现 `read_workspace_file`、`write_workspace_file`、`ensure_workspace_exists`
3. 修改 `handle_submit_code_with_id`：从工作区读取代码提交
4. 修改题目详情子菜单：新增"加载上次代码"选项
5. 实现 `get_last_submission_code` 和 `handle_load_last_code`
6. 实现 `submit_code`：将代码写入数据库

### Phase 2：AI 上下文增强

1. 实现 `get_problem_info` 和 `get_problem_submission_info`
2. 修改 `handle_ai_assistant`：读取工作区代码和题目信息
3. 更新 `ai_service.py` 的 SYSTEM_PROMPT

### Phase 3：历史记录管理

1. 实现 `get_submission_list`：查询并格式化显示
2. 重写 `handle_view_submissions`：完整的交互界面
3. 实现 `get_submission_code`：获取指定提交代码
4. 实现 `download_submission_code`：下载到 `history/` 目录
5. 实现 `load_submission_to_workspace`：加载到工作区

### 依赖关系

```
Phase 1（工作区）──→ Phase 2（AI增强）──→ Phase 3（历史管理）
     │                                          ↑
     └──────────────────────────────────────────┘
          （Phase 3 的"加载到工作区"依赖 Phase 1）
```

---

## 附录：现有代码对照

| 功能点         | 当前状态                         | 需修改的文件                     |
| -------------- | -------------------------------- | -------------------------------- |
| 提交代码       | `cin` 输入，`submit_code` 待实现 | `user_view.cpp`, `user.cpp`      |
| 查看提交记录   | `view_my_submissions` 待实现     | `user.cpp`, `user_view.cpp`      |
| AI 助手        | 已接入，但无代码上下文           | `user_view.cpp`, `ai_service.py` |
| 题目详情子菜单 | 3 个选项                         | `user_view.cpp`                  |
| 工作区文件     | 不存在                           | 新增实现                         |
| 历史下载       | 不存在                           | 新增实现                         |
