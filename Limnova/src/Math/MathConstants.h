#pragma once


namespace Limnova
{

    constexpr double PI         = 3.1415926535897932384626433832795028841971693993751058209;    /// PI
    constexpr double PI2        = PI * 2.0;                                                     /// PI * 2
    constexpr double PIover2    = PI * 0.5;                                                     /// PI / 2
    constexpr double PIover4    = PI * 0.25;                                                    /// PI / 4
    constexpr double PIover8    = PI * 0.125;                                                   /// PI / 8
    constexpr double PI3over2   = PI * 1.5;                                                     /// PI * 3 / 2
    constexpr double OverPI     = 1.0 / PI;                                                     /// 1 / PI
    constexpr double OverPI2    = 0.5 / PI;                                                     /// 1 / (PI * 2)

    constexpr float PIf         = (float)PI;                                                    /// (float) PI
    constexpr float PI2f        = (float)PI2;                                                   /// (float) PI * 2
    constexpr float PIover2f    = (float)PIover2;                                               /// (float) PI / 2
    constexpr float PIover4f    = (float)PIover4;                                               /// (float) PI / 4
    constexpr float PIover8f    = (float)PIover8;                                               /// (float) PI / 8
    constexpr float PI3over2f   = (float)PI3over2;                                              /// (float) PI * 3 / 2
    constexpr float OverPIf     = (float)OverPI;                                                /// (float) 1 / PI
    constexpr float OverPI2f    = (float)OverPI2;                                               /// (float) 1 / (PI * 2)

    constexpr float     kEps        = std::numeric_limits<float>::epsilon(); /* std::numeric_limits<float>::epsilon() */
    constexpr double    kEpsd       = std::numeric_limits<double>::epsilon(); /* std::numeric_limits<double>::epsilon() */
    constexpr float     kDotProductEpsilon = 1e-5f; /* Minimum permissible magnitude of the dot product of two non-perpendicular unit vectors */
    constexpr float     kParallelDotProductLimit = 1.f - 1e-5f; /* Maximum permissible magnitude of the dot product of two non-parallel unit vectors */

}