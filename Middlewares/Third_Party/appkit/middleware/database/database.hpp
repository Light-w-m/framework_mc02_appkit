#pragma once

#include <common_type.hpp>
#include <flash.hpp>

#define APPKIT_DATABASE_RECYCLE_THRESHOLD 0.8f ///< 数据库回收阈值，单位为百分比

namespace appkit
{
    /**
     * @brief 数据库原始类，提供基本的数据库操作接口
     */
    class DataBaseRaw
    {
        DECL_STATIC_PROPERTY(protected, uint32_t, DATABASE_VERSION, 1)

    public:
        /**
         * @brief 数据库键原始类，封装键名称和数据
         */
        class KeyRaw
        {
        private:
            size_t offset_{};     ///< 键在数据库中的偏移量
            size_t total_size_{}; ///< 键总大小

            friend class DataBaseRaw; ///< 允许DataBaseRaw访问私有成员
            friend class DataBase;    ///< 允许DataBase访问私有成员

        public:
            const char *keyName_{}; ///< 键名称
            RawData keyData_{};     ///< 键数据

            /**
             * @brief 构造函数
             * @param name 键名称
             * @param data 键数据
             */
            KeyRaw(const char *name, RawData data)
                : keyName_(name), keyData_(data)
            {
            }

            /**
             * @brief 检查键名称是否匹配
             * @param name 键名称
             * @return 是否匹配
             */
            bool CheckName(const char *name) const;
        };

        /**
         * @brief 获取键值
         * @param key 键对象引用
         * @return 错误码
         */
        virtual ErrorCode Get(KeyRaw &key) = 0;

        /**
         * @brief 设置键值
         * @param key 键对象引用
         * @return 错误码
         */
        virtual ErrorCode Set(KeyRaw &key) = 0;

        /**
         * @brief 添加新键值
         * @param key 键对象引用
         * @return 错误码
         */
        virtual ErrorCode Add(KeyRaw &key) = 0;
    };

    /**
     * @brief 数据库键类模板，封装具体类型的键值操作
     * @tparam DType 键值类型
     * @note DType必须是平凡可拷贝类型（trivially copyable type）
     */
    template <typename DType>
        requires std::is_trivially_copyable_v<DType>
    class Key : public DataBaseRaw::KeyRaw
    {
    private:
        DType data_{};    ///< 键值
        DataBaseRaw &db_; ///< 所属数据库引用

    public:
        /**
         * @brief 构造函数
         * @param db 所属数据库引用
         * @param name 键名称
         * @param defval 默认值
         */
        template <typename T>
            requires std::is_base_of_v<DataBaseRaw, T>
        Key(T &db, const char *name, const DType &defval = DType{})
            : db_(db), KeyRaw(name, RawData{data_})
        {
            if (ErrorCode::NO_FOUND == db_.Get(*this))
            {
                data_ = defval;
                db_.Add(*this);
            }
        }

        /**
         * @brief 类型转换操作符
         * @return 键值引用
         */
        operator const DType &() const { return data_; }

        /**
         * @brief 获取键值
         * @return 键值引用
         */
        const DType &Get() const { return data_; }

        /**
         * @brief 从数据库加载键值
         * @return 错误码
         */
        ErrorCode Load() { return db_.Get(*this); }

        /**
         * @brief 设置键值并保存到数据库
         * @param value 键值
         * @return 错误码
         */
        ErrorCode Set(const DType &value)
        {
            data_ = value;
            return db_.Set(*this);
        }

        /**
         * @brief 赋值操作符重载
         * @param value 键值
         * @return 错误码
         */
        ErrorCode operator=(const DType &value) { return Set(value); }
    };

    /**
     * @brief 数据库类，基于Flash实现简单的键值存储
     * @note 该数据库类不保证多线程情况下的数据一致性，使用时请自行保证线程安全
     */
    class DataBase : public DataBaseRaw
    {
        DECL_STATIC_PROPERTY(private, uint32_t, DATABASE_HEADER, (0xDEADBEAF + PROPERTY(DATABASE_VERSION))) ///< 数据库文件头标识
        DECL_STATIC_PROPERTY(private, uint8_t, BLOCK_HEADER, 0x55)                                          ///< 块头标识
        DECL_STATIC_PROPERTY(private, uint8_t, BLOCK_TAIL, 0xAA)                                            ///< 块尾标识
        DECL_STATIC_PROPERTY(private, uint8_t, KEY_HEADER, 0x5A)                                            ///< 键头标识

        DECL_STATIC_PROPERTY(private, uint32_t, MAX_KEY_NAME_LEN, 255) ///< 最大键名称长度
        DECL_STATIC_PROPERTY(private, uint32_t, MAX_DATA_LEN, 4096)    ///< 最大数据长度

        DECL_PROPERTY(size_t, blockSize, 512)        ///< 块大小，单位为字节
        DECL_PROPERTY(size_t, databaseHeaderSize, 8) ///< 数据库头大小，单位为字节
        DECL_PROPERTY(size_t, recycleSize, 0)        ///< 触发回收的使用大小

    private:
        /**
         * @brief 键状态枚举定义
         */
        enum class KeyStatus : uint8_t
        {
            FREE = 0xF,    ///< 空闲
            WRITING = 0xB, ///< 写入中
            USED = 0x9,    ///< 已使用
            BACKUP = 0x8,  ///< 备份
            DELETED = 0x0, ///< 已删除
        };

        /**
         * @brief 键块结构体定义
         */
        struct [[gnu::packed]] KeyBlock
        {
            uint32_t header : 8;       ///< 块头标识
            uint32_t key_name_len : 8; ///< 键名称长度
            uint32_t data_len : 12;    ///< 数据长度
            uint32_t status : 4;       ///< 块状态
            // 后续为键名称和数据内容
        };

        static_assert(sizeof(KeyBlock) == 4, "KeyBlock结构体大小错误"); ///< 静态断言，确保KeyBlock结构体大小为4字节

        /**
         * @brief 块状态枚举定义
         */
        enum class BlockStatus : uint8_t
        {
            FREE = 0xFF,  ///< 空闲
            USED = 0xFA,  ///< 已使用
            FULL = 0xAA,  ///< 已满
            DIRTY = 0x88, ///< 脏块
        };

        /**
         * @brief 块头结构体定义
         */
        struct [[gnu::packed]] BlockHeader
        {
            uint8_t block_header; ///< 块头标识，固定为BLOCK_HEADER
            BlockStatus status;   ///< 块状态
            bool is_first : 1;    ///< 是否为第一个块
            bool has_next : 1;    ///< 是否有下一个块
            uint8_t block_tail;   ///< 块尾标识，固定为BLOCK_TAIL
        };

        static_assert(sizeof(BlockHeader) == 4, "BlockHeader结构体大小错误"); ///< 静态断言，确保BlockHeader结构体大小为4字节

        /**
         * @brief 数据库头结构体定义
         */
        struct [[gnu::packed]] DataBaseHeader
        {
            uint32_t database_header; ///< 文件头标识，固定为DATABASE_HEADER
            uint8_t version;          ///< 数据库版本号
            uint16_t block_size;      ///< 块大小，单位为字节
            uint8_t check_sum;        ///< 校验码
        };

        static_assert(sizeof(DataBaseHeader) == 8, "DataBaseHeader结构体大小错误"); ///< 静态断言，确保DataBaseHeader结构体大小为8字节

        Flash flash_{};    ///< flash设备
        RawData buffer_{}; ///< 缓冲区

        DataBaseHeader header_{}; ///< 数据库头信息
        bool is_open_{};          ///< 数据库是否已打开
        bool is_initialized_{};   ///< 数据库是否已初始化

        uint32_t *blockAvailableSizeArray_{}; ///< 块可用大小数组指针
        size_t block_count_{};                ///< 块数量

        /**
         * @brief 检查数据库头是否有效
         * @return 数据库头是否有效
         */
        bool CheckDatabaseHeader();

        /**
         * @brief 检查块头是否有效
         * @param block_header 块头引用
         * @param block_index 块索引
         * @return 块头是否有效
         */
        bool CheckBlockHeader(const BlockHeader &block_header, uint32_t block_index);

        /**
         * @brief 检查键块是否有效
         * @param key_block 键块引用
         * @return 键块是否有效
         */
        bool CheckKeyHeader(const KeyBlock &key_block);

        /**
         * @brief 检查键名称是否匹配
         * @param key 键对象引用
         * @param key_block 键块引用
         * @return 键名称是否匹配
         */
        bool CheckKeyName(const KeyRaw &key, const KeyBlock &key_block);

        /**
         * @brief 获取块在Flash中的偏移地址
         * @param block_index 块索引
         * @return 块在Flash中的偏移地址
         */
        uint32_t GetBlockOffset(uint32_t block_index) const;

        /**
         * @brief 根据偏移地址获取块索引
         * @param offset 偏移地址
         * @return 块索引
         */
        uint32_t GetBlockIndexByOffset(uint32_t offset) const;

        /**
         * @brief 获取块内第一个键的偏移地址
         * @param block_index 块索引
         * @return 第一个键的偏移地址 | 错误码
         */
        Result<uint32_t> GetFirstKeyOffset(uint32_t block_index);

        /**
         * @brief 获取该块内下一个键的偏移地址
         * @param key_block 键块引用
         * @param current_offset 当前键的偏移地址
         * @return 下一个键的偏移地址 | 错误码
         */
        Result<uint32_t> GetNextKeyOffset(const KeyBlock &key_block, uint32_t current_offset);

        /**
         * @brief 查找键
         * @param key 键对象引用
         * @return 错误码
         */
        ErrorCode SearchKey(KeyRaw &key);

        /**
         * @brief 加载块头
         * @param block_index 块索引
         * @return 块头对象 | 错误码
         */
        Result<BlockHeader> LoadBlockHeader(uint32_t block_index);

        /**
         * @brief 从Flash加载块数据
         * @param block_index 块索引
         */
        ErrorCode LoadBlock(uint32_t block_index);

        /**
         * @brief 加载键块
         * @param key_offset 键偏移地址
         * @param key_block 键块引用
         * @return 错误码
         */
        ErrorCode LoadKeyBlock(uint32_t key_offset, KeyBlock &key_block);

        /**
         * @brief 加载flash键数据到缓冲区
         * @param key 键对象引用
         * @return 键块对象 | 错误码
         */
        Result<KeyBlock> LoadKeyToBuffer(KeyRaw &key);

        /**
         * @brief 获取可用块索引
         * @param size 需要的大小
         * @return 可用块索引 | 错误码
         */
        Result<uint32_t> GetAvailableBlockIndex(uint32_t size);

        /**
         * @brief 更新块可用大小
         * @param block_index 块索引
         */
        void UpdateBlockAvailableSize(uint32_t block_index);

        /**
         * @brief 保存块头到Flash
         * @param block_index 块索引
         * @param header 块头对象
         * @return 错误码
         */
        ErrorCode SaveBlockHeader(uint32_t block_index, const BlockHeader header);

        /**
         * @brief 保存键块到Flash
         * @param key_offset 键偏移地址
         * @param key_block 键块对象
         * @return 错误码
         */
        ErrorCode SaveKeyBlock(uint32_t key_offset, const KeyBlock &key_block);

        /**
         * @brief 保存键到Flash
         * @param key_offset 键偏移地址
         * @param key 键数据缓冲区
         * @return 错误码
         */
        ErrorCode SaveKey(uint32_t key_offset, ConstRawData key);

        /**
         * @brief 初始化数据库
         * @note 将重新格式化Flash，清除所有数据
         */
        void Init();

        /**
         * @brief 回收数据库空间
         */
        void Recycle();

        /**
         * @brief 检查是否需要回收
         * @return 是否需要回收
         */
        bool CheckRecycleNeeded() const;

    public:
        /**
         * @brief 构造函数
         */
        constexpr DataBase()
        {
        }

        /**
         * @brief 构造函数
         * @param flash_name Flash设备名称
         * @param buffer_size 缓冲区大小，默认512字节
         */
        DataBase(const char *flash_name, size_t block_size = 512);

        /**
         * @brief 重置数据库
         * @note 将重新格式化Flash，清除所有数据
         */
        FORCE_INLINE void Reset()
        {
            Init();
        }

        /**
         * @brief 获取键值
         * @param key 键对象引用
         * @return 错误码
         */
        ErrorCode Get(KeyRaw &key) override;

        /**
         * @brief 设置键值
         * @param key 键对象引用
         * @return 错误码
         */
        ErrorCode Set(KeyRaw &key) override;

        /**
         * @brief 添加新键值
         * @param key 键对象引用
         * @return 错误码
         */
        ErrorCode Add(KeyRaw &key) override;
    };
}