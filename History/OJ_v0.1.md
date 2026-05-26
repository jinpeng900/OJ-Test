# OJ 在线判题系统 v0.1

## 版本概述

**版本号**: v0.1  
**发布日期**: 2025年  
**技术栈**: C++17 / MySQL / CMake 3.10+

v0.1 版本实现了基础的 OJ 系统框架，包括管理员题目管理和用户功能模块。

---

## 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│                   (程序入口点)                           │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│                   ViewManager                           │
│              (主控制器 - 负责模式切换)                     │
│  - start_login_menu()    显示主菜单                      │
│  - clear_screen()        清屏                           │
└─────────────────────┬───────────────────────────────────┘
                      │
          ┌───────────┴───────────┐
          ▼                       ▼
┌──────────────────┐    ┌──────────────────┐
│   AdminView      │    │    UserView      │
│   (管理员界面)    │    │   (用户界面)      │
├──────────────────┤    ├──────────────────┤
│ - start()        │    │ - start()        │
│ - show_menu()    │    │ - show_guest_menu()│
│ - handle_*()     │    │ - show_user_menu() │
└────────┬─────────┘    │ - handle_*()     │
         │              └────────┬─────────┘
         │                       │
         ▼                       ▼
┌──────────────────┐    ┌──────────────────┐
│   Admin          │    │   User           │
│   (业务逻辑)      │    │   (业务逻辑)      │
├──────────────────┤    ├──────────────────┤
│ - add_problem()  │    │ - login()        │
│ - list_all_*()   │    │ - register_user()│
│ - show_*()       │    │ - list_problems()│
└────────┬─────────┘    │ - view_problem() │
         │              │ - submit_code()  │
         │              │ - view_my_sub()  │
         │              │ - change_pwd()   │
         │              └────────┬─────────┘
         │                       │
         └───────────┬───────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                DatabaseManager                          │
│              (MySQL 数据库操作层)                         │
│  - run_sql()              执行 SQL 语句                  │
│  - query()                执行查询并返回结果集            │
│  - execute_sql_file()     从文件执行 SQL                 │
└─────────────────────────────────────────────────────────┘
```

---

## 核心类接口详解

### 1. DatabaseManager 类

**头文件**: `include/db_manager.h`  
**源文件**: `src/db_manager.cpp`

#### 构造函数
```cpp
DatabaseManager(const std::string &host, 
                const std::string &user, 
                const std::string &password, 
                const std::string &dbname = "");
```

#### 方法

| 方法               | 签名                                                  | 功能                     |
| ------------------ | ----------------------------------------------------- | ------------------------ |
| `get_connection`   | `MYSQL* get_connection() const`                       | 获取 MySQL 连接句柄      |
| `run_sql`          | `bool run_sql(const std::string &sql)`                | 执行任意 SQL 并打印结果  |
| `query`            | `vector<map<string,string>> query(const string &sql)` | 执行查询，返回结构化结果 |
| `execute_sql_file` | `void execute_sql_file(const string &filepath)`       | 批量执行 SQL 文件        |

---

### 2. Admin 类

**头文件**: `include/admin.h`  
**源文件**: `src/admin.cpp`

#### 构造函数
```cpp
Admin(DatabaseManager* db);
```

#### 方法

| 方法                  | 签名                                       | 功能                  |
| --------------------- | ------------------------------------------ | --------------------- |
| `add_problem`         | `bool add_problem(const std::string& sql)` | 通过 SQL 发布新题目   |
| `list_all_problems`   | `void list_all_problems()`                 | 表格形式显示所有题目  |
| `show_problem_detail` | `void show_problem_detail(int id)`         | JSON 格式显示题目详情 |

---

### 3. User 类

**头文件**: `include/user.h`  
**源文件**: `src/user.cpp`

#### 构造函数
```cpp
User(DatabaseManager* db);
```

#### 方法

| 方法                  | 签名                                                                                     | 功能           |
| --------------------- | ---------------------------------------------------------------------------------------- | -------------- |
| `login`               | `bool login(const std::string& account, const std::string& password)`                    | 用户登录       |
| `register_user`       | `bool register_user(const std::string& account, const std::string& password)`            | 用户注册       |
| `change_my_password`  | `bool change_my_password(const std::string& old_pwd, const std::string& new_pwd)`        | 修改密码       |
| `list_problems`       | `void list_problems()`                                                                   | 查看题目列表   |
| `view_problem`        | `void view_problem(int id)`                                                              | 查看题目详情   |
| `submit_code`         | `void submit_code(int problem_id, const std::string& code, const std::string& language)` | 提交代码       |
| `view_my_submissions` | `void view_my_submissions()`                                                             | 查看我的提交   |
| `is_logged_in`        | `bool is_logged_in() const`                                                              | 获取登录状态   |
| `get_current_user_id` | `int get_current_user_id() const`                                                        | 获取当前用户ID |
| `get_current_account` | `std::string get_current_account() const`                                                | 获取当前账号   |

---

### 4. ViewManager 类

**头文件**: `include/view_manager.h`  
**源文件**: `src/view_manager.cpp`

#### 构造函数
```cpp
ViewManager();
```

#### 方法

| 方法               | 签名                      | 功能                 |
| ------------------ | ------------------------- | -------------------- |
| `start_login_menu` | `void start_login_menu()` | 启动主菜单（主循环） |
| `clear_screen`     | `void clear_screen()`     | 清屏                 |
| `show_main_menu`   | `void show_main_menu()`   | 显示主菜单           |
| `clear_input`      | `void clear_input()`      | 清空输入缓冲区       |

---

### 5. AdminView 类

**头文件**: `include/admin_view.h`  
**源文件**: `src/admin_view.cpp`

#### 构造函数
```cpp
AdminView();
```

#### 方法

| 方法                   | 签名                          | 功能             |
| ---------------------- | ----------------------------- | ---------------- |
| `start`                | `void start()`                | 启动管理员模式   |
| `show_menu`            | `void show_menu()`            | 显示管理员菜单   |
| `handle_list_problems` | `void handle_list_problems()` | 处理查看题目列表 |
| `handle_show_problem`  | `void handle_show_problem()`  | 处理查看题目详情 |
| `handle_add_problem`   | `void handle_add_problem()`   | 处理添加题目     |
| `clear_input`          | `void clear_input()`          | 清空输入缓冲区   |

---

### 6. UserView 类

**头文件**: `include/user_view.h`  
**源文件**: `src/user_view.cpp`

#### 构造函数
```cpp
UserView();
```

#### 方法

| 方法                      | 签名                             | 功能                   |
| ------------------------- | -------------------------------- | ---------------------- |
| `start`                   | `void start()`                   | 启动用户模式           |
| `clear_screen`            | `void clear_screen()`            | 清屏                   |
| `show_guest_menu`         | `void show_guest_menu()`         | 显示游客菜单（未登录） |
| `show_user_menu`          | `void show_user_menu()`          | 显示用户菜单（已登录） |
| `handle_login`            | `void handle_login()`            | 处理登录               |
| `handle_register`         | `void handle_register()`         | 处理注册               |
| `handle_list_problems`    | `void handle_list_problems()`    | 处理查看题目列表       |
| `handle_view_problem`     | `void handle_view_problem()`     | 处理查看题目详情       |
| `handle_submit_code`      | `void handle_submit_code()`      | 处理提交代码           |
| `handle_view_submissions` | `void handle_view_submissions()` | 处理查看提交记录       |
| `handle_change_password`  | `void handle_change_password()`  | 处理修改密码           |
| `clear_input`             | `void clear_input()`             | 清空输入缓冲区         |

---

## 数据库表结构

### problems 表（题目表）

```sql
CREATE TABLE IF NOT EXISTS problems (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,          -- 题目名称
    description TEXT,                      -- 题目描述
    time_limit INT,                        -- 时间限制(ms)
    memory_limit INT,                      -- 内存限制(MB)
    test_data_path VARCHAR(256),           -- 测试数据路径
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;
```

### users 表（用户表）

```sql
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    account VARCHAR(50) UNIQUE NOT NULL,   -- 登录账号
    password_hash VARCHAR(255) NOT NULL,   -- 密码哈希（SHA256）
    submission_count INT DEFAULT 0,        -- 提交题目数量
    solved_count INT DEFAULT 0,            -- 解决题目数量
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP NULL DEFAULT NULL,
    INDEX idx_account (account),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB;
```

### submissions 表（提交记录表）

```sql
CREATE TABLE IF NOT EXISTS submissions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,                  -- 提交用户ID
    problem_id INT NOT NULL,               -- 题目ID
    code TEXT,                             -- 提交的代码
    status ENUM('Pending', 'AC', 'WA', 'TLE', 'MLE', 'RE', 'CE') DEFAULT 'Pending',
    submit_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (problem_id) REFERENCES problems(id),
    INDEX idx_user_id (user_id),
    INDEX idx_problem_id (problem_id)
) ENGINE=InnoDB;
```

---

## 数据库用户配置

| 用户       | 密码      | 权限                                                                                            | 用途         |
| ---------- | --------- | ----------------------------------------------------------------------------------------------- | ------------ |
| `oj_admin` | `090800`  | SELECT/INSERT/UPDATE/DELETE on OJ.*                                                             | 管理员后台   |
| `oj_user`  | `user123` | SELECT on problems; SELECT/UPDATE(account,password_hash) on users; SELECT/INSERT on submissions | 普通用户前台 |

---

## 工具类

### Color 命名空间（color_codes.h）

**头文件**: `include/color_codes.h`

提供 ANSI 颜色代码：

| 常量             | 颜色 |
| ---------------- | ---- |
| `Color::RESET`   | 重置 |
| `Color::RED`     | 红色 |
| `Color::GREEN`   | 绿色 |
| `Color::YELLOW`  | 黄色 |
| `Color::BLUE`    | 蓝色 |
| `Color::MAGENTA` | 紫色 |
| `Color::CYAN`    | 青色 |
| `Color::WHITE`   | 白色 |

---

## 文件结构

```
OJ/
├── CMakeLists.txt              # CMake 构建配置 (C++17)
├── init.sql                    # 数据库初始化脚本
├── include/
│   ├── admin.h                 # Admin 类头文件
│   ├── admin_view.h            # AdminView 类头文件
│   ├── color_codes.h           # 颜色代码定义
│   ├── db_manager.h            # DatabaseManager 类头文件
│   ├── user.h                  # User 类头文件
│   ├── user_view.h             # UserView 类头文件
│   └── view_manager.h          # ViewManager 类头文件
├── src/
│   ├── admin.cpp               # Admin 类实现
│   ├── admin_view.cpp          # AdminView 类实现
│   ├── db_manager.cpp          # DatabaseManager 类实现
│   ├── main.cpp                # 程序入口
│   ├── user.cpp                # User 类实现
│   ├── user_view.cpp           # UserView 类实现
│   └── view_manager.cpp        # ViewManager 类实现
└── History/
    └── OJ_v0.1.md              # 本文档
```

---

## 功能清单

### 已实现功能

- [x] 管理员模式
  - [x] 查看所有题目列表
  - [x] 查看题目详情
  - [x] 发布新题目（SQL方式）
- [x] 用户模式
  - [x] 用户注册
  - [x] 用户登录
  - [x] 查看题目列表
  - [x] 查看题目详情
  - [x] 提交代码（C++固定）
  - [x] 查看我的提交
  - [x] 修改密码
- [x] CLI 界面优化
  - [x] 清屏功能
  - [x] 绿色分隔线
  - [x] 视图按角色拆分

### 待实现功能（v0.2+）
- [ ] 用户模式与数据库的交互
- [ ] 代码评测核心（编译+运行+判题）
- [ ] 沙箱安全机制
- [ ] 题目标签分类
- [ ] 排行榜功能
- [ ] Docker 部署支持

---

## 常用操作速查

### 初始化数据库
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

### 测试账号
- 管理员：oj_admin / 090800
- 普通用户：test_user / 123456（需先执行 init.sql）

---

*文档版本: v0.1*  
*最后更新: 2025年*
