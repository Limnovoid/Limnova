#pragma once

#include <xhash>


namespace Limnova
{

    class UUID
    {
    public:
        UUID();
        UUID(const UUID&) = default;
        UUID(uint64_t value);

        operator uint64_t() const { return m_Value; }

        bool operator==(const UUID rhs) { return m_Value == rhs.m_Value; }
        bool operator!=(const UUID rhs) { return m_Value != rhs.m_Value; }

        static UUID Null;
    private:
        uint64_t m_Value;
    };


    const std::ostream& operator<<(const std::ostream& ostr, const UUID uuid);

}


namespace std
{

    template<>
    struct hash<Limnova::UUID>
    {
        std::size_t operator()(const Limnova::UUID uuid) const
        {
            return hash<uint64_t>()((uint64_t)uuid);
        }
    };

}
