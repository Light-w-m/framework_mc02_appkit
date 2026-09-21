#pragma once

#include <common_time.hpp>
#include <matrix.hpp>

#include <cfloat>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <array>
#include <algorithm>

namespace algorithm
{
    class VQF
    {
    public:
        using Scalar = float; // 标量类型
        using Double = double; // 双精度类型

        template <std::size_t ROWS>
        using Vector = appkit::math::Vector<Scalar, ROWS>;  // 向量类型

        template <std::size_t ROWS>
        using VectorD = appkit::math::Vectord<ROWS>;

        template <std::size_t ROWS, std::size_t COLS>
        using Matrix = appkit::math::Matrix<Scalar, ROWS, COLS>; // 矩阵类型

        template <std::size_t ROWS, std::size_t COLS>
        using MatrixD = appkit::math::Matrixd<ROWS, COLS>;

    private:
        struct Coeffs final
        {
            Scalar gyrTs{};                         ///< 陀螺仪采样周期
            Scalar accTs{};                         ///< 加速度计采样周期
            std::array<Double, 3> accLpB{};         ///< 加速度计低通滤波器分子系数 (b0, b1, b2)
            std::array<Double, 2> accLpA{};         ///< 加速度计低通滤波器分母系数 (a1, a2)
            Scalar biasP0{};                        ///< 零偏初始协方差
            Scalar biasV{};                         ///< 零偏过程噪声
            Scalar biasMotionW{};                   ///< 运动零偏权重
            Scalar biasVerticalW{};                 ///< 垂直零偏权重
            Scalar biasRestW{};                     ///< 静止零偏权重
            std::array<Double, 3> restGyrLpB{};     ///< 静止陀螺仪低通分子系数
            std::array<Double, 2> restGyrLpA{};     ///< 静止陀螺仪低通分母系数
            std::array<Double, 3> restAccLpB{};     ///< 静止加速度计低通分子系数
            std::array<Double, 2> restAccLpA{};     ///< 静止加速度计低通分母系数
        };

        struct State final {
            VectorD<4> gyrQuat{};                   ///< 陀螺仪积分得到的四元数 
            VectorD<4> accQuat{};                   ///< 加速度计修正后的四元数 
            bool restDetected{false};               ///< 是否检测到静止状态
            VectorD<3> lastAccLp{};                 ///< 上一时刻加速度计低通滤波值
            std::array<Double, 6> accLpState{};     ///< 加速度计低通滤波器状态 
            Scalar lastAccCorrAngularRate{};        ///< 上一次加速度计修正角速率
            VectorD<3> bias{};                      ///< 当前零偏估计值
            MatrixD<3, 3> biasP{};                  ///< 零偏协方差矩阵 
            std::array<Double, 18> motionBiasEstRLpState{};   ///< 运动零偏估计旋转矩阵低通状态
            std::array<Double, 4> motionBiasEstBiasLpState{}; ///< 运动零偏估计零偏低通状态
            std::array<Scalar, 2> restLastSquaredDeviations{}; ///< 静止检测的偏差平方
            Scalar restT{};                         ///< 静止检测计时器
            VectorD<3> restLastGyrLp{};             ///< 静止检测陀螺仪低通滤波值
            std::array<Double, 6> restGyrLpState{}; ///< 静止陀螺仪低通状态
            VectorD<3> restLastAccLp{};             ///< 静止检测加速度计低通滤波值
            std::array<Double, 6> restAccLpState{}; ///< 静止加速度计低通状态
        };

        /* ==================== 内部数据 ==================== */

        Coeffs coeffs_{}; ///< 由 Init 计算得到的滤波系数
        State state_{};   ///< 滤波器状态

        static constexpr Scalar EPS{FLT_EPSILON};                  ///< 零阈值（沿用源文件的 FLT_EPSILON）
        static constexpr Double DEG2RAD{std::numbers::pi / 180.0}; ///< 度转弧度
        static constexpr Double RAD2DEG{180.0 / std::numbers::pi}; ///< 弧度转度

        /**
         * @brief 平方
         * @param x 输入
         * @return T x 的平方
         */
        template <typename T>
        [[nodiscard]] static constexpr T square(T x) noexcept
        {
            return x * x;
        }

        /**
         * @brief 取较大值
         * @tparam T 比较类型
         * @param a 左操作数
         * @param b 右操作数
         * @return T a 与 b 中的较大者
         */
        template <typename T>
        [[nodiscard]] static constexpr T max(T a, T b) noexcept
        {
            return a > b ? a : b;
        }

        /**
         * @brief 向量二范数
         * @tparam T 元素类型
         * @param vec 输入向量首地址
         * @param N 元素个数
         * @return T 向量的模长
         */
        template <typename T>
        [[nodiscard]] [[using gnu: optimize("O3")]] static T norm(const T vec[], std::size_t N) noexcept
        {
            T s = 0;
            for (std::size_t i = 0; i < N; ++i)
            {
                s += vec[i] * vec[i];
            }
            return std::sqrt(s);
        }

        /**
         * @brief 向量原地归一化
         * @tparam T 元素类型
         * @param vec 输入输出向量首地址
         * @param N 元素个数
         * @note 模长小于 EPS 时不做任何修改
         */
        template <typename T>
        [[using gnu: optimize("O3")]] static void normalize(T vec[], std::size_t N) noexcept
        {
            const T n = norm(vec, N);
            if (n < EPS)
            {
                return;
            }
            for (std::size_t i = 0; i < N; ++i)
            {
                vec[i] /= n;
            }
        }

        /**
         * @brief 逐元素限幅
         * @tparam T 向量元素类型
         * @tparam U 上下限类型
         * @param vec 输入输出向量首地址
         * @param N 元素个数
         * @param min 下限
         * @param max 上限
         */
        template <typename T, typename U>
        [[using gnu: optimize("O3")]] static void clip(T vec[], std::size_t N, U min, U max) noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (vec[i] < min)
                    vec[i] = min;
                else if (vec[i] > max)
                    vec[i] = max;
            }
        }

        /**
         * @brief 四元数乘法 out = q1 * q2，四元数按 (w, x, y, z) 排列
         * @tparam T 元素类型
         * @param q1 左乘四元数
         * @param q2 右乘四元数
         * @param out 结果四元数
         * @note out 允许与 q1 或 q2 为同一块内存
         */
        template <typename T>
        [[using gnu: always_inline, optimize("O3")]] static void quatMultiply(const T q1[4], const T q2[4], T out[4]) noexcept
        {
            const T w = q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3];
            const T x = q1[0] * q2[1] + q1[1] * q2[0] + q1[2] * q2[3] - q1[3] * q2[2];
            const T y = q1[0] * q2[2] - q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1];
            const T z = q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0];
            out[0] = w;
            out[1] = x;
            out[2] = y;
            out[3] = z;
        }

        /**
         * @brief 用四元数旋转向量 out = R(q) * v
         * @tparam T 元素类型
         * @param q 四元数，按 (w, x, y, z) 排列
         * @param v 输入向量
         * @param out 输出向量
         * @note out 允许与 v 为同一块内存
         */
        template <typename T>
        [[using gnu: always_inline, optimize("O3")]] static void quatRotate(const T q[4], const T v[3], T out[3]) noexcept
        {
            const T x = (1 - 2 * q[2] * q[2] - 2 * q[3] * q[3]) * v[0] + 2 * v[1] * (q[2] * q[1] - q[0] * q[3]) + 2 * v[2] * (q[0] * q[2] + q[3] * q[1]);
            const T y = 2 * v[0] * (q[0] * q[3] + q[2] * q[1]) + v[1] * (1 - 2 * q[1] * q[1] - 2 * q[3] * q[3]) + 2 * v[2] * (q[2] * q[3] - q[1] * q[0]);
            const T z = 2 * v[0] * (q[3] * q[1] - q[0] * q[2]) + 2 * v[1] * (q[0] * q[1] + q[3] * q[2]) + v[2] * (1 - 2 * q[1] * q[1] - 2 * q[2] * q[2]);
            out[0] = x;
            out[1] = y;
            out[2] = z;
        }

        /**
         * @brief 由时间常数和采样周期计算二阶低通滤波器的 b/a 系数
         * @tparam T tau 与 Ts 的输入类型
         * @param tau 时间常数
         * @param Ts 采样周期
         * @param outB 分子系数输出，长度 3
         * @param outA 分母系数输出，长度 2
         */
        template <typename T>
        [[using gnu: optimize("O3")]] static void filterCoeffs(T tau, T Ts, Double outB[3], Double outA[2]) noexcept
        {
            const Double fc = (std::numbers::sqrt2 / (2.0 * std::numbers::pi)) / static_cast<Double>(tau);
            const Double C = std::tan(std::numbers::pi * fc * static_cast<Double>(Ts));
            const Double D = C * C + std::numbers::sqrt2 * C + 1;
            const Double b0 = C * C / D;
            outB[0] = b0;
            outB[1] = 2 * b0;
            outB[2] = b0;
            outA[0] = 2 * (C * C - 1) / D;
            outA[1] = (1 - std::numbers::sqrt2 * C + C * C) / D;
        }

        /**
         * @brief 计算滤波器的初始状态，使阶跃输入 x0 下无暂态
         * @tparam T x0 的类型
         * @param x0 初始输入值
         * @param b 分子系数，长度 3
         * @param a 分母系数，长度 2
         * @param out 滤波器状态输出，长度 2
         */
        template <typename T>
        [[using gnu: optimize("O3")]] static void filterInitialState(T x0, const Double b[3], const Double a[2], Double out[2]) noexcept
        {
            out[0] = x0 * (1 - b[0]);
            out[1] = x0 * (b[2] - a[1]);
        }

        /**
         * @brief 单通道低通滤波器单步迭代
         * @param x 本次输入
         * @param b 分子系数，长度 3
         * @param a 分母系数，长度 2
         * @param state 滤波器状态，长度 2
         * @return Double 滤波输出
         * @note 状态量与内部运算为 double；赋给 float 输出时截断
         */
        [[nodiscard]] [[using gnu: always_inline, optimize("O3")]] static Double filterStep(Double x, const Double b[3], const Double a[2], Double state[2]) noexcept
        {
            const Double y = b[0] * x + state[0];
            state[0] = b[1] * x - a[0] * y + state[1];
            state[1] = b[2] * x - a[1] * y;
            return y;
        }

        /**
         * @brief 多通道低通滤波，支持启动阶段的均值预热
         * @tparam T 输入输出元素类型
         * @param x 输入向量首地址，长度 N
         * @param N 通道数
         * @param tau 时间常数
         * @param Ts 采样周期
         * @param b 分子系数，长度 3
         * @param a 分母系数，长度 2
         * @param state 滤波器状态，长度 2*N
         * @param out 滤波输出，长度 N，允许与 x 为同一块内存
         * @note 启动阶段以 state[0] 为 NaN 作标志：用前若干个样本的均值预热，
         *       累计时长达到 tau 后再交给 filterStep 正式滤波
         */
        template <typename T>
        [[using gnu: always_inline, optimize("O3")]] static void filterVec(const T x[], std::size_t N, Scalar tau, Scalar Ts, const Double b[3],
                                                                          const Double a[2], Double state[], T out[]) noexcept
        {
            if (std::isnan(state[0]))
            {
                if (std::isnan(state[1]))
                {
                    state[1] = 0;
                    for (std::size_t i = 0; i < N; ++i)
                    {
                        state[2 + i] = 0;
                    }
                }
                state[1]++;
                for (std::size_t i = 0; i < N; ++i)
                {
                    state[2 + i] += x[i];
                    out[i] = state[2 + i] / state[1];
                }
                if (state[1] * Ts >= tau)
                {
                    for (std::size_t i = 0; i < N; ++i)
                    {
                        filterInitialState(out[i], b, a, state + 2 * i);
                    }
                }
                return;
            }

            for (std::size_t i = 0; i < N; ++i)
            {
                out[i] = filterStep(x[i], b, a, state + 2 * i);
            }
        }

        /**
         * @brief 将 3x3 矩阵置为 scale * I
         * @tparam T 矩阵元素类型
         * @tparam U scale 的类型
         * @param scale 缩放系数
         * @param out 输出矩阵，长度 9
         */
        template <typename T, typename U>
        [[using gnu: optimize("O3")]] static void matrix3SetToScaledIdentity(U scale, T out[9]) noexcept
        {
            for (std::size_t i = 0; i < 9; ++i)
            {
                out[i] = 0;
            }
            out[0] = out[4] = out[8] = scale;
        }

        /**
         * @brief 3x3 矩阵乘法 out = in1 * in2
         * @tparam T 元素类型
         * @param in1 左矩阵，长度 9
         * @param in2 右矩阵，长度 9
         * @param out 输出矩阵，长度 9，允许与 in1 或 in2 为同一块内存
         */
        template <typename T>
        [[using gnu: always_inline, optimize("O3")]] static void matrix3Multiply(const T in1[9], const T in2[9], T out[9]) noexcept
        {
            T tmp[9];
            tmp[0] = in1[0] * in2[0] + in1[1] * in2[3] + in1[2] * in2[6];
            tmp[1] = in1[0] * in2[1] + in1[1] * in2[4] + in1[2] * in2[7];
            tmp[2] = in1[0] * in2[2] + in1[1] * in2[5] + in1[2] * in2[8];
            tmp[3] = in1[3] * in2[0] + in1[4] * in2[3] + in1[5] * in2[6];
            tmp[4] = in1[3] * in2[1] + in1[4] * in2[4] + in1[5] * in2[7];
            tmp[5] = in1[3] * in2[2] + in1[4] * in2[5] + in1[5] * in2[8];
            tmp[6] = in1[6] * in2[0] + in1[7] * in2[3] + in1[8] * in2[6];
            tmp[7] = in1[6] * in2[1] + in1[7] * in2[4] + in1[8] * in2[7];
            tmp[8] = in1[6] * in2[2] + in1[7] * in2[5] + in1[8] * in2[8];
            for (std::size_t i = 0; i < 9; ++i)
            {
                out[i] = tmp[i];
            }
        }

        /**
         * @brief 3x3 矩阵乘法 out = in1^T * in2
         * @tparam T 元素类型
         * @param in1 左矩阵，长度 9
         * @param in2 右矩阵，长度 9
         * @param out 输出矩阵，长度 9，允许与 in1 或 in2 为同一块内存
         */
        template <typename T>
        [[using gnu: always_inline, optimize("O3")]] static void matrix3MultiplyTpsFirst(const T in1[9], const T in2[9], T out[9]) noexcept
        {
            T tmp[9];
            tmp[0] = in1[0] * in2[0] + in1[3] * in2[3] + in1[6] * in2[6];
            tmp[1] = in1[0] * in2[1] + in1[3] * in2[4] + in1[6] * in2[7];
            tmp[2] = in1[0] * in2[2] + in1[3] * in2[5] + in1[6] * in2[8];
            tmp[3] = in1[1] * in2[0] + in1[4] * in2[3] + in1[7] * in2[6];
            tmp[4] = in1[1] * in2[1] + in1[4] * in2[4] + in1[7] * in2[7];
            tmp[5] = in1[1] * in2[2] + in1[4] * in2[5] + in1[7] * in2[8];
            tmp[6] = in1[2] * in2[0] + in1[5] * in2[3] + in1[8] * in2[6];
            tmp[7] = in1[2] * in2[1] + in1[5] * in2[4] + in1[8] * in2[7];
            tmp[8] = in1[2] * in2[2] + in1[5] * in2[5] + in1[8] * in2[8];
            for (std::size_t i = 0; i < 9; ++i)
            {
                out[i] = tmp[i];
            }
        }

        /**
         * @brief 3x3 矩阵乘法 out = in1 * in2^T
         * @tparam T 元素类型
         * @param in1 左矩阵，长度 9
         * @param in2 右矩阵，长度 9
         * @param out 输出矩阵，长度 9，允许与 in1 或 in2 为同一块内存
         */
        template <typename T>
        [[using gnu: always_inline, optimize("O3")]] static void matrix3MultiplyTpsSecond(const T in1[9], const T in2[9], T out[9]) noexcept
        {
            T tmp[9];
            tmp[0] = in1[0] * in2[0] + in1[1] * in2[1] + in1[2] * in2[2];
            tmp[1] = in1[0] * in2[3] + in1[1] * in2[4] + in1[2] * in2[5];
            tmp[2] = in1[0] * in2[6] + in1[1] * in2[7] + in1[2] * in2[8];
            tmp[3] = in1[3] * in2[0] + in1[4] * in2[1] + in1[5] * in2[2];
            tmp[4] = in1[3] * in2[3] + in1[4] * in2[4] + in1[5] * in2[5];
            tmp[5] = in1[3] * in2[6] + in1[4] * in2[7] + in1[5] * in2[8];
            tmp[6] = in1[6] * in2[0] + in1[7] * in2[1] + in1[8] * in2[2];
            tmp[7] = in1[6] * in2[3] + in1[7] * in2[4] + in1[8] * in2[5];
            tmp[8] = in1[6] * in2[6] + in1[7] * in2[7] + in1[8] * in2[8];
            for (std::size_t i = 0; i < 9; ++i)
            {
                out[i] = tmp[i];
            }
        }

        /**
         * @brief 3x3 矩阵求逆
         * @tparam T 元素类型
         * @param in 输入矩阵，长度 9
         * @param out 输出矩阵，长度 9，允许与 in 为同一块内存
         * @return bool true 求逆成功；false 行列式接近 0，out 被置零
         */
        template <typename T>
        [[using gnu: always_inline, optimize("O3")]] static bool matrix3Inv(const T in[9], T out[9]) noexcept
        {
            const Double A = in[4] * in[8] - in[5] * in[7];
            const Double D = in[2] * in[7] - in[1] * in[8];
            const Double G = in[1] * in[5] - in[2] * in[4];
            const Double B = in[5] * in[6] - in[3] * in[8];
            const Double E = in[0] * in[8] - in[2] * in[6];
            const Double H = in[2] * in[3] - in[0] * in[5];
            const Double C = in[3] * in[7] - in[4] * in[6];
            const Double F = in[1] * in[6] - in[0] * in[7];
            const Double I = in[0] * in[4] - in[1] * in[3];
            const Double det = in[0] * A + in[1] * B + in[2] * C;
            if (det >= -EPS && det <= EPS)
            {
                for (std::size_t i = 0; i < 9; ++i)
                {
                    out[i] = 0;
                }
                return false;
            }
            out[0] = A / det;
            out[1] = D / det;
            out[2] = G / det;
            out[3] = B / det;
            out[4] = E / det;
            out[5] = H / det;
            out[6] = C / det;
            out[7] = F / det;
            out[8] = I / det;
            return true;
        }
    
    public:
        struct Output final {
            Vector<4> q{};                          ///< 最终姿态四元数
            float pitch{};                          ///< 俯仰角
            float roll{};                           ///< 横滚角
            float yaw{};                            ///< 偏航角
            float yaw_laps{};                       ///< 偏航圈数
            float last_yaw{};                       ///< 上一次偏航角
            float YawTotalAngle{};                  ///< 累积偏航角
            Matrix<3, 3> rMat{};                    ///< 旋转矩阵
        };

        Output output{};

        /**
         * @brief 可调参数，须在 Init 之前设置
         */
        struct Param final
        {
            Scalar tauAcc{3.0f};                           ///< 加速度计低通滤波时间常数
            bool motionBiasEstEnabled{true};               ///< 是否启用运动中的零偏估计
            bool restBiasEstEnabled{true};                 ///< 是否启用静止时的零偏估计
            Scalar biasSigmaInit{0.5f};                    ///< 零偏初始标准差
            Scalar biasForgettingTime{100.0f};             ///< 零偏遗忘时间
            Scalar biasClip{2.0f};                         ///< 零偏限幅
            Scalar biasSigmaMotion{0.1f};                  ///< 运动零偏标准差
            Scalar biasVerticalForgettingFactor{0.0001f};  ///< 垂直方向零偏遗忘因子
            Scalar biasSigmaRest{0.03f};                   ///< 静止零偏标准差
            Scalar restMinT{1.5f};                         ///< 静止检测最短时间
            Scalar restFilterTau{0.5f};                    ///< 静止检测滤波时间常数
            Scalar restThGyr{2.0f};                        ///< 静止检测陀螺仪阈值
            Scalar restThAcc{0.5f};                        ///< 静止检测加速度计阈值
        };

        Param param{}; ///< 滤波器可调参数

        /**
         * @brief 构造函数
         */
        constexpr VQF() noexcept
        {
        }

        /**
         * @brief 初始化：由采样周期计算全部滤波系数并复位状态
         * @param gyrTs 陀螺仪采样周期，单位 s
         * @param accTs 加速度计采样周期，单位 s
         */
        void Init(Scalar gyrTs, Scalar accTs) noexcept;

        /**
         * @brief 初始化，陀螺仪与加速度计采样周期相同
         * @param dt 采样周期，单位 s
         */
        [[using gnu: always_inline]] void Init(Scalar dt) noexcept
        {
            Init(dt, dt);
        }

        /**
         * @brief 单步更新
         * @param gyr 陀螺仪测量值
         * @param acc 加速度计测量值
         */
        void Update(const Vector<3> &gyr, const Vector<3> &acc) noexcept;

        /**
         * @brief 由当前姿态四元数更新欧拉角与累积偏航角
         */
        void UpdateOutput() noexcept;
    };

    [[using gnu: optimize("O3")]] void VQF::Init(Scalar gyrTs, Scalar accTs) noexcept
    {
        /* ==================== 由采样周期计算系数 ==================== */

        coeffs_.gyrTs = gyrTs;
        coeffs_.accTs = accTs;

        filterCoeffs(param.tauAcc, coeffs_.accTs, coeffs_.accLpB.data(), coeffs_.accLpA.data());

        coeffs_.biasP0 = square(param.biasSigmaInit * 100.0f);
        coeffs_.biasV = square(0.1f * 100.0f) * coeffs_.accTs / param.biasForgettingTime;

        const Scalar pMotion = square(param.biasSigmaMotion * 100.0f);
        coeffs_.biasMotionW = square(pMotion) / coeffs_.biasV + pMotion;
        coeffs_.biasVerticalW = coeffs_.biasMotionW / max(param.biasVerticalForgettingFactor, 1e-10f);

        const Scalar pRest = square(param.biasSigmaRest * 100.0f);
        coeffs_.biasRestW = square(pRest) / coeffs_.biasV + pRest;

        filterCoeffs(param.restFilterTau, coeffs_.gyrTs, coeffs_.restGyrLpB.data(), coeffs_.restGyrLpA.data());
        filterCoeffs(param.restFilterTau, coeffs_.accTs, coeffs_.restAccLpB.data(), coeffs_.restAccLpA.data());

        /* ==================== 复位状态 ==================== */

        state_.gyrQuat = VectorD<4>{1, 0, 0, 0};
        state_.accQuat = VectorD<4>{1, 0, 0, 0};
        state_.restDetected = false;
        state_.lastAccLp = VectorD<3>{};
        state_.bias = VectorD<3>{};
        matrix3SetToScaledIdentity(coeffs_.biasP0, state_.biasP.data());

        // 低通状态置 NaN，filterVec 以此作为启动预热的标志
        state_.accLpState.fill(NAN);
        state_.restGyrLpState.fill(NAN);
        state_.restAccLpState.fill(NAN);
        state_.motionBiasEstRLpState.fill(NAN);
        state_.motionBiasEstBiasLpState.fill(NAN);
        state_.restLastGyrLp = VectorD<3>{NAN, NAN, NAN};
        state_.restLastAccLp = VectorD<3>{};
        state_.restLastSquaredDeviations.fill(0.0f);
        state_.restT = 0.0f;

        output = Output{};
    }

    [[using gnu: optimize("O3")]] void VQF::Update(const Vector<3> &gyr, const Vector<3> &acc) noexcept
    {
        const auto &[gx, gy, gz] = gyr;
        const auto &[ax, ay, az] = acc;

        // 输入落到局部数组
        const Double gyrBuf[3] = {static_cast<Double>(gx), static_cast<Double>(gy), static_cast<Double>(gz)};
        const Double accBuf[3] = {static_cast<Double>(ax), static_cast<Double>(ay), static_cast<Double>(az)};

        // 状态别名，直接读写成员，免去来回拷贝
        Double *gyrQuat = state_.gyrQuat.data();
        Double *accQuat = state_.accQuat.data();
        Double *bias = state_.bias.data();
        Double *biasP = state_.biasP.data();
        Double *lastAccLp = state_.lastAccLp.data();
        Double *restLastGyrLp = state_.restLastGyrLp.data();
        Double *restLastAccLp = state_.restLastAccLp.data();

        const Double biasClipRad = static_cast<Double>(param.biasClip) * DEG2RAD;

        /* ----------------- 静止检测：陀螺仪低通与偏差 ----------------- */
        if (param.restBiasEstEnabled)
        {
            filterVec(gyrBuf, 3, param.restFilterTau, coeffs_.gyrTs, coeffs_.restGyrLpB.data(), coeffs_.restGyrLpA.data(),
                      state_.restGyrLpState.data(), restLastGyrLp);

            Double sqDev = 0;
            for (std::size_t i = 0; i < 3; ++i)
            {
                sqDev += square(gyrBuf[i] - restLastGyrLp[i]);
            }
            state_.restLastSquaredDeviations[0] = static_cast<Scalar>(sqDev);

            if (state_.restLastSquaredDeviations[0] >= square(param.restThGyr * DEG2RAD) ||
                std::abs(restLastGyrLp[0]) > biasClipRad || std::abs(restLastGyrLp[1]) > biasClipRad ||
                std::abs(restLastGyrLp[2]) > biasClipRad)
            {
                state_.restT = 0.0f;
                state_.restDetected = false;
            }
        }

        /* ----------------- 陀螺仪积分 ----------------- */
        const Double gyrNoBias[3] = {gyrBuf[0] - bias[0], gyrBuf[1] - bias[1], gyrBuf[2] - bias[2]};
        const Double gyrNorm = norm(gyrNoBias, 3);
        const Double angle = gyrNorm * static_cast<Double>(coeffs_.gyrTs);
        if (gyrNorm > EPS)
        {
            const Double c = std::cos(angle / 2);
            const Double s = std::sin(angle / 2) / gyrNorm;
            const Double gyrStepQuat[4] = {c, s * gyrNoBias[0], s * gyrNoBias[1], s * gyrNoBias[2]};
            quatMultiply(gyrQuat, gyrStepQuat, gyrQuat);
            normalize(gyrQuat, 4);
        }

        /* ----------------- 加速度计修正 ----------------- */
        if (accBuf[0] == 0.0 && accBuf[1] == 0.0 && accBuf[2] == 0.0)
        {
            return;
        }

        if (param.restBiasEstEnabled)
        {
            filterVec(accBuf, 3, param.restFilterTau, coeffs_.accTs, coeffs_.restAccLpB.data(), coeffs_.restAccLpA.data(),
                      state_.restAccLpState.data(), restLastAccLp);

            Double sqDev = 0;
            for (std::size_t i = 0; i < 3; ++i)
            {
                sqDev += square(accBuf[i] - restLastAccLp[i]);
            }
            state_.restLastSquaredDeviations[1] = static_cast<Scalar>(sqDev);

            if (state_.restLastSquaredDeviations[1] >= square(param.restThAcc))
            {
                state_.restT = 0.0f;
                state_.restDetected = false;
            }
            else
            {
                state_.restT += coeffs_.accTs;
                if (state_.restT >= param.restMinT)
                {
                    state_.restDetected = true;
                }
            }
        }

        Double accEarth[3];
        quatRotate(gyrQuat, accBuf, accEarth);
        filterVec(accEarth, 3, param.tauAcc, coeffs_.accTs, coeffs_.accLpB.data(), coeffs_.accLpA.data(),
                  state_.accLpState.data(), lastAccLp);

        quatRotate(accQuat, lastAccLp, accEarth);
        normalize(accEarth, 3);

        Double accCorrQuat[4];
        const Double qW = std::sqrt((accEarth[2] + 1) / 2);
        if (qW > 1e-6)
        {
            accCorrQuat[0] = qW;
            accCorrQuat[1] = 0.5 * accEarth[1] / qW;
            accCorrQuat[2] = -0.5 * accEarth[0] / qW;
            accCorrQuat[3] = 0;
        }
        else
        {
            accCorrQuat[0] = 0;
            accCorrQuat[1] = 1;
            accCorrQuat[2] = 0;
            accCorrQuat[3] = 0;
        }
        quatMultiply(accCorrQuat, accQuat, accQuat);
        normalize(accQuat, 4);

        state_.lastAccCorrAngularRate = static_cast<Scalar>(std::acos(accEarth[2]) / static_cast<Double>(coeffs_.accTs));

        /* ----------------- 卡尔曼零偏估计 ----------------- */
        if (param.motionBiasEstEnabled || param.restBiasEstEnabled)
        {
            Double accGyrQuat[4];
            Double r[9];
            Double biasLp[2];

            quatMultiply(accQuat, gyrQuat, accGyrQuat);
            r[0] = 1 - 2 * square(accGyrQuat[2]) - 2 * square(accGyrQuat[3]);
            r[1] = 2 * (accGyrQuat[2] * accGyrQuat[1] - accGyrQuat[0] * accGyrQuat[3]);
            r[2] = 2 * (accGyrQuat[0] * accGyrQuat[2] + accGyrQuat[3] * accGyrQuat[1]);
            r[3] = 2 * (accGyrQuat[0] * accGyrQuat[3] + accGyrQuat[2] * accGyrQuat[1]);
            r[4] = 1 - 2 * square(accGyrQuat[1]) - 2 * square(accGyrQuat[3]);
            r[5] = 2 * (accGyrQuat[2] * accGyrQuat[3] - accGyrQuat[1] * accGyrQuat[0]);
            r[6] = 2 * (accGyrQuat[3] * accGyrQuat[1] - accGyrQuat[0] * accGyrQuat[2]);
            r[7] = 2 * (accGyrQuat[0] * accGyrQuat[1] + accGyrQuat[3] * accGyrQuat[2]);
            r[8] = 1 - 2 * square(accGyrQuat[1]) - 2 * square(accGyrQuat[2]);

            biasLp[0] = r[0] * bias[0] + r[1] * bias[1] + r[2] * bias[2];
            biasLp[1] = r[3] * bias[0] + r[4] * bias[1] + r[5] * bias[2];

            filterVec(r, 9, param.tauAcc, coeffs_.accTs, coeffs_.accLpB.data(), coeffs_.accLpA.data(),
                      state_.motionBiasEstRLpState.data(), r);
            filterVec(biasLp, 2, param.tauAcc, coeffs_.accTs, coeffs_.accLpB.data(), coeffs_.accLpA.data(),
                      state_.motionBiasEstBiasLpState.data(), biasLp);

            Double w[3];
            Double e[3];
            if (state_.restDetected && param.restBiasEstEnabled)
            {
                for (std::size_t i = 0; i < 3; ++i)
                {
                    e[i] = restLastGyrLp[i] - bias[i];
                }
                matrix3SetToScaledIdentity(1.0, r);
                w[0] = w[1] = w[2] = static_cast<Double>(coeffs_.biasRestW);
            }
            else if (param.motionBiasEstEnabled)
            {
                e[0] = -accEarth[1] / coeffs_.accTs + biasLp[0] - r[0] * bias[0] - r[1] * bias[1] - r[2] * bias[2];
                e[1] = accEarth[0] / coeffs_.accTs + biasLp[1] - r[3] * bias[0] - r[4] * bias[1] - r[5] * bias[2];
                e[2] = -r[6] * bias[0] - r[7] * bias[1] - r[8] * bias[2];
                w[0] = static_cast<Double>(coeffs_.biasMotionW);
                w[1] = w[0];
                w[2] = static_cast<Double>(coeffs_.biasVerticalW);
            }
            else
            {
                w[0] = w[1] = w[2] = -1.0;
            }

            if (biasP[0] < coeffs_.biasP0)
            {
                biasP[0] += coeffs_.biasV;
            }
            if (biasP[4] < coeffs_.biasP0)
            {
                biasP[4] += coeffs_.biasV;
            }
            if (biasP[8] < coeffs_.biasP0)
            {
                biasP[8] += coeffs_.biasV;
            }

            if (w[0] >= 0)
            {
                clip(e, 3, -biasClipRad, biasClipRad);

                Double K[9];
                matrix3MultiplyTpsSecond(biasP, r, K);
                matrix3Multiply(r, K, K);
                K[0] += w[0];
                K[4] += w[1];
                K[8] += w[2];
                matrix3Inv(K, K);
                matrix3MultiplyTpsFirst(r, K, K);
                matrix3Multiply(biasP, K, K);

                bias[0] += K[0] * e[0] + K[1] * e[1] + K[2] * e[2];
                bias[1] += K[3] * e[0] + K[4] * e[1] + K[5] * e[2];
                bias[2] += K[6] * e[0] + K[7] * e[1] + K[8] * e[2];

                matrix3Multiply(K, r, K);
                matrix3Multiply(K, biasP, K);
                for (std::size_t i = 0; i < 9; ++i)
                {
                    biasP[i] -= K[i];
                }

                clip(bias, 3, -biasClipRad, biasClipRad);
            }
        }
    }

    [[using gnu: optimize("O3")]] void VQF::UpdateOutput() noexcept
    {
        Double q[4];
        quatMultiply(state_.accQuat.data(), state_.gyrQuat.data(), q);

        for (std::size_t i = 0; i < 4; ++i)
        {
            output.q.data()[i] = static_cast<float>(q[i]);
        }

        const float *q4 = output.q.data();
        const float q0 = q4[0], q1 = q4[1], q2 = q4[2], q3 = q4[3];

        output.pitch = std::asin(std::clamp(2.0f * (q0 * q2 - q1 * q3), -1.0f, 1.0f)) * static_cast<float>(RAD2DEG);
        output.roll = std::atan2(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) * static_cast<float>(RAD2DEG);
        output.yaw = std::atan2(2.0f * (q1 * q2 + q0 * q3), 1.0f - 2.0f * (q2 * q2 + q3 * q3)) * static_cast<float>(RAD2DEG);

        float yawDiff = output.yaw - output.last_yaw;
        if (yawDiff > 180.0f)
        {
            yawDiff -= 360.0f;
            output.yaw_laps--;
        }
        else if (yawDiff < -180.0f)
        {
            yawDiff += 360.0f;
            output.yaw_laps++;
        }
        output.YawTotalAngle += yawDiff;
        output.last_yaw = output.yaw;

        const float w = q0, x = q1, y = q2, z = q3;
        output.rMat = Matrix<3, 3>{1 - 2 * y * y - 2 * z * z, 2 * (y * x - w * z), 2 * (w * y + z * x),
                                   2 * (w * z + y * x), 1 - 2 * x * x - 2 * z * z, 2 * (y * z - x * w),
                                   2 * (z * x - w * y), 2 * (w * x + z * y), 1 - 2 * x * x - 2 * y * y};
    }
} // namespace algorithm
