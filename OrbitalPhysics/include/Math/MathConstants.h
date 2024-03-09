#pragma once

#include <limits>

namespace Limnova
{

constexpr double	PI			= 3.1415926535897932384626433832795028841971693993751058209;	/// PI
constexpr double	PI2			= PI * 2.0;														/// PI * 2
constexpr double	PIover2		= PI * 0.5;														/// PI / 2
constexpr double	PIover4		= PI * 0.25;													/// PI / 4
constexpr double	PIover8		= PI * 0.125;													/// PI / 8
constexpr double	PI3over2	= PI * 1.5;														/// PI * 3 / 2
constexpr double	OverPI		= 1.0 / PI;														/// 1 / PI
constexpr double	OverPI2		= 0.5 / PI;														/// 1 / (PI * 2)

constexpr float		PIf			= static_cast<float>(PI);										/// (float) PI
constexpr float		PI2f		= static_cast<float>(PI2);										/// (float) PI * 2
constexpr float		PIover2f	= static_cast<float>(PIover2);									/// (float) PI / 2
constexpr float		PIover4f	= static_cast<float>(PIover4);									/// (float) PI / 4
constexpr float		PIover8f	= static_cast<float>(PIover8);									/// (float) PI / 8
constexpr float		PI3over2f	= static_cast<float>(PI3over2);									/// (float) PI * 3 / 2
constexpr float		OverPIf		= static_cast<float>(OverPI);									/// (float) 1 / PI
constexpr float		OverPI2f	= static_cast<float>(OverPI2);									/// (float) 1 / (PI * 2)

constexpr float		kEps		= ::std::numeric_limits<float>::epsilon(); /// std::numeric_limits float ::epsilon()
constexpr double	kEpsd		= ::std::numeric_limits<double>::epsilon(); /// std::numeric_limits double ::epsilon()
constexpr float		kDotProductEpsilon = 1e-5f; /// Minimum permissible magnitude of the dot product of two non-perpendicular unit vectors.
constexpr float		kParallelDotProductLimit = 1.f - 1e-5f; /// Maximum permissible magnitude of the dot product of two non-parallel unit vectors.

}
