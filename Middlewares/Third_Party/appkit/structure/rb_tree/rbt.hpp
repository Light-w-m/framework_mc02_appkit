/**
 * @file rbt.h
 * @brief 红黑树头文件
 * @author dusk
 */
#pragma once

#include <algorithm>
#include <cstring>
#include <memory>

#include <common_assert.hpp>
#include <common_type.hpp>
#include <osal_mutex.hpp>

namespace appkit
{
    /**
     * @brief 红黑树
     * @tparam KType 键类型
     */
    template <typename KType>
    class RBTree final
    {
        enum class Color
        {
            RED,
            BLACK
        };

        /**
         * @brief 比较函数
         */
        using CompareFunc = std::strong_ordering (*)(std::add_const_t<KType> &lfs, std::add_const_t<KType> &rfs);

        class NodeBase
        {
            KType key_{};             ///< 键名
            Color color_{Color::RED}; ///< 颜色 默认红色
            size_t size_{};           ///< 数据类型标签

            NodeBase *left_{};   ///< 左节点
            NodeBase *right_{};  ///< 右节点
            NodeBase *parent_{}; ///< 父节点

            friend class RBTree;

        protected:
            constexpr explicit NodeBase(const size_t size) : size_{size}
            {
            }

        public:
            FORCE_INLINE constexpr std::add_const_t<KType> &GetKey() const
            {
                return key_;
            }
        };

    public:
        /**
         * @brief 红黑树节点
         * @tparam DType 节点数据类型
         */
        template <typename DType>
        class Node : public NodeBase
        {
            DType data{};

        public:
            /**
             * @brief 默认构造函数
             */
            constexpr Node() : NodeBase(sizeof(DType))
            {
            }

            /**
             * @brief 复制数据
             * @param data 要存入的数据
             */
            constexpr explicit Node(const DType &data) : NodeBase(sizeof(DType)), data(data)
            {
            }

            /**
             * @brief 原地构建数据
             * @tparam AType 构造参数类型
             * @param arg 构造参数
             */
            template <typename... Args>
            constexpr explicit Node(Args &&...arg) : NodeBase(sizeof(DType)), data(std::forward<Args>(arg)...)
            {
            }

            /* Node */
            /**
             * @brief 获取数据类型标签
             */
            FORCE_INLINE operator DType &()
            {
                return data;
            }

            /**
             * @brief 获取数据常量类型标签
             */
            FORCE_INLINE constexpr operator std::add_const_t<DType> &() const
            {
                return data;
            }

            /**
             * @brief 复制数据
             * @param _data 要存入的数据
             * @return *this
             */
            FORCE_INLINE Node &operator=(const DType &_data)
            {
                this->data = _data;

                return *this;
            }

            /* Data */
            /**
             * @brief 获取数据引用
             * @return 数据引用
             */
            FORCE_INLINE auto &operator*()
            {
                return data;
            }

            /**
             * @brief 获取数据常量引用
             * @return 数据常量引用
             */
            FORCE_INLINE constexpr std::add_const_t<DType> &operator*() const
            {
                return data;
            }
        };

        /**
         * @brief 构造函数
         * @param compare_func 比较函数
         */
        constexpr explicit RBTree(const CompareFunc compare_func) : compare_{compare_func}
        {
            APPKIT_RAISE_IF_NOT(compare_ != nullptr, "Compare function must not be null");
        }

        /**
         * @brief 析构函数
         */
        ~RBTree() = default;

        /**
         * @brief 插入节点
         * @tparam DType 节点数据类型
         * @param key 键值
         * @param node 节点引用
         */
        template <typename DType>
        ErrorCode Insert(KType key, Node<DType> &node)
        {
            NodeBase *y = nullptr;
            NodeBase *x = root_;

            node.key_ = key;

            [[maybe_unused]] osal::LockGuard lock(mutex_);

            // 1. 将红黑树当作一棵二叉查找树，将节点添加到二叉查找树中。
            while (x != nullptr)
            {
                y = x;
                if (auto ret = compare_(node.key_, x->key_); ret == std::strong_ordering::less)
                    x = x->left_;
                else if (ret == std::strong_ordering::greater)
                    x = x->right_;
                else
                    return ErrorCode::FAILED; // 键值已存在，直接返回
            }
            node.parent_ = y;

            if (y != nullptr)
            {
                if (compare_(node.key_, y->key_) == std::strong_ordering::less)
                    y->left_ = &node; // 情况2：若“node所包含的值” < “y所包含的值”，则将node设为“y的左孩子”
                else
                    y->right_ = &node; // 情况3：(“node所包含的值” >= “y所包含的值”)将node设为“y的右孩子”
            }
            else
            {
                root_ = &node; // 情况1：若y是空节点，则将node设为根
            }

            // 2. 设置节点的颜色为红色
            node.color_ = Color::RED;

            // 3. 将它重新修正为一棵二叉查找树
            FixInsert(&node);

            return ErrorCode::OK;
        }

        /**
         * @brief 从链表中删除节点
         * @param key 键值
         * @param current_node 当前节点（如果已知要删除的节点，可以传入该节点以加快查找速度）
         */
        void Remove(const KType &key, NodeBase *current_node = nullptr)
        {
            NodeBase *child, *parent;
            Color color;

            [[maybe_unused]] osal::LockGuard lock(mutex_);

            NodeBase *node = !current_node ? SearchBst(root_, key) : current_node;

            // 被删除节点的"左右孩子都不为空"的情况。
            if (nullptr != node->left_ && nullptr != node->right_)
            {
                // 被删节点的后继节点。(称为"取代节点")
                // 用它来取代"被删节点"的位置，然后再将"被删节点"去掉。
                NodeBase *replace = node;

                // 获取后继节点
                replace = replace->right_;
                while (replace->left_ != nullptr)
                    replace = replace->left_;

                // "node节点"不是根节点(只有根节点不存在父节点)
                if (node->parent_ != nullptr)
                {
                    if (node->parent_->left_ == node)
                        node->parent_->left_ = replace;
                    else
                        node->parent_->right_ = replace;
                }
                else
                    // "node节点"是根节点，更新根节点。
                    root_ = replace;

                // child是"取代节点"的右孩子，也是需要"调整的节点"。
                // "取代节点"肯定不存在左孩子！因为它是一个后继节点。
                child = replace->right_;
                parent = replace->parent_;
                // 保存"取代节点"的颜色
                color = replace->color_;

                // "被删除节点"是"它的后继节点的父节点"
                if (parent == node)
                {
                    parent = replace;
                }
                else
                {
                    // child不为空
                    if (child)
                        child->parent_ = parent;
                    parent->left_ = child;

                    replace->right_ = node->right_;
                    node->right_->parent_ = replace;
                }

                replace->parent_ = node->parent_;
                replace->color_ = node->color_;
                replace->left_ = node->left_;
                node->left_->parent_ = replace;

                if (color == Color::BLACK)
                    FixDelete(child, parent);

                return;
            }

            if (node->left_ != nullptr)
                child = node->left_;
            else
                child = node->right_;

            parent = node->parent_;
            // 保存"取代节点"的颜色
            color = node->color_;

            if (child)
                child->parent_ = parent;

            // "node节点"不是根节点
            if (parent)
            {
                if (parent->left_ == node)
                    parent->left_ = child;
                else
                    parent->right_ = child;
            }
            else
                root_ = child;

            if (color == Color::BLACK)
                FixDelete(child, parent);
        }

        /**
         * @brief 判断是否包含指定键值的节点
         * @param key 键值
         * @return 是否包含
         */
        bool Contains(const KType &key) const
        {
            [[maybe_unused]] osal::LockGuard lock(mutex_);
            return SearchBst(root_, key) != nullptr;
        }

        /**
         * @brief 获取包含指定键值的节点
         * @tparam DType 节点数据类型
         * @tparam Mode 类型检查模式
         * @param key 键值
         * @return 节点指针，未找到返回nullptr
         */
        template <typename DType, Assert::SizeLimitMode Mode = Assert::SizeLimitMode::EQUAL>
        Node<DType> *Search(const KType &key) const
        {
            [[maybe_unused]] osal::LockGuard lock{mutex_};
            NodeBase *node = SearchBst(root_, key);
            if (node != nullptr)
            {
                if constexpr (Assert::SizeLimitMode::LESS == Mode)
                {
                    if (node->size_ <= sizeof(DType))
                    {
                        return static_cast<Node<DType> *>(node);
                    }
                }
                else if constexpr (Assert::SizeLimitMode::EQUAL == Mode)
                {
                    if (node->size_ == sizeof(DType))
                    {
                        return static_cast<Node<DType> *>(node);
                    }
                }
                else if constexpr (Assert::SizeLimitMode::GREAT == Mode)
                {
                    if (node->size_ >= sizeof(DType))
                    {
                        return static_cast<Node<DType> *>(node);
                    }
                }
                else
                {
                    static_assert(false, "Invalid SizeLimitMode");
                }
            }
            return nullptr;
        }

        /**
         * @brief 遍历红黑树
         * @tparam DType 节点数据类型
         * @tparam FType 回调函数类型
         * @tparam Mode 类型检查模式
         * @param callback 回调函数
         * @return 错误码
         * @note 回调函数原型为 `ErrorCode func(Node<DType> &data)`，返回非`ErrorCode::Ok`会终止遍历
         * @note 如果Mode为`TypeCheckMode::Strict`，则只会回调数据类型完全匹配的节点
         * @note 如果Mode为`TypeCheckMode::Relaxed`，则会回调数据类型大小相同的节点
         * @note 如果Mode为`TypeCheckMode::None`，则不会进行类型检查，可能会导致类型不匹配的节点被回调，使用时请确保类型安全
         */
        template <typename DType, typename FType, Assert::SizeLimitMode Mode = Assert::SizeLimitMode::EQUAL>
            requires std::disjunction_v<std::is_invocable_r<ErrorCode, FType, const Node<DType> &>,
                                        std::is_invocable_r<ErrorCode, FType, Node<DType> &>>
        ErrorCode ForEach(FType callback)
        {
            // 回调函数指针不能为空
            if constexpr (std::is_pointer_v<FType>)
                if (callback == nullptr) [[unlikely]]
                    return ErrorCode::INVALID_ARG;

            if (root_ == nullptr)
                return ErrorCode::EMPTY;

            auto ret = ErrorCode::OK;
            {
                [[maybe_unused]] osal::LockGuard lock(mutex_);
                ForEachFunc<DType, FType, Mode>(callback, root_, ret);
            }
            return ret;
        }

    private:
        NodeBase *root_{nullptr};     ///< 根节点
        mutable osal::Mutex mutex_{}; ///< 互斥锁

        CompareFunc compare_{}; ///< 比较函数

        template <typename DType, typename FType, Assert::SizeLimitMode Mode>
        void ForEachFunc(const FType &callback, NodeBase *node_base, ErrorCode &ret)
        {
            if (node_base != nullptr && ret == ErrorCode::OK) [[likely]]
            {
                if constexpr (Assert::SizeLimitMode::LESS == Mode)
                {
                    if (node_base->size_ <= sizeof(DType))
                    {
                        ret = callback(static_cast<Node<DType> &>(*node_base));
                    }
                }
                else if constexpr (Assert::SizeLimitMode::EQUAL == Mode)
                {
                    if (node_base->size_ == sizeof(DType))
                    {
                        ret = callback(static_cast<Node<DType> &>(*node_base));
                    }
                }
                else if constexpr (Assert::SizeLimitMode::GREAT == Mode)
                {
                    if (node_base->size_ >= sizeof(DType))
                    {
                        ret = callback(static_cast<Node<DType> &>(*node_base));
                    }
                }
                else
                {
                    static_assert(false, "Invalid SizeLimitMode");
                }

                ForEachFunc<DType, FType, Mode>(callback, node_base->left_, ret);
                ForEachFunc<DType, FType, Mode>(callback, node_base->right_, ret);
            }
        }

        /*
         * 对红黑树的节点(x)进行左旋转
         *
         * 左旋示意图(对节点x进行左旋)：
         *      px                              px
         *     /                               /
         *    x                               y
         *   /  \      --(左旋)-->           / \                #
         *  lx   y                          x  ry
         *     /   \                       /  \
         *    ly   ry                     lx  ly
         *
         */
        void LeftRotate(NodeBase *x)
        {
            // 设置x的右孩子为y
            auto y = x->right_;

            // 将 “y的左孩子” 设为 “x的右孩子”；
            // 如果y的左孩子非空，将 “x” 设为 “y的左孩子的父亲”
            x->right_ = y->left_;
            if (y->left_ != nullptr)
                y->left_->parent_ = x;

            // 将 “x的父亲” 设为 “y的父亲”
            y->parent_ = x->parent_;

            if (x->parent_ == nullptr)
            {
                root_ = y; // 如果 “x的父亲” 是空节点，则将y设为根节点
            }
            else
            {
                if (x->parent_->left_ == x)
                    x->parent_->left_ = y; // 如果 x是它父节点的左孩子，则将y设为“x的父节点的左孩子”
                else
                    x->parent_->right_ = y;
            }

            // 将 “x” 设为 “y的左孩子”
            y->left_ = x;
            // 将 “x的父节点” 设为 “y”
            x->parent_ = y;
        }

        /*
         * 对红黑树的节点(y)进行右旋转
         *
         * 右旋示意图(对节点y进行左旋)：
         *            py                               py
         *           /                                /
         *          y                                x
         *         /  \      --(右旋)-->            /  \                     #
         *        x   ry                           lx   y
         *       / \                                   / \                   #
         *      lx  rx                                rx  ry
         *
         */
        void RightRotate(NodeBase *y)
        {
            // 设置x是当前节点的左孩子。
            auto x = y->left_;

            // 将 “x的右孩子” 设为 “y的左孩子”；
            // 如果"x的右孩子"不为空的话，将 “y” 设为 “x的右孩子的父亲”
            y->left_ = x->right_;
            if (x->right_ != nullptr)
                x->right_->parent_ = y;

            // 将 “y的父亲” 设为 “x的父亲”
            x->parent_ = y->parent_;

            if (y->parent_ == nullptr)
            {
                root_ = x; // 如果 “y的父亲” 是空节点，则将x设为根节点
            }
            else
            {
                if (y == y->parent_->right_)
                    y->parent_->right_ = x; // 如果 y是它父节点的右孩子，则将x设为“y的父节点的右孩子”
                else
                    y->parent_->left_ = x; // (y是它父节点的左孩子) 将x设为“x的父节点的左孩子”
            }

            // 将 “y” 设为 “x的右孩子”
            x->right_ = y;

            // 将 “y的父节点” 设为 “x”
            y->parent_ = x;
        }

        NodeBase *SearchBst(NodeBase *node, const KType &key) const
        {
            while (node != nullptr)
            {
                if (const std::strong_ordering cmp = compare_(key, node->key_);
                    cmp == std::strong_ordering::less)
                {
                    node = node->left_;
                }
                else if (cmp == std::strong_ordering::greater)
                {
                    node = node->right_;
                }
                else
                {
                    return node; // 找到节点
                }
            }

            return nullptr; // 未找到节点
        }

        void FixInsert(NodeBase *node)
        {
            NodeBase *parent;

            // 若“父节点存在，并且父节点的颜色是红色”
            while ((parent = node->parent_) && parent->color_ == Color::RED)
            {
                NodeBase *gparent = parent->parent_;

                // 若“父节点”是“祖父节点的左孩子”
                if (parent == gparent->left_)
                {
                    // Case 1条件：叔叔节点是红色
                    {
                        NodeBase *uncle = gparent->right_;
                        if (uncle && uncle->color_ == Color::RED)
                        {
                            uncle->color_ = Color::BLACK;
                            parent->color_ = Color::BLACK;
                            gparent->color_ = Color::RED;
                            node = gparent;
                            continue;
                        }
                    }

                    // Case 2条件：叔叔是黑色，且当前节点是右孩子
                    if (parent->right_ == node)
                    {
                        LeftRotate(parent);
                        std::swap(parent, node);
                    }

                    // Case 3条件：叔叔是黑色，且当前节点是左孩子。
                    parent->color_ = Color::BLACK;
                    gparent->color_ = Color::RED;
                    RightRotate(gparent);
                }
                else // 若“z的父节点”是“z的祖父节点的右孩子”
                {
                    // Case 1条件：叔叔节点是红色
                    {
                        NodeBase *uncle = gparent->left_;
                        if (uncle && uncle->color_ == Color::RED)
                        {
                            uncle->color_ = Color::BLACK;
                            parent->color_ = Color::BLACK;
                            gparent->color_ = Color::RED;
                            node = gparent;
                            continue;
                        }
                    }

                    // Case 2条件：叔叔是黑色，且当前节点是左孩子
                    if (parent->left_ == node)
                    {
                        RightRotate(parent);
                        std::swap(parent, node);
                    }

                    // Case 3条件：叔叔是黑色，且当前节点是右孩子。
                    parent->color_ = Color::BLACK;
                    gparent->color_ = Color::RED;
                    LeftRotate(gparent);
                }
            }

            // 将根节点设为黑色
            root_->color_ = Color::BLACK;
        }

        void FixDelete(NodeBase *node, NodeBase *parent)
        {
            NodeBase *other;

            while ((!node || node->color_ == Color::BLACK) && node != root_)
            {
                if (parent->left_ == node)
                {
                    other = parent->right_;
                    if (other->color_ == Color::RED)
                    {
                        // Case 1: x的兄弟w是红色的
                        other->color_ = Color::BLACK;
                        parent->color_ = Color::RED;
                        LeftRotate(parent);
                        other = parent->right_;
                    }
                    if ((!other->left_ || other->left_->color_ == Color::BLACK) &&
                        (!other->right_ || other->right_->color_ == Color::BLACK))
                    {
                        // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的
                        other->color_ = Color::RED;
                        node = parent;
                        parent = node->parent_;
                    }
                    else
                    {
                        if (!other->right_ || other->right_->color_ == Color::BLACK)
                        {
                            // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。
                            other->left_->color_ = Color::BLACK;
                            other->color_ = Color::RED;
                            RightRotate(other);
                            other = parent->right_;
                        }
                        // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                        other->color_ = parent->color_;
                        parent->color_ = Color::BLACK;
                        other->right_->color_ = Color::BLACK;
                        LeftRotate(parent);
                        node = root_;
                        break;
                    }
                }
                else
                {
                    other = parent->left_;
                    if (other->color_ == Color::RED)
                    {
                        // Case 1: x的兄弟w是红色的
                        other->color_ = Color::BLACK;
                        parent->color_ = Color::RED;
                        RightRotate(parent);
                        other = parent->left_;
                    }
                    if ((!other->left_ || other->left_->color_ == Color::BLACK) &&
                        (!other->right_ || other->right_->color_ == Color::BLACK))
                    {
                        // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的
                        other->color_ = Color::RED;
                        node = parent;
                        parent = node->parent_;
                    }
                    else
                    {
                        if (!other->left_ || other->left_->color_ == Color::BLACK)
                        {
                            // Case 3: x的兄弟w是黑色的，并且w的左孩子是红色，右孩子为黑色。
                            other->right_->color_ = Color::BLACK;
                            other->color_ = Color::RED;
                            LeftRotate(other);
                            other = parent->left_;
                        }
                        // Case 4: x的兄弟w是黑色的；并且w的右孩子是红色的，左孩子任意颜色。
                        other->color_ = parent->color_;
                        parent->color_ = Color::BLACK;
                        other->left_->color_ = Color::BLACK;
                        RightRotate(parent);
                        node = root_;
                        break;
                    }
                }
            }
            if (node)
                node->color_ = Color::BLACK;
        }
    };
}