/**
 * @file common_cb.h
 * @brief 通用回调定义
 * @author dusk
 * @date 2025-12-25
 */
#pragma once

#include <macros.hpp>

namespace appkit
{
    /**
     * @brief 通用回调类
     * @tparam ATypes 回调参数类型列表
     */
    template <typename... ATypes>
    class Callback final
    {
    private:
        using FuncType = void (*)(bool, void *, ATypes...); ///< 回调函数类型

        void *cbBlock_{};   ///< 回调块智能指针
        FuncType cbFunc_{}; ///< 回调函数指针

        /**
         * @brief 构造函数
         * @param cb_block 回调块智能指针
         * @param cb_func 回调函数指针
         */
        FORCE_INLINE Callback(void *cb_block, FuncType cb_func)
            : cbBlock_(cb_block), cbFunc_(cb_func)
        {
        }

    public:
        /**
         * @brief 默认构造函数
         */
        Callback() = default;

        /**
         * @brief 创建回调对象
         * @tparam FType 函数类型
         * @tparam AType 参数类型
         * @param arg 回调参数
         * @param func 回调函数
         * @return 回调对象
         * @note 函数原型可以是 `void func(bool in_isr, AType arg, ATypes... args)` 或 `void func(bool in_isr, const AType &arg, ATypes... args)`
         */
        template <typename Args, typename FType>
            requires std::disjunction_v<std::is_invocable_r<void, FType, bool, Args, ATypes...>, std::is_invocable_r<void, FType, bool, const Args &, ATypes...>>
        static Callback Create(FType func, Args arg)
        {
            /**
             * @brief 回调块结构体
             */
            struct CallbackBlock final
            {
                FType func;
                Args arg;
            };

            // 定义回调函数
            static auto cb_func = +[](bool in_isr, void *cb_block, ATypes... args)
            {
                auto cb = static_cast<CallbackBlock *>(cb_block);
                cb->func(in_isr, cb->arg, std::forward<ATypes>(args)...);
            };

            void *cb_block = new CallbackBlock(func, arg);

            return Callback{cb_block, cb_func};
        }

        /**
         * @brief 创建无参数回调对象
         * @tparam FType 函数类型
         * @param func 回调函数
         * @return 回调对象
         * @note 函数原型必须是 `void func(bool in_isr, ATypes... args)`
         */
        template <typename FType>
            requires std::is_invocable_r_v<void, FType, bool, ATypes...>
        static Callback Create(FType func)
        {
            /**
             * @brief 回调块结构体
             */
            struct CallbackBlock final
            {
                FType func;
            };

            // 定义回调函数
            static auto cb_func = +[](bool in_isr, void *cb_block, ATypes... args)
            {
                auto cb = static_cast<CallbackBlock *>(cb_block);
                cb->func(in_isr, std::forward<ATypes>(args)...);
            };

            void *cb_block = new CallbackBlock(func);

            return Callback{cb_block, cb_func};
        }

        /**
         * @brief 调用回调函数
         * @tparam PassArgs 传递参数类型
         * @param in_isr 是否在中断中调用
         * @param args 传递参数
         */
        void Call(bool in_isr, auto &&...args) const
        {
            if (cbFunc_) [[likely]]
            {
                cbFunc_(in_isr, cbBlock_, std::forward<decltype(args)>(args)...);
            }
        }

        /**
         * @brief 检查回调是否为空
         * @return 是否为空
         */
        FORCE_INLINE bool Empty() const
        {
            return !(cbBlock_ && cbFunc_);
        }

        /**
         * @brief 重载布尔类型转换运算符
         * @return 是否非空
         */
        FORCE_INLINE operator bool() const
        {
            return cbBlock_ && cbFunc_;
        }
    };
}