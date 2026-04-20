# OJ 评测沙箱镜像
# 构建命令：docker build -t judge-sandbox:latest .

FROM ubuntu:22.04

# 禁用交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 安装 C++ 编译环境和计时工具
RUN apt-get update && apt-get install -y \
    g++ \
    gcc \
    make \
    time \
    && rm -rf /var/lib/apt/lists/*

# 创建非特权用户，评测代码在此用户下运行
RUN useradd -m -s /bin/bash runner

# 沙箱工作目录（运行时通过 --tmpfs 挂载为内存文件系统）
WORKDIR /sandbox

# 切换到非特权用户
USER runner

# 常驻模式入口：保持容器存活，等待 docker exec 调用
CMD ["sleep", "infinity"]

