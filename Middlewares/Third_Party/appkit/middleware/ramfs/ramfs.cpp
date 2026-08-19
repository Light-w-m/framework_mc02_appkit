#include <ramfs.hpp>
#include <cstring>

template class appkit::RBTree<appkit::RamFs::String::HashType>;

#ifdef USE_DEFAULT_RAMFS_MAX_BLOCK_NAME_LEN
#pragma message "APPKIT_RAMFS_MAX_BLOCK_NAME_LEN is not defined, using default value 64"
#endif

namespace appkit
{
    RamFs::Dir &RamFs::Root()
    {
        static auto &root = []() -> Dir &
        {
            static Dir _root = CreateDir(""); // 根目录名称为空字符串

            _root.Add(Bin());
            _root.Add(Sys());
            _root.Add(Dev());
            _root.Add(Mnt());

            return _root;
        }();
        return root;
    }

    RamFs::Dir &RamFs::Sys()
    {
        static Dir sys = CreateDir("sys");
        return sys;
    }

    RamFs::Dir &RamFs::Dev()
    {
        static Dir dev = CreateDir("dev");
        return dev;
    }

    RamFs::Dir &RamFs::Bin()
    {
        static Dir bin = CreateDir("bin");
        return bin;
    }

    RamFs::Dir &RamFs::Mnt()
    {
        static Dir mnt = CreateDir("mnt");
        return mnt;
    }

    int RamFs::File::Run(const int argc, const char **argv) const
    {
        APPKIT_RAISE_IF_NOT(NodeType::FILE == Block(*this).type, "Node is not a file");                // 断言是文件
        APPKIT_RAISE_IF_NOT(permission_ == FilePermission::EXECUTE, "File permission is not execute"); // 断言是可执行文件
        APPKIT_RAISE_IF_NOT(nullptr != data_.exec.func, "Function pointer is null");                   // 断言函数指针不为空

        return data_.exec.func(data_.exec.block, argc, argv);
    }

    void RamFs::Dir::Add(Dir &dir)
    {
        APPKIT_RAISE_IF_NOT(NodeType::DICT == (*dir).type, "Node is not a directory"); // 断言是目录

        children_.Insert(Block(dir).name.GetHashValue(), dir);
        (*dir).parent_dir = this;
    }

    void RamFs::Dir::Add(File &file)
    {
        APPKIT_RAISE_IF_NOT(NodeType::FILE == (*file).type, "Node is not a file"); // 断言是文件

        children_.Insert(Block(file).name.GetHashValue(), file);
        (*file).parent_dir = this;
    }

    RamFs::Head *RamFs::Dir::Find(const char *name) const
    {
        // name不能为空
        if (nullptr == name)
        {
            APPKIT_RAISE("File name must not be null");
            return nullptr;
        }

        return children_.Search<Block>(String::CreateHash(name));
    }

    RamFs::Head *RamFs::Dir::RecFind(const std::initializer_list<const char *> &path)
    {
        // 路径列表不能为空
        if (path.size() == 0)
        {
            APPKIT_RAISE("Path list must not be empty");
            return nullptr;
        }

        const size_t path_count = path.size();
        auto current_dir = this;

        for (size_t current_count = 1; const auto &name : path)
        {
            const auto head = current_dir->Find(name);
            if (nullptr == head)
                return nullptr;

            // 如果是最后一个路径且是文件则返回该节点
            if (NodeType::FILE == Block(*head).type && path_count == current_count)
            {
                return head;
            }

            // 不是目录则返回nullptr
            if (!(NodeType::DICT == Block(*head).type && Check(AsDir(head, &current_dir))))
            {
                return nullptr;
            }

            current_count++;
        }

        return current_dir;
    }

    ErrorCode RamFs::Dir::GetParent(Dir **dir) const
    {
        // dir不能为空
        if (nullptr == dir)
        {
            return ErrorCode::INVALID_ARG;
        }

        // 根目录没有父目录
        if (nullptr == Block(*this).parent_dir)
        {
            return ErrorCode::NO_FOUND;
        }

        *dir = Block(*this).parent_dir;
        return ErrorCode::OK;
    }

    ErrorCode RamFs::AsFile(Head *src, File **dst)
    {
        // src和dst不能为空
        if (nullptr == src || nullptr == dst)
        {
            return ErrorCode::INVALID_ARG;
        }

        // 不是文件类型
        if (Block(*src).type != NodeType::FILE)
        {
            return ErrorCode::FAILED;
        }

        *dst = static_cast<File *>(src);
        return ErrorCode::OK;
    }

    ErrorCode RamFs::AsDir(Head *src, Dir **dst)
    {
        // src和dst不能为空
        if (nullptr == src || nullptr == dst)
        {
            return ErrorCode::INVALID_ARG;
        }

        // 不是目录类型
        if (Block(*src).type != NodeType::DICT)
        {
            return ErrorCode::FAILED;
        }

        *dst = static_cast<Dir *>(src);
        return ErrorCode::OK;
    }
} // namespace LIB_NAMESPACE