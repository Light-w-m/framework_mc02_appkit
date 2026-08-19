#pragma once

#include <hash_string.hpp>
#include <rbt.hpp>

#include <atomic>
#include <cstring>
#include <string>

#ifndef APPKIT_RAMFS_MAX_BLOCK_NAME_LEN
#define USE_DEFAULT_RAMFS_MAX_BLOCK_NAME_LEN
#define APPKIT_RAMFS_MAX_BLOCK_NAME_LEN 64
#endif

namespace appkit
{
    class RamFs final
    {
    public:
        using String = HashString<APPKIT_RAMFS_MAX_BLOCK_NAME_LEN>; ///< 文件系统字符串类型

        /**
         * @brief 文件系统节点类型
         */
        enum class NodeType : int8_t
        {
            FILE, ///< 普通文件
            DICT, ///< 目录
        };

        /**
         * @brief 文件权限
         */
        enum class FilePermission : int8_t
        {
            READ = 0x01,       ///< 可读
            READ_WRITE = 0x02, ///< 可写
            EXECUTE = 0x04     ///< 可执行
        };

        class Dir;

    private:
        /**
         * @brief 文件系统节点基类
         */
        struct Block final
        {
            String name{};     ///< 节点名称
            NodeType type{};   ///< 节点类型
            Dir *parent_dir{}; ///< 父目录指针
        };

    public:
        /**
         * @brief 红黑树节点，存储文件系统节点
         */
        using Head = RBTree<String::HashType>::Node<Block>;

        /**
         * @brief 文件系统文件类
         */
        class File final : protected Head
        {
            friend class RamFs;

            union
            {
                struct
                {
                    void *addr{};
                    size_t size{};
                } normal{};

                struct
                {
                    int (*func)(void *block, int argc, const char **argv);
                    void *block;
                } exec;
            } data_{};

            FilePermission permission_{FilePermission::READ};

        public:
            constexpr File() = default;

            /**
             * @brief 获取文件权限
             * @return 文件权限
             */
            [[nodiscard]] FORCE_INLINE FilePermission GetPermission() const
            {
                return permission_;
            }

            /**
             * @brief 检查文件数据类型
             * @tparam DType 数据类型
             * @return 是否匹配
             */
            template <typename DType>
            FORCE_INLINE bool CheckSize() const
            {
                if (permission_ == FilePermission::EXECUTE) [[unlikely]]
                {
                    return false;
                }

                return (data_.normal.size == sizeof(DType));
            }

            /**
             * @brief 以可执行文件方式运行
             * @param argc 参数个数
             * @param argv 参数列表
             * @return 返回值
             */
            int Run(int argc, const char **argv) const;

            /**
             * @brief 获取文件内容
             * @tparam DType 数据类型
             * @tparam Mode 大小限制模式
             * @return 数据引用
             * @note 如果文件是可执行文件或只读文件，调用该函数会触发断言
             */
            template <typename DType, Assert::SizeLimitMode Mode = Assert::SizeLimitMode::EQUAL>
            DType &Get()
            {
                APPKIT_RAISE_IF_NOT(permission_ == FilePermission::READ_WRITE, "File permission does not allow write");
                SizeLimitAssert(Mode, data_.normal.size, sizeof(DType), "File size does not match expected size");

                return *static_cast<DType *>(data_.normal.addr);
            }

            /**
             * @brief 获取文件内容（常量版本）
             * @tparam DType 数据类型
             * @tparam Mode 大小限制模式
             * @return 数据常量引用
             * @note 如果文件是可执行文件，调用该函数会触发断言
             */
            template <typename DType, Assert::SizeLimitMode Mode = Assert::SizeLimitMode::EQUAL>
            std::add_const_t<DType> &Get() const
            {
                APPKIT_RAISE_IF_NOT(permission_ != FilePermission::EXECUTE, "File permission does not allow execute");
                SizeLimitAssert(Mode, data_.normal.size, sizeof(DType), "File size does not match expected size");

                return *static_cast<std::add_const_t<DType> *>(data_.normal.addr);
            }

            /**
             * @brief 获取文件名称
             * @return 文件名称
             */
            [[nodiscard]] FORCE_INLINE const char *GetName() const
            {
                return Block(*this).name.GetString();
            }

            /**
             * @brief 获取文件名称哈希值
             * @return 文件名称哈希值
             */
            [[nodiscard]] FORCE_INLINE String::HashType GetNameHash() const
            {
                return Block(*this).name.GetHashValue();
            }
        };

        /**
         * @brief 文件系统目录类
         */
        class Dir final : protected Head
        {
            friend class RamFs;

            RBTree<String::HashType> children_{
                [](auto lfs, auto rfs) -> std::strong_ordering
                {
                    return lfs <=> rfs;
                }}; ///< 子目录和文件

        public:
            constexpr Dir() = default;

            /**
             * @brief 添加子文件
             * @param file 文件引用
             */
            void Add(File &file);

            /**
             * @brief 添加子目录
             * @param dir 目录引用
             */
            void Add(Dir &dir);

            /**
             * @brief 查找子节点
             * @param name 节点名称
             * @return 节点指针，未找到返回nullptr
             */
            Head *Find(const char *name) const;

            /**
             * @brief 递归查找子节点
             * @param path 路径列表
             * @return 目录指针，未找到返回nullptr
             * @note 路径列表不能为空
             */
            Head *RecFind(const std::initializer_list<const char *> &path);

            /**
             * @brief 获取父目录
             * @param dir 目录指针
             * @return 错误码
             */
            ErrorCode GetParent(Dir **dir) const;

            /**
             * @brief 获取目录名称
             * @return 目录名称
             */
            [[nodiscard]] FORCE_INLINE const char *GetName() const
            {
                return Block(*this).name.GetString();
            }

            /**
             * @brief 获取目录名称哈希值
             * @return 目录名称哈希值
             */
            [[nodiscard]] FORCE_INLINE String::HashType GetNameHash() const
            {
                return Block(*this).name.GetHashValue();
            }

            /**
             * @brief 遍历子文件
             * @tparam FType 回调函数类型
             * @param func 回调函数
             * @return 错误码
             * @note 回调函数原型为 `ErrorCode func(Head &head)`，返回非`ErrorCode::Ok`会终止遍历
             */
            template <typename FType>
                requires std::is_invocable_r_v<ErrorCode, FType, Head &>
            FORCE_INLINE ErrorCode ForEach(FType func)
            {
                return children_.template ForEach<Block>(func);
            }
        };

        /**
         * @brief 创建一个文件
         * @tparam DType 数据类型
         * @param name 文件名称
         * @param data 数据引用
         * @return 文件对象
         * @note 如果数据类型是函数类型，则调用该函数会触发断言
         */
        template <typename DType>
            requires(!std::is_function_v<DType>)
        static File CreateFile(const char *name, DType &data)
        {
            APPKIT_RAISE_IF_NOT(nullptr != name, "File name must not be null");

            File file{};
            (*file).name = name;
            (*file).type = NodeType::FILE;

            if constexpr (std::is_const_v<DType>)
            {
                file.permission_ = FilePermission::READ;
            }
            else
            {
                file.permission_ = FilePermission::READ_WRITE;
            }

            file.data_.normal.addr = std::addressof(data);
            file.data_.normal.size = sizeof(DType);

            return file;
        }

        /**
         * @brief 创建一个可执行文件
         * @tparam FType 函数类型
         * @param name 文件名称
         * @param exec 可执行函数
         * @return 文件对象
         * @note 函数类型必须满足 `int func(int, const char**)`
         */
        template <typename FType>
            requires std::is_invocable_r_v<int, FType, int, const char **>
        static File CreateFile(const char *name, FType exec)
        {
            APPKIT_RAISE_IF_NOT(nullptr != name, "File name must not be null");

            File file{};
            (*file).name = name;
            (*file).type = NodeType::FILE;
            file.permission_ = FilePermission::EXECUTE;

            struct ExecBlock
            {
                FType exec;
            };

            file.data_.exec.block = new ExecBlock{exec};
            file.data_.exec.func = [](void *bind_data, int argc, const char **argv) -> int
            {
                APPKIT_RAISE_IF_NOT(nullptr != bind_data, "Bind data must not be null");
                auto *block = static_cast<ExecBlock *>(bind_data);
                return block->exec(argc, argv);
            };

            return file;
        }

        /**
         * @brief 创建一个目录
         * @param name 目录名称
         * @return 目录对象
         */
        static Dir CreateDir(const char *name)
        {
            APPKIT_RAISE_IF_NOT(nullptr != name, "Directory name must not be null");

            Dir dict{};
            (*dict).name = name;
            (*dict).type = NodeType::DICT;

            return dict;
        }

        /**
         * @brief 将节点转换为文件
         * @param src 源节点
         * @param dst 目标文件指针
         * @return 错误码
         */
        static ErrorCode AsFile(Head *src, File **dst);

        /**
         * @brief 将节点转换为目录
         * @param src 源节点
         * @param dst 目标目录指针
         * @return 错误码
         */
        static ErrorCode AsDir(Head *src, Dir **dst);

        /**
         * @brief 获取根目录
         * @return 目录引用
         */
        static Dir &Root();

        /**
         * @brief 获取系统目录
         * @return 目录引用
         */
        static Dir &Sys();

        /**
         * @brief 获取设备目录
         * @return 目录引用
         */
        static Dir &Dev();

        /**
         * @brief 获取二进制目录
         * @return 目录引用
         */
        static Dir &Bin();

        /**
         * @brief 获取挂载目录
         * @return 目录引用
         * @note 该目录用于挂载外部文件系统
         */
        static Dir &Mnt();
    };
}