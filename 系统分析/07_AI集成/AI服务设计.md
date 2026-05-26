# AI 服务设计

## 架构

```text
UserView
  |
  v
AIClient
  |
  v
popen("python ai/ai_service.py ...")
  |
  v
LangChain + DeepSeek API
```

## AIClient

`AIClient` 是 C++ 侧封装，职责包括：

- 检查 Python 脚本是否可用
- 组装问题、代码、题目信息
- 调用 Python 子进程
- 读取 stdout 作为 AI 回答

## ai_service.py

Python 服务负责：

- 读取 `DEEPSEEK_API_KEY`
- 构建系统提示词
- 接收命令行传入的问题和上下文
- 调用 DeepSeek
- 输出回答

## 上下文来源

题目详情页 AI 可接收：

- 题目信息
- 当前代码
- 上次评测结果
- 用户问题

题目列表页 AI 可接收：

- 题库列表
- 用户筛选或推荐问题

## 配置

真实密钥写入：

```text
ai/.env
```

模板文件：

```text
ai/.env.example
```

`.env` 不进入版本库。

