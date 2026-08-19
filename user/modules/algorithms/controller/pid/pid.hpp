#pragma once

#include <common_time.hpp>
#include <algorithm>

namespace algorithm
{
    /**
     * @brief PID控制器基类
     */
    class PID
    {
    public:
        /**
         * @brief PID参数结构体
         */
        struct Param final
        {
            float kp{}; ///< 比例系数
            float ki{}; ///< 积分系数
            float kd{}; ///< 微分系数

            float max_output{};     ///< 最大输出，等于0表示不限制
            float dead_zone{};      ///< 死区，等于0表示无死区
            float integral_limit{}; ///< 积分限幅，等于0表示不限制

            bool use_derivative_measure{};  ///< 是否使用测量值的微分(微分先行)
            bool use_integral_saturation{}; ///< 是否使用积分饱和（当输出达到最大值时停止积分）
        };

    protected:
        Param param_; // PID参数

        /**
         * @brief 最大输出限幅
         * @param output 输出值
         */
        void f_MaxOutput(float &output) const noexcept;

        /**
         * @brief 死区处理
         * @param error 误差值
         */
        void f_DeadZone(float &error) const noexcept;

        /**
         * @brief 积分限幅
         * @param integral 积分值
         */
        void f_IntegralLimit(float &integral) const noexcept;

        /**
         * @brief 积分饱和处理
         * @param i_item 积分项输出变化量
         * @param error 当前误差
         */
        void f_IntegralSaturation(float &i_item, float error) const noexcept;

    public:
        /**
         * @brief 构造函数
         * @param param PID参数结构体
         */
        constexpr PID(Param param) noexcept : param_(param) {}

        /**
         * @brief 更新PID控制器
         * @param target 目标值
         * @param measure 测量值
         * @param dt 时间间隔
         * @return 控制输出
         * @note 时间差dt单位为秒
         */
        virtual float Update(float target, float measure, const appkit::Duration &dt) noexcept = 0;

        /**
         * @brief 重置PID状态
         */
        virtual void Reset() noexcept = 0;

        /**
         * @brief 获取PID参数
         * @return PID参数结构体
         */
        FORCE_INLINE const Param &GetParam() const noexcept
        {
            return param_;
        }

        /**
         * @brief 设置PID参数
         * @param param PID参数结构体
         */
        FORCE_INLINE void SetParam(const Param &param) noexcept
        {
            param_ = param;
        }
    };

    /**
     * @brief 位置式PID控制器
     * @note output = kp * error + ki * sum(error) * dt + kd * (error - last_error) / dt
     */
    class PositionPID final : public PID
    {
    public:
        /**
         * @brief 构造函数
         * @note 使用基类构造函数
         */
        using PID::PID;

        /**
         * @brief 更新位置式PID控制器
         * @param target 目标值
         * @param measure 测量值
         * @param dt 时间间隔
         * @return 控制输出
         * @note 时间差dt单位为秒
         */
        float Update(float target, float measure, const appkit::Duration &dt) noexcept override;

        /**
         * @brief 重置PID状态
         */
        void Reset() noexcept override;

    private:
        float lastError_{};    ///< 上次误差
        float lastMeasure_{};  ///< 上次测量值
        float integral_{0.0f}; ///< 积分值

        float pOut_{}; ///< 比例项输出
        float iOut_{}; ///< 积分项输出
        float dOut_{}; ///< 微分项输出
    };

    /**
     * @brief 增量式PID控制器
     * @note output_delta = kp * delta_error + ki * error * dt + kd * (delta_error - last_delta_error) / dt
     */
    class IncrementalPID final : public PID
    {
    public:
        /**
         * @brief 构造函数
         * @note 使用基类构造函数
         */
        using PID::PID;

        /**
         * @brief 更新位置式PID控制器
         * @param target 目标值
         * @param measure 测量值
         * @param dt 时间间隔
         * @return 控制输出
         * @note 时间差dt单位为秒
         */
        float Update(float target, float measure, const appkit::Duration &dt) noexcept override;

        /**
         * @brief 重置PID状态
         */
        void Reset() noexcept override;

    private:
        float lastError_{};  ///< 上次误差
        float lastError2_{}; ///< 上上次误差

        float lastMeasure_{};  ///< 上次测量值
        float lastMeasure2_{}; ///< 上上次测量值

        float integral_{0.0f}; ///< 积分值

        float pOut_{};   ///< 比例项输出
        float iOut_{};   ///< 积分项输出
        float dOut_{};   ///< 微分项输出
        float output_{}; ///< 输出值
    };

    /**
     * @brief 模糊PID控制器
     */
    class FuzzyPID final : public PID
    {
    private:
        /**
         * @brief 模糊集合枚举
         */
        enum class FuzzySet : int8_t
        {
            NB = -3, ///< Negative Big 负大
            NM = -2, ///< Negative Medium 负中
            NS = -1, ///< Negative Small 负小
            ZO = 0,  ///< Zero 零
            PS = 1,  ///< Positive Small 正小
            PM = 2,  ///< Positive Medium 正中
            PB = 3   ///< Positive Big 正大
        };

        /**
         * @brief kp模糊规则表
         */
        static constexpr FuzzySet kpFuzzySet[7][7] = {
            /* e\ec             NB            NM            NS            ZO            PS            PM            PB */
            /* NB */ {FuzzySet::PB, FuzzySet::PB, FuzzySet::PM, FuzzySet::PM, FuzzySet::PS, FuzzySet::ZO, FuzzySet::ZO},
            /* NM */ {FuzzySet::PB, FuzzySet::PB, FuzzySet::PM, FuzzySet::PS, FuzzySet::PS, FuzzySet::ZO, FuzzySet::NS},
            /* NS */ {FuzzySet::PM, FuzzySet::PM, FuzzySet::PM, FuzzySet::PS, FuzzySet::ZO, FuzzySet::NS, FuzzySet::NS},
            /* ZO */ {FuzzySet::PM, FuzzySet::PM, FuzzySet::PS, FuzzySet::ZO, FuzzySet::NS, FuzzySet::NM, FuzzySet::NM},
            /* PS */ {FuzzySet::PS, FuzzySet::PS, FuzzySet::ZO, FuzzySet::NS, FuzzySet::NS, FuzzySet::NM, FuzzySet::NM},
            /* PM */ {FuzzySet::PS, FuzzySet::ZO, FuzzySet::NS, FuzzySet::NM, FuzzySet::NM, FuzzySet::NM, FuzzySet::NB},
            /* PB */ {FuzzySet::ZO, FuzzySet::ZO, FuzzySet::NM, FuzzySet::NM, FuzzySet::NM, FuzzySet::NB, FuzzySet::NB}};

        /**
         * @brief ki模糊规则表
         */
        static constexpr FuzzySet kiFuzzySet[7][7] = {
            /* e\ec             NB            NM            NS            ZO            PS            PM            PB */
            /* NB */ {FuzzySet::NB, FuzzySet::NB, FuzzySet::NM, FuzzySet::NM, FuzzySet::NS, FuzzySet::ZO, FuzzySet::ZO},
            /* NM */ {FuzzySet::NB, FuzzySet::NB, FuzzySet::NM, FuzzySet::NS, FuzzySet::NS, FuzzySet::ZO, FuzzySet::ZO},
            /* NS */ {FuzzySet::NB, FuzzySet::NM, FuzzySet::NS, FuzzySet::NS, FuzzySet::ZO, FuzzySet::PS, FuzzySet::PS},
            /* ZO */ {FuzzySet::NM, FuzzySet::NM, FuzzySet::NS, FuzzySet::ZO, FuzzySet::PS, FuzzySet::PM, FuzzySet::PM},
            /* PS */ {FuzzySet::NM, FuzzySet::NS, FuzzySet::ZO, FuzzySet::PS, FuzzySet::PS, FuzzySet::NM, FuzzySet::PB},
            /* PM */ {FuzzySet::ZO, FuzzySet::ZO, FuzzySet::PS, FuzzySet::PS, FuzzySet::PM, FuzzySet::PB, FuzzySet::PB},
            /* PB */ {FuzzySet::ZO, FuzzySet::ZO, FuzzySet::PS, FuzzySet::PM, FuzzySet::PM, FuzzySet::PB, FuzzySet::PB}};

        /**
         * @brief kd模糊规则表
         */
        static constexpr FuzzySet kdFuzzySet[7][7] = {
            /* e\ec             NB            NM            NS            ZO            PS            PM            PB */
            /* NB */ {FuzzySet::PS, FuzzySet::NS, FuzzySet::NB, FuzzySet::NB, FuzzySet::NB, FuzzySet::NM, FuzzySet::PS},
            /* NM */ {FuzzySet::PS, FuzzySet::NS, FuzzySet::NB, FuzzySet::NM, FuzzySet::NM, FuzzySet::NS, FuzzySet::ZO},
            /* NS */ {FuzzySet::ZO, FuzzySet::NS, FuzzySet::NM, FuzzySet::NM, FuzzySet::NS, FuzzySet::NS, FuzzySet::ZO},
            /* ZO */ {FuzzySet::ZO, FuzzySet::NS, FuzzySet::NS, FuzzySet::NS, FuzzySet::NS, FuzzySet::NS, FuzzySet::ZO},
            /* PS */ {FuzzySet::ZO, FuzzySet::ZO, FuzzySet::ZO, FuzzySet::ZO, FuzzySet::ZO, FuzzySet::ZO, FuzzySet::ZO},
            /* PM */ {FuzzySet::PB, FuzzySet::NS, FuzzySet::PS, FuzzySet::PS, FuzzySet::PS, FuzzySet::PS, FuzzySet::PB},
            /* PB */ {FuzzySet::PB, FuzzySet::PM, FuzzySet::PM, FuzzySet::PM, FuzzySet::PS, FuzzySet::PS, FuzzySet::PB}};

    public:
        /**
         * @brief 模糊PID参数结构体
         */
        struct FuzzyParam final
        {
            PID::Param baseParam; ///< 基础PID参数

            float ke{};  ///< 误差模糊化系数
            float kec{}; ///< 误差变化率模糊化系数

            float kp_scale{}; ///< 比例系数调整量缩放因子
            float ki_scale{}; ///< 积分系数调整量缩放因子
            float kd_scale{}; ///< 微分系数调整量缩放因子

            float max_kp{}; ///< 最大比例系数
            float max_ki{}; ///< 最大积分系数
            float max_kd{}; ///< 最大微分系数
        };

        /**
         * @brief 构造函数
         * @param param 模糊PID参数结构体
         */
        constexpr FuzzyPID(FuzzyParam param) noexcept : PID(param.baseParam), fuzzyParam_(param), pid_(param.baseParam) {}

        /**
         * @brief 更新模糊PID控制器
         * @param target 目标值
         * @param measure 测量值
         * @param dt 时间间隔
         * @return 控制输出
         */
        float Update(float target, float measure, const appkit::Duration &dt) noexcept override;

        /**
         * @brief 重置PID状态
         */
        void Reset() noexcept override;

        /**
         * @brief 获取模糊PID参数
         * @return 模糊PID参数结构体
         */
        FORCE_INLINE const FuzzyParam &GetFuzzyParam() const noexcept
        {
            return fuzzyParam_;
        }

        /**
         * @brief 设置模糊PID参数
         * @param param 模糊PID参数结构体
         */
        void SetFuzzyParam(const FuzzyParam &param) noexcept;

    private:
        FuzzyParam fuzzyParam_; ///< 模糊PID参数
        PositionPID pid_;       ///< 内部位置式PID控制器

        float lastError_{}; ///< 上次误差

        /**
         * @brief 模糊化函数
         * @param value 输入值
         * @param scale 模糊化系数
         * @return 模糊集合索引
         */
        size_t Fuzzify(float value, float scale) const noexcept;
    };
} // namespace algorithm
