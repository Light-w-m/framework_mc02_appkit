#pragma once

#include <common_cb.hpp>
#include <lockfree_list.hpp>
#include <driver.hpp>

namespace appkit
{
    /**
     * @brief CAN设备
     */
    class CAN final
    {
    public:
        /**
         * @brief 数据包类型
         */
        enum class PackType : uint8_t
        {
            STANDARD = 0,        ///< 标准帧
            EXTENDED = 1,        ///< 扩展帧
            REMOTE_STANDARD = 2, ///< 远程标准帧
            REMOTE_EXTENDED = 3, ///< 远程扩展帧
        };

        /**
         * @brief 经典CAN数据包结构体
         */
        struct ClassicPack final
        {
            uint32_t id;      ///< 标识符
            PackType type;    ///< 数据包类型
            uint8_t data_len; ///< 数据长度
            uint8_t data[8];  ///< 数据内容
        };

        /**
         * @brief 过滤模式
         */
        enum class FilterMode : uint8_t
        {
            MASK = 0, ///< 掩码过滤
            RANGE,    ///< 范围过滤
            EQUAL     ///< 等于过滤
        };

        /**
         * @brief 回调函数类型
         */
        using Callback = appkit::Callback<const ClassicPack &>;

        /**
         * @brief 过滤器结构体
         */
        struct Filter final
        {
            FilterMode mode;        ///< 过滤模式
            uint32_t mask_start_id; ///< 掩码或范围起始ID
            uint32_t match_end_id;  ///< 匹配或范围结束ID
            PackType type;          ///< 数据包类型
            Callback callback;      ///< 回调函数
        };

        /**
         * @brief 构造函数
         */
        constexpr CAN() = default;

        /**
         * @brief 打开CAN设备
         * @param name 设备名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 检查设备是否已打开
         * @return 设备是否已打开
         */
        FORCE_INLINE bool IsOpen() const { return block_ != nullptr; }

        /**
         * @brief 注册过滤器
         * @param callback 回调函数
         * @param type 数据包类型
         * @param mode 过滤模式
         * @param mask_start_id 掩码或范围起始ID
         * @param match_end_id 匹配或范围结束ID
         * @return 错误码
         */
        [[nodiscard]] ErrorCode Register(const Callback &callback, PackType type, FilterMode mode = FilterMode::RANGE,
                                         uint32_t mask_start_id = 0, uint32_t match_end_id = UINT32_MAX) const;

        /**
         * @brief 添加数据包
         * @param frame 数据包
         * @return 错误码
         */
        [[nodiscard]] ErrorCode AddMessage(const ClassicPack &frame) const;

    public:
        /**
         * @brief CAN控制器块基类
         */
        class Block : public Driver<Block>
        {
        public:
            LockFreeList id_filter_list[4]{}; ///< ID过滤列表

            /**
             * @brief 析构函数
             */
            virtual ~Block() = default;

            /**
             * @brief 添加数据包
             * @param frame 数据包
             */
            virtual ErrorCode AddMessage(const ClassicPack &frame) = 0;

        protected:
            /**
             * @brief 收到数据包回调
             * @param frame 数据包
             * @param in_isr 是否在中断中调用
             */
            void OnMessage(const ClassicPack &frame, bool in_isr);
        };

    private:
        Block *block_{nullptr}; ///< CAN控制器块
    };

    /**
     * @brief FDCAN设备
     */
    class FDCAN final
    {
    public:
        /**
         * @brief 数据包类型
         */
        using PackType = CAN::PackType;

        /**
         * @brief FDCAN数据包结构体
         */
        struct FDPack final
        {
            uint32_t id;      ///< 标识符
            PackType type;    ///< 数据包类型
            uint8_t data_len; ///< 数据长度
            uint8_t data[64]; ///< 数据内容
        };

        /**
         * @brief 过滤模式
         */
        using FilterMode = CAN::FilterMode;

        /**
         * @brief 回调函数类型
         */
        using Callback = appkit::Callback<const FDPack &>;

        /**
         * @brief 过滤器结构体
         */
        struct Filter final
        {
            FilterMode mode;        ///< 过滤模式
            uint32_t mask_start_id; ///< 掩码或范围起始ID
            uint32_t match_end_id;  ///< 匹配或范围结束ID
            PackType type;          ///< 数据包类型
            Callback callback;      ///< 回调函数
        };

        /**
         * @brief 构造函数
         */
        constexpr FDCAN() = default;

        /**
         * @brief 打开FDCAN设备
         * @param name 设备名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 检查设备是否已打开
         * @return 设备是否已打开
         */
        bool IsOpen() const { return block_ != nullptr; }

        /**
         * @brief 注册过滤器
         * @param callback 回调函数
         * @param type 数据包类型AN
         * @param mode 过滤模式
         * @param mask_start_id 掩码或范围起始ID
         * @param match_end_id 匹配或范围结束ID
         * @return 错误码
         */
        [[nodiscard]] ErrorCode Register(const Callback &callback, PackType type, FilterMode mode = FilterMode::RANGE,
                                         uint32_t mask_start_id = 0, uint32_t match_end_id = UINT32_MAX) const;

        /**
         * @brief 添加数据包
         * @param frame 数据包
         * @return 错误码
         */
        [[nodiscard]] ErrorCode AddMessage(const FDPack &frame) const;

    public:
        /**
         * @brief FDCAN控制器块基类
         */
        class Block : public Driver<Block>
        {
        public:
            LockFreeList id_filter_list[4]{}; ///< ID过滤列表

            /**
             * @brief 析构函数
             */
            virtual ~Block() = default;

            /**
             * @brief 添加数据包
             * @param frame 数据包
             * @return 错误码
             */
            virtual ErrorCode AddMessage(const FDPack &frame) = 0;

        protected:
            /**
             * @brief 收到数据包回调
             * @param frame 数据包
             * @param in_isr 是否在中断中调用
             */
            void OnMessage(const FDPack &frame, bool in_isr);
        };

    private:
        Block *block_{nullptr}; ///< FDCAN控制器块
    };
} // namespace appkit
