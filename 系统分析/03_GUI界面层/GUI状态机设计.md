# GUI 状态机设计

## 设计背景

v1.x 是终端菜单程序，可以通过 `cin` 阻塞等待输入。v2.0 改为 ImGui 后，程序每帧都要重新绘制界面，因此必须通过状态机管理页面。

## 核心状态

`GUIState` 当前包含：

- `MAIN_MENU`
- `USER_LOGIN`
- `USER_REGISTER`
- `USER_MENU`
- `PROBLEM_LIST`
- `PROBLEM_DETAIL`
- `SUBMIT_RESULT`
- `SUBMISSIONS_LIST`
- `SUBMISSION_DETAIL`
- `CHANGE_PASSWORD`
- `AI_ASSISTANT`
- `ADMIN_LOGIN`
- `ADMIN_MENU`
- `ADMIN_ADD_PROBLEM`

## AppState

`AppState` 是页面之间共享的数据容器，保存：

- 当前页面状态
- 登录输入框
- 密码输入框
- 代码编辑缓冲区
- 当前题目
- 当前提交
- 题目列表
- 提交列表
- 已 AC 题目集合
- AI 聊天记录
- 最近评测报告

## 页面切换原则

- 按钮点击后只修改 `st.state`。
- 页面需要的数据应提前加载到 `AppState`。
- 长耗时任务目前仍同步执行，是后续异步化优化点。
- 视图层不应重复实现业务逻辑，应调用 `User` / `Admin`。

