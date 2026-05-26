# 远程用户视图设计文档

## 1. 需求概述

实现一个 TCP 服务端，允许多个客户端通过 `nc localhost 8888` 连接，
每个客户端都能看到完整的 `ViewManager` → `UserView` 交互界面（登录、题目列表、提交代码、AI 助手等），
各自独立操作，互不干扰。

## 2. 核心设计：每个连接一个 ViewManager

**不新建 Session、不新建界面类**，直接复用现有 `ViewManager` + `UserView` + `AdminView`。

```
                         ┌──────────────────────────────────────────────┐
                         │               OJServer (8888)                │
                         │                                              │
   nc localhost 8888  ──>│  accept()                                    │
                         │     │                                        │
                         │     ├── 创建 ViewManager(client_fd=3) ─────>│
                         │     │   ├── UserView(fd=3)                   │
                         │     │   │   ├── User(fd=3)                   │
                         │     │   │   └── AIClient                     │
                         │     │   └── AdminView(fd=3)                  │
                         │     │                                        │
                         │     ├── 创建 ViewManager(client_fd=4) ─────>│
                         │     │   ├── UserView(fd=4)                   │
                         │     │   │   ├── User(fd=4)                   │
                         │     │   │   └── AIClient                     │
                         │     │   └── AdminView(fd=4)                  │
                         │     │                                        │
                         │     └── ...                                  │
                         │                                              │
                         │         ↓ 所有 ViewManager 共享              │
                         │              JudgeCore::getSharedPool()      │
                         │              (4 Docker 容器)                 │
                         └──────────────────────────────────────────────┘
```

## 3. 复用现有组件

| 现有组件          | 复用方式                                                                     |
| ----------------- | ---------------------------------------------------------------------------- |
| `ViewManager`     | 构造函数接收 `client_fd`，所有 `cout` 改为 `send(fd)`，`cin` 改为 `recv(fd)` |
| `UserView`        | 同上，从 ViewManager 传入 fd                                                 |
| `AdminView`       | 同上                                                                         |
| `User`            | 无改动，业务逻辑不变                                                         |
| `AIClient`        | 无改动                                                                       |
| `JudgeCore`       | `ContainerPool` 已改为全局共享，所有 ViewManager 共用                        |
| `DatabaseManager` | 每个 ViewManager 内部独立创建连接                                            |

## 4. I/O 抽象

给 `ViewManager`、`UserView`、`AdminView` 添加统一的 socket I/O 能力：

```cpp
class ViewManager {
public:
    // fd = -1 表示本地终端模式（默认）
    explicit ViewManager(int client_fd = -1);
private:
    int client_fd_;
    void sendLine(const std::string& s);   // cout << s << endl
    void sendRaw(const std::string& s);    // cout << s
    std::string recvLine();                // getline(cin, s)
};
```

同理给 `UserView` 和 `AdminView` 添加相同机制。

## 5. 连接生命周期

```
客户端 nc localhost 8888
    │
    ▼ accept() 返回 client_fd
OJServer::handleClient(fd)
    │
    ├── 创建 ViewManager vm(fd)
    ├── vm.start_login_menu()   // 状态机循环
    │   └── 通过 fd 收发数据
    └── 客户端断开 → vm 析构 → close(fd)
```

## 6. 状态机（完全复用现有）

```
[CONNECTED]
    │
    ▼ ViewManager::start_login_menu()
[主菜单] <── 0.返回 ──┐
    │                  │
    ├── 1.管理员 ──> AdminView::start()
    │                  └── 管理员功能...
    │
    └── 2.用户 ──> UserView::start()
                       │
                       ├── 未登录：登录/注册菜单
                       │      ├── 1.登录 ──> [USER_MENU]
                       │      └── 2.注册 ──> 返回登录
                       │
                       └── [USER_MENU] ◄────────────┐
                              │                      │
                              ├── 1.题目列表          │
                              ├── 2.题目详情 ──> [PROBLEM_DETAIL]
                              │      ├── 1.提交代码
                              │      ├── 2.AI 助手
                              │      └── 0.返回 ────┤
                              ├── 3.提交记录          │
                              ├── 4.修改密码          │
                              └── 0.返回 ─────────────┘
```

## 7. 通信协议

服务端→客户端：
- 逐行发送文本，行末 `\n`
- 菜单用分隔线装饰，和 CLI 终端输出一致

客户端→服务端：
- 用户输入的一行文本
- 数字选择菜单项，或字符串输入（账号密码等）

## 8. 提交代码机制

用户选择"提交代码"时，服务端自动读取 `workspace/<user_id>/solution.cpp`：
- 不需要通过 socket 传输代码
- 用户通过其他方式（本地编辑器、VSCode、scp）将代码写入工作区
- TCP 连接只负责交互菜单和显示结果

## 9. 边界情况

| 场景                       | 处理                                                          |
| -------------------------- | ------------------------------------------------------------- |
| 客户端断开连接             | `recvLine()` 返回空字符串，ViewManager 退出循环，析构关闭 fd  |
| 多个客户端同时登录同一账号 | 允许，各自独立 ViewManager + DB 连接，提交记录归该账号        |
| 未登录访问题目             | 禁止，UserView 登录前只显示登录/注册选项                      |
| 容器池已满                 | JudgeCore::acquire() 返回 nullptr，提示"系统繁忙，请稍后重试" |
| 提交代码时工作区文件不存在 | 提示用户先编辑 `workspace/<user_id>/solution.cpp`             |

## 10. 启动流程

```bash
# 终端 1：启动服务端
./oj_app
# 输出：
# [OJServer] TCP 服务端已启动，监听端口 8888

# 终端 2：客户端 1 先编辑代码
echo '#include <bits/stdc++.h>' > workspace/1/solution.cpp

# 终端 3：客户端 1 连接
nc localhost 8888
# 看到 ViewManager 主菜单 -> 选 2 用户 -> 登录 -> 完整 UserView 界面

# 终端 4：客户端 2 连接
nc localhost 8888
# 独立的 ViewManager 实例，互不影响
```

## 11. 代码文件变更

| 操作 | 文件                     | 说明                                             |
| ---- | ------------------------ | ------------------------------------------------ |
| 修改 | `include/view_manager.h` | 添加 `client_fd_`、socket I/O 辅助函数           |
| 修改 | `src/view_manager.cpp`   | `cout/cin` 改为 socket I/O，支持远程模式         |
| 修改 | `include/user_view.h`    | 添加 `client_fd_`、socket I/O 辅助函数           |
| 修改 | `src/user_view.cpp`      | 所有 `cout` → `sendLine()`，`cin` → `recvLine()` |
| 修改 | `include/admin_view.h`   | 同上（可选，管理员暂不开放远程也可）             |
| 修改 | `src/admin_view.cpp`     | 同上（可选）                                     |
| 新建 | `include/oj_server.h`    | TCP 服务端头文件                                 |
| 新建 | `src/oj_server.cpp`      | TCP 服务端：accept → 创建 ViewManager(fd) → 运行 |
| 修改 | `src/main.cpp`           | 启动 OJServer                                    |
| 修改 | `src/judge_core.cpp`     | `ContainerPool` 改为全局共享（已完成）           |
