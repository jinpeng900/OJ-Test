#include "../include/container_pool.h"
#include <iostream>

// ----------------------------------------------------------------
// 析构：销毁所有常驻容器
// ----------------------------------------------------------------

ContainerPool::~ContainerPool()
{
    for (auto &c : resident_)
        c->destroy();
}

// ----------------------------------------------------------------
// 内部：创建并启动单个容器
// ----------------------------------------------------------------

std::shared_ptr<SandboxContainer> ContainerPool::createContainer()
{
    auto c = std::make_shared<SandboxContainer>();
    if (c->start(image_))
        return c;
    return nullptr;
}

// ----------------------------------------------------------------
// 初始化：预创建常驻容器
// ----------------------------------------------------------------

bool ContainerPool::initialize(int min_size, int max_size, const std::string &image)
{
    std::lock_guard<std::mutex> lock(mutex_);
    max_size_ = max_size;
    image_ = image;

    for (int i = 0; i < min_size; ++i)
    {
        auto c = createContainer();
        if (!c)
        {
            std::cerr << "[ContainerPool] 第 " << i + 1
                      << " 个常驻容器启动失败" << std::endl;
            return false;
        }
        resident_.push_back(c);
        std::cout << "[ContainerPool] 常驻容器已就绪: "
                  << c->getId().substr(0, 12) << std::endl;
    }
    return true;
}

// ----------------------------------------------------------------
// 获取容器
// ----------------------------------------------------------------

std::shared_ptr<SandboxContainer> ContainerPool::acquire(bool &is_temporary)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 优先使用空闲的常驻容器
    for (auto &c : resident_)
    {
        if (c->getState() == ContainerState::IDLE && c->isAlive())
        {
            c->setState(ContainerState::BUSY);
            is_temporary = false;
            return c;
        }
    }

    // 所有常驻容器繁忙，检查是否可以创建临时容器
    // （临时容器不加入 resident_，所以用 resident_.size() 做简单统计）
    if (static_cast<int>(resident_.size()) >= max_size_)
    {
        std::cerr << "[ContainerPool] 已达最大容器数 "
                  << max_size_ << "，评测请求被拒绝" << std::endl;
        is_temporary = false;
        return nullptr;
    }

    // 按需创建临时容器
    auto c = createContainer();
    if (c)
    {
        c->setState(ContainerState::BUSY);
        is_temporary = true;
        std::cout << "[ContainerPool] 临时容器已创建: "
                  << c->getId().substr(0, 12) << std::endl;
    }
    return c;
}

// ----------------------------------------------------------------
// 归还常驻容器
// ----------------------------------------------------------------

void ContainerPool::release(std::shared_ptr<SandboxContainer> container)
{
    if (!container)
        return;
    container->reset();                        // 清理 /sandbox/ 内容
    container->setState(ContainerState::IDLE); // 置回空闲
}

// ----------------------------------------------------------------
// 销毁临时容器
// ----------------------------------------------------------------

void ContainerPool::destroyTemporary(std::shared_ptr<SandboxContainer> container)
{
    if (container)
        container->destroy();
}

// ----------------------------------------------------------------
// 健康检查：重建失联的常驻容器
// ----------------------------------------------------------------

void ContainerPool::healthCheck()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &c : resident_)
    {
        if (!c->isAlive())
        {
            std::cerr << "[ContainerPool] 容器 "
                      << c->getId().substr(0, 12)
                      << " 失联，正在重建..." << std::endl;
            c->destroy();
            auto fresh = createContainer();
            if (fresh)
                c = fresh;
        }
    }
}

// ----------------------------------------------------------------
// 统计查询
// ----------------------------------------------------------------

int ContainerPool::idleCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    int cnt = 0;
    for (const auto &c : resident_)
        if (c->getState() == ContainerState::IDLE)
            ++cnt;
    return cnt;
}

int ContainerPool::residentCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(resident_.size());
}
