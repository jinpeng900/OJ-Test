#!/bin/bash
# ============================================================
# OJ 系统容器启动入口
#
# 职责：
#   1. 等待 MySQL 完全就绪
#   2. 检查并构建评测沙箱镜像（judge-sandbox:latest）
#   3. 提示 AI 配置状态
#   4. 启动 OJ 主程序
# ============================================================

set -e

# ── 颜色输出 ──────────────────────────────────────────────────
GREEN='\033[32m'
YELLOW='\033[33m'
RED='\033[31m'
CYAN='\033[36m'
RESET='\033[0m'

info()    { echo -e "${GREEN}[OJ]${RESET} $*"; }
warn()    { echo -e "${YELLOW}[OJ]${RESET} $*"; }
error()   { echo -e "${RED}[OJ]${RESET} $*"; }
section() { echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"; echo -e "${CYAN} $*${RESET}"; echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"; }

# ── 1. 等待 MySQL 就绪 ─────────────────────────────────────────
section "正在连接数据库..."

DB_HOST="${OJ_DB_HOST:-localhost}"
DB_PORT="${OJ_DB_PORT:-3306}"
MAX_RETRIES=30

for i in $(seq 1 $MAX_RETRIES); do
    if mysqladmin ping -h "$DB_HOST" -P "$DB_PORT" -u root -pojroot2024 --silent 2>/dev/null; then
        info "数据库连接成功 (${DB_HOST}:${DB_PORT})"
        break
    fi
    if [ "$i" -eq "$MAX_RETRIES" ]; then
        error "数据库连接超时，请检查 oj-db 服务状态"
        exit 1
    fi
    echo -n "."
    sleep 2
done

# ── 2. 检查评测沙箱镜像 ────────────────────────────────────────
section "检查评测沙箱镜像..."

# 判断是否能访问 Docker（通过 socket 共享）
if ! docker info > /dev/null 2>&1; then
    warn "无法连接 Docker，评测功能将不可用"
    warn "请确保启动时挂载了 /var/run/docker.sock"
else
    if docker image inspect judge-sandbox:latest > /dev/null 2>&1; then
        info "评测沙箱镜像已存在：judge-sandbox:latest"
    else
        info "首次运行，正在构建评测沙箱镜像（约需 1-2 分钟）..."
        # judge-sandbox/Dockerfile 已在镜像中，从工作目录构建
        if [ -f "judge-sandbox/Dockerfile" ]; then
            docker build -t judge-sandbox:latest judge-sandbox/ \
                && info "评测沙箱镜像构建完成" \
                || { error "评测沙箱镜像构建失败，评测功能将不可用"; }
        else
            warn "未找到 judge-sandbox/Dockerfile，跳过镜像构建"
        fi
    fi
fi

# ── 3. 检查 AI 配置 ────────────────────────────────────────────
section "检查 AI 配置..."

if [ -n "$DEEPSEEK_API_KEY" ] && [ "$DEEPSEEK_API_KEY" != "your_api_key_here" ]; then
    info "DeepSeek API Key 已配置，AI 助手功能可用"
else
    warn "未检测到有效的 DEEPSEEK_API_KEY"
    warn "AI 助手功能将不可用，其他功能正常运行"
    warn "如需启用 AI，请编辑 ai/.env 文件并重新启动容器"
fi

# ── 4. 显示启动信息 ────────────────────────────────────────────
echo ""
echo -e "${GREEN}╔════════════════════════════════════════╗${RESET}"
echo -e "${GREEN}║      OJ 在线评测系统 已就绪            ║${RESET}"
echo -e "${GREEN}╚════════════════════════════════════════╝${RESET}"
echo ""
echo "  测试账号：test_user / 123456"
echo "  代码路径：workspace/solution.cpp"
echo ""

# ── 5. 启动 OJ 主程序 ──────────────────────────────────────────
exec ./oj_app
