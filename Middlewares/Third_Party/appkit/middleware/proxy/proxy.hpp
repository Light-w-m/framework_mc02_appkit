#pragma once

#include <message.hpp>
#include <common_mem.hpp>
#include <crc8.hpp>

namespace appkit
{
    /**
     * @brief 判断类型是否具有静态成员HASH
     */
    template <typename DType>
    concept HasHash = requires(DType a) {
        { DType::HASH } -> std::convertible_to<uint32_t>;
    };

    /**
     * @brief 代理类
     */
    class Proxy final
    {
    public:
        /**
         * @brief 节点类型枚举
         */
        enum class NodeType : uint8_t
        {
            PUBLISHER = 0, ///< 发布者
            SUBSCRIBER = 1 ///< 订阅者
        };

        /**
         * @brief 消息头结构体
         */
        struct [[gnu::packed]] MsgHeader final
        {
            uint8_t start_byte; ///< 起始字节
            uint32_t hash;      ///< 消息哈希值
            uint16_t seq_num;   ///< 消息序列号
            uint8_t crc8;       ///< CRC8校验码
        };

        using SendCallback = Callback<ConstRawData>; ///< 发送回调函数类型

        static inline uint8_t SE_START_BYTE{0xA5}; ///< 消息起始字节

    private:
        /**
         * @brief 节点基础结构体
         */
        struct NodeBase
        {
            NodeType type{NodeType::PUBLISHER}; ///< 节点类型
            uint32_t msg_hash{0};               ///< 消息哈希值
            RawData msg_buffer{};               ///< 消息缓冲区

            Callback<> update_callback{}; ///< 更新回调函数
        };

        LockFreeList node_list_{};           ///< 节点链表
        LockFreeQueue<uint8_t> *rx_queue_{}; ///< 接收队列
        SendCallback send_callback_{};       ///< 发送回调函数

    public:
        /**
         * @brief 节点类模板
         * @tparam DType 节点消息类型
         */
        template <HasHash DType>
        class Node final : LockFreeList::Node<NodeBase>
        {
        private:
            struct
            {
                MsgHeader header{};  ///< 消息头
                DType data{};        ///< 消息对象
            } message_{}, buffer_{}; ///< 消息存储/缓冲区

            friend class Proxy;
            Proxy *proxy_{}; ///< 所属代理对象

            union
            {
                Topic pub_topic_;             ///< 发布者主题
                Topic::CallbackSuber *suber_; ///< 订阅者对象
            };

            /**
             * @brief 处理接收到的消息
             * @param in_isr 是否在中断中调用
             * @param self 节点自身指针
             * @param data 消息数据
             * @note 该函数仅用于订阅者节点
             */
            static void OnMessage(bool in_isr, Node *self, const ConstRawData &data)
            {
                if (!self || !self->proxy_) [[unlikely]]
                {
                    return;
                }

                // 更新消息头
                self->message_.header.seq_num++;
                self->message_.header.crc8 = math::Crc8::Calculate(ConstRawData(&self->message_.header, sizeof(MsgHeader) - sizeof(uint8_t)));
                data.CopyTo_n(RawData{self->message_.data}, sizeof(DType));

                self->proxy_->send_callback_.Call(in_isr, ConstRawData{self->message_});
            }

            /**
             * @brief 更新并发布消息
             * @param in_isr 是否在中断中调用
             * @param self 节点自身指针
             * @note 该函数仅用于发布者节点
             */
            static void UpdateMessage(bool in_isr, Node *self)
            {
                if (!self || !self->proxy_) [[unlikely]]
                {
                    return;
                }

                // 判断是否是新数据(差值超过8视为新数据)
                if ((self->buffer_.header.seq_num >= self->message_.header.seq_num + 1) || (self->buffer_.header.seq_num - self->message_.header.seq_num >= 8))
                {
                    // 更新数据
                    Memory::Copy(&self->message_, &self->buffer_, sizeof(self->message_));

                    // 发布消息
                    UNUSED(self->pub_topic_.PublishFromCallback(in_isr, ConstRawData{self->message_.data}));
                }
            }

        public:
            /**
             * @brief 构造函数
             * @param topic 主题对象
             * @param type 节点类型
             */
            Node(const Topic &topic, NodeType type)
            {
                // 设置消息哈希值和数据指针
                this->GetData().type = type;
                this->GetData().msg_hash = (DType::HASH & 0x00FFFFFF) | ((topic.GetNameHash() & 0xFF) << 24);
                this->GetData().msg_buffer = RawData{buffer_};

                // 初始化节点头
                if (type == NodeType::PUBLISHER)
                {
                    // 关联发布者主题
                    pub_topic_ = topic;

                    // 创建更新回调函数
                    this->GetData().update_callback = Callback<>::Create(UpdateMessage, this);
                }
                else
                {
                    // 初始化消息头
                    message_.header.start_byte = SE_START_BYTE;
                    message_.header.hash = this->GetData().msg_hash;
                    message_.header.seq_num = 0;

                    // 创建回调订阅者
                    suber_ = new Topic::CallbackSuber{
                        topic,
                        Topic::Callback::Create(OnMessage, this)};
                }
            }

            /**
             * @brief 析构函数
             */
            ~Node()
            {
                // 从代理删除节点
                if (proxy_)
                {
                    proxy_->DelNode(*this);
                }

                // 删除订阅者对象
                if (this->GetData().type == NodeType::SUBSCRIBER)
                {
                    delete suber_;
                }
            }
        };

        /**
         * @brief 构造函数
         * @note 如无订阅节点，发送函数不会被调用
         */
        Proxy() = default;

        /**
         * @brief 构造函数
         * @param rx_queue_size 接收队列大小
         * @param send_callback 发送回调函数
         * @note 如无订阅节点，发送函数不会被调用
         */
        Proxy(size_t rx_queue_size, const SendCallback &send_callback);

        /**
         * @brief 添加节点到代理
         * @tparam DType 节点消息类型
         * @param node 节点引用
         */
        template <HasHash DType>
        FORCE_INLINE void AddNode(Proxy::Node<DType> &node)
        {
            if (rx_queue_ && node.GetData().type == NodeType::SUBSCRIBER)
            {
                APPKIT_RAISE_IF(node.GetData().msg_buffer.GetSize() + sizeof(MsgHeader) > rx_queue_->Capacity(),
                                "Node message size exceeds RX queue capacity");
            }

            node.proxy_ = this;
            node_list_.Add(node);
        }

        /**
         * @brief 从代理删除节点
         * @tparam DType 节点消息类型
         * @param node 节点引用
         */
        template <HasHash DType>
        FORCE_INLINE void DelNode(Proxy::Node<DType> &node)
        {
            node.proxy_ = nullptr;
            node_list_.Delete(node);
        }

        /**
         * @brief 推送数据到代理
         * @param in_isr 是否在中断中调用
         * @param data 消息数据
         * @return 错误码
         */
        ErrorCode PushData(bool in_isr, const ConstRawData &data);
    };
}