#pragma once

#include <common_assert.hpp>
#include <common_type.hpp>
#include <common_cb.hpp>

#include <osal_semaphore.hpp>
#include <osal_queue.hpp>
#include <ramfs.hpp>

#include <crc8.hpp>
#include <lockfree_list.hpp>
#include <lockfree_queue.hpp>

namespace appkit
{
    /**
     * @brief 消息主题
     */
    class Topic final
    {
        /**
         * @brief 锁状态
         */
        enum class LockState : uint8_t
        {
            UNLOCKED = 0,         ///< 未锁定
            LOCKED = 1,           ///< 已锁定
            USE_MUTEX = UINT8_MAX ///< 使用互斥锁
        };

        /**
         * @brief 主题块
         */
        struct Block
        {
            size_t data_size; ///< 数据类型标签

            bool enable_cache;   ///< 是否启用缓存
            bool mult_publisher; ///< 是否支持多发布者

            alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<LockState> state; ///< 锁状态
            LockFreeList subscribers{};                                   ///< 订阅者列表

            RawData cache;      ///< 缓存数据
            osal::Mutex *mutex; ///< 互斥锁
        };

    public:
        /**
         * @brief 主题句柄
         */
        using TopicHandle = Block *;

        /**
         * @brief 主题域
         */
        class Domain final
        {
            RamFs::Dir *domain_{}; ///< 主题域目录

            static RamFs::Dir &GetBusDict(); ///< 获取总线目录

            friend class Topic; ///< 友元类

        public:
            /**
             * @brief 构造函数
             * @param domain 主题域目录
             * @note 如果domain为nullptr，则使用默认主题域
             */
            explicit Domain(RamFs::Dir *domain);

            /**
             * @brief 构造函数
             * @param name 主题域名称
             * @note 如果名称为nullptr或空字符串，则使用默认主题域
             */
            explicit Domain(const char *name);

            /**
             * @brief 获取默认主题域
             * @return 默认主题域
             */
            static Domain &Default();

            /**
             * @brief 获取主题域名称
             * @return 主题域名称
             */
            [[nodiscard]] FORCE_INLINE const char *GetName() const
            {
                return (*domain_).GetName();
            }
        };

        /**
         * @brief 订阅者类型
         */
        enum class SuberType : uint8_t
        {
            SYNC = 0,     ///< 同步订阅者
            ASYNC = 1,    ///< 异步订阅者
            FIFO = 2,     ///< FIFO 订阅者
            CALLBACK = 3, ///< 回调订阅者
        };

        /**
         * @brief 订阅者块
         */
        struct SuberBlock
        {
            SuberType type; ///< 订阅者类型
        };

        /**
         * @brief 同步订阅者块
         */
        struct SyncSuberBlock final : SuberBlock
        {
            RawData data;              ///< 数据存储
            osal::Semaphore semaphore; ///< 信号量
        };

        /**
         * @brief 同步订阅者
         * @tparam DType 数据类型
         */
        template <typename DType>
        class SyncSuber final
        {
            LockFreeList::Node<SyncSuberBlock> block_{SuberType::SYNC, RawData{data_}, osal::Semaphore{}}; ///< 订阅者块节点
            TopicHandle topic_;                                                                            ///< 主题句柄
            DType data_{};                                                                                 ///< 数据存储

        public:
            /**
             * @brief 构造函数
             * @param name 主题名称
             * @param domain 主题域
             * @note 如果domain为nullptr，则使用默认主题域
             */
            FORCE_INLINE explicit SyncSuber(const char *name, const Domain *domain = nullptr)
                : SyncSuber{Topic{WaitTopic(name, Duration::Max(), domain)}}
            {
            }

            /**
             * @brief 构造函数
             * @param topic 主题
             */
            explicit SyncSuber(const Topic topic)
                : topic_{topic.GetHandle()}
            {
                SizeLimitAssert(Assert::SizeLimitMode::EQUAL, topic.block_->data_size, sizeof(DType), "Topic data type size mismatch");

                topic.block_->subscribers.Add(block_);
            }

            /**
             * @brief 析构函数
             */
            ~SyncSuber()
            {
                topic_->subscribers.Delete(block_);
            }

            /**
             * @brief 等待数据
             * @param timeout 超时时间，默认无限等待
             * @return 错误码
             * @note 如果timeout为0，则立即返回
             */
            [[nodiscard]] FORCE_INLINE ErrorCode Wait(const Duration timeout = Duration::Max())
            {
                return (*block_).semaphore.Wait(timeout);
            }

            /**
             * @brief 获取数据引用
             * @return 数据引用
             */
            FORCE_INLINE DType Get()
            {
                return data_;
            }

            /**
             * @brief 获取数据引用（常量）
             * @return 数据引用
             */
            FORCE_INLINE std::add_const_t<DType> Get() const
            {
                return data_;
            }
        };

        /**
         * @brief 异步订阅者状态
         */
        enum class AsyncSuberState : uint8_t
        {
            Idle = 0, ///< 空闲
            Waiting,  ///< 等待中
            Ready,    ///< 准备就绪
        };

        /**
         * @brief 异步订阅者块
         */
        struct AsyncSuberBlock final : SuberBlock
        {
            RawData data;                       ///< 数据存储
            std::atomic<AsyncSuberState> state; ///< 订阅者状态
        };

        /**
         * @brief 异步订阅者
         * @tparam DType 数据类型
         */
        template <typename DType>
        class AsyncSuber final
        {
            LockFreeList::Node<AsyncSuberBlock> block_{SuberType::ASYNC, RawData{data_}, AsyncSuberState::Idle}; ///< 订阅者块节点
            TopicHandle topic_;                                                                                  ///< 主题句柄
            DType data_{};                                                                                       ///< 数据存储

        public:
            /**
             * @brief 构造函数
             * @param name 主题名称
             * @param domain 主题域
             * @note 如果domain为nullptr，则使用默认主题域
             */
            FORCE_INLINE explicit AsyncSuber(const char *name, Domain *domain = nullptr)
                : AsyncSuber{Topic{WaitTopic(name, Duration::Max(), domain)}}
            {
            }

            /**
             * @brief 构造函数
             * @param topic 主题
             */
            explicit AsyncSuber(const Topic topic) : topic_{topic.GetHandle()}
            {
                SizeLimitAssert(Assert::SizeLimitMode::EQUAL, topic.block_->data_size, sizeof(DType), "Topic data type size mismatch");

                topic_->subscribers.Add(block_);
            }

            /**
             * @brief 析构函数
             */
            ~AsyncSuber()
            {
                topic_->subscribers.Delete(block_);
            }

            /**
             * @brief 检查数据是否可用
             * @return 数据是否可用
             */
            [[nodiscard]] FORCE_INLINE bool Available() const
            {
                return block_.GetData().state.load(std::memory_order_acquire) == AsyncSuberState::Ready;
            }

            /**
             * @brief 获取数据引用
             * @return 数据引用
             * @note 获取数据后，状态变为Idle
             */
            DType &Get()
            {
                block_.GetData().state.store(AsyncSuberState::Idle, std::memory_order_release);
                return data_;
            }
        };

        /**
         * @brief FIFO订阅者块
         */
        struct FifoSuberBlock final : SuberBlock
        {
            void *queue{};                                               ///< 队列指针
            void (*func)(void *queue, ConstRawData data, bool in_isr){}; ///< 处理函数
        };

        /**
         * @brief FIFO订阅者
         * @tparam DType 数据类型
         */
        template <typename DType>
        class FifoSuber final
        {
            LockFreeList::Node<FifoSuberBlock> block_{SuberType::FIFO, std::addressof(data_),
                                                      [](void *addr, ConstRawData data, const bool in_isr)
                                                      {
                                                          UNUSED(in_isr);
                                                          auto *queue = static_cast<osal::Queue<DType> *>(addr);
                                                          auto *data_ptr = data.GetData<DType>();
                                                          queue->Push(*data_ptr, Duration::Max());
                                                      }}; ///< 订阅者块节点
            TopicHandle topic_;       ///< 主题句柄
            osal::Queue<DType> data_; ///< 数据队列

        public:
            /**
             * @brief 构造函数
             * @param name 主题名称
             * @param queue_depth 队列深度
             * @param domain 主题域
             * @note 如果domain为nullptr，则使用默认主题域
             */
            FORCE_INLINE explicit FifoSuber(const char *name, uint32_t queue_depth, Domain *domain = nullptr)
                : FifoSuber{Topic{WaitTopic(name, Duration::Max(), domain)}, queue_depth}
            {
            }

            /**
             * @brief 构造函数
             * @param topic 主题
             * @param queue_depth 队列深度
             */
            explicit FifoSuber(const Topic topic, uint32_t queue_depth) : topic_{topic.GetHandle()}, data_{queue_depth}
            {
                topic_->subscribers.Add(block_);
            }

            /**
             * @brief 析构函数
             */
            ~FifoSuber()
            {
                topic_->subscribers.Delete(block_);
            }

            /**
             * @brief 获取队列指针
             * @return 队列指针
             */
            FORCE_INLINE osal::Queue<DType> *operator->()
            {
                return &data_;
            }

            /**
             * @brief 获取队列引用
             * @return 队列引用
             */
            FORCE_INLINE osal::Queue<DType> &operator*()
            {
                return data_;
            }
        };

        using Callback = appkit::Callback<const ConstRawData &>; ///< 回调函数类型

        /**
         * @brief 回调订阅者块
         */
        struct CallbackSuberBlock final : SuberBlock
        {
            Callback func; ///< 回调函数
        };

        /**
         * @brief 回调订阅者
         */
        class CallbackSuber final
        {
            LockFreeList::Node<CallbackSuberBlock> block_{}; ///< 订阅者块节点
            TopicHandle topic_;                              ///< 主题句柄

        public:
            /**
             * @brief 构造函数
             * @param name 主题名称
             * @param func 回调函数
             * @param domain 主题域
             * @note 如果domain为nullptr，则使用默认主题域
             */
            FORCE_INLINE explicit CallbackSuber(const char *name, Callback func, Domain *domain = nullptr)
                : CallbackSuber{Topic{WaitTopic(name, Duration::Max(), domain)}, func}
            {
            }

            /**
             * @brief 构造函数
             * @param topic 主题
             * @param func 回调函数
             */
            explicit CallbackSuber(const Topic topic, Callback func) : block_{SuberType::CALLBACK, func}, topic_{topic.GetHandle()}
            {
                topic_->subscribers.Add(block_);
            }

            /**
             * @brief 析构函数
             */
            ~CallbackSuber()
            {
                topic_->subscribers.Delete(block_);
            }
        };

        /**
         * @brief 构造函数
         */
        Topic() = default;

        /**
         * @brief 构造函数
         * @param handle 主题句柄
         */
        FORCE_INLINE explicit Topic(TopicHandle handle)
            : block_(handle)
        {
            APPKIT_RAISE_IF_NOT(nullptr != handle, "Topic handle is null");
        }

    private:
        /**
         * @brief 构造函数
         * @param topic_name 主题名称
         * @param type 数据类型标签
         * @param domain 主题域
         * @param cache 是否启用缓存
         * @param multi_publisher 是否支持多发布者
         * @note 如果domain为nullptr，则使用默认主题域
         */
        explicit Topic(const char *topic_name, size_t size, const Domain *domain = nullptr, bool cache = false,
                       bool multi_publisher = false);

    public:
        /**
         * @brief 获取主题句柄
         * @return 主题句柄
         */
        [[nodiscard]] FORCE_INLINE TopicHandle GetHandle() const
        {
            return block_;
        }

        /**
         * @brief 创建主题
         * @param topic_name 主题名称
         * @param data_size 数据类型大小
         * @param domain 主题域
         * @param cache 是否启用缓存
         * @param multi_publisher 是否支持多发布者
         * @return 主题
         */
        FORCE_INLINE static Topic CreateTopic(const char *topic_name, size_t data_size, Domain *domain = nullptr, const bool cache = false,
                                              const bool multi_publisher = false)
        {
            return Topic{topic_name, data_size, domain, cache, multi_publisher};
        }

        /**
         * @brief 创建主题
         * @tparam DType 数据类型
         * @param topic_name 主题名称
         * @param domain 主题域
         * @param cache 是否启用缓存
         * @param multi_publisher 是否支持多发布者
         * @return 主题
         * @note 如果domain为nullptr，则使用默认主题域
         * @note DType不能为ConstRawData类型或RawData类型，防止与拷贝构造函数冲突
         */
        template <typename DType>
        FORCE_INLINE static Topic CreateTopic(const char *topic_name, Domain *domain = nullptr, const bool cache = false,
                                              const bool multi_publisher = false)
        {
            return Topic{topic_name, sizeof(DType), domain, cache, multi_publisher};
        }

        /**
         * @brief 锁定主题
         * @param topic 主题
         * @note 如果主题已被锁定，则阻塞等待
         */
        static void Lock(const Topic &topic);

        /**
         * @brief 解锁主题
         * @param topic 主题
         */
        static void Unlock(const Topic &topic);

        /**
         * @brief 等待主题
         * @param topic_name 主题名称
         * @param timeout 超时时间，默认无限等待
         * @param domain 主题域
         * @return 主题句柄
         * @note 如果domain为nullptr，则使用默认主题域
         * @note 如果timeout为0，则立即返回
         */
        static TopicHandle WaitTopic(const char *topic_name, Duration timeout = Duration::Max(),
                                     const Domain *domain = nullptr);

        /**
         * @brief 从中断服务函数中发布数据
         * @param in_isr 是否在中断服务函数中调用
         * @param data 数据
         * @return 错误码
         * @note 如果in_isr为true，则表示在中断服务函数中调用
         */
        [[nodiscard]] ErrorCode PublishFromCallback(bool in_isr, ConstRawData data) const;

        /**
         * @brief 发布数据
         * @param data 数据
         * @return 错误码
         * @note 该函数不可在中断服务函数中调用
         */
        [[nodiscard]] FORCE_INLINE ErrorCode Publish(ConstRawData data) const
        {
            return PublishFromCallback(false, data);
        }

        /**
         * @brief 从中断服务函数中发布数据
         * @tparam DType 数据类型
         * @tparam Mode 大小限制模式，默认等于数据类型大小
         * @param in_isr 是否在中断服务函数中调用
         * @param data 数据
         * @return 错误码
         * @note DType不能为ConstRawData类型，防止与上一个重载函数冲突
         * @note 如果in_isr为true，则表示在中断服务函数中调用
         * @note Mode参数用于限制数据大小，可以是以下值：
         * - SizeLimitMode::Less：数据大小必须小于等于主题数据类型大小
         * - SizeLimitMode::Equal：数据大小必须等于主题数据类型大小
         * - SizeLimitMode::Greater：数据大小必须大于等于主题数据类型大小
         * @note 该函数不可在中断服务函数中调用
         */
        template <typename DType, Assert::SizeLimitMode Mode = Assert::SizeLimitMode::EQUAL>
            requires(!std::is_same_v<ConstRawData, std::decay_t<DType>>)
        FORCE_INLINE ErrorCode PublishFromCallback(bool in_isr, const DType &data) const
        {
            SizeLimitAssert(Mode, block_->data_size, sizeof(DType), "Topic data type size mismatch");
            return PublishFromCallback(in_isr, ConstRawData{data});
        }

        /**
         * @brief 发布数据
         * @tparam DType 数据类型
         * @tparam Mode 大小限制模式，默认等于数据类型大小
         * @param data 数据
         * @return 错误码
         * @note DType不能为ConstRawData类型，防止与上一个重载函数冲突
         * @note Mode参数用于限制数据大小，可以是以下值：
         * - SizeLimitMode::Less：数据大小必须小于等于主题数据类型大小
         * - SizeLimitMode::Equal：数据大小必须等于主题数据类型大小
         * - SizeLimitMode::Greater：数据大小必须大于等于主题数据类型大小
         * @note 该函数不可在中断服务函数中调用
         */
        template <typename DType, Assert::SizeLimitMode Mode = Assert::SizeLimitMode::EQUAL>
            requires(!std::is_same_v<ConstRawData, std::decay_t<DType>>)
        FORCE_INLINE ErrorCode Publish(const DType &data) const
        {
            return PublishFromCallback<DType, Mode>(false, data);
        }

        /**
         * @brief 获取主题名称
         * @return 主题名称
         */
        [[nodiscard]] const char *GetName() const;

        /**
         * @brief 获取主题名称哈希值
         * @return 主题名称哈希值
         */
        [[nodiscard]] const RamFs::String::HashType GetNameHash() const;

    private:
        TopicHandle block_{nullptr}; ///< 主题块
        RamFs::File *file_{nullptr}; ///< 主题文件
    };
}
