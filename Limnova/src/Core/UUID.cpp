#include "UUID.h"

#include <random>
#include <pcg_random.hpp>


namespace Limnova
{

    UUID UUID::Null = 0;


    //static std::random_device s_RandomDevice;
    //static std::mt19937_64 s_Engine(s_RandomDevice());
    static pcg_extras::seed_seq_from<std::random_device> s_SeedSource;
    static pcg64 s_RNG(s_SeedSource);
    static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

    UUID::UUID()
        : m_Value(s_UniformDistribution(s_RNG) + 1) /* 0 means invalid ID */
    {
    }


    UUID::UUID(uint64_t value)
        : m_Value(value)
    {
    }


    const std::ostream& operator<<(const std::ostream& ostr, const UUID uuid)
    {
        return ostr << (uint64_t)uuid;
    }

}
