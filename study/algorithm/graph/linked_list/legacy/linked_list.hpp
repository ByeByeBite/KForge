#pragma once
#include "modules/cpp/KF.hpp"
/// @brief 单链表
/// @note  这里使用蟒蛇命名是为了模仿STL 

namespace KLIST
{
    namespace SINGLY
    {
        template <typename T>
        struct Node
        {
            T data;
            Node<T> *next;
            Node(T data) : data(data), next(nullptr) {};
        };
        template <typename T>
        class List
        {
            public:
                List() : head(nullptr), len(0) {};
                ~List() { clear(); };

                /// @brief  获取链表长度
                /// @return 链表长度
                size_t size()
                {
                    return len;
                }

                /// @brief  判断链表是否为空
                /// @return 空返回true，非空返回false
                bool empty()
                {
                    return len == 0;
                }

                /// @brief 链表的第 index 个元素变成data 其后元素后移
                /// @param data 元素
                /// @param index 下标
                void insert(T data,size_t index = 0)
                {
                    Node<T> *New = new Node<T>(data);
                    if(index == 0)
                    {
                        New->next = head;
                        head = New;
                        len++;
                        return;
                    }
                    if(index > len)
                        KLOG_FATAL(UNKNOWN, "linked_list index invalid");
                    Node<T> *pre = head;
                    for(size_t i = 0; i < index - 1;i++)
                        pre = pre->next;
                    New->next = pre->next;
                    pre->next = New;
                    len++;
                }

                void push_front(T data)
                {
                    insert(data,0);
                }

                void push_back(T data)
                {
                    insert(data,len);
                }

                /// @brief  获取指定下标的元素
                /// @param index 下标
                /// @return 元素
                T& at(size_t index = 0)
                {
                    if(index >= len || empty())
                        KLOG_FATAL(UNKNOWN, "linked_list index invalid");
                    Node<T> *p = head;
                    for(size_t i = 0; i < index; i++)
                        p = p->next;
                    return p->data;
                }

                /// @brief  查找第 nth 个元素的下标
                /// @param target 元素 
                /// @param nth 第几个
                /// @return 下标 -1表示未找到
                size_t find(T target,size_t nth = 1)
                {
                    Node<T> *p = head;
                    for(size_t i = 0; i < len; i++)
                    {
                        if(p->data == target)
                        {
                            if(nth == 1)
                                return i;
                            else
                                nth--;
                        }
                        p = p->next;
                    }
                    return std::string::npos;
                }

                void erase(size_t index = 0)
                {
                    if(index >= len || empty())
                        KLOG_FATAL(UNKNOWN, "linked_list index invalid");
                    Node<T> *temp;
                    if(index == 0)
                    {
                        temp = head;
                        head = head->next;
                    }
                    else
                    {
                        Node<T> *p = head;
                        for(size_t i = 0; i < index - 1; i++)
                            p = p->next;
                        temp = p->next;
                        p->next = temp->next;
                    }
                    delete temp;
                    len--;
                }

                void erase_back()
                {
                    erase(len - 1);
                }

                void erase_front()
                {
                    erase(0);
                }

                void clear()
                {
                    while(!empty())
                        erase_front();
                }
                void print(int64_t highlight = -1, const char* color = nullptr)
                {
                    Node<T> *p = head;
                    size_t i = 0;
                    while(p != nullptr)
                    {
                        if((int64_t)i == highlight && color)
                            std::cout << color << p->data << "\033[0m" << " -> ";
                        else
                            std::cout << p->data << " -> ";
                        p = p->next;
                        i++;
                    }
                    std::cout << std::endl;
                }
            private:
                Node<T> *head;
                size_t len;
        };

    }
}
