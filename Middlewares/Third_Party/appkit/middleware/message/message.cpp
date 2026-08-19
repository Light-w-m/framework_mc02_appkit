#include <message.hpp>

#include <osal_thread.hpp>

namespace appkit
{
    Topic::Topic(const char *topic_name, size_t size, const Domain *domain, const bool cache,
                 const bool multi_publisher)
    {
        if (domain == nullptr)
        {
            domain = &Topic::Domain::Default();
        }

        if (const auto topic_block = domain->domain_->Find(topic_name);
            nullptr != topic_block)
        {
            if (Check(RamFs::AsFile(topic_block, &file_)))
            {
                auto &topic = file_->Get<Block>();
                APPKIT_RAISE_IF_NOT(topic.mult_publisher || !topic.mutex, "Topic already exists and is multi-publisher");
                block_ = &topic;
                return;
            }
        }

        block_ = new Block();

        block_->data_size = size;
        block_->enable_cache = cache;
        block_->mult_publisher = multi_publisher;

        if (multi_publisher)
        {
            block_->mutex = new osal::Mutex();
            block_->state.store(LockState::USE_MUTEX, std::memory_order_release);
        }
        else
        {
            block_->mutex = nullptr;
            block_->state.store(LockState::UNLOCKED, std::memory_order_release);
        }

        Lock(*this);
        if (cache)
        {
            block_->cache = RawData{new uint8_t[size], size};
        }
        Unlock(*this);

        file_ = new RamFs::File{RamFs::CreateFile<Block>(topic_name, *block_)};
        domain->domain_->Add(*file_);
    }

    void Topic::Lock(const Topic &topic)
    {
        if (topic.block_->mutex)
        {
            APPKIT_RAISE_IF_NOT(Check(topic.block_->mutex->Lock()), "Failed to lock topic mutex");
        }
        else
        {
            auto expected = LockState::UNLOCKED;
            APPKIT_RAISE_IF_NOT(topic.block_->state.compare_exchange_strong(expected, LockState::LOCKED), "Failed to lock topic state");
        }
    }

    void Topic::Unlock(const Topic &topic)
    {
        if (topic.block_->mutex)
        {
            topic.block_->mutex->Unlock();
        }
        else
        {
            topic.block_->state.store(LockState::UNLOCKED, std::memory_order_release);
        }
    }

    RamFs::Dir &Topic::Domain::GetBusDict()
    {
        static auto &bus_dict{[]() -> RamFs::Dir &
                              {
                                  static RamFs::Dir bus_dir{RamFs::CreateDir("bus")};
                                  RamFs::Sys().Add(bus_dir);

                                  return bus_dir;
                              }()};
        return bus_dict;
    }

    Topic::Domain::Domain(RamFs::Dir *domain)
        : domain_(domain)
    {
        APPKIT_RAISE_IF_NOT(nullptr != domain_, "Domain directory pointer is null");
    }

    Topic::Domain::Domain(const char *name)
    {
        APPKIT_RAISE_IF_NOT(nullptr != name, "Domain name pointer is null");

        if (const auto dir = GetBusDict().Find(name);
            nullptr != dir)
        {
            APPKIT_RAISE_IF_NOT(Check(RamFs::AsDir(dir, &domain_)), "Failed to convert to directory");
            return;
        }

        domain_ = new RamFs::Dir{RamFs::CreateDir(name)};
        APPKIT_RAISE_IF_NOT(nullptr != domain_, "Failed to create domain directory");

        GetBusDict().Add(*domain_);
    }

    Topic::Domain &Topic::Domain::Default()
    {
        static Domain default_domain_{
            []()
            {
                static RamFs::Dir default_dir{RamFs::CreateDir("_default")};
                GetBusDict().Add(default_dir);

                return &default_dir;
            }()};
        return default_domain_;
    }

    Topic::TopicHandle Topic::WaitTopic(const char *topic_name, Duration timeout, const Domain *domain)
    {
        using namespace appkit::time_literals;

        if (domain == nullptr)
        {
            domain = &Topic::Domain::Default();
        }

        RamFs::File *file{nullptr};
        TopicHandle handle{nullptr};
        do
        {
            constinit static const auto interval = 1_ms;
            if (!Check(RamFs::AsFile(domain->domain_->Find(topic_name), &file)))
            {
                timeout -= interval;
                osal::this_thread::SleepFor(interval);
                continue;
            }

            handle = &file->Get<Block>();
        } while (nullptr == file && timeout > Duration::Zero());

        return handle;
    }

    ErrorCode Topic::PublishFromCallback(bool in_isr, ConstRawData data) const
    {
        auto ret{ErrorCode::OK};
        Lock(*this);

        if (block_->enable_cache)
        {
            const auto new_data_size = data.GetSize();
            data.CopyTo(block_->cache);

            data = ConstRawData{block_->cache, new_data_size};
        }

        auto foreach_func = [&](SuberBlock &block) -> ErrorCode
        {
            switch (block.type)
            {
            case SuberType::SYNC:
            {
                auto &sync = static_cast<SyncSuberBlock &>(block);
                data.CopyTo(sync.data);
                sync.semaphore.PostFromCallback(in_isr);
                break;
            }
            case SuberType::ASYNC:
            {
                if (auto &async = static_cast<AsyncSuberBlock &>(block);
                    async.state.load(std::memory_order_acquire) == AsyncSuberState::Waiting)
                {
                    data.CopyTo(async.data);
                    async.state.store(AsyncSuberState::Ready, std::memory_order_release);
                }
                break;
            }
            case SuberType::FIFO:
            {
                auto &fifo = static_cast<FifoSuberBlock &>(block);
                fifo.func(fifo.queue, data, in_isr);
                break;
            }
            case SuberType::CALLBACK:
            {
                auto &callback = static_cast<CallbackSuberBlock &>(block);
                callback.func.Call(in_isr, data);
                break;
            }
            default:
            {
                return ErrorCode::FAILED;
            }
            }

            return ErrorCode::OK;
        };

        ret = block_->subscribers.ForEach<SuberBlock, decltype(foreach_func), Assert::SizeLimitMode::GREAT>(foreach_func);

        Unlock(*this);
        return ret;
    }

    [[nodiscard]] const char *Topic::GetName() const
    {
        if (file_ != nullptr)
        {
            return file_->GetName();
        }
        else
        {
            return "";
        }
    }

    [[nodiscard]] const RamFs::String::HashType Topic::GetNameHash() const
    {
        if (file_ != nullptr)
        {
            return file_->GetNameHash();
        }
        else
        {
            return 0;
        }
    }
}
