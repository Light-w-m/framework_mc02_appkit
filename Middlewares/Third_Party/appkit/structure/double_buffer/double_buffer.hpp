#pragma once

#include <common_type.hpp>

namespace appkit
{
    /**
     * @brief 双缓冲区封装
     */
    class DoubleBuffer
    {
        const size_t capacity_; ///< 缓冲区容量（每个缓冲区）
        RawData buffer_[2]{};   ///< 双缓冲区

        uint8_t active_{0};         ///< 当前活动缓冲区索引，0或1
        bool pending_valid_{false}; ///< 待切换缓冲区是否有效
        size_t pending_length_{0};  ///< 待切换缓冲区已填充数据长度

    public:
        /**
         * @brief 构造函数
         * @param raw_data 用于双缓冲区的原始数据
         * @note raw_data的大小必须大于等于2
         */
        explicit DoubleBuffer(RawData &raw_data);

        /**
         * @brief 获取当前活动缓冲区
         * @return 当前活动缓冲区
         */
        [[nodiscard]] FORCE_INLINE RawData &GetActiveBuffer()
        {
            return buffer_[active_];
        }

        /**
         * @brief 获取待切换缓冲区
         * @return 待切换缓冲区
         */
        [[nodiscard]] FORCE_INLINE RawData &GetPendingBuffer()
        {
            return buffer_[1 - active_];
        }

        /**
         * @brief 获取当前活动缓冲区（常量）
         * @return 当前活动缓冲区
         */
        [[nodiscard]] FORCE_INLINE ConstRawData GetActiveBuffer() const
        {
            return ConstRawData{buffer_[active_]};
        }

        /**
         * @brief 获取待切换缓冲区（常量）
         * @return 待切换缓冲区
         */
        [[nodiscard]] FORCE_INLINE ConstRawData GetPendingBuffer() const
        {
            return ConstRawData{buffer_[1 - active_]};
        }

        /**
         * @brief 获取缓冲区容量
         * @return 缓冲区容量
         * @note 缓冲区容量为构造函数传入的raw_data大小的一半
         * @note 如果raw_data大小为奇数，则缓冲区容量为(raw_data大小 - 1) / 2
         */
        [[nodiscard]] FORCE_INLINE size_t GetCapacity() const
        {
            return capacity_;
        }

        /**
         * @brief 切换缓冲区
         * @param force 是否强制切换
         * @note 如果force为true，则强制切换缓冲区
         * @note 如果force为false，则只有在待切换缓冲区有效时才切换缓冲区
         * @note 切换缓冲区后，待切换缓冲区无效，已填充数据长度清零
         */
        void Switch(bool force = false);

        /**
         * @brief 待切换缓冲区是否有效
         * @return 待切换缓冲区是否有效
         * @note 如果待切换缓冲区已填充数据，则返回true，否则返回false
         */
        [[nodiscard]] FORCE_INLINE bool IsPendingValid() const
        {
            return pending_valid_;
        }

        /**
         * @brief 向待切换缓冲区填充数据
         * @param buffer 数据引用
         * @return 是否填充成功
         * @note 如果buffer大小大于缓冲区剩余容量，则返回false，否则返回true
         * @note 如果填充成功，待切换缓冲区已填充数据长度增加buffer大小
         */
        bool FillPendingBuffer(ConstRawData buffer);

        /**
         * @brief 向当前活动缓冲区填充数据
         * @param buffer 数据引用
         * @return 是否填充成功
         * @note 如果buffer大小大于缓冲区容量，则返回false，否则返回true
         * @note 如果填充成功，当前活动缓冲区数据被buffer覆盖
         */
        bool FillActiveBuffer(ConstRawData buffer);

        /**
         * @brief 获取待切换缓冲区已填充数据长度
         * @return 待切换缓冲区已填充数据长度
         * @note 如果待切换缓冲区无效，返回0
         */
        [[nodiscard]] FORCE_INLINE size_t GetPendingLength() const
        {
            return pending_length_;
        }

        /**
         * @brief 设置待切换缓冲区已填充数据长度
         * @param length 已填充数据长度
         * @note 该函数不会检查length是否合法
         */
        void SetPendingUsed(size_t length);
    };
}
