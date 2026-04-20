"""
OJ AI 助手 - 本地调用版本
通过命令行参数接收输入，返回 AI 回答
"""

import os
import sys
import argparse
from dotenv import load_dotenv

from langchain_core.messages import SystemMessage, HumanMessage
from langchain_deepseek import ChatDeepSeek
from langchain_community.chat_message_histories import ChatMessageHistory

# 加载环境变量
load_dotenv()

# 系统提示词 - "严师"模式
SYSTEM_PROMPT = """你是一位冷酷但博学的算法导师。你的职责是引导用户思考，而不是直接给出答案。

严格遵循以下规则：
1. 严禁提供完整的可运行代码。
2. 只能提供伪代码或不超过 3 行的核心逻辑片段。
3. 必须通过提问来引导用户思考。
4. 当用户询问代码错误时，指出问题所在但不直接修复。
5. 鼓励用户自己调试和优化代码。
"""

# 会话记忆存储
session_memories = {}


def get_or_create_memory(session_id: str):
    """获取或创建会话记忆"""
    if session_id not in session_memories:
        session_memories[session_id] = ChatMessageHistory()
    return session_memories[session_id]


def ask_ai(message: str, session_id: str = "default", code_context: str = None, problem_context: str = None) -> str:
    """向 AI 提问并获取回答"""
    api_key = os.getenv("DEEPSEEK_API_KEY")
    if not api_key:
        return "错误：未设置 DEEPSEEK_API_KEY 环境变量"

    try:
        model = ChatDeepSeek(
            model="deepseek-chat",
            temperature=0.7,
            max_tokens=2048,
            api_key=api_key
        )

        memory = get_or_create_memory(session_id)

        # 构建用户输入
        user_input = message
        if code_context:
            user_input = f"【我的代码】\n```\n{code_context}\n```\n\n【问题】{user_input}"
        if problem_context:
            user_input = f"【题目信息】\n{problem_context}\n\n{user_input}"

        # 构建消息列表
        messages = [SystemMessage(content=SYSTEM_PROMPT)]
        messages.extend(memory.messages)
        messages.append(HumanMessage(content=user_input))

        # 调用模型
        response = model.invoke(messages)

        # 检查响应是否为空
        if not response or not response.content:
            return "错误：AI 返回空响应"

        # 保存到记忆
        memory.add_user_message(user_input)
        memory.add_ai_message(response.content)

        # 限制记忆长度（只保留最近10轮）
        while len(memory.messages) > 20:
            memory.messages = memory.messages[2:]

        return response.content

    except Exception as e:
        import traceback
        error_msg = f"AI 调用出错: {str(e)}\n{traceback.format_exc()}"
        # 将错误输出到 stderr，方便调试
        print(error_msg, file=sys.stderr)
        return f"错误：{str(e)}"


def main():
    parser = argparse.ArgumentParser(description="OJ AI 助手")
    parser.add_argument("--message", type=str, required=True, help="用户问题")
    parser.add_argument("--session", type=str, default="default", help="会话ID")
    parser.add_argument("--code", type=str, help="代码上下文")
    parser.add_argument("--problem", type=str, help="题目上下文")

    args = parser.parse_args()

    response = ask_ai(
        message=args.message,
        session_id=args.session,
        code_context=args.code,
        problem_context=args.problem
    )
    print(response)


if __name__ == "__main__":
    main()
