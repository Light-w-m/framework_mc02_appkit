#pragma once

#include <osal_def.hpp>
#include <common_type.hpp>

namespace appkit::osal
{
    /**
     * @brief 模板化队列类，封装底层队列实现
     * @tparam T 队列项类型，必须为平凡可复制类型
     */
    template <typename T>
    class Queue;

    /**
     * @brief 队列基础类，封装底层队列实现
     */
    class QueueBase final
    {
    public:
        /**
         * @brief 析构函数
         */
        ~QueueBase();

        /**
         * @brief 将项推送到队列
         * @param data 要推送的数据
         * @param timeout 超时时间
         * @return 错误码
         */
        ErrorCode Send(ConstRawData data, const Duration &timeout);

        /**
         * @brief 从队列接收项
         * @param data 接收数据的缓冲区
         * @param timeout 超时时间
         * @return 错误码
         */
        ErrorCode Receive(RawData data, const Duration &timeout);

        /**
         * @brief 获取队列当前项数
         * @return 当前项数
         */
        std::size_t Size() const;

        /**
         * @brief 获取队列最大项数
         * @return 最大项数
         */
        std::size_t Capacity() const;

        /**
         * @brief 获取队列可用项数
         * @return 可用项数
         */
        std::size_t Available() const;

    private:
        QueueId handle_{}; ///< 队列句柄

        /**
         * @brief 友元类声明，允许Queue访问私有成员
         */
        template <typename T>
        friend class Queue;

        /**
         * @brief 构造函数
         * @param item_size 队列项大小
         * @param max_items 队列最大项数
         */
        QueueBase(std::size_t item_size, std::size_t max_items);
    };

    /**
     * @brief 模板化队列类，封装底层队列实现
     * @tparam T 队列项类型，必须为平凡可复制类型
     */
    template <typename T>
    class Queue final
    {
        DECL_COPY_DISABLE(Queue)
        DECL_MOVE_DISABLE(Queue)

    public:
        /**
         * @brief 构造函数
         * @param max_items 队列最大项数
         */
        Queue(std::size_t max_items)
            : base_(sizeof(T), max_items)
        {
        }

        /**
         * @brief 析构函数
         */
        ~Queue() = default;

        /**
         * @brief 将项推送到队列
         * @param item 要推送的项
         * @param timeout 超时时间
         * @return 错误码
         */
        FORCE_INLINE ErrorCode Push(const T &item, const Duration timeout = {})
        {
            return base_.Send(ConstRawData(item), timeout);
        }

        /**
         * @brief 将项推送到队列
         * @param timeout 超时时间
         * @return 错误码
         */
        FORCE_INLINE ErrorCode Pop(T &item, const Duration timeout = {})
        {
            return base_.Receive(RawData(item), timeout);
        }

        /**
         * @brief 将项推送到队列
         * @param timeout 超时时间
         * @return 错误码
         */
        ErrorCode Pop(const Duration &timeout)
        {
            alignas(alignof(T)) std::byte buffer[sizeof(T)];
            return base_.Receive(RawData(buffer, sizeof(T)), timeout);
        }

        /**
         * @brief 获取队列当前项数
         * @return 当前项数
         */
        FORCE_INLINE std::size_t Size() const
        {
            return base_.Size();
        }

        /**
         * @brief 获取队列最大项数
         * @return 最大项数
         */
        FORCE_INLINE std::size_t Capacity() const
        {
            return base_.Capacity();
        }

        /**
         * @brief 获取队列可用项数
         * @return 可用项数
         */
        FORCE_INLINE std::size_t Available() const
        {
            return base_.Available();
        }

    private:
        QueueBase base_; ///< 队列基础对象
    };
} // namespace appkit::osal
