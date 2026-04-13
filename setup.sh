#!/bin/bash

# ==========================================================
# OJ 系统一键部署脚本 (setup.sh)
# 用于快速创建文件夹、初始化数据库环境
# ==========================================================

# 1. 创建必要的文件夹
echo "📂 正在创建项目目录..."
mkdir -p build
mkdir -p test_data/1
echo "✅ 已创建 build/ 和 test_data/ 目录。"

# 2. 检查 MySQL 环境并初始化数据库
echo ""
echo "🗄️  正在初始化 MySQL 数据库 (需要 root 权限)..."
if [ -f "init.sql" ]; then
    echo "请输入您的 MySQL root 密码来执行初始化脚本:"
    mysql -u root -p < init.sql
    if [ $? -eq 0 ]; then
        echo "✅ 数据库及用户初始化成功！"
    else
        echo "❌ 数据库初始化失败，请检查密码或 MySQL 服务状态。"
        exit 1
    fi
else
    echo "❌ 错误: 未找到 init.sql 文件，请确保该文件在当前目录下。"
    exit 1
fi

# 3. 提示编译步骤
echo ""
echo "🚀 环境准备就绪！"
echo "您可以按照以下步骤进行编译运行:"
echo "----------------------------------------"
echo "  cd build"
echo "  cmake .."
echo "  make"
echo "  ./oj_app"
echo "----------------------------------------"
