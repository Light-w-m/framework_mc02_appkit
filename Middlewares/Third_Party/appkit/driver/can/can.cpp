#include <can.hpp>

#include <logger.hpp>

template class appkit::Callback<const appkit::CAN::ClassicPack &>;
template class appkit::Callback<const appkit::FDCAN::FDPack &>;

namespace appkit
{
    ErrorCode CAN::Open(const char *name)
    {
        if (nullptr == name) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device already open");
            return ErrorCode::FAILED;
        }

        return Block::GetDeviceBlock(name, &block_);
    }

    ErrorCode CAN::Register(const Callback &callback, PackType type, FilterMode mode, uint32_t mask_start_id, uint32_t match_end_id) const
    {
        if (!block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("CAN device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        // 创建过滤器结构体
        const auto filter = new LockFreeList::Node<Filter>{mode, mask_start_id, match_end_id, type, callback};
        if (nullptr == filter) [[unlikely]]
        {
            delete filter;
            return ErrorCode::NO_MEMORY;
        }

        block_->id_filter_list[std::to_underlying(type)].Add<Filter>(*filter);

        return ErrorCode::OK;
    }

    ErrorCode CAN::AddMessage(const ClassicPack &frame) const
    {
        if (!block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("CAN device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        return block_->AddMessage(frame);
    }

    void CAN::Block::OnMessage(const ClassicPack &frame, bool in_isr)
    {
        auto foreach_func = [&frame, in_isr](Filter &filter)
        {
            switch (filter.mode)
            {
            [[likely]] case FilterMode::EQUAL:
            {
                if (frame.id == filter.mask_start_id)
                {
                    filter.callback.Call(in_isr, frame);
                }
                break;
            }
            case FilterMode::MASK:
            {
                if ((frame.id & filter.mask_start_id) == filter.match_end_id)
                {
                    filter.callback.Call(in_isr, frame);
                }
                break;
            }
            case FilterMode::RANGE:
            {
                if (frame.id >= filter.mask_start_id && frame.id <= filter.match_end_id)
                {
                    filter.callback.Call(in_isr, frame);
                }
                break;
            }
            [[unlikely]] default:
            {
                APPKIT_RAISE_FROM_CALLBACK(in_isr, "Invalid filter mode");
            }
            };

            return ErrorCode::OK;
        };

        id_filter_list[std::to_underlying(frame.type)].ForEach<Filter>(foreach_func);
    }

    ErrorCode FDCAN::Open(const char *name)
    {
        if (nullptr == name) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device already open");
            return ErrorCode::FAILED;
        }

        return Block::GetDeviceBlock(name, &block_);
    }

    ErrorCode FDCAN::Register(const Callback &callback, PackType type, FilterMode mode, uint32_t mask_start_id, uint32_t match_end_id) const
    {
        if (!block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("FDCAN device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        // 创建过滤器结构体
        const auto filter = new LockFreeList::Node<Filter>{mode, mask_start_id, match_end_id, type, callback};
        if (nullptr == filter) [[unlikely]]
        {
            delete filter;
            return ErrorCode::NO_MEMORY;
        }

        block_->id_filter_list[std::to_underlying(type)].Add<Filter>(*filter);

        return ErrorCode::OK;
    }

    ErrorCode FDCAN::AddMessage(const FDPack &frame) const
    {
        if (!block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("FDCAN device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        return block_->AddMessage(frame);
    }

    void FDCAN::Block::OnMessage(const FDPack &frame, bool in_isr)
    {
        auto foreach_func = [&frame, in_isr](Filter &filter)
        {
            switch (filter.mode)
            {
            [[likely]] case FilterMode::EQUAL:
            {
                if (frame.id == filter.mask_start_id)
                {
                    filter.callback.Call(in_isr, frame);
                }
                break;
            }
            case FilterMode::MASK:
            {
                if ((frame.id & filter.mask_start_id) == filter.match_end_id)
                {
                    filter.callback.Call(in_isr, frame);
                }
                break;
            }
            case FilterMode::RANGE:
            {
                if (frame.id >= filter.mask_start_id && frame.id <= filter.match_end_id)
                {
                    filter.callback.Call(in_isr, frame);
                }
                break;
            }
            [[unlikely]] default:
            {
                APPKIT_RAISE_FROM_CALLBACK(in_isr, "Invalid filter mode");
            }
            };

            return ErrorCode::OK;
        };

        id_filter_list[std::to_underlying(frame.type)].ForEach<Filter>(foreach_func);
    }
} // namespace appkit
