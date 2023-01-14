#pragma once


namespace Limnova
{

    template<typename T, typename V = T>
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
        SortedQueue() {}

        SortedQueue(std::function<bool(const T& lhs, const T& rhs)> compareFn) : m_CompareFn(compareFn) {}

        SortedQueue(std::function<bool(const T& lhs, const T& rhs)> compareFn, std::function<T& (T& lhs, const V& rhs)> assignFn)
            : m_CompareFn(compareFn), m_AssignFn(assignFn) {}

        ~SortedQueue()
        {
            if (m_Front != nullptr)
            {
                SinglyLinkedNode* pnode = m_Front;
                SinglyLinkedNode* pnext;
                while (pnode->next != nullptr)
                {
                    pnext = pnode->Next;
                    delete pnode;
                    pnode = pnext;
                }
                delete pnode;
            }
        }


        void Insert(const T newValue)
        {
            auto* newNode = new SinglyLinkedNode({ newValue, nullptr });

            if (m_Front == nullptr)
            {
                m_Front = newNode;
                return;
            }

            if (m_CompareFn(newNode->Value, m_Front->Value))
            {
                newNode->Next = m_Front;
                m_Front = newNode;
                return;
            }

            SinglyLinkedNode* node = m_Front;
            while (node->Next != nullptr)
            {
                if (m_CompareFn(newNode->Value, node->Next->Value))
                {
                    newNode->Next = node->Next;
                    node->Next = newNode;
                    return;
                }
                node = node->Next;
            }
            node->Next = newNode;
        }


        T& Front()
        {
            return m_Front->Value;
        }


        T PopFront()
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


        void ResetFront(const V newValue)
        {
            m_AssignFn(m_Front->Value, newValue);
            if (m_Front->Next == nullptr || m_CompareFn(m_Front->Value, m_Front->Next->Value))
            {
                return;
            }

            SinglyLinkedNode* oldFront = m_Front;
            m_Front = m_Front->Next;

            SinglyLinkedNode* other = m_Front;
            while (other->Next != nullptr)
            {
                if (m_CompareFn(oldFront->Value, other->Next->Value))
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
        std::function<bool(const T& lhs, const T& rhs)> m_CompareFn = [](const uint32_t& lhs, const uint32_t& rhs) -> bool { return lhs < rhs; };
        std::function<T& (T& lhs, const V& rhs)> m_AssignFn = [](T& lhs, const V& rhs) -> T& { lhs = rhs; return lhs; };
    };


    template<typename T, typename V>
    std::ostream& operator<<(std::ostream& os, const SortedQueue<T,V>& queue)
    {
        return os << queue.Print();
    }

}
