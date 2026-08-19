#include <referee.hpp>

namespace referee
{
    Referee::Referee(const char *name, const char *uart_name, std::initializer_list<Param> params, uint32_t read_buffer_size, uint32_t read_thread_stack_size)
    {
        // 初始化UART
        APPKIT_RAISE_IF_NOT(appkit::Check(uart_.Open(uart_name)),
                            "Failed to open UART device");
        APPKIT_RAISE_IF_NOT(uart_.GetReadPort()->Readable(),
                            "UART read port is not readable");
        APPKIT_RAISE_IF_NOT(uart_.GetReadPort()->buffer_->Capacity() >= 128,
                            "UART read buffer size is insufficient");

        // 创建主题域
        appkit::Topic::Domain topic_domain(name);

        // 创建接收实例
        recvInstances_.reserve(params.size());
        for (const auto &param : params)
        {
            APPKIT_RAISE_IF_NOT(param.type_size <= read_buffer_size, "Read buffer size is insufficient for message type");
            recvInstances_.emplace_back(param.cmd_id, appkit::Topic::CreateTopic(param.topic_name, param.type_size, &topic_domain), param.type_size);
        }

        visionTopic_ = appkit::Topic::CreateTopic<vision::VisionRemoteControl>("vt03_remote_control", &topic_domain);

        // 创建接收缓冲区
        readBuffer_ = new uint8_t[read_buffer_size];

        // 创建事件
        APPKIT_RAISE_IF_NOT(appkit::Check(refereeEvent_.Open(name)), "Failed to create referee event");

        // 创建读取线程
        readThread_.Create(
            ReadThreadFunc, this,
            name,
            read_thread_stack_size, appkit::osal::Thread::Priority::HIGH);
    }

    Referee::~Referee()
    {
        delete[] readBuffer_;
    }

    void Referee::ParseRefereeData(size_t available_size)
    {
        if (parserState_ == RefereeParserState::WAIT_SOF)
        {
            // 解析帧起始标志
            if (available_size >= 1)
            {
                uint8_t sof;
                uart_.GetReadPort()->buffer_->Peek(sof);
                if (sof == RefereeHeader::FRAME_SOF)
                {
                    parserState_ = RefereeParserState::WAIT_HEADER;
                }
            }
        }

        if (parserState_ == RefereeParserState::WAIT_HEADER)
        {
            // 解析帧头
            if (available_size >= sizeof(RefereeHeader))
            {
                uart_.GetReadPort()->buffer_->PeekBatch(reinterpret_cast<uint8_t *>(&currentHeader_), sizeof(RefereeHeader));
                if (currentHeader_.crc8 == RefereeHeader::CalCrc8(currentHeader_))
                {
                    parserState_ = RefereeParserState::WAIT_DATA;
                }
                else
                {
                    // CRC8校验失败，重新寻找帧起始标志
                    parserState_ = RefereeParserState::WAIT_SOF;
                }
            }
        }

        if (parserState_ == RefereeParserState::WAIT_DATA)
        {
            // 解析数据区
            if (available_size >= currentHeader_.data_length + sizeof(uint16_t))
            {
                // 数据区已接收完整，准备解析CRC16
                parserState_ = RefereeParserState::WAIT_CRC16;
            }
        }

        if (parserState_ == RefereeParserState::WAIT_CRC16)
        {
            auto &read_port = *uart_.GetReadPort();

            // 解析CRC16校验码
            if (available_size >= 2 * sizeof(uint16_t) + currentHeader_.data_length)
            {
                uint16_t cmd_id, crc16;

                read_port.buffer_->PopBatch(reinterpret_cast<uint8_t *>(&cmd_id), sizeof(uint16_t));
                read_port.buffer_->PopBatch(readBuffer_, currentHeader_.data_length);
                read_port.buffer_->PopBatch(reinterpret_cast<uint8_t *>(&crc16), sizeof(uint16_t));

                if (crc16 == CalculateCRC16({appkit::ConstRawData(currentHeader_),
                                             appkit::ConstRawData(cmd_id),
                                             appkit::ConstRawData(readBuffer_, currentHeader_.data_length)}))
                {
                    // CRC16校验通过，发布消息
                    PublishMessage(
                        /* cmd_id */ cmd_id,
                        /* data */ appkit::ConstRawData(readBuffer_, currentHeader_.data_length));
                }

                // 重置状态，准备解析下一个数据包
                parserState_ = RefereeParserState::WAIT_SOF;
            }
        }
    }

    void Referee::ParseVisionData(size_t available_size)
    {
        if (visionParserState_ == VisionParserState::WAIT_SOF_1)
        {
            // 解析帧起始标志
            if (available_size >= 1)
            {
                uint8_t sof;
                uart_.GetReadPort()->buffer_->Peek(sof);
                if (sof == vision::VisionRemoteControl::FRAME_SOF_1)
                {
                    visionParserState_ = VisionParserState::WAIT_SOF_2;
                }
            }
        }

        if (visionParserState_ == VisionParserState::WAIT_SOF_2)
        {
            // 解析第二个帧起始标志
            if (available_size >= 2)
            {
                uart_.GetReadPort()->buffer_->PeekBatch(reinterpret_cast<uint8_t *>(&currentVisionData_), 2);
                if (currentVisionData_.sof_2 == vision::VisionRemoteControl::FRAME_SOF_2)
                {
                    visionParserState_ = VisionParserState::WAIT_DATA;
                }
                else
                {
                    // 帧起始标志错误，重新寻找帧起始标志
                    visionParserState_ = VisionParserState::WAIT_SOF_1;
                }
            }
        }

        if (visionParserState_ == VisionParserState::WAIT_DATA)
        {
            // 解析数据区
            if (available_size >= sizeof(vision::VisionRemoteControl) - 2 * sizeof(uint16_t))
            {
                // 数据区已接收完整，准备解析CRC16
                visionParserState_ = VisionParserState::WAIT_CRC16;
            }
        }

        if (visionParserState_ == VisionParserState::WAIT_CRC16)
        {
            auto &read_port = *uart_.GetReadPort();

            // 解析CRC16校验码
            if (available_size >= sizeof(vision::VisionRemoteControl))
            {
                read_port.buffer_->PeekBatch(reinterpret_cast<uint8_t *>(&currentVisionData_), sizeof(vision::VisionRemoteControl));

                if (currentVisionData_.crc16 == CalculateCRC16({appkit::ConstRawData(&currentVisionData_,
                                                                                     sizeof(vision::VisionRemoteControl) - sizeof(uint16_t))}))
                {
                    read_port.buffer_->PopBatch(sizeof(vision::VisionRemoteControl));

                    // CRC16校验通过，发布消息
                    visionTopic_.Publish(currentVisionData_);
                }

                // 重置状态，准备解析下一个数据包
                visionParserState_ = VisionParserState::WAIT_SOF_1;
            }
        }
    }

    void Referee::ReadThreadFunc(Referee *self)
    {
        auto &read_port = *self->uart_.GetReadPort();
        size_t lost_count = 0, connected_count = 0;

        while (true)
        {
            if (Check(read_port({nullptr, 0}, self->op_)))
            {
                size_t available_size = read_port.buffer_->Size();

                if (available_size == 0)
                {
                    goto timeout_wrap;
                }

                // 解析图传数据
                if (self->parserState_ == RefereeParserState::WAIT_SOF)
                {
                    self->ParseVisionData(available_size);
                }

                // 解析裁判系统数据
                if (self->visionParserState_ == VisionParserState::WAIT_SOF_1)
                {
                    self->ParseRefereeData(available_size);
                }

                if (self->parserState_ == RefereeParserState::WAIT_SOF &&
                    self->visionParserState_ == VisionParserState::WAIT_SOF_1)
                {
                    // 两种协议均处于等待帧起始标志状态，说明缓存开头没有相关帧头，丢弃一个字节继续解析
                    read_port.buffer_->Pop();
                    continue;
                }
            }
            else
            {
                goto timeout_wrap;
            }

            if (lost_count != 0 || ++connected_count >= 100)
            {
                // 恢复连接，激活连接事件
                self->refereeEvent_.Activate(static_cast<uint32_t>(RefereeStatus::CONNECTED));
                lost_count = 0;
                connected_count = 0;
            }

            continue;

        timeout_wrap:
            // 等待数据到来
            if (++lost_count >= 100)
            {
                // 每100次超时打印一次日志
                self->refereeEvent_.Activate(static_cast<uint32_t>(RefereeStatus::DISCONNECTED));
                lost_count = 0;
                connected_count = 0;
            }
        }
    }

    void Referee::PublishMessage(uint16_t cmd_id, appkit::ConstRawData data)
    {
        for (const auto &instance : recvInstances_)
        {
            if (instance.cmd_id == cmd_id)
            {
                if (data.GetSize() == instance.type_size)
                {
                    if (!appkit::Check(instance.topic.Publish(data)))
                    {
                        APPKIT_LOG_ERROR("Failed to publish referee message, cmd_id: %d", (int)cmd_id);
                    }
                }
                break;
            }
        }
    }
} // namespace referee
