# OJ 评测机实现方案文档

## 1. 概述

本文档详细描述基于 Docker 容器化的 OJ 评测机实现方案，包括安全隔离、资源限制、并行评测等核心功能。

---

## 2. 系统架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        OJ 主程序 (C++)                           │
│                    ┌─────────────────────┐                      │
│                    │   JudgeCore 接口    │                      │
│                    │  (judge_core.h)     │                      │
│                    └──────────┬──────────┘                      │
│                               │                                 │
│                               ▼                                 │
│                    ┌─────────────────────┐                      │
│                    │   DockerManager     │                      │
│                    │   (容器管理器)       │                      │
│                    └──────────┬──────────┘                      │
└───────────────────────────────┼─────────────────────────────────┘
                                │
                    ┌───────────┴───────────┐
                    │                       │
                    ▼                       ▼
        ┌──────────────────┐    ┌──────────────────┐
        │   Docker Pool    │    │   Monitor        │
        │   (容器池管理)    │    │   (资源监控)      │
        └────────┬─────────┘    └────────┬─────────┘
                 │                       │
                 ▼                       ▼
        ┌──────────────────┐    ┌──────────────────┐
        │  Docker Container│    │  Stats Collector │
        │  (评测容器 1..N) │    │  (指标收集器)     │
        └──────────────────┘    └──────────────────┘
```

### 2.2 核心组件说明

| 组件            | 职责                   | 技术实现              |
| --------------- | ---------------------- | --------------------- |
| JudgeCore       | 对外接口，封装评测流程 | C++ 类                |
| DockerManager   | 管理容器生命周期       | Docker REST API / CLI |
| ContainerPool   | 容器池管理，并行调度   | 线程池 + 队列         |
| ResourceMonitor | 实时监控资源使用       | Docker Stats API      |
| Sandbox         | 容器内安全限制         | Seccomp + Cap Drop    |

---

## 3. Docker 容器化运行环境

### 3.1 容器镜像设计

#### 基础镜像 Dockerfile

```dockerfile
# judge-sandbox/Dockerfile
FROM ubuntu:22.04

# 安装编译环境
RUN apt-get update && apt-get install -y \
    g++ \
    gcc \
    make \
    time \
    && rm -rf /var/lib/apt/lists/*

# 创建非特权用户
RUN useradd -m -s /bin/bash runner

# 设置工作目录
WORKDIR /sandbox

# 复制沙箱启动脚本
COPY sandbox_runner.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/sandbox_runner.sh

# 切换到非特权用户
USER runner

ENTRYPOINT ["/usr/local/bin/sandbox_runner.sh"]
```

#### 沙箱启动脚本

```bash
#!/bin/bash
# sandbox_runner.sh
# 在容器内执行编译和运行

SOURCE_FILE="/sandbox/main.cpp"
EXECUTABLE="/sandbox/program"
INPUT_FILE="/sandbox/input.txt"
OUTPUT_FILE="/sandbox/output.txt"
ERROR_FILE="/sandbox/error.txt"

# 编译
g++ -O2 -std=c++17 "$SOURCE_FILE" -o "$EXECUTABLE" 2>"$ERROR_FILE"
if [ $? -ne 0 ]; then
    echo "COMPILE_ERROR"
    cat "$ERROR_FILE"
    exit 1
fi

# 运行（使用 timeout 限制时间）
timeout "$TIME_LIMIT" \
    /usr/bin/time -v "$EXECUTABLE" < "$INPUT_FILE" > "$OUTPUT_FILE" 2>"$ERROR_FILE"

EXIT_CODE=$?

if [ $EXIT_CODE -eq 124 ]; then
    echo "TIME_LIMIT_EXCEEDED"
elif [ $EXIT_CODE -ne 0 ]; then
    echo "RUNTIME_ERROR"
    cat "$ERROR_FILE"
else
    echo "SUCCESS"
    cat "$OUTPUT_FILE"
fi
```

### 3.2 容器池管理系统

#### 容器池设计

```cpp
// ContainerPool 类设计
class ContainerPool {
public:
    // 初始化容器池
    void initialize(int min_size, int max_size);
    
    // 获取可用容器
    std::shared_ptr<Container> acquire();
    
    // 释放容器回池
    void release(std::shared_ptr<Container> container);
    
    // 动态扩容
    void expand(int count);
    
    // 健康检查
    void healthCheck();

private:
    std::queue<std::shared_ptr<Container>> available_;
    std::vector<std::shared_ptr<Container>> all_containers_;
    std::mutex mutex_;
    std::condition_variable cv_;
    int min_size_;
    int max_size_;
};
```

#### 并行评测流程

```
┌──────────────┐
│  评测请求队列  │
└──────┬───────┘
       │
       ▼
┌──────────────────┐
│  调度器 (Scheduler)│
│  - 轮询可用容器    │
│  - 负载均衡       │
└──────┬───────────┘
       │
       ├──────────┬──────────┬──────────┐
       ▼          ▼          ▼          ▼
   ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐
   │容器 1│  │容器 2│  │容器 3│  │容器 N│
   └──────┘  └──────┘  └──────┘  └──────┘
```

### 3.3 容器生命周期管理

```cpp
class Container {
public:
    enum State {
        IDLE,       // 空闲
        RUNNING,    // 运行中
        BUSY,       // 繁忙
        ERROR       // 错误
    };
    
    // 创建容器
    bool create(const ContainerConfig& config);
    
    // 启动容器
    bool start();
    
    // 停止容器
    bool stop();
    
    // 销毁容器
    bool destroy();
    
    // 重置容器状态（复用）
    bool reset();
    
    State getState() const;
    std::string getId() const;

private:
    std::string container_id_;
    State state_;
    std::chrono::steady_clock::time_point last_used_;
};
```

---

## 4. 安全权限控制

### 4.1 Docker 安全选项

```cpp
// 容器安全配置
struct SecurityConfig {
    // 禁止网络访问
    bool disable_network = true;
    
    // 只读文件系统
    bool read_only_rootfs = true;
    
    // 禁止特权模式
    bool no_privileged = true;
    
    // 丢弃所有 capabilities
    std::vector<std::string> cap_drop = {"ALL"};
    
    // 添加最小必要 capabilities
    std::vector<std::string> cap_add = {};
    
    // Seccomp 配置文件路径
    std::string seccomp_profile = "/etc/docker/seccomp-default.json";
    
    // AppArmor 配置文件
    std::string apparmor_profile = "docker-default";
    
    // 资源限制
    int max_pids = 50;           // 最大进程数
    int max_open_files = 64;     // 最大打开文件数
};
```

### 4.2 Seccomp 系统调用过滤

```json
// seccomp-default.json
{
    "defaultAction": "SCMP_ACT_ERRNO",
    "architectures": ["SCMP_ARCH_X86_64"],
    "syscalls": [
        {
            "names": [
                "read", "write", "open", "close",
                "mmap", "mprotect", "munmap",
                "brk", "exit", "exit_group",
                "execve", "arch_prctl"
            ],
            "action": "SCMP_ACT_ALLOW"
        },
        {
            "names": [
                "socket", "connect", "bind", "listen",
                "accept", "sendto", "recvfrom"
            ],
            "action": "SCMP_ACT_ERRNO"
        }
    ]
}
```

### 4.3 文件系统隔离

```cpp
// 挂载配置
struct MountConfig {
    // 只读挂载测试数据
    std::string test_data_host;
    std::string test_data_container = "/sandbox/data:ro";
    
    // 临时可写目录（内存文件系统）
    std::string tmpfs_size = "64m";
    std::string tmpfs_path = "/sandbox/tmp";
    
    // 禁止挂载设备
    bool no_devices = true;
};

// Docker run 参数示例
docker run \
    --read-only \
    --tmpfs /tmp:noexec,nosuid,size=64m \
    -v /host/data:/sandbox/data:ro \
    --security-opt seccomp=seccomp-default.json \
    --cap-drop ALL \
    --network none \
    judge-sandbox:latest
```

---

## 5. 资源监控与限制

### 5.1 资源限制配置

```cpp
// 资源限制结构体
struct ResourceLimits {
    // CPU 限制
    float cpu_quota = 1.0;      // CPU 核心数
    int cpu_period = 100000;    // 调度周期 (us)
    
    // 内存限制
    int memory_limit_mb;        // 内存限制 (MB)
    int memory_swap_mb = 0;     // 禁止交换分区
    
    // 时间限制
    int time_limit_ms;          // 时间限制 (毫秒)
    int wall_time_limit_ms;     // 墙上时间限制 (考虑多核)
    
    // 输出限制
    int output_limit_mb = 64;   // 输出文件大小限制
    
    // 进程限制
    int max_processes = 1;      // 最大进程数
};
```

### 5.2 实时监控实现

```cpp
class ResourceMonitor {
public:
    // 开始监控容器
    void startMonitoring(const std::string& container_id);
    
    // 获取实时资源使用
    ResourceUsage getUsage();
    
    // 检查是否超出限制
    bool checkLimits(const ResourceLimits& limits);
    
    // 停止监控
    void stopMonitoring();

private:
    // 解析 Docker stats 输出
    ResourceUsage parseStats(const std::string& stats_json);
    
    // 计算内存使用（RSS）
    int64_t calculateMemoryUsage(const json& stats);
    
    // 计算 CPU 使用率
    float calculateCPUUsage(const json& stats);
};

// 资源使用结构体
struct ResourceUsage {
    int64_t memory_usage_bytes;  // 当前内存使用
    int64_t memory_peak_bytes;   // 内存使用峰值
    float cpu_percent;           // CPU 使用率
    int64_t cpu_time_us;         // CPU 时间 (微秒)
    bool oom_killed;             // 是否被 OOM killer 终止
};
```

### 5.3 监控数据采集

```bash
# Docker stats 命令
docker stats --no-stream --format \
    '{"memory":"{{.MemUsage}}","cpu":"{{.CPUPerc}}","pids":"{{.PIDs}}"}' \
    container_id

# cgroup 直接读取（更精确）
cat /sys/fs/cgroup/memory/docker/<container_id>/memory.usage_in_bytes
cat /sys/fs/cgroup/cpu/docker/<container_id>/cpuacct.usage
cat /sys/fs/cgroup/pids/docker/<container_id>/pids.current
```

---

## 6. 评测结果处理

### 6.1 评测流程

```
┌─────────────┐
│  接收代码    │
└──────┬──────┘
       ▼
┌─────────────┐
│  创建临时目录 │
└──────┬──────┘
       ▼
┌─────────────┐     ┌─────────────┐
│  写入源代码  │────▶│  编译代码    │
└─────────────┘     └──────┬──────┘
                           │ 编译失败?
                           ▼
                    ┌─────────────┐
                    │ 返回 CE 结果 │
                    └─────────────┘
                           │ 编译成功
                           ▼
┌─────────────┐     ┌─────────────┐
│  逐点测试   │◀────│  启动容器    │
└──────┬──────┘     └─────────────┘
       │
       ├──────┬──────┬──────┬──────┐
       ▼      ▼      ▼      ▼      ▼
    ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐
    │测试1│ │测试2│ │测试3│ │... │ │测试N│
    └──┬─┘ └──┬─┘ └──┬─┘ └────┘ └──┬─┘
       │      │      │            │
       └──────┴──────┴────────────┘
                   │
                   ▼
          ┌─────────────┐
          │  汇总结果    │
          └──────┬──────┘
                 ▼
          ┌─────────────┐
          │ 写入数据库   │
          └──────┬──────┘
                 ▼
          ┌─────────────┐
          │ 返回给用户   │
          └─────────────┘
```

### 6.2 数据库写入

```cpp
void JudgeCore::saveResult(const JudgeReport& report, int submission_id) {
    std::string sql = R"(
        UPDATE submissions 
        SET status = ?, 
            time_used = ?, 
            memory_used = ?, 
            error_message = ?,
            passed_cases = ?,
            total_cases = ?,
            judged_at = NOW()
        WHERE id = ?
    )";
    
    db_manager_->execute(sql, {
        resultToString(report.result),
        report.time_used_ms,
        report.memory_used_mb,
        report.error_message,
        report.passed_test_cases,
        report.total_test_cases,
        submission_id
    });
}
```

### 6.3 结果返回格式

```cpp
struct JudgeResponse {
    JudgeResult result;           // 总体结果
    int score;                    // 得分 (百分比)
    int time_used_ms;             // 最大时间使用
    int memory_used_mb;           // 最大内存使用
    std::vector<TestCaseResult> details;  // 每个测试点详情
};

struct TestCaseResult {
    int case_id;                  // 测试点编号
    JudgeResult result;           // 该点结果
    int time_ms;                  // 时间使用
    int memory_mb;                // 内存使用
    std::string output_diff;      // 差异信息 (WA 时)
};
```

---

## 7. 与现有接口的配合

### 7.1 JudgeCore 实现框架

```cpp
// src/judge_core.cpp
#include "judge_core.h"
#include "docker_manager.h"
#include "resource_monitor.h"

class JudgeCore::Impl {
public:
    DockerManager docker_mgr_;
    ResourceMonitor monitor_;
    ContainerPool pool_;
};

JudgeCore::JudgeCore() : impl_(std::make_unique<Impl>()) {
    // 初始化容器池
    impl_->pool_.initialize(2, 10);  // 最小2个，最大10个容器
}

JudgeCore::~JudgeCore() = default;

void JudgeCore::setConfig(const JudgeConfig& config) {
    impl_->config_ = config;
}

void JudgeCore::setSourceCode(const std::string& source_code) {
    impl_->source_code_ = source_code;
}

void JudgeCore::setTestDataPath(const std::string& test_data_path) {
    impl_->test_data_path_ = test_data_path;
}

void JudgeCore::setWorkDirectory(const std::string& work_dir) {
    impl_->work_dir_ = work_dir;
}

JudgeReport JudgeCore::judge() {
    // 1. 获取可用容器
    auto container = impl_->pool_.acquire();
    
    // 2. 准备评测环境
    prepareEnvironment(container);
    
    // 3. 编译代码
    auto compile_result = compile(container);
    if (compile_result != JudgeResult::ACCEPTED) {
        return createReport(compile_result);
    }
    
    // 4. 运行测试
    auto run_result = runTests(container);
    
    // 5. 释放容器
    impl_->pool_.release(container);
    
    // 6. 返回结果
    return run_result;
}
```

### 7.2 调用示例

```cpp
// 使用示例
JudgeCore judge_core;

JudgeConfig config;
config.time_limit_ms = 1000;
config.memory_limit_mb = 128;
config.language = "cpp";

judge_core.setConfig(config);
judge_core.setSourceCode(user_code);
judge_core.setTestDataPath("/home/oj/data/1");
judge_core.setWorkDirectory("/tmp/judge_12345");

JudgeReport report = judge_core.judge();

// 处理结果
switch (report.result) {
    case JudgeResult::ACCEPTED:
        std::cout << "✅ 通过! 时间: " << report.time_used_ms << "ms" << std::endl;
        break;
    case JudgeResult::WRONG_ANSWER:
        std::cout << "❌ 答案错误" << std::endl;
        break;
    case JudgeResult::TIME_LIMIT_EXCEEDED:
        std::cout << "⏱️ 超时" << std::endl;
        break;
    // ...
}
```

---

## 8. 异常处理与错误恢复

### 8.1 异常分类

| 异常类型   | 说明             | 处理策略               |
| ---------- | ---------------- | ---------------------- |
| 编译错误   | 代码语法错误     | 返回 CE，记录错误信息  |
| 运行时错误 | 段错误、异常退出 | 返回 RE，记录退出码    |
| 资源超限   | TLE/MLE          | 强制终止，返回对应结果 |
| 系统错误   | Docker 故障      | 销毁容器，重新创建     |
| 网络错误   | 与容器通信失败   | 重试3次，失败则标记 SE |

### 8.2 错误恢复机制

```cpp
class ErrorRecovery {
public:
    // 容器故障恢复
    bool recoverContainer(std::shared_ptr<Container> container) {
        // 1. 尝试停止容器
        if (!container->stop()) {
            // 强制终止
            system("docker kill " + container->getId());
        }
        
        // 2. 销毁故障容器
        container->destroy();
        
        // 3. 创建新容器替换
        auto new_container = std::make_shared<Container>();
        if (new_container->create(default_config_)) {
            pool_.replace(container, new_container);
            return true;
        }
        return false;
    }
    
    // 评测超时处理
    void handleTimeout(const std::string& container_id) {
        // 强制停止容器
        docker_mgr_.stopContainer(container_id, /*force=*/true);
        
        // 记录超时结果
        last_report_.result = JudgeResult::TIME_LIMIT_EXCEEDED;
    }
};
```

---

## 9. 性能考虑

### 9.1 容器预热

```cpp
// 系统启动时预创建容器
void ContainerPool::initialize(int min_size, int max_size) {
    min_size_ = min_size;
    max_size_ = max_size;
    
    // 预创建最小数量容器
    for (int i = 0; i < min_size; ++i) {
        auto container = createContainer();
        available_.push(container);
        all_containers_.push_back(container);
    }
}
```

### 9.2 容器复用

```cpp
// 评测完成后重置容器而非销毁
bool Container::reset() {
    // 1. 清理临时文件
    execInContainer("rm -rf /sandbox/*");
    
    // 2. 重置状态
    state_ = State::IDLE;
    last_used_ = std::chrono::steady_clock::now();
    
    return true;
}
```

### 9.3 性能指标

| 指标         | 目标值            | 优化措施         |
| ------------ | ----------------- | ---------------- |
| 容器启动时间 | < 500ms           | 镜像精简、预创建 |
| 编译时间     | 正常              | 缓存编译结果     |
| 单点测试     | < 时间限制 + 50ms | 精确计时         |
| 并发评测数   | 10+               | 容器池管理       |
| 内存占用     | < 200MB/容器      | Alpine Linux     |

---

## 10. 部署配置

### 10.1 Docker 安装要求

```bash
# 安装 Docker
sudo apt-get install docker.io

# 配置 Docker
sudo usermod -aG docker $USER

# 创建沙箱镜像
cd /home/oj/judge-sandbox
docker build -t judge-sandbox:latest .
```

### 10.2 系统配置

```bash
# 启用 cgroup 内存控制
sudo grub-edit /etc/default/grub
# GRUB_CMDLINE_LINUX="cgroup_enable=memory swapaccount=1"
sudo update-grub

# 配置 Docker 守护进程
sudo tee /etc/docker/daemon.json <<EOF
{
    "storage-driver": "overlay2",
    "log-driver": "json-file",
    "log-opts": {
        "max-size": "10m",
        "max-file": "3"
    }
}
EOF
```

---

## 11. 总结

本方案通过 Docker 容器化实现安全隔离的评测环境，配合容器池管理和资源监控，能够满足：

1. ✅ **安全隔离** - Docker + Seccomp + Cap Drop 多重防护
2. ✅ **并行评测** - 容器池动态管理，支持多任务并发
3. ✅ **精确监控** - cgroup 级资源监控，准确判定 TLE/MLE
4. ✅ **易于扩展** - 基于现有 JudgeCore 接口，无缝集成

下一步实现计划：
1. 实现 DockerManager 类
2. 实现 ContainerPool 容器池
3. 实现 ResourceMonitor 监控器
4. 编写单元测试
5. 集成到主程序

---

*文档版本: v1.0*  
*创建日期: 2025年*  
*关联文件: [judge_core.h](file://include/judge_core.h), [OJ_v0.2.md](file://History/OJ_v0.2.md)*
