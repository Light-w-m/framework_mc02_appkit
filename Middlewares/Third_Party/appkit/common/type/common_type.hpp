/**
 * @file common_type.h
 * @brief 通用类型定义
 * @author dusk
 * @date 2025-12-25
 */
#pragma once

#include <macros.hpp>
#include <string_view>

namespace appkit
{
    /**
     * @brief 错误码枚举定义
     * @enum ErrorCode
     */
    enum class ErrorCode : int32_t
    {
        OK = 0,             ///< 成功
        FAILED = -1,        ///< 失败
        INIT_FAILED = -2,   ///< 初始化失败
        NO_FOUND = -3,      ///< 未找到
        NOT_SUPPORTED = -4, ///< 不支持
        NO_MEMORY = -5,     ///< 内存耗尽
        NO_BUFFER = -6,     ///< 缓冲区不存在
        INVALID_ARG = -7,   ///< 参数错误
        TIMEOUT = -8,       ///< 超时
        OUT_OF_RANGE = -9,  ///< 超出范围
        FULL = -10,         ///< 缓冲区已满
        EMPTY = -11,        ///< 缓冲区为空
        BUSY = -12,         ///< 任务忙
    };

    /**
     * @brief 检查错误码是否为成功
     * @param code 错误码
     * @return 是否成功
     */
    FORCE_INLINE constexpr bool Check(const ErrorCode code)
    {
        return code == ErrorCode::OK;
    }

    /**
     * @brief 强制在编译期求值
     * @tparam DType 数据类型
     * @param value 数据值
     * @return 编译期求值结果
     */
    template <typename DType>
    FORCE_INLINE consteval DType EvalData(DType value)
    {
        return value;
    }

    class TypeInfo final
    {
		/**
		 * @brief 类型信息
		 */
		struct alignas(SYSTEM_CACHE_LINE_SIZE) Info
		{
			std::string_view name{};   ///< 类型名称
			size_t size{};			   ///< 类型大小
		};

		/**
		 * @brief 获取当前函数的函数签名
		 */
		template <typename DType>
		static consteval std::string_view PrettyFunction()
		{
			return __PRETTY_FUNCTION__;
		}

	public:
		/**
		 * @brief 类型标签
		 */
		using ID = std::add_pointer_t<std::add_const_t<Info>>;

		/**
		 * @brief 获取类型标签
		 */
		template <typename DType>
		FORCE_INLINE static consteval ID GetID()
		{
			return &tagInfo_<std::decay_t<DType>>;
		}

		/**
		 * @brief 获取类型大小（编译期）
		 */
		template <typename DType>
		FORCE_INLINE static consteval size_t GetSize()
		{
			return sizeof(DType);
		}

		/**
		 * @brief 获取类型大小（运行期）
         * @param id 类型标签
		 */
		FORCE_INLINE static constexpr size_t GetSize(const ID &id)
		{
			return id->size;
		}

	private:
		/**
		 * @brief 获取类型名称（编译期）
		 * @return 类型名称
		 * @note 该函数在不同编译器下的表现可能不同，建议仅用于调试和日志输出
		 */
		template <typename DType>
		FORCE_INLINE static consteval std::string_view GetName()
		{
			std::string_view name = PrettyFunction<DType>();
			name.remove_prefix(sizeof("static consteval std::string_view appkit::TypeInfo::PrettyFunction() [with DType = ") - 1);
			name.remove_suffix(sizeof("; std::string_view = std::basic_string_view<char>]") - 1);
			return name;
		}

        /**
         * @brief 类型标签信息
         * @tparam DType 数据类型
         */
		template <typename DType>
		static constinit inline const Info tagInfo_{
			GetName<DType>(),
			GetSize<DType>()};
    };

    /**
     * @brief 原始数据封装
     */
    class RawData final
    {
    private:
        void *data_{nullptr}; ///< 数据指针
        size_t size_{};       ///< 数据大小

        friend class ConstRawData; /// 允许ConstRawData访问私有成员

    public:
        /**
         * @brief 构造函数
         */
        constexpr RawData() = default;

        /**
         * @brief 构造函数
         * @param data 数据指针
         * @param size 数据大小
         * @note data可以为nullptr，表示空数据
         * @note size可以为0，表示空数据
         */
        constexpr RawData(void *data, const size_t size)
            : data_(data), size_(size)
        {
        }

        /**
         * @brief 构造函数
         * @param data 数据引用
         */
        constexpr RawData(const RawData &data, const size_t new_size)
            : data_(data.data_), size_((new_size < data.size_) ? new_size : data.size_)
        {
        }

        /**
         * @brief 构造函数
         * @param data 数据引用
         * @note 该构造函数不接受RawData类型，防止与拷贝构造函数冲突
         */
        template <typename DType>
            requires(!std::is_same_v<RawData, std::decay_t<DType>>)
        explicit constexpr RawData(DType &data)
            : data_(std::addressof(data)), size_(sizeof(DType))
        {
        }

        /**
         * @brief 获取数据指针
         * @tparam DType 数据类型
         * @return 数据指针
         */
        template <typename DType>
        FORCE_INLINE constexpr auto GetData()
        {
            return static_cast<std::add_pointer_t<DType>>(data_);
        }

        /**
         * @brief 常量获取数据指针
         * @tparam DType 数据类型
         * @return 常量数据指针
         */
        template <typename DType>
        FORCE_INLINE constexpr auto GetData() const
        {
            return static_cast<std::add_pointer_t<std::add_const_t<DType>>>(data_);
        }

        /**
         * @brief 获取数据大小
         * @return 数据大小
         */
        [[nodiscard]] FORCE_INLINE constexpr size_t GetSize() const
        {
            return size_;
        }

        /**
         * @brief 获取子数据块
         * @param offset 偏移量
         * @param new_size 新的数据大小
         * @return 子数据块
         * @note 如果offset大于size_，则返回空数据块
         * @note 如果offset + new_size大于size_，则new_size取为size_ - offset
         */
        RawData SubData(size_t offset, size_t new_size) const;

        /**
         * @brief 复制数据到目标RawData
         * @param dest 目标RawData
         * @note 如果data_或dest.data_为nullptr，则不进行复制
         * @note 复制的大小为dest.size_与size_中的较小值
         */
        void CopyTo(const RawData &dest) const;

        /**
         * @brief 复制指定大小的数据到目标RawData
         * @param dest 目标RawData
         * @param n 复制的数据大小
         * @note 如果data_或dest.data_为nullptr，则不进行复制
         * @note 复制的大小为dest.size_、size_与n中的较小值
         * @note 如果n大于size_，则只复制size_大小的数据
         * @note 如果n大于dest.size_，则只复制dest.size_大小的数据
         */
        void CopyTo_n(const RawData &dest, size_t n) const;

        /**
         * @brief 用指定值填充数据
         * @param value 填充值
         * @note 如果data_为nullptr，则不进行填充
         */
        void Fill(uint8_t value) const;
    };

    /**
     * @brief 原始数据常量封装
     */
    class ConstRawData final
    {
    private:
        const void *data_{nullptr}; ///< 数据指针
        size_t size_{};             ///< 数据大小

    public:
        /**
         * @brief 构造函数
         */
        constexpr ConstRawData() = default;

        /**
         * @brief 构造函数
         * @param data 数据指针
         * @param size 数据大小
         * @note data可以为nullptr，表示空数据
         * @note size可以为0，表示空数据
         */
        constexpr ConstRawData(const void *const data, const size_t size)
            : data_(data), size_(size)
        {
        }

        /**
         * @brief 构造函数
         * @param data 数据引用
         */
        constexpr ConstRawData(const RawData &data)
            : data_(data.data_), size_(data.size_)
        {
        }

        /**
         * @brief 构造函数
         * @param data 数据引用
         * @param new_size 新的数据大小
         * @note new_size不能大于data.size_
         */
        constexpr ConstRawData(const RawData &data, size_t new_size)
            : data_(data.data_), size_((data.size_ < new_size) ? data.size_ : new_size)
        {
        }

        /**
         * @brief 构造函数
         * @param data 数据引用
         * @param new_size 新的数据大小
         * @note new_size不能大于data.size_
         */
        constexpr ConstRawData(const ConstRawData &data, size_t new_size)
            : data_(data.data_), size_((data.size_ < new_size) ? data.size_ : new_size)
        {
        }

        /**
         * @brief 构造函数
         * @param data 数据引用
         * @note 该构造函数不接受ConstRawData类型和RawData类型，防止与拷贝构造函数冲突
         */
        template <typename DType>
            requires(!std::is_same_v<ConstRawData, std::decay_t<DType>> && !std::is_same_v<RawData, std::decay_t<DType>>)
        explicit constexpr ConstRawData(const DType &data)
            : data_(std::addressof(data)), size_(sizeof(DType))
        {
        }

        /**
         * @brief 构造函数
         * @param data 数据引用
         * @note 该构造函数不接受ConstRawData类型和RawData类型，防止与拷贝构造函数冲突
         */
        template <typename DType>
            requires(!std::is_same_v<ConstRawData, std::decay_t<DType>> && !std::is_same_v<RawData, std::decay_t<DType>>)
        explicit constexpr ConstRawData(DType &&data)
            : data_(std::addressof(data)), size_(sizeof(DType))
        {
        }

        /**
         * @brief 获取数据指针
         * @tparam DType 数据类型
         * @return 数据指针
         */
        template <typename DType>
        FORCE_INLINE constexpr std::add_pointer_t<std::add_const_t<DType>> GetData() const
        {
            return static_cast<std::add_pointer_t<std::add_const_t<DType>>>(data_);
        }

        /**
         * @brief 获取数据大小
         * @return 数据大小
         */
        [[nodiscard]] FORCE_INLINE constexpr size_t GetSize() const
        {
            return size_;
        }

        /**
         * @brief 获取子数据块
         * @param offset 偏移量
         * @param new_size 新的数据大小
         * @return 子数据块
         * @note 如果offset大于size_，则返回空数据块
         * @note 如果offset + new_size大于size_，则new_size取为size_ - offset
         */
        ConstRawData SubData(size_t offset, size_t new_size) const;

        /**
         * @brief 复制数据到目标RawData
         * @param dest 目标RawData
         * @note 如果data_或dest.data_为nullptr，则不进行复制
         * @note 复制的大小为dest.size_与size_中的较小值
         */
        void CopyTo(const RawData &dest) const;

        /**
         * @brief 复制指定大小的数据到目标RawData
         * @param dest 目标RawData
         * @param n 复制的数据大小
         * @note 如果data_或dest.data_为nullptr，则不进行复制
         * @note 复制的大小为dest.size_、size_与n中的较小值
         * @note 如果n大于size_，则只复制size_大小的数据
         * @note 如果n大于dest.size_，则只复制dest.size_大小的数据
         */
        void CopyTo_n(const RawData &dest, size_t n) const;
    };

    /**
     * @brief 通用结果封装
     * @tparam T 值类型
     */
    template <typename T>
    class Result
    {
    private:
        ErrorCode errorCode_; ///< 错误码
        T value_;             ///< 值

        /**
         * @brief 私有构造函数
         * @param error_code 错误码
         * @param value 值
         */
        Result(ErrorCode error_code, const T &value)
            : errorCode_(error_code), value_(value)
        {
        }

    public:
        /**
         * @brief 默认构造函数，表示无值状态
         */
        constexpr Result() : errorCode_(ErrorCode::FAILED), value_() {}

        /**
         * @brief 构造函数，表示有值状态
         * @param value 值
         */
        FORCE_INLINE static Result Ok(const T &value)
        {
            return Result(ErrorCode::OK, value);
        }

        /**
         * @brief 构造函数，表示错误状态
         * @param error_code 错误码
         */
        FORCE_INLINE static Result Error(ErrorCode error_code, const T &value = T{})
        {
            return Result(error_code, value);
        }

        /**
         * @brief 转换为错误码
         * @return 错误码
         */
        FORCE_INLINE operator ErrorCode() const { return errorCode_; }

        /**
         * @brief 检查是否有值
         * @return 是否有值
         */
        FORCE_INLINE operator bool() const { return errorCode_ == ErrorCode::OK; }

        /**
         * @brief 检查是否有错误
         * @return 错误码
         */
        FORCE_INLINE ErrorCode GetError() const { return errorCode_; }

        /**
         * @brief 获取值
         * @return 值
         */
        FORCE_INLINE T GetValue() const { return value_; }

        /**
         * @brief 获取值或默认值
         * @param default_value 默认值
         * @return 值或默认值
         */
        FORCE_INLINE T GetValueOr(const T &default_value) const
        {
            return operator bool() ? value_ : default_value;
        }

        /**
         * @brief 链式调用，当有值时执行传入的函数
         * @tparam F 函数类型
         * @param func 函数对象
         * @return 当前Result对象的右值引用
         */
        template <typename F>
            requires std::disjunction_v<std::is_invocable_r<void, F, T>, std::is_invocable_r<void, F, const T &>>
        FORCE_INLINE Result<T> &&AndThen(const F &func)
        {
            if (operator bool())
            {
                func(value_);
            }
            return std::move(*this);
        }

        /**
         * @brief 链式调用，当有错误时执行传入的函数
         * @tparam F 函数类型
         * @param func 函数对象
         * @return 当前Result对象的右值引用
         */
        template <typename F>
            requires std::is_invocable_r_v<void, F, ErrorCode>
        FORCE_INLINE Result<T> &&Expect(const F &func)
        {
            if (!operator bool())
            {
                func(errorCode_);
            }
            return std::move(*this);
        }
    };

    /**
     * @brief 可比较相等概念
     * @tparam T 类型
     */
    template <typename T>
    concept EqualComparable = requires(T a, T b)
    {
        { a == b } -> std::convertible_to<bool>;
    };

    /**
     * @brief 检查变量是否在给定的初始化列表中
     * @tparam T 变量类型
     * @param var 变量
     * @param list 初始化列表
     * @return 是否在列表中
     */
    template <EqualComparable T>
    inline constexpr bool RangeIn(const T var, const std::initializer_list<T> &list)
    {
        for (const auto &item : list)
        {
            if (var == item)
            {
                return true;
            }
        }
        return false;
    }
}