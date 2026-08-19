#include <database.hpp>

#include <logger.hpp>
#include <crc8.hpp>

namespace appkit
{
    bool DataBaseRaw::KeyRaw::CheckName(const char *name) const
    {
        return (std::strncmp(keyName_, name, std::strlen(name)) == 0);
    }

    bool DataBase::CheckDatabaseHeader()
    {
        // 读取数据库头
        if (ErrorCode::OK != flash_.Read(0, RawData(header_))) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to read database header");
            return false;
        }

        // 检查文件头标识
        if (header_.database_header != PROPERTY(DATABASE_HEADER)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid database header: 0x%X", header_.database_header);
            return false;
        }

        // 检查版本号
        if (header_.version != PROPERTY(DATABASE_VERSION)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Unsupported database version: %u", header_.version);
            return false;
        }

        // 检查块大小
        if (header_.block_size != PROPERTY(blockSize)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid block size: %u", header_.block_size);
            return false;
        }

        // 检查校验码
        if (!math::Crc8::Checksum(ConstRawData(&header_, sizeof(DataBaseHeader) - 1), header_.check_sum)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid database header checksum: 0x%X", header_.check_sum);
            return false;
        }

        return true;
    }

    bool DataBase::CheckBlockHeader(const BlockHeader &block_header, uint32_t block_index)
    {
        if (block_header.block_header != PROPERTY(BLOCK_HEADER)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid block header at index %u: 0x%X", block_index, block_header.block_header);
            return false;
        }

        if (block_header.block_tail != PROPERTY(BLOCK_TAIL)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid block tail at index %u: 0x%X", block_index, block_header.block_tail);
            return false;
        }

        return true;
    }

    bool DataBase::CheckKeyName(const KeyRaw &key, const KeyBlock &key_block)
    {
        size_t key_name_len = std::strlen(key.keyName_);
        return (key_name_len == key_block.key_name_len);
    }

    uint32_t DataBase::GetBlockOffset(uint32_t block_index) const
    {
        return PROPERTY(databaseHeaderSize) + block_index * PROPERTY(blockSize);
    }

    uint32_t DataBase::GetBlockIndexByOffset(uint32_t offset) const
    {
        if (offset < PROPERTY(databaseHeaderSize))
        {
            return static_cast<uint32_t>(-1);
        }

        return (offset - PROPERTY(databaseHeaderSize)) / PROPERTY(blockSize);
    }

    Result<uint32_t> DataBase::GetFirstKeyOffset(uint32_t block_index)
    {
        const uint32_t block_offset = GetBlockOffset(block_index);
        if (block_offset == static_cast<uint32_t>(-1))
        {
            return Result<uint32_t>::Error(ErrorCode::INVALID_ARG);
        }

        const uint32_t first_key_offset = block_offset + sizeof(BlockHeader);
        return Result<uint32_t>::Ok(first_key_offset);
    }

    Result<uint32_t> DataBase::GetNextKeyOffset(const KeyBlock &key_block, uint32_t current_offset)
    {
        if (key_block.status == static_cast<uint8_t>(KeyStatus::FREE))
        {
            return Result<uint32_t>::Error(ErrorCode::NO_FOUND);
        }

        const uint32_t next_key_offset = current_offset + sizeof(KeyBlock) + key_block.key_name_len + key_block.data_len;
        if (GetBlockIndexByOffset(next_key_offset) != GetBlockIndexByOffset(current_offset))
        {
            // 超出当前块范围
            return Result<uint32_t>::Error(ErrorCode::OUT_OF_RANGE);
        }

        return Result<uint32_t>::Ok(next_key_offset);
    }

    ErrorCode DataBase::SearchKey(KeyRaw &key)
    {
        ErrorCode ret = ErrorCode::NO_FOUND;

        // 遍历所有块查找键
        for (size_t i = 0; i < block_count_; ++i)
        {
            // 加载块头数据到缓冲区
            Result<BlockHeader> block_header = LoadBlockHeader(i);
            if (!block_header) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Failed to load block %zu for key search", i);
                continue;
            }

            // 检查块状态
            if (block_header.GetValue().status == BlockStatus::FREE) [[unlikely]]
            {
                // 空闲块，跳过
                continue;
            }

            // 遍历块内所有键
            KeyBlock &key_block = *buffer_.GetData<KeyBlock>();
            for (Result<uint32_t> key_offset = GetFirstKeyOffset(i);
                 key_offset;
                 key_offset = GetNextKeyOffset(key_block, key_offset.GetValue()))
            {
                // 读取键头
                if (ErrorCode::OK != LoadKeyBlock(key_offset.GetValue(), key_block)) [[unlikely]]
                {
                    APPKIT_LOG_ERROR("Failed to read key block at offset %u", key_offset.GetValue());
                    break;
                }

                if (key_block.header != PROPERTY(KEY_HEADER) ||
                    key_block.header == 0xFF ||
                    key_block.status == static_cast<uint8_t>(KeyStatus::FREE))
                {
                    // 到达空闲或无效键块，停止搜索该块
                    break;
                }

                // 检查键状态
                if (key_block.status != static_cast<uint8_t>(KeyStatus::WRITING) &&
                    key_block.status != static_cast<uint8_t>(KeyStatus::DELETED))
                {
                    // 跳过正在写入或已删除的键块
                    // 匹配键名称
                    if (CheckKeyName(key, key_block))
                    {
                        // 找到匹配的键
                        key.offset_ = key_offset.GetValue();
                        key.total_size_ = sizeof(KeyBlock) + key_block.key_name_len + key_block.data_len;

                        if (key_block.status == static_cast<uint8_t>(KeyStatus::USED))
                        {
                            // 键状态为USED，返回成功
                            return ErrorCode::OK;
                        }

                        // 键状态为BACKUP，继续搜索
                        ret = ErrorCode::OK;
                    }

                    // 键名称不匹配，继续下一个键块
                }
            }
        }

        return ret;
    }

    Result<DataBase::BlockHeader> DataBase::LoadBlockHeader(uint32_t block_index)
    {
        BlockHeader block_header;
        const uint32_t block_offset = GetBlockOffset(block_index);
        if (ErrorCode::OK != flash_.Read(block_offset, RawData(block_header))) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to read block header at index %u", block_index);
            return Result<BlockHeader>::Error(ErrorCode::FAILED);
        }

        // 验证块头
        if (!CheckBlockHeader(block_header, block_index)) [[unlikely]]
        {
            return Result<BlockHeader>::Error(ErrorCode::FAILED);
        }

        return Result<BlockHeader>::Ok(block_header);
    }

    ErrorCode DataBase::LoadBlock(uint32_t block_index)
    {
        const uint32_t block_offset = GetBlockOffset(block_index);
        if (ErrorCode::OK != flash_.Read(block_offset, buffer_)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to load block %u", block_index);
            return ErrorCode::FAILED;
        }

        return ErrorCode::OK;
    }

    ErrorCode DataBase::LoadKeyBlock(uint32_t key_offset, KeyBlock &key_block)
    {
        if (key_offset + sizeof(KeyBlock) > GetBlockOffset(GetBlockIndexByOffset(key_offset) + 1)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Key offset %u out of range", key_offset);
            return ErrorCode::OUT_OF_RANGE;
        }

        if (ErrorCode::OK != flash_.Read(key_offset, RawData(key_block))) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to read key block at offset %u", key_offset);
            return ErrorCode::FAILED;
        }

        return ErrorCode::OK;
    }

    Result<DataBase::KeyBlock> DataBase::LoadKeyToBuffer(KeyRaw &key)
    {
        KeyBlock key_block;
        if (ErrorCode::OK != LoadKeyBlock(key.offset_, key_block)) [[unlikely]]
        {
            return Result<KeyBlock>::Error(ErrorCode::FAILED);
        }

        // 验证键名称长度
        if (!CheckKeyName(key, key_block)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Key name length mismatch for key '%s'", key.keyName_);
            return Result<KeyBlock>::Error(ErrorCode::FAILED);
        }

        // 读取键数据到缓冲区
        const uint32_t key_total_size = key.offset_ + sizeof(KeyBlock) + key_block.key_name_len + key_block.data_len;
        if (ErrorCode::OK != flash_.Read(key.offset_, buffer_.SubData(0, key_total_size))) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to read key data for key '%s'", key.keyName_);
            return Result<KeyBlock>::Error(ErrorCode::FAILED);
        }

        return Result<KeyBlock>::Ok(key_block);
    }

    Result<uint32_t> DataBase::GetAvailableBlockIndex(uint32_t size)
    {
        if (size == 0 || size > PROPERTY(MAX_DATA_LEN) + PROPERTY(MAX_KEY_NAME_LEN))
        {
            return Result<uint32_t>::Error(ErrorCode::INVALID_ARG);
        }

        const uint32_t required_size = sizeof(KeyBlock) + size;
        if (required_size > PROPERTY(blockSize))
        {
            return Result<uint32_t>::Error(ErrorCode::OUT_OF_RANGE);
        }

        // 寻找剩余空间足够的块
        for (size_t i = 0; i < block_count_; ++i)
        {
            if (blockAvailableSizeArray_[i] >= required_size)
            {
                return Result<uint32_t>::Ok(i);
            }
        }

        return Result<uint32_t>::Error(ErrorCode::NO_FOUND);
    }

    void DataBase::UpdateBlockAvailableSize(uint32_t block_index)
    {
        // 读取块头
        Result<BlockHeader> block_header_result = LoadBlockHeader(block_index);
        if (!block_header_result) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to load block header at index %u", block_index);
            blockAvailableSizeArray_[block_index] = 0;
            return;
        }

        BlockHeader block_header = block_header_result.GetValue();
        if (block_header.status == BlockStatus::FREE)
        {
            blockAvailableSizeArray_[block_index] = PROPERTY(blockSize) - sizeof(BlockHeader);
            return;
        }

        // 计算已使用空间
        bool has_used_or_backup_key = false;
        uint32_t used_size = sizeof(BlockHeader);

        // 遍历键块，计算已使用大小
        KeyBlock &key_block = *buffer_.GetData<KeyBlock>();
        for (Result<uint32_t> key_offset = GetFirstKeyOffset(block_index);
             key_offset;
             key_offset = GetNextKeyOffset(key_block, key_offset.GetValue()))
        {
            // 读取键头
            if (ErrorCode::OK != LoadKeyBlock(key_offset.GetValue(), key_block)) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Failed to read key block at offset %u", key_offset.GetValue());
                break;
            }

            if (key_block.header != PROPERTY(KEY_HEADER) && key_block.header != 0xFF)
            {
                // 无效键块
                APPKIT_LOG_ERROR("Invalid key block header at offset %u", key_offset.GetValue());
                break;
            }

            if (key_block.status == static_cast<uint8_t>(KeyStatus::FREE) || key_block.header == 0xFF)
            {
                // 到达空闲键块，停止搜索该块
                break;
            }

            // 记录键大小
            uint32_t total_key_size = sizeof(KeyBlock) + key_block.key_name_len + key_block.data_len;
            used_size += total_key_size;

            has_used_or_backup_key |= (key_block.status == static_cast<uint8_t>(KeyStatus::USED) ||
                                       key_block.status == static_cast<uint8_t>(KeyStatus::BACKUP));
        }

        // 更新块状态
        uint32_t remaining_size = PROPERTY(blockSize) - used_size;
        if (remaining_size < sizeof(KeyBlock) + 2)
        {
            // 初步判定为已满
            if (has_used_or_backup_key == false)
            {
                // 块内无有效数据，标记为脏块
                block_header.status = BlockStatus::DIRTY;
            }
            else
            {
                // 更新块状态为FULL
                block_header.status = BlockStatus::FULL;
            }

            flash_.Write(GetBlockOffset(block_index), ConstRawData(block_header));
            blockAvailableSizeArray_[block_index] = 0; // 块已满
        }

        blockAvailableSizeArray_[block_index] = remaining_size;
    }

    ErrorCode DataBase::SaveBlockHeader(uint32_t block_index, const BlockHeader header)
    {
        const uint32_t block_offset = GetBlockOffset(block_index);
        if (ErrorCode::OK != flash_.Write(block_offset, ConstRawData(header))) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to write block header at index %u", block_index);
            return ErrorCode::FAILED;
        }

        return ErrorCode::OK;
    }

    ErrorCode DataBase::SaveKeyBlock(uint32_t key_offset, const KeyBlock &key_block)
    {
        if (key_offset + sizeof(KeyBlock) > GetBlockOffset(GetBlockIndexByOffset(key_offset) + 1)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Key offset %u out of range", key_offset);
            return ErrorCode::OUT_OF_RANGE;
        }

        if (ErrorCode::OK != flash_.Write(key_offset, ConstRawData(key_block))) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to write key block at offset %u", key_offset);
            return ErrorCode::FAILED;
        }

        return ErrorCode::OK;
    }

    ErrorCode DataBase::SaveKey(uint32_t key_offset, ConstRawData key)
    {
        if (ErrorCode::OK != flash_.Write(key_offset, key)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to write key data at offset %u", key_offset);
            return ErrorCode::FAILED;
        }

        return ErrorCode::OK;
    }

    void DataBase::Init()
    {
        // 格式化Flash
        const uint32_t flash_size = flash_.GetSize();
        if (ErrorCode::OK != flash_.Erase(0, flash_size))
        {
            APPKIT_LOG_ERROR("Failed to format database flash");
            return;
        }

        // 初始化数据库头
        header_.database_header = PROPERTY(DATABASE_HEADER);
        header_.version = PROPERTY(DATABASE_VERSION);
        header_.block_size = PROPERTY(blockSize);
        header_.check_sum = math::Crc8::Calculate(RawData(&header_, sizeof(DataBaseHeader) - 1));

        // 写入数据库头
        if (ErrorCode::OK != flash_.Write(0, ConstRawData(header_)))
        {
            APPKIT_LOG_ERROR("Failed to write database header");
            return;
        }

        // 初始化块头
        BlockHeader *block_headers = buffer_.GetData<BlockHeader>();
        block_headers->block_header = PROPERTY(BLOCK_HEADER);
        block_headers->status = BlockStatus::FREE;
        block_headers->block_tail = PROPERTY(BLOCK_TAIL);

        // 初始化所有块为FREE状态
        ConstRawData block_header_data(buffer_.SubData(0, sizeof(BlockHeader)));
        for (size_t i = 0; i < block_count_; ++i)
        {
            block_headers->is_first = (i == 0);
            block_headers->has_next = (i < block_count_ - 1);

            SaveBlockHeader(i, *block_headers);

            // 更新块大小数组
            blockAvailableSizeArray_[i] = PROPERTY(blockSize) - sizeof(BlockHeader);
        }

        APPKIT_LOG_DEBUG("Database init with %zu blocks", block_count_);
        is_initialized_ = true;
    }

    void DataBase::Recycle()
    {
        APPKIT_LOG_DEBUG("Recycling database...");

        // 查找脏块并擦除
        for (size_t i = 0; i < block_count_; ++i)
        {
            if (blockAvailableSizeArray_[i] == 0)
            {
                // 读取块头
                Result<BlockHeader> block_header_result = LoadBlockHeader(i);
                if (!block_header_result) [[unlikely]]
                {
                    APPKIT_LOG_ERROR("Failed to load block header at index %zu", i);
                    continue;
                }

                BlockHeader block_header = block_header_result.GetValue();
                if (block_header.status == BlockStatus::DIRTY)
                {
                    // 擦除脏块
                    if (ErrorCode::OK != flash_.Erase(GetBlockOffset(i), PROPERTY(blockSize)))
                    {
                        APPKIT_LOG_ERROR("Failed to erase dirty block %zu", i);
                        continue;
                    }

                    // 重置块头
                    block_header.status = BlockStatus::FREE;
                    if (ErrorCode::OK != SaveBlockHeader(i, block_header))
                    {
                        APPKIT_LOG_ERROR("Failed to reset block header for block %zu", i);
                        continue;
                    }

                    // 更新块可用大小
                    blockAvailableSizeArray_[i] = PROPERTY(blockSize) - sizeof(BlockHeader);

                    APPKIT_LOG_DEBUG("Recycled block %zu", i);
                }
            }
        }

        // @TODO: 合并数据到更少块中
    }

    bool DataBase::CheckRecycleNeeded() const
    {
        size_t total_free_size = 0;
        for (size_t i = 0; i < block_count_; ++i)
        {
            total_free_size += blockAvailableSizeArray_[i];
        }

        return total_free_size <= (PROPERTY(blockSize) * block_count_ - PROPERTY(recycleSize));
    }

    DataBase::DataBase(const char *flash_name, size_t block_size)
    {
        // 打开Flash设备
        APPKIT_RAISE_IF_NOT(ErrorCode::OK == flash_.Open(flash_name), "Failed to open flash device");

        // 更新属性
        PROPERTY(databaseHeaderSize) = std::max<uint32_t>(sizeof(DataBaseHeader), flash_.GetSectorSize());
        PROPERTY(blockSize) = std::max<uint32_t>(block_size, flash_.GetSectorSize());
        block_count_ = (flash_.GetSize() - PROPERTY(databaseHeaderSize)) / PROPERTY(blockSize);
        PROPERTY(recycleSize) = static_cast<size_t>(PROPERTY(blockSize) * block_count_ * APPKIT_DATABASE_RECYCLE_THRESHOLD);

        // 分配缓冲区
        buffer_ = RawData(new uint8_t[PROPERTY(blockSize)], PROPERTY(blockSize));
        APPKIT_RAISE_IF_NOT(buffer_.GetData<void>(), "Failed to allocate database buffer");

        // 分配块状态数组
        blockAvailableSizeArray_ = new uint32_t[block_count_];
        APPKIT_RAISE_IF_NOT(blockAvailableSizeArray_, "Failed to allocate block available size array");

        // 读取数据库头
        APPKIT_RAISE_IF_NOT(
            ErrorCode::OK == flash_.Read(0, RawData(header_)),
            "Failed to read database header");

        // 校验数据库头
        if (header_.database_header != PROPERTY(DATABASE_HEADER) ||
            header_.version != PROPERTY(DATABASE_VERSION) ||
            header_.check_sum != math::Crc8::Calculate(RawData(&header_, sizeof(DataBaseHeader) - 1)))
        {
            APPKIT_LOG_WARNING("Database header invalid, initializing new database");
            Init(); // 初始化数据库
        }
        else
        {
            // 读取块状态
            for (size_t i = 0; i < block_count_; ++i)
            {
                UpdateBlockAvailableSize(i);
            }
        }

        is_open_ = true;
    }

    ErrorCode DataBase::Get(KeyRaw &key)
    {
        if (key.offset_ == 0 || key.total_size_ == 0)
        {
            APPKIT_LOG_DEBUG("Key '%s' offset or size is zero, searching key", key.keyName_);
            if (ErrorCode::OK != SearchKey(key))
            {
                APPKIT_LOG_DEBUG("Key '%s' not found in database", key.keyName_);
                return ErrorCode::NO_FOUND;
            }
        }

        // 读出键数据
        auto header_result = LoadKeyToBuffer(key);
        if (!header_result)
        {
            APPKIT_LOG_ERROR("Failed to load key data for key '%s' at offset %zu", key.keyName_, key.offset_);
            return header_result.GetError();
        }

        // 提取键数据
        const auto &key_block = header_result.GetValue();
        buffer_.SubData(sizeof(KeyBlock) + key_block.key_name_len, key_block.data_len).CopyTo(key.keyData_);

        APPKIT_LOG_DEBUG("Loaded key '%s' at offset %zu", key.keyName_, key.offset_);

        return ErrorCode::OK;
    }

    ErrorCode DataBase::Set(KeyRaw &key)
    {
        // 检查是否需要回收
        if (CheckRecycleNeeded())
        {
            APPKIT_LOG_DEBUG("Database recycle needed before setting key '%s'", key.keyName_);
            Recycle();
        }

        if (key.offset_ == 0 || key.total_size_ == 0)
        {
            APPKIT_LOG_DEBUG("Key '%s' not found, adding new key", key.keyName_);
            return ErrorCode::FAILED;
        }

        // 查找可用块
        auto index = GetAvailableBlockIndex(key.total_size_);
        if (!index)
        {
            APPKIT_LOG_ERROR("No available block found for key '%s'", key.keyName_);
            return index.GetError();
        }

        // 读出原数据
        Result<KeyBlock> original_data_result = LoadKeyToBuffer(key);
        if (!original_data_result)
        {
            APPKIT_LOG_ERROR("Failed to load original key data at offset %zu", key.offset_);
            return ErrorCode::FAILED;
        }

        // 更新旧数据为备份状态
        KeyBlock original_key_block = original_data_result.GetValue();
        original_key_block.status = static_cast<uint8_t>(KeyStatus::BACKUP);
        if (ErrorCode::OK != SaveKeyBlock(key.offset_, original_key_block))
        {
            APPKIT_LOG_ERROR("Failed to update original key block status at offset %zu", key.offset_);
            return ErrorCode::FAILED;
        }

        // 遍历键块，查找空闲键块
        size_t new_offset{};
        KeyBlock key_block;
        for (Result<uint32_t> key_offset = GetFirstKeyOffset(index.GetValue());
             key_offset;
             key_offset = GetNextKeyOffset(key_block, key_offset.GetValue()))
        {
            // 读取键头
            if (ErrorCode::OK != LoadKeyBlock(key_offset.GetValue(), key_block)) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Failed to read key block at offset %u", key_offset.GetValue());
                return ErrorCode::FAILED;
            }

            if (key_block.header != PROPERTY(KEY_HEADER) && key_block.header != 0xFF)
            {
                // 无效键块
                APPKIT_LOG_ERROR("Invalid key block header at offset %u", key_offset.GetValue());
                return ErrorCode::FAILED;
            }

            if (key_block.status == static_cast<uint8_t>(KeyStatus::FREE))
            {
                // 到达空闲键块，停止搜索该块
                new_offset = key_offset.GetValue();
                break;
            }
        }

        // 更新新数据为写入状态
        original_key_block.status = static_cast<uint8_t>(KeyStatus::WRITING);
        key.keyData_.CopyTo(buffer_.SubData(sizeof(KeyBlock) + original_key_block.key_name_len, original_key_block.data_len));

        // 更新块头
        Result<BlockHeader> new_block_header_result = LoadBlockHeader(index.GetValue());
        if (!new_block_header_result) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to load new block header at index %u", index.GetValue());
            return ErrorCode::FAILED;
        }

        BlockHeader new_block_header = new_block_header_result.GetValue();
        if (new_block_header.status == BlockStatus::FREE)
        {
            new_block_header.status = BlockStatus::USED;
            if (ErrorCode::OK != SaveBlockHeader(index.GetValue(), new_block_header))
            {
                APPKIT_LOG_ERROR("Failed to update new block header status at index %u", index.GetValue());
                return ErrorCode::FAILED;
            }
        }

        // 保存新键数据
        const size_t key_size = sizeof(KeyBlock) + original_key_block.key_name_len + original_key_block.data_len;
        if (ErrorCode::OK != SaveKey(new_offset, buffer_.SubData(0, key_size)))
        {
            APPKIT_LOG_ERROR("Failed to save new key data to block %u", index.GetValue());
            return ErrorCode::FAILED;
        }

        // 更新新数据为使用状态
        original_key_block.status = static_cast<uint8_t>(KeyStatus::USED);
        if (ErrorCode::OK != SaveKeyBlock(new_offset, original_key_block))
        {
            APPKIT_LOG_ERROR("Failed to update new key block status at offset %u", new_offset);
            return ErrorCode::FAILED;
        }

        // 更新旧数据为已删除状态
        original_key_block.status = static_cast<uint8_t>(KeyStatus::DELETED);
        if (ErrorCode::OK != SaveKeyBlock(key.offset_, original_key_block))
        {
            APPKIT_LOG_ERROR("Failed to update old key block status at offset %zu", key.offset_);
            return ErrorCode::FAILED;
        }

        // 更新KeyRaw信息
        const auto old_offset = key.offset_;
        key.offset_ = new_offset;
        APPKIT_LOG_DEBUG("Updated key '%s' from offset %zu to %zu", key.keyName_, old_offset, key.offset_);

        // 更新块可用大小
        UpdateBlockAvailableSize(GetBlockIndexByOffset(old_offset));
        UpdateBlockAvailableSize(index.GetValue());

        return ErrorCode::OK;
    }

    ErrorCode DataBase::Add(KeyRaw &key)
    {
        // 检查是否需要回收
        if (CheckRecycleNeeded())
        {
            APPKIT_LOG_DEBUG("Database recycle needed before adding new key");
            Recycle();
        }

        // 初始化KeyRaw信息
        const auto name_len = static_cast<uint8_t>(std::strlen(key.keyName_));
        key.total_size_ = sizeof(KeyBlock) + name_len + key.keyData_.GetSize();

        // 检查参数合法性
        if (name_len == 0 || name_len > PROPERTY(MAX_KEY_NAME_LEN) ||
            key.keyData_.GetSize() == 0 || key.keyData_.GetSize() > PROPERTY(MAX_DATA_LEN))
        {
            APPKIT_LOG_ERROR("Invalid key name length %u or data size %zu", name_len, key.keyData_.GetSize());
            return ErrorCode::INVALID_ARG;
        }

        // 查找可用块
        auto index = GetAvailableBlockIndex(key.total_size_);
        if (!index)
        {
            APPKIT_LOG_ERROR("No available block found for key '%s'", key.keyName_);
            return index.GetError();
        }

        // 遍历键块，查找空闲键块
        size_t new_offset{};
        KeyBlock &key_block = *buffer_.GetData<KeyBlock>();
        for (Result<uint32_t> key_offset = GetFirstKeyOffset(index.GetValue());
             key_offset;
             key_offset = GetNextKeyOffset(key_block, key_offset.GetValue()))
        {
            // 读取键头
            if (ErrorCode::OK != LoadKeyBlock(key_offset.GetValue(), key_block)) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Failed to read key block at offset %u", key_offset.GetValue());
                return ErrorCode::FAILED;
            }

            if (key_block.header != PROPERTY(KEY_HEADER) && key_block.header != 0xFF)
            {
                // 无效键块
                APPKIT_LOG_ERROR("Invalid key block header at offset %u", key_offset.GetValue());
                return ErrorCode::FAILED;
            }

            if (key_block.status == static_cast<uint8_t>(KeyStatus::FREE))
            {
                // 到达空闲键块，停止搜索该块
                new_offset = key_offset.GetValue();
                break;
            }
        }

        // 准备键数据
        RawData key_data(buffer_.SubData(0, key.total_size_));
        key_block = KeyBlock{
            .header = PROPERTY(KEY_HEADER),
            .key_name_len = name_len,
            .data_len = static_cast<uint16_t>(key.keyData_.GetSize()),
            .status = static_cast<uint8_t>(KeyStatus::WRITING),
        };
        ConstRawData(key.keyName_, name_len).CopyTo(key_data.SubData(sizeof(KeyBlock), name_len));
        key.keyData_.CopyTo(key_data.SubData(sizeof(KeyBlock) + name_len, key_block.data_len));

        // 更新块头
        Result<BlockHeader> block_header_result = LoadBlockHeader(index.GetValue());
        if (!block_header_result) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to load block header at index %u", index.GetValue());
            return ErrorCode::FAILED;
        }

        BlockHeader block_header = block_header_result.GetValue();
        if (block_header.status == BlockStatus::FREE)
        {
            block_header.status = BlockStatus::USED;
            if (ErrorCode::OK != SaveBlockHeader(index.GetValue(), block_header))
            {
                APPKIT_LOG_ERROR("Failed to update block header status at index %u", index.GetValue());
                return ErrorCode::FAILED;
            }
        }

        // 保存键数据
        if (ErrorCode::OK != SaveKey(new_offset, key_data))
        {
            APPKIT_LOG_ERROR("Failed to save key data to block %u", index.GetValue());
            return ErrorCode::FAILED;
        }

        // 更新键状态为USED
        key_block.status = static_cast<uint8_t>(KeyStatus::USED);
        if (ErrorCode::OK != SaveKeyBlock(new_offset, key_block))
        {
            APPKIT_LOG_ERROR("Failed to update key block status at offset %u", new_offset);
            return ErrorCode::FAILED;
        }

        // 更新KeyRaw信息
        key.offset_ = new_offset;
        APPKIT_LOG_DEBUG("Added key '%s' at offset %zu", key.keyName_, key.offset_);

        // 更新块可用大小
        UpdateBlockAvailableSize(index.GetValue());

        return ErrorCode::OK;
    }
}