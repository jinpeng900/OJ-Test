# Docker 沙箱与容器池

## SandboxContainer

`SandboxContainer` 管理单个 Docker 容器：

- 启动容器
- 写入源代码
- 编译代码
- 运行程序
- 重置容器状态
- 销毁临时容器

## Docker 安全参数

评测容器使用：

```bash
--network none
--memory=256m
--pids-limit=64
--cap-drop=ALL
--read-only
--tmpfs /sandbox:exec,size=128m,mode=1777
--tmpfs /tmp:exec,size=64m,mode=1777
```

这些配置提供：

- 禁止网络访问
- 限制内存
- 限制进程数量
- 移除 Linux capabilities
- 根文件系统只读
- 只允许 `/sandbox` 和 `/tmp` 写入

## ContainerPool

容器池策略：

```text
请求容器
  |
  +-- 有空闲常驻容器：直接返回
  |
  +-- 无空闲常驻容器：
        |
        +-- 未达到最大数量：创建临时容器
        |
        +-- 达到最大数量：返回 nullptr
```

## v2.0 修复点

- 增加临时容器活跃计数，避免超过最大并发。
- 增加 `/tmp` tmpfs，提升编译和运行兼容性。
- 常驻容器与临时容器分开处理。

