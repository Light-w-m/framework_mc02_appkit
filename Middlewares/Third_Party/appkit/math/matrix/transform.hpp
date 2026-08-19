#pragma once

#include <quaternion.hpp>
#include <matrix.hpp>

namespace appkit::math
{
    /**
     * @brief 变换类
     */
    class Transform final
    {
    private:
        Matrixf<4, 4> matrix_;  ///< 4x4变换矩阵

    public:
        /**
         * @brief 默认构造函数，初始化为单位矩阵
         */
        constexpr Transform() noexcept : matrix_(Matrixf<4, 4>::Identity()) {}

        /**
         * @brief 使用给定矩阵构造变换
         * @param matrix 4x4变换矩阵
         */
        explicit constexpr Transform(const Matrixf<4, 4> &matrix) noexcept : matrix_(matrix) {}

        /**
         * @brief 使用旋转四元数和平移向量构造变换
         * @param rotation 旋转四元数
         * @param translation 平移向量
         */
        explicit constexpr Transform(const Quaternionf &rotation, const Vectorf<3> &translation = Vectorf<3>::Zero()) noexcept
            : matrix_(Matrixf<4, 4>::Identity())
        {
            matrix_.GetBlock<3, 3>(0, 0) = rotation.ToRotationMatrix();
            matrix_.GetBlock<3, 1>(0, 3) = translation;
        }

        /**
         * @brief 获取变换矩阵
         * @return 4x4变换矩阵常量引用
         */
        FORCE_INLINE constexpr const Matrixf<4, 4> &GetMatrix() const noexcept
        {
            return matrix_;
        }

        /**
         * @brief 获取变换矩阵的转置
         * @return 转置后的变换
         */
        FORCE_INLINE constexpr Transform Transpose() const noexcept
        {
            return Transform(matrix_.Transpose());
        }

        /**
         * @brief 获取变换矩阵的逆
         * @return 逆变换
         */
        constexpr Transform Inverse() const noexcept
        {
            Matrixf<4, 4> inv_matrix = Matrixf<4, 4>::Identity();
            Matrixf<3, 3> rot_transpose = matrix_.GetBlock<3, 3>(0, 0).Transpose();
            Vectorf<3> trans = matrix_.GetBlock<3, 1>(0, 3);

            inv_matrix.GetBlock<3, 3>(0, 0) = rot_transpose;
            inv_matrix.GetBlock<3, 1>(0, 3) = -rot_transpose * trans;

            return Transform(inv_matrix);
        }

        /**
         * @brief 变换乘法运算符
         * @param other 其他变换
         * @return 复合变换
         */
        constexpr Transform operator*(const Transform &other) const noexcept
        {
            return Transform(matrix_ * other.matrix_);
        }

        /**
         * @brief 使用变换作用于3D点
         * @param point 3D点向量
         * @return 变换后的3D点向量
         */
        constexpr Vectorf<3> operator*(const Vectorf<3> &point) const noexcept
        {
            const Vectorf<4> homogenous_point{point.X(), point.Y(), point.Z(), 1.0f};
            const Vectorf<4> transformed_point = matrix_ * homogenous_point;
            return transformed_point.GetBlock<3, 1>(0, 0);
        }

        /**
         * @brief 使用变换作用于4D点
         * @param point 4D点向量
         * @return 变换后的4D点向量
         */
        FORCE_INLINE constexpr Vectorf<4> operator*(const Vectorf<4> &point) const noexcept
        {
            return matrix_ * point;
        }

        /**
         * @brief 获取旋转矩阵和平移向量
         * @return 旋转矩阵和平移向量
         */
        FORCE_INLINE constexpr Matrixf<3, 3> GetRotationMatrix() const noexcept
        {
            return matrix_.GetBlock<3, 3>(0, 0);
        }

        /**
         * @brief 获取平移向量
         * @return 平移向量
         */
        FORCE_INLINE constexpr Vectorf<3> GetTranslation() const noexcept
        {
            return matrix_.GetBlock<3, 1>(0, 3);
        }
    };
} // namespace appkit::math
