#ifndef CONTAINER_POOL_H
#define CONTAINER_POOL_H

#include "sandbox_container.h"
#include <vector>
#include <memory>
#include <mutex> // 保护容器池数据结构的线程安全
#include <string>
using namespace std;

/**
 * @brief 容器池管理器
 *
 * 调度策略：
 *   - 程序启动时预创建 min_size 个常驻容器（预热）
 *   - 评测请求优先从常驻容器中取空闲容器
 *   - 常驻容器全忙时，按需创建临时容器（不加入常驻池）
 *   - 临时容器评测完毕后立即销毁
 *   - 同时存在的容器总数不超过 max_size
 */
class ContainerPool
{
public:
    ContainerPool() = default;
    ~ContainerPool();

    /**
     * @brief 初始化容器池，预创建常驻容器
     * @param min_size 常驻容器数量（建议 1）
     * @param max_size 最大并发容器总数
     * @param image    Docker 镜像名称
     * @return 全部常驻容器启动成功返回 true
     */
    bool initialize(int min_size = 1, int max_size = 4,
                    const string &image = "judge-sandbox:latest");

    /**
     * @brief 获取可用容器
     *
     * 调度逻辑：
     *   1. 优先返回空闲常驻容器（标记为 BUSY）
     *   2. 无空闲常驻容器时，若未达 max_size，创建临时容器
     *   3. 已达上限则返回 nullptr
     *
     * @param is_temporary 输出参数：true 表示临时容器，调用方用完后须调 destroyTemporary()
     * @return 容器指针，失败返回 nullptr
     */
    shared_ptr<SandboxContainer> acquire(bool &is_temporary);

    /**
     * @brief 归还常驻容器
     *
     * 重置容器内文件系统并将状态置回 IDLE，使其可再次被获取。
     * @param container 待归还的常驻容器
     */
    void release(shared_ptr<SandboxContainer> container);

    /**
     * @brief 销毁临时容器
     * @param container 临时容器
     */
    void destroyTemporary(shared_ptr<SandboxContainer> container);

private:
    vector<shared_ptr<SandboxContainer>> resident_; // 常驻容器列表
    mutable mutex mutex_;                           // 保护 resident_ 和相关状态的线程安全
    int max_size_ = 4;
    int temporary_active_ = 0; // 当前存活的临时容器数量
    string image_ = "judge-sandbox:latest";

    // 创建并启动一个新容器
    shared_ptr<SandboxContainer> createContainer();
};

#endif // CONTAINER_POOL_H
