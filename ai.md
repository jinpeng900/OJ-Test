# OJ 智能助手 (AI Assistant) 详细设计思路

本文档旨在描述如何在 OJ 平台中深度集成基于 **DeepSeek-V3** 与 **LangChain** 的智能助手。

---

## 一、 整体架构设计 (Architecture)

系统采用 **“C++ 宿主 + Python AI 微服务”** 的解耦架构：

1.  **C++ 业务层 (Client)**:
    - 负责 CLI 界面交互。
    - 通过 `libcurl` 或 `httplib` 向 Python 服务发起请求。
    - 维护用户的 `session_id`。
2.  **Python AI 服务层 (Service)**:
    - 使用 **FastAPI** 搭建 RESTful 接口。
    - 使用 **LangChain** 管理对话链 (Chains) 和记忆 (Memory)。
    - 调用 **DeepSeek API** 获取模型响应。
3.  **数据层 (Data)**:
    - 将题目背景、报错日志作为上下文输入。

---

## 二、 核心功能实现思路

### 1. 智能记忆管理 (Context Control)
- **Session 隔离**：在 Python 端使用 `dict` 映射 `session_id` 到独立的 `ConversationBufferWindowMemory` 对象，确保用户 A 的代码不会出现在用户 B 的对话中。
- **窗口限制**：设置 `k=5`，仅保留最近 5 轮对话细节，既节省 Token 成本，又避免 AI 因上下文过长而产生幻觉。
- **持久化备份**：定期将对话摘要存入 MySQL 的 `chat_history` 表，实现跨 Session 的长期记忆。

### 2. “严师”模式策略 (Anti-Cheating)
通过 **System Prompt** 进行硬约束，防止 AI 沦为“代写工具”：
- **Prompt 模板**：
  > “你是一位冷酷但博学的算法导师。你的职责是引导用户思考。
  > 1. 严禁提供完整的可运行代码。
  > 2. 只能提供伪代码或不超过 3 行的核心逻辑片段。
  > 3. 必须通过提问（如：‘你觉得这里用队列还是栈更合适？’）来回应用户的求助。”
- **后置校验**：若 AI 返回内容中包含大量代码块，系统自动拦截并提示“AI 正在思考更具启发性的方式”。

### 3. 题目生成器 (Admin Tool)
- **自动化出题**：管理员只需输入“动态规划，难度中等”，AI 自动生成符合数据库结构的 SQL 语句（包含标题、描述、时限、内限）。
- **测试数据生成**：AI 根据题目逻辑，自动生成多组输入输出样例。

---

## 三、 C++ 与 Python 的联动方案

### 方案：HTTP 通信
- **C++ 端**：
  ```cpp
  // 伪代码
  string response = AIClient::ask(session_id, "这段代码为什么段错误了？", current_code);
  ```
- **Python 端**：
  ```python
  @app.post("/chat")
  async def chat(request: ChatRequest):
      memory = get_user_memory(request.session_id)
      return ai_manager.invoke(request.message, memory)
  ```

---

## 四、 安全与性能
- **流式传输 (Streaming)**：支持流式返回，让 C++ 界面能像打印机一样逐字显示 AI 的回答，提升用户体验。
- **敏感词过滤**：对用户输入和 AI 输出进行敏感词检测，确保平台环境健康。
- **API Key 保护**：通过 `.env` 环境文件配置 DeepSeek 密钥，不在代码中硬编码。

---

## 五、 开发路线图 (Roadmap)
1.  **Phase 1**: 搭建 Python FastAPI 基础服务，实现简单的 DeepSeek 调通。
2.  **Phase 2**: 集成 LangChain 记忆管理，实现 Session 隔离。
3.  **Phase 3**: C++ 编写网络请求模块，在菜单中增加“AI 咨询”选项。
4.  **Phase 4**: 优化 System Prompt，上线“严师”模式和题目自动生成功能。
