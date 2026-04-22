#include "../include/container_pool.h"
#include <iostream>

using namespace std;

// 析构：销毁所有常驻容器

ContainerPool::~ContainerPool()
{
    for (auto &c : resident_)
        c->destroy();
}

// 创建并启动一个新容器

shared_ptr<SandboxContainer> ContainerPool::createContainer()
{ // 容器在池与调用方之间共享，使用 make_shared 与返回类型保持一致
    auto c = make_shared<SandboxContainer>();
    if (c->start(image_))
        return c;
    return nullptr;
}

// 初始化：预创建常驻容器

bool ContainerPool::initialize(int min_size, int max_size, const string &image)
{
    // lock_guard 在函数结束时自动释放锁，确保线程安全 , lock 保护整个初始化过程，防止多个线程同时调用 initialize 导致竞态条件
    lock_guard<mutex> lock(mutex_);
    max_size_ = max_size;
    temporary_active_ = 0;
    image_ = image;

    for (int i = 0; i < min_size; ++i)
    {
        auto c = createContainer();
        if (!c)
        {
            cerr << "[ContainerPool] 第 " << i + 1
                 << " 个常驻容器启动失败" << endl;
            return false;
        }
        resident_.push_back(c);
        cout << "[ContainerPool] 常驻容器已就绪: "
             << c->getId().substr(0, 12) << endl;
    }
    return true;
}

// 获取可用容器

shared_ptr<SandboxContainer> ContainerPool::acquire(bool &is_temporary)
{
    lock_guard<mutex> lock(mutex_);

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
    // 总量限制 = 常驻容器 + 当前存活临时容器
    int total_active = static_cast<int>(resident_.size()) + temporary_active_;
    if (total_active >= max_size_)
    {
        cerr << "[ContainerPool] 已达最大容器数 "
             << max_size_ << "，评测请求被拒绝" << endl;
        is_temporary = false;
        return nullptr;
    }

    // 按需创建临时容器
    auto c = createContainer();
    if (c)
    {
        c->setState(ContainerState::BUSY);
        ++temporary_active_;
        is_temporary = true;
        cout << "[ContainerPool] 临时容器已创建: "
             << c->getId().substr(0, 12) << endl;
    }
    return c;
}

// 归还常驻容器

void ContainerPool::release(shared_ptr<SandboxContainer> container)
{
    if (!container)
        return;
    container->reset();                        // 清理 /sandbox/ 内容
    container->setState(ContainerState::IDLE); // 置回空闲
}

// 销毁临时容器

void ContainerPool::destroyTemporary(shared_ptr<SandboxContainer> container)
{
    if (!container)
        return;

    // 同一个临时容器只计数一次：destroy() 后 getId() 为空
    bool alive_before_destroy = !container->getId().empty();
    if (alive_before_destroy)
    {
        // 只在锁内更新共享计数；destroy() 会执行外部命令，放到锁外避免长时间占锁。
        lock_guard<mutex> lock(mutex_);
        if (temporary_active_ > 0)
            --temporary_active_;
    }

    if (alive_before_destroy)
        container->destroy();
}
