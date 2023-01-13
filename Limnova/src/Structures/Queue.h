#pragma once


namespace Limnova
{

    template<typename T>
    class SortedQueue
    {
    private:
        struct SinglyLinkedNode
        {
            T Value;
            SinglyLinkedNode* Next;
        };
        SinglyLinkedNode* m_Front = nullptr;
    public:
        SortedQueue() : m_Compare([](const uint32_t& lhs, const uint32_t& rhs) -> bool { return lhs < rhs; }) {}
        SortedQueue(const std::function<bool(const T& lhs, const T& rhs)>& fn) : m_Compare(fn) {}
        ~SortedQueue() {}


        void Insert(const T newValue)
        {
            auto* newNode = new SinglyLinkedNode({ newValue, nullptr });

            if (m_Front == nullptr)
            {
                m_Front = newNode;
                return;
            }

            if (m_Compare(newNode->Value, m_Front->Value))
            {
                newNode->Next = m_Front;
                m_Front = newNode;
                return;
            }

            SinglyLinkedNode* node = m_Front;
            while (node->Next != nullptr)
            {
                if (m_Compare(newNode->Value, node->Next->Value))
                {
                    newNode->Next = node->Next;
                    node->Next = newNode;
                    return;
                }
                node = node->Next;
            }
            node->Next = newNode;
        }


        int Front()
        {
            return m_Front->Value;
        }


        int PopFront()
        {
            SinglyLinkedNode oldFront = *m_Front;
            delete m_Front;
            if (oldFront.Next == nullptr)
            {
                m_Front = nullptr;
                return oldFront.Value;
            }
            m_Front = oldFront.Next;
            return oldFront.Value;
        }


        void ResetFront(const T newValue)
        {
            m_Front->Value = newValue;
            if (m_Front->Next == nullptr || newValue < m_Front->Next->Value)
            {
                return;
            }

            SinglyLinkedNode* oldFront = m_Front;
            m_Front = m_Front->Next;

            SinglyLinkedNode* other = m_Front;
            while (other->Next != nullptr)
            {
                if (m_Compare(newValue, other->Next->Value))
                {
                    oldFront->Next = other->Next;
                    other->Next = oldFront;
                    return;
                }
                other = other->Next;
            }
            other->Next = oldFront;
            oldFront->Next = nullptr;
        }


        void Print()
        {
            std::ostringstream oss;
            SinglyLinkedNode* node = m_Front;
            while (node->Next != nullptr)
            {
                oss << node->Value << ", ";
                node = node->Next;
            }
            oss << node->Value;
            std::cout << oss.str() << std::endl;
        }

    private:
        std::function<bool(const T& lhs, const T& rhs)> m_Compare;
    };

}
