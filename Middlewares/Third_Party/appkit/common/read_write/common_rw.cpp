/**
 * @file common_rw.cpp
 * @brief 通用读写通信实现
 * @author dusk
 * @date 2025-12-26
 */
#include <common_rw.hpp>

namespace appkit
{
    ReadPort::ReadPort(const size_t buffer_size)
        : buffer_{buffer_size > 0 ? new (std::align_val_t{SYSTEM_CACHE_LINE_SIZE}) LockFreeQueue<uint8_t>{buffer_size} : nullptr}
    {
    }

    ReadPort &ReadPort::operator=(const ReadFunc func)
    {
        func_ = func;
        return *this;
    }

    void ReadPort::Finish(const bool in_isr, ErrorCode ret, ReadInfo &info, const uint32_t size)
    {
        read_size_ = size;
        state_.store(State::IDLE, std::memory_order_release);
        info.operation.UpdateStatus(in_isr, std::forward<ErrorCode>(ret));
    }

    ErrorCode ReadPort::operator()(RawData data, ReadOperation &operation)
    {
        // 检查读取端口是否可用
        if (!Readable()) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        // 检查读取端口是否已经被占用
        if (const State current_state = state_.exchange(State::PENDING, std::memory_order_acquire);
            current_state == State::PENDING) [[unlikely]]
        {
            return ErrorCode::BUSY;
        }

        while (true)
        {
            // 检查缓存是否启用
            if (nullptr != buffer_) [[likely]]
            {
                // 尝试直接读取缓存数据
                if (const auto readable_size = buffer_->Size();
                    readable_size > 0 && readable_size >= data.GetSize())
                {
                    APPKIT_RAISE_IF_NOT(Check(buffer_->PopBatch(data.GetData<uint8_t>(), data.GetSize())), "Failed to read data from buffer");
                    read_size_ = data.GetSize();

                    if (operation.GetType() != ReadOperation::Type::BLOCK)
                    {
                        // 阻塞读取将会在后续做额外处理
                        operation.UpdateStatus(false, ErrorCode::OK);
                    }
                    state_.store(State::IDLE, std::memory_order_release);
                    return ErrorCode::OK;
                }
            }

            info_ = ReadInfo{data, operation};
            operation.MarkRunning();
            state_.store(State::IDLE, std::memory_order_release);

            if (const auto ret = func_(*this);
                ret != ErrorCode::OK) [[unlikely]]
            {
                // 尝试重试接收
                if (auto expected_state = State::IDLE; state_.compare_exchange_weak(
                        expected_state, State::PENDING, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    // 已有其他任务插入读取
                    break;
                }

                // 跳转至读取阶段
                continue;
            }

            // 成功接收到数据
            read_size_ = data.GetSize();
            if (operation.GetType() != ReadOperation::Type::BLOCK)
            {
                // 阻塞读取将会在后续做额外处理
                operation.UpdateStatus(false, ErrorCode::OK);
            }
            return ErrorCode::OK;
        }

        // 完成预读取操作，等待数据实际写入
        if (operation.GetType() == ReadOperation::Type::BLOCK)
        {
            return operation.Wait();
        }

        return ErrorCode::OK;
    }

    void ReadPort::ProcessPendingReads(const bool in_isr)
    {
        APPKIT_RAISE_IF_NOT(buffer_ != nullptr, "Buffer is not initialized");

        if (state_.load(std::memory_order_acquire) == State::PENDING)
        {
            if (buffer_->Size() >= info_.data.GetSize())
            {
                if (info_.data.GetSize() > 0)
                {
                    APPKIT_RAISE_IF_NOT(Check(buffer_->PopBatch(info_.data.GetData<uint8_t>(), info_.data.GetSize())),
                                        "Failed to read data from buffer");
                }
                Finish(in_isr, ErrorCode::OK, info_, info_.data.GetSize());
            }
        }
    }

    WritePort::WritePort(const size_t queue_size, const size_t buffer_size)
        : info_buffer_{queue_size > 0 ? new (std::align_val_t{SYSTEM_CACHE_LINE_SIZE}) LockFreeQueue<WriteInfo>{queue_size} : nullptr},
          data_buffer_{buffer_size > 0 ? new (std::align_val_t{SYSTEM_CACHE_LINE_SIZE}) LockFreeQueue<uint8_t>{buffer_size} : nullptr}
    {
    }

    WritePort &WritePort::operator=(const WriteFunc func)
    {
        func_ = func;
        return *this;
    }

    void WritePort::Finish(const bool in_isr, ErrorCode ret, WriteInfo &info, const uint32_t size)
    {
        write_size_ = size;
        info.operation.UpdateStatus(in_isr, std::forward<ErrorCode>(ret));
    }

    ErrorCode WritePort::operator()(const ConstRawData data, WriteOperation &operation)
    {
        if (!Writable()) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        // 写入数据长度为0
        if (data.GetSize() == 0) [[unlikely]]
        {
            write_size_ = 0;
            if (operation.GetType() != WriteOperation::Type::BLOCK)
            {
                operation.UpdateStatus(false, ErrorCode::OK);
            }
            return ErrorCode::OK;
        }

        // 写入端口是否被锁定
        if (auto expected = State::UNLOCK;
            !state_.compare_exchange_strong(expected, State::LOCK)) [[unlikely]]
        {
            return ErrorCode::BUSY;
        }

        const WriteInfo info{data, operation};
        if (const auto ret = info_buffer_->Push(info);
            ret != ErrorCode::OK)
        {
            state_.store(State::UNLOCK, std::memory_order_release);
            return ret;
        }

        if (const auto ret = data_buffer_->PushBatch(data.GetData<uint8_t>(), data.GetSize());
            ret != ErrorCode::OK)
        {
            // @TODO 回滚写入信息

            state_.store(State::UNLOCK, std::memory_order_release);
            return ret;
        }

        operation.MarkRunning();
        const auto ret = func_(*this);
        state_.store(State::UNLOCK, std::memory_order_release);

        if (ret == ErrorCode::OK) [[unlikely]]
        {
            write_size_ = data.GetSize();
            if (operation.GetType() != WriteOperation::Type::BLOCK)
            {
                operation.UpdateStatus(false, ErrorCode::OK);
            }
            return ErrorCode::OK;
        }

        if (operation.GetType() == WriteOperation::Type::BLOCK)
        {
            return operation.Wait();
        }

        return ErrorCode::OK;
    }

#if defined(APPKIT_PRINTF_BUFFER_SIZE) && (APPKIT_PRINTF_BUFFER_SIZE > 0)
    osal::Mutex STDIO::mutex_{};
    constinit char STDIO::buffer_[APPKIT_PRINTF_BUFFER_SIZE]{};
#endif

    int STDIO::VPrintf(const char *format, va_list args) noexcept
    {
#if defined(APPKIT_PRINTF_BUFFER_SIZE) && (APPKIT_PRINTF_BUFFER_SIZE > 0)
        if (nullptr == write_ || !write_->Writable())
        {
            return -1;
        }

        volatile osal::LockGuard lock{mutex_};

        const auto ret = vsnprintf(buffer_, APPKIT_PRINTF_BUFFER_SIZE, format, args);
        if (ret < 0)
        {
            return -1;
        }

        const ConstRawData data{buffer_, static_cast<size_t>(ret)};
        WriteOperation operation;

        if (ErrorCode::OK == (*write_)(data, operation))
        {
            return ret;
        }
        else
        {
            return -1;
        }
#else
        UNUSED(format);
        UNUSED(args);
        return 0;
#endif
    }

    int STDIO::Printf(const char *format, ...) noexcept
    {
#if defined(APPKIT_PRINTF_BUFFER_SIZE) && (APPKIT_PRINTF_BUFFER_SIZE > 0)
        if (nullptr == write_ || !write_->Writable())
        {
            return -1;
        }

        va_list args;
        va_start(args, format);

        const auto ret = VPrintf(format, args);

        va_end(args);
        return ret;
#else
        UNUSED(format);
        return 0;
#endif
    }
}