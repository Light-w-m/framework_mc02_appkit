#pragma once

#include <common_cb.hpp>

#include <ramfs.hpp>
#include <lockfree_list.hpp>

namespace appkit
{
    /**
     * @brief 事件类，用于事件管理和回调注册
     */
    class Event final
    {
    private:
        /**
         * @brief 事件块结构体，包含事件状态和回调列表
         */
        struct Block
        {
            bool masked{true};
            LockFreeList list_{};
        };

    public:
        using Callback = appkit::Callback<uint32_t>;

        /**
         * @brief 默认构造函数
         */
        constexpr Event()
        {
        }

        /**
         * @brief 打开或创建一个事件
         * @param name 事件名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 注册事件回调
         * @param cb 回调函数
         * @return 错误码
         */
        ErrorCode Register(Callback cb);

        /**
         * @brief 屏蔽或使能事件
         * @param enable 使能标志，true为使能，false为屏蔽
         */
        void Mask(bool enable);

        /**
         * @brief 检查事件是否使能
         * @return true为使能，false为屏蔽
         */
        [[nodiscard]] bool IsMasked() const;

        /**
         * @brief 激活事件，触发所有注册的回调
         * @param value 传递给回调的值
         * @return 错误码
         */
        FORCE_INLINE ErrorCode Activate(uint32_t value) const
        {
            return ActivateFromCallback(false, value);
        }

        /**
         * @brief 从中断上下文激活事件，触发所有注册的回调
         * @param in_isr 是否在中断上下文中调用
         * @param value 传递给回调的值
         * @return 错误码
         */
        ErrorCode ActivateFromCallback(bool in_isr, uint32_t value) const;

        /**
         * @brief 获取事件回调列表
         * @return 回调列表指针
         */
        LockFreeList *GetList() const;

        /**
         * @brief 绑定事件
         * @note 绑定另一个事件，当源事件被激活时，当前事件也会被激活
         * @param source 源事件
         * @return 错误码
         */
        ErrorCode Bind(Event &source);

    private:
        RamFs::File *file_{}; ///< 事件文件指针
        Block *block_{};      ///< 事件块指针

        static RamFs::Dir &event_dict_; ///< 事件目录
    };
}
