#ifndef DOCKER_MANAGER_H
#define DOCKER_MANAGER_H

/**
 * @brief Docker 容器管理模块聚合头文件
 *
 * 包含所有容器相关的类定义，引入此头文件即可使用完整评测容器功能。
 *
 *   SandboxContainer  - 单个沙箱容器的生命周期与评测操作
 *   ContainerState    - 容器运行状态枚举
 *   ContainerPool     - 容器池（常驻 + 按需扩容）
 */

#include "sandbox_container.h"
#include "container_pool.h"

#endif // DOCKER_MANAGER_H
