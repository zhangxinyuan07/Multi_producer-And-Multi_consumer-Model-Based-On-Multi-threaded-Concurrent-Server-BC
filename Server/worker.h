// 定义了一个任务队列和两个角色, 分别为生产者服务员和消费者服务员

#ifndef WORKER_H
#define WORKER_H

#include <list>
#include <iostream>
#include "locker.h"

using namespace std;

// 任务管理者类, 可操作任务中心, 增删任务
// 生产者和消费者操作该类
// T是任务类型, 简单起见会设为int
template <typename T>
class CTaskQueue
{
public:
    list<T> m_task_queue; // 任务队列, 任务都存放在这里
    int m_maxSize;        // 队列最大容量
    Clocker m_lock;       // 互斥量, 保护任务队列
    Ccond m_not_full;     // 条件变量, 指示任务队列不满
    Ccond m_not_empty;    // 条件变量, 指示任务队列不空

    CTaskQueue(int maxSize) : m_maxSize(maxSize){};

    //消费者可调用此函数, 获得队列为空的信息
    bool isEmpty() const
    {
        return m_task_queue.empty();
    }

    //生产者可调用此函数, 获得队列已满的信息
    bool isFull() const
    {
        return m_task_queue.size() == m_maxSize;
    }

    ~CTaskQueue(){};
};

// 生产者服务员
// 用于处理连接到服务器的生产者客户端的请求, 根据其发来的消息, 向任务队列中增加任务
template <typename T>
class CProducerWaiter
{
public:
    CProducerWaiter(){};

    // 把生产者发来的任务加入队列的行为
    // 需要传入一个任务队列, 队列满了就阻塞等待, 不为满则可以添加
    // 还需要传入一个任务内容(简单起见为int类型, 表示任务编号)
    // 这里的行为是: 向任务队列里添加一个编号为item的任务
    void add_task(CTaskQueue<T> &TaskQueue, T item)
    {
        TaskQueue.m_lock.lock(); // 加锁以保护任务队列

        while (TaskQueue.isFull())
        {
            cout << "生产者正在等待任务队列不为满..." << endl;
            TaskQueue.m_not_full.wait(TaskQueue.m_lock.get()); // 生产者等待"任务队列不为满"这一条件发生
        }

        TaskQueue.m_task_queue.push_back(item); // 写入任务

        TaskQueue.m_not_empty.signal(); // 通知消费者任务队列不为空

        TaskQueue.m_lock.unlock(); // 解锁
    }

    ~CProducerWaiter(){};
};

// 消费者服务员
// 用于处理连接到服务器的消费者客户端的请求, 根据其发来的消息, 向任务队列中取出任务
template <typename T>
class CConsumerWaiter
{
public:
    CConsumerWaiter(){};

    // 为消费者取出任务的行为
    // 需要传入一个任务队列, 队列空了就就阻塞等待, 不为空则可以取
    // 这里的行为是: 从任务队列里取走一个任务, 返回该任务
    T consumeProduct(CTaskQueue<T> &TaskQueue)
    {
        T data;
        TaskQueue.m_lock.lock(); // 加锁以保护任务队列

        while (TaskQueue.isEmpty())
        {
            cout << "消费者正在等待任务队列不为空..." << endl;
            TaskQueue.m_not_empty.wait(TaskQueue.m_lock.get()); // 消费者等待"任务队列不为空"这一条件发生
        }

        data = TaskQueue.m_task_queue.front(); // 读取任务
        TaskQueue.m_task_queue.pop_front();    // 队列中删除任务

        TaskQueue.m_not_full.signal(); // 通知生产者任务队列不为满

        TaskQueue.m_lock.unlock(); // 解锁
        return data;
    }

    ~CConsumerWaiter(){};
};

#endif