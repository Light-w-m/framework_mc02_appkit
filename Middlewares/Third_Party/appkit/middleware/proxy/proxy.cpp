#include <proxy.hpp>

#include <logger.hpp>

namespace appkit
{
    // 接收队列只分配一次、随进程存活
    Proxy::Proxy(size_t rx_queue_size, const SendCallback &send_callback)
        : rx_queue_(new LockFreeQueue<uint8_t>(rx_queue_size)), send_callback_(send_callback)
    {
    }

    ErrorCode Proxy::PushData(bool in_isr, const ConstRawData &data)
    {
        if (!rx_queue_)
        {
            APPKIT_RAISE_FROM_CALLBACK(in_isr, "RX queue is not initialized");
        }

        // 推入数据到接收队列
        if (const auto ret = rx_queue_->PushBatch(data.GetData<uint8_t>(), data.GetSize());
            !Check(ret))
        {
            return ret;
        }

        // 查找帧头
        uint8_t byte{0};
        while (rx_queue_->Size() > sizeof(MsgHeader))
        {
            // 读取一个字节
            if (const auto ret = rx_queue_->Peek(byte);
                !Check(ret))
            {
                // 队列空
                return ret;
            }

            // 查找起始字节
            if (byte == SE_START_BYTE)
            {
                // 找到帧头
                if (rx_queue_->Size() >= sizeof(MsgHeader))
                {
                    // 读取消息头
                    MsgHeader header{};
                    if (const auto ret = rx_queue_->PeekBatch(reinterpret_cast<uint8_t *>(&header), sizeof(MsgHeader));
                        !Check(ret))
                    {
                        return ret;
                    }

                    // 校验消息头CRC8
                    if (!math::Crc8::Checksum(ConstRawData(&header, sizeof(MsgHeader) - sizeof(uint8_t)), header.crc8))
                    {
                        if (!in_isr)
                        {
                            APPKIT_LOG_WARNING("Invalid message header CRC8");
                        }

                        // 丢弃错误帧头
                        rx_queue_->PopBatch(sizeof(MsgHeader));
                        continue;
                    }

                    // 查找对应节点
                    auto foreach_func = [this, in_isr, node_hash = header.hash](NodeBase &node) -> ErrorCode
                    {
                        // 匹配消息哈希值
                        if (node.msg_hash == node_hash)
                        {
                            // 判断剩余数据是否足够
                            if (rx_queue_->Size() >= node.msg_buffer.GetSize())
                            {
                                // 找到对应节点，处理消息
                                rx_queue_->PopBatch(node.msg_buffer.GetData<uint8_t>(), node.msg_buffer.GetSize());
                                node.update_callback.Call(in_isr);

                                return ErrorCode::EMPTY; // 停止遍历
                            }

                            // 数据不足，等待下一次处理
                            return ErrorCode::OUT_OF_RANGE; // 停止遍历
                        }

                        return ErrorCode::OK;
                    };

                    if (auto ret = node_list_.ForEach<NodeBase>(foreach_func);
                        ret == ErrorCode::EMPTY)
                    {
                        continue; // 消息已处理，继续查找下一个帧头
                    }
                    else if (ret == ErrorCode::OUT_OF_RANGE)
                    {
                        // 数据不足，等待下一次处理
                        break;
                    }
                    else
                    {
                        // 未找到对应节点，丢弃该消息
                        if (!in_isr)
                        {
                            APPKIT_LOG_DEBUG("No matching node for message hash: 0x%08X", header.hash);
                        }
                        rx_queue_->PopBatch(sizeof(MsgHeader) + rx_queue_->Size());
                        continue;
                    }
                }
            }
            else
            {
                // 丢弃无效数据
                rx_queue_->Pop();
            }
        }

        // 数据不足，等待下一次处理
        return ErrorCode::OK;
    }
}