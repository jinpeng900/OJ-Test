# JudgeCore 评测流程

## 职责

`JudgeCore` 是评测主入口，负责：

- 接收源码
- 设置评测配置
- 加载测试数据
- 调用容器编译
- 调用容器运行
- 比对输出
- 汇总 `JudgeReport`

## 核心数据结构

| 结构 | 说明 |
| ---- | ---- |
| `JudgeConfig` | 时间、内存、输出限制、语言 |
| `TestCaseResult` | 单个测试点结果 |
| `JudgeReport` | 最终评测报告 |
| `JudgeResult` | 评测结果枚举 |

## 流程

```text
setConfig()
setSourceCode()
setTestDataPath()
judge()
  |
  +-- acquire container
  +-- write source
  +-- compile
  +-- load test cases
  +-- run each case
  +-- compare output
  +-- release container
  +-- return JudgeReport
```

## 输出比对

当前比对重点是标准输入输出结果。若输出与标准答案不一致，返回 `WA`。

## 当前限制

- 编译命令主要支持 C++17。
- 逐测试点详情没有完整持久化到数据库。
- GUI 提交仍同步等待评测完成。

