# ============================================================
# OJ 在线评测系统 - 主应用镜像
# 多阶段构建：
#   Stage 1 (builder) : 编译 C++ 源码
#   Stage 2 (runtime) : 仅包含运行时所需的文件
# ============================================================

# ── Stage 1: 编译 C++ 源码 ────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 安装编译依赖
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    gcc \
    make \
    pkg-config \
    libmysqlclient-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# 复制源码
COPY CMakeLists.txt .
COPY src/       src/
COPY include/   include/

# 编译（Release 模式）
RUN cmake -DCMAKE_BUILD_TYPE=Release . \
    && make -j"$(nproc)"

# ── Stage 2: 运行时镜像 ───────────────────────────────────────
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# 安装运行时依赖：
#   - libmysqlclient21   : MySQL C 客户端库（运行时动态链接）
#   - libssl3            : OpenSSL（SHA256）
#   - python3 / pip      : AI 服务脚本
#   - docker.io          : docker CLI（在容器内调用宿主机 Docker 评测沙箱）
#   - ca-certificates    : HTTPS 请求（DeepSeek API）
RUN apt-get update && apt-get install -y \
    libmysqlclient21 \
    libssl3 \
    python3 \
    python3-pip \
    python3-venv \
    docker.io \
    ca-certificates \
    default-mysql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 从 builder 阶段复制编译好的二进制
COPY --from=builder /build/oj_app .

# 复制 AI 服务脚本和依赖配置
COPY ai/ ai/

# 安装 Python 依赖（使用虚拟环境，与 ai_client.cpp 中的路径匹配）
RUN python3 -m venv ai/venv \
    && ai/venv/bin/pip install --no-cache-dir --upgrade pip \
    && ai/venv/bin/pip install --no-cache-dir -r ai/requirements.txt

# 复制测试数据和工作区（运行时通过 volume 覆盖）
COPY data/           data/
COPY workspace/      workspace/
COPY judge-sandbox/  judge-sandbox/

# 复制启动脚本
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
