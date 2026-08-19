#include <event.hpp>

#include <logger.hpp>

namespace appkit
{
    RamFs::Dir &Event::event_dict_{
        []() -> RamFs::Dir &
        {
            static RamFs::Dir event_dir{RamFs::CreateDir("event")};
            RamFs::Sys().Add(event_dir);

            return event_dir;
        }()};

    ErrorCode Event::Open(const char *name)
    {
        if (name == nullptr)
        {
            return ErrorCode::INVALID_ARG;
        }

        if (file_ != nullptr)
        {
            APPKIT_LOG_ERROR("Event has been opened");
            return ErrorCode::FAILED;
        }

        if (const auto event_block = event_dict_.Find(name); nullptr != event_block)
        {
            if (Check(RamFs::AsFile(event_block, &file_)))
            {
                block_ = &file_->Get<Block>();
                return ErrorCode::OK;
            }
        }

        block_ = new Block{};
        file_ = new RamFs::File{RamFs::CreateFile(name, *block_)};

        event_dict_.Add(*file_);
        APPKIT_LOG_INFO("Create event %s", name);

        return ErrorCode::OK;
    }

    ErrorCode Event::Register(Callback cb)
    {
        if (block_ == nullptr)
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        LockFreeList::Node<Callback> *node = new LockFreeList::Node<Callback>{cb};
        block_->list_.Add(*node);

        return ErrorCode::OK;
    }

    void Event::Mask(bool mask)
    {
        if (block_ == nullptr)
        {
            return;
        }

        block_->masked = mask;
    }

    bool Event::IsMasked() const
    {
        if (block_ == nullptr)
        {
            return false;
        }

        return block_->masked;
    }

    ErrorCode Event::ActivateFromCallback(bool in_isr, uint32_t value) const
    {
        if (block_ == nullptr)
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        if (!block_->masked)
        {
            return ErrorCode::OK;
        }

        auto foreach_func = [&](Callback &cb) -> ErrorCode
        {
            cb.Call(in_isr, value);
            return ErrorCode::OK;
        };
        return block_->list_.ForEach<Callback>(foreach_func);
    }

    LockFreeList *Event::GetList() const
    {
        if (block_ == nullptr)
        {
            return nullptr;
        }

        return &block_->list_;
    }

    ErrorCode Event::Bind(Event &source)
    {
        if (block_ == nullptr)
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        if (source.block_ == nullptr)
        {
            return ErrorCode::INVALID_ARG;
        }

        static constinit auto bind_func = [](bool in_isr, const Event &source, uint32_t value) -> ErrorCode
        {
            return source.ActivateFromCallback(in_isr, value);
        };

        return Register(Callback::Create(bind_func, source));
    }
}
