# Docker 部署

## 推荐方式

v2.0 推荐 Docker Compose：

```bash
xhost +local:docker
cp ai/.env.example ai/.env
docker compose up -d oj-db
docker compose run --rm oj-app
```

没有 AI Key 也能运行主系统，只是 AI 助手不可用。

## 服务

| 服务 | 功能 |
| ---- | ---- |
| `oj-db` | MySQL 8.0 数据库 |
| `oj-app` | C++ ImGui 主程序 |
| `judge-sandbox` | 用户代码评测容器镜像 |

## 首次启动做的事情

- 初始化数据库
- 插入示例题目
- 创建数据库用户
- 构建主程序镜像
- 构建评测沙箱镜像
- 启动 ImGui GUI 窗口

## 常用命令

```bash
docker compose run --rm oj-app
docker compose logs oj-db
docker compose build oj-app
docker compose down
docker compose down -v
```

## 常见问题

| 问题 | 处理 |
| ---- | ---- |
| GUI 不显示 | 检查 X11，执行 `xhost +local:docker` |
| 数据库初始化失败 | `docker compose down -v` 后重试 |
| AI 不可用 | 检查 `ai/.env` 和 API Key |
| 评测容器创建失败 | 检查 Docker Socket 和 `judge-sandbox:latest` |

