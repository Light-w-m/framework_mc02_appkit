/**
 * @file common_rw.h
 * @brief 通用读写通信定义
 * @author dusk
 * @date 2025-12-26
 */
#pragma once

#include <macros.hpp>
#include <common_type.hpp>

#include <common_assert.hpp>
#include <common_cb.hpp>
#include <common_time.hpp>

#include <osal_mutex.hpp>
#include <osal_semaphore.hpp>
#include <lockfree_queue.hpp>

#include <atomic>
#include <cstdarg>
#include <cstring>

namespace appkit
{
    /**
     * @brief 操作类型
     * @tparam Args 回调参数类型
     */
    template <typename... Args>
    class Operation final
    {
    public:
        /**
         * @brief 回调函数类型
         */
        using Callback = appkit::Callback<Args...>;

        /**
         * @brief 操作类型
         */
        enum class Type : uint8_t
        {
            NONE = 0, ///< 无操作
            BLOCK,    ///< 阻塞
            POLLING,  ///< 轮询
            CALLBACK  ///< 回调
        };

        /**
         * @brief 轮询状态
         */
        enum class PollingStatus : uint8_t
        {
            READY = 0, ///< 准备就绪
            RUNNING,   ///< 运行中
            DONE       ///< 已完成
        };

    private:
        Type type_{Type::NONE}; ///< 操作类型

        union Data
        {
            Callback *callback{nullptr}; ///< 回调函数指针

            struct
            {
                osal::Semaphore *semaphore; ///< 信号量指针
                Duration timeout;           ///< 超时时间，单位毫秒
            } sem_info;

            PollingStatus status; ///< 轮询状态

            /**
             * @brief 默认构造函数
             */
            explicit Data() : callback(nullptr) {}

            Data(const Data &other)
            {
                std::memcpy((void *)this, (void *)&other, sizeof(Data));
            }

            Data(Data &&other) noexcept
            {
                std::memcpy((void *)this, (void *)&other, sizeof(Data));
            }

            Data &operator=(const Data &other)
            {
                if (this != &other)
                {
                    std::memcpy((void *)this, (void *)&other, sizeof(Data));
                }
                return *this;
            }
        } data_{}; ///< 操作数据

    public:
        /**
         * @brief 默认构造函数
         */
        Operation() = default;

        /**
         * @brief 构造函数 - 回调类型
         * @param callback 回调函数
         */
        explicit Operation(Callback &callback) : type_{Type::CALLBACK}
        {
            data_.callback = &callback;
        }

        /**
         * @brief 构造函数 - 阻塞类型
         * @param semaphore 信号量
         * @param timeout 超时时间，单位毫秒
         */
        explicit Operation(osal::Semaphore &semaphore, Duration timeout) : type_{Type::BLOCK}
        {
            data_.sem_info.semaphore = &semaphore;
            data_.sem_info.timeout = timeout;
        }

        /**
         * @brief 构造函数 - 轮询类型
         * @param status 轮询状态
         */
        explicit Operation(PollingStatus status) : type_{Type::POLLING}
        {
            data_.status = status;
        }

        Operation(const Operation &other) = default;
        Operation(Operation &&other) noexcept = default;

        Operation &operator=(const Operation &other) = default;
        Operation &operator=(Operation &&other) noexcept = default;

        /**
         * @brief 获取操作类型
         * @return 操作类型
         */
        FORCE_INLINE Type GetType() const
        {
            return type_;
        }

        /**
         * @brief 更新操作状态
         * @tparam ATypes 回调参数类型
         * @param in_isr 是否在中断中调用
         * @param args 回调参数
         * @note 如果操作类型为阻塞，则释放信号量
         * @note 如果操作类型为轮询，则将状态更新为已完成
         * @note 如果操作类型为回调，则调用回调函数
         */
        template <typename... ATypes>
            requires(sizeof...(ATypes) == sizeof...(Args) && (std::is_convertible_v<ATypes, Args> && ...))
        void UpdateStatus(bool in_isr, ATypes &&...args)
        {
            switch (type_)
            {
            case Type::BLOCK:
            {
                data_.sem_info.semaphore->PostFromCallback(in_isr);
                break;
            }
            case Type::POLLING:
            {
                data_.status = PollingStatus::DONE;
                break;
            }
            case Type::CALLBACK:
            {
                data_.callback->Call(in_isr, std::forward<ATypes>(args)...);
                break;
            }
            case Type::NONE:
            {
                break;
            }
            default:
            {
                APPKIT_RAISE_FROM_CALLBACK(in_isr, "Invalid operation type");
            }
            }
        }

        /**
         * @brief 标记操作为运行中
         * @note 仅当操作类型为轮询时有效
         */
        void MarkRunning()
        {
            if (Type::POLLING == type_)
            {
                data_.status = PollingStatus::RUNNING;
            }
        }

        /**
         * @brief 测试并设置操作为就绪状态
         * @return 错误码
         * @note 仅当操作类型为轮询时有效
         */
        ErrorCode TestAndSetPolling()
        {
            if (Type::POLLING == type_)
            {
                if (PollingStatus::DONE == data_.status)
                {
                    data_.status = PollingStatus::READY;
                    return ErrorCode::OK;
                }
                else
                {
                    return ErrorCode::BUSY;
                }
            }
            return ErrorCode::NOT_SUPPORTED;
        }

        /**
         * @brief 等待操作完成
         * @return 错误码
         * @note 仅当操作类型为阻塞时有效
         */
        ErrorCode Wait() const
        {
            if (Type::BLOCK == type_)
            {
                return data_.sem_info.semaphore->Wait(data_.sem_info.timeout);
            }
            return ErrorCode::NOT_SUPPORTED;
        }
    };

    class ReadPort;
    class WritePort;

    using ReadOperation = Operation<ErrorCode>;
    using ReadFunc = ErrorCode (*)(ReadPort &);

    using WriteOperation = Operation<ErrorCode>;
    using WriteFunc = ErrorCode (*)(WritePort &);

    /**
     * @brief 读信息块
     */
    struct ReadInfo final
    {
        RawData data{};            ///< 待写数据内存块
        ReadOperation operation{}; ///< 读操作
    };

    /**
     * @brief 写信息块
     */
    struct WriteInfo final
    {
        ConstRawData data{};        ///< 待读数据内存块
        WriteOperation operation{}; ///< 写操作
    };

    /**
     * @brief 读端口类
     */
    class ReadPort final
    {
        DECL_COPY_DISABLE(ReadPort)

    public:
        /**
         * @brief 读取状态
         */
        enum class State : uint8_t
        {
            IDLE = 0, ///< 空闲
            PENDING,  ///< 挂起
            EVENT     ///< 事件
        };

        ReadFunc func_{};                       ///< 读取函数
        LockFreeQueue<uint8_t> *buffer_{};      ///< 缓存队列
        size_t read_size_{0};                   ///< 上次读取大小
        ReadInfo info_{};                       ///< 当前读信息块
        std::atomic<State> state_{State::IDLE}; ///< 读取状态

        /**
         * @brief 构造函数
         * @param buffer_size 缓存大小，默认为128字节
         * @note 如果buffer_size为0，则不使用缓存
         * @note 如果buffer_size大于0，则分配一个环形队列作为缓存
         * @note 如果内存分配失败，会触发断言
         */
        explicit ReadPort(size_t buffer_size = 128);

        /**
         * @brief 注册读取函数
         * @param func 读取函数
         * @return 读取端口引用
         * @note 该函数用于注册一个读取函数，以便在读取数据时调用
         * @note 该函数返回对当前读取端口的引用，以便进行链式调用
         */
        ReadPort &operator=(ReadFunc func);

        /**
         * @brief 检查读取端口是否可用
         * @return 如果可用返回true，否则返回false
         * @note 仅当注册了读取函数时，返回true
         */
        [[nodiscard]] FORCE_INLINE bool Readable() const
        {
            return func_ != nullptr;
        }

        /**
         * @brief 完成读取操作
         * @param in_isr 是否在中断中调用
         * @param ret 读取结果
         * @param info 读取信息块
         * @param size 实际读取大小
         * @note 该函数会更新读取状态，并根据操作类型进行相应处理
         */
        void Finish(bool in_isr, ErrorCode ret, ReadInfo &info, uint32_t size);

        /**
         * @brief 读取数据
         * @param data 待读数据内存块
         * @param operation 读操作
         * @return 错误码
         * @note 如果读取端口不可用，返回ErrorCode::NotSupported
         * @note 如果读取端口已被占用，返回ErrorCode::Busy
         * @note 如果缓存中有足够的数据，直接从缓存中读取数据
         * @note 如果缓存中没有足够的数据，调用注册的读取函数进行读取
         * @note 如果读取操作类型为阻塞，且数据未能立即读取完成，则等待数据读取完成
         */
        ErrorCode operator()(RawData data, ReadOperation &operation);

        /**
         * @brief 处理挂起的读取操作
         * @param in_isr 是否在中断中调用
         * @note 该函数会检查是否有挂起的读取操作，如果有且缓存中有足够的数据，则完成读取操作
         * @note 该函数通常在中断中调用
         */
        void ProcessPendingReads(bool in_isr);

        /**
         * @brief 标记操作为运行中
         * @note 仅当操作类型为轮询时有效
         */
        FORCE_INLINE static void MarkRunning(ReadInfo &info)
        {
            info.operation.MarkRunning();
        }
    };

    /**
     * @brief 写端口类
     */
    class WritePort final
    {
        DECL_COPY_DISABLE(WritePort)

    public:
        /**
         * @brief 写入状态
         */
        enum class State : uint8_t
        {
            LOCK = 0, ///< 锁定
            UNLOCK,   ///< 解锁
        };

        std::atomic<State> state_{State::UNLOCK}; ///< 写入状态
        LockFreeQueue<WriteInfo> *info_buffer_{}; ///< 写入信息队列
        LockFreeQueue<uint8_t> *data_buffer_{};   ///< 写入数据队列

        WriteFunc func_{};     ///< 写入函数
        size_t write_size_{0}; ///< 上次写入大小

        /**
         * @brief 构造函数
         * @param queue_size 写入信息队列大小，默认为3
         * @param buffer_size 写入数据队列大小，默认为512字节
         * @note 如果内存分配失败，会触发断言
         * @note 写入信息队列按系统缓存行对齐分配
         * @note 写入信息队列大小必须大于0
         */
        explicit WritePort(size_t queue_size = 3, size_t buffer_size = 512);

        /**
         * @brief 注册写入函数
         * @param func 写入函数
         * @return 写入端口引用
         * @note 该函数用于注册一个写入函数，以便在写入数据时调用
         * @note 该函数返回对当前写入端口的引用，以便进行链式调用
         */
        WritePort &operator=(WriteFunc func);

        /**
         * @brief 检查写入端口是否可用
         * @return 如果可用返回true，否则返回false
         * @note 仅当注册了写入函数时，返回true
         */
        [[nodiscard]] FORCE_INLINE bool Writable() const
        {
            return func_ != nullptr && data_buffer_ != nullptr;
        }

        /**
         * @brief 完成写入操作
         * @param in_isr 是否在中断中调用
         * @param ret 写入结果
         * @param info 写入信息块
         * @param size 实际写入大小
         * @note 该函数会更新写入状态，并根据操作类型进行相应处理
         */
        void Finish(bool in_isr, ErrorCode ret, WriteInfo &info, uint32_t size);

        /**
         * @brief 写入数据
         * @param data 待读数据内存块
         * @param operation 写操作
         * @return 错误码
         * @note 如果写入端口不可用，返回ErrorCode::NotSupported
         * @note 如果写入端口已被占用，返回ErrorCode::Busy
         * @note 如果写入数据长度为0，直接完成写入操作
         * @note 调用注册的写入函数进行写入
         */
        ErrorCode operator()(ConstRawData data, WriteOperation &operation);

        /**
         * @brief 标记操作为运行中
         * @note 仅当操作类型为轮询时有效
         */
        FORCE_INLINE static void MarkRunning(WriteInfo &info)
        {
            info.operation.MarkRunning();
        }
    };

    /**
     * @brief 标准输入输出类
     */
    class STDIO final
    {
    private:
#if defined(APPKIT_PRINTF_BUFFER_SIZE) && (APPKIT_PRINTF_BUFFER_SIZE > 0)
        static osal::Mutex mutex_;                      ///< 写入互斥锁
        static char buffer_[APPKIT_PRINTF_BUFFER_SIZE]; ///< 静态缓冲区
#endif

    public:
        static inline ReadPort *read_{};   ///< 读取端口
        static inline WritePort *write_{}; ///< 写入端口

        /**
         * @brief 打印格式化字符串
         * @param format 格式化字符串
         * @param ... 格式化参数
         * @return 成功返回写入的字符数，失败返回-1
         * @note 该函数使用传统的C风格格式化字符串
         */
        static int Printf(const char *format, ...) noexcept;

        /**
         * @brief 打印格式化字符串
         * @param format 格式化字符串
         * @param args 格式化参数列表
         * @return 成功返回写入的字符数，失败返回-1
         * @note 该函数使用传统的C风格格式化字符串
         */
        static int VPrintf(const char *format, va_list args) noexcept;
    };
}
