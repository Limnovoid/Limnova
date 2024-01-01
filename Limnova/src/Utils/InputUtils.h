#pragma once

#include "Core/Typedefs.h"

namespace Limnova
{
	namespace Utils
	{

		template<typename NUMERIC>
		constexpr size_t MaxAsciiCharacters()
		{
			LV_CORE_ERROR("MaxAsciiCharacters(): unrecognised type");
			return 0;
		}

		/// <summary>
		/// Convert an ASCII encoding of a decimal number to uint64_t.
		/// Note: 'value' is unchanged if conversion fails.
		/// </summary>
		/// <param name="pData">Source of ASCII data</param>
		/// <param name="length">Length of ASCII data (number of characters)</param>
		/// <param name="value">Storage for output value</param>
		/// <returns>True if successfully converted, otherwise false</returns>
		ResultCode ConvertAsciiDecimalToUint64(const char *pData, size_t length, uint64_t &value);

		/// <summary>
		/// Encode a uint64_t as a decimal number into ASCII representation.
		/// Note: 'pBuffer' and 'dataLength' are unchanged if conversion fails.
		/// </summary>
		/// <param name="value">Value to encode</param>
		/// <param name="pBuffer">Destination of ASCII data</param>
		/// <param name="bufferLength">Length of destination buffer (max number of characters)</param>
		/// <param name="dataLength">Length of encoded representation (number of characters)</param>
		/// <returns></returns>
		ResultCode ConvertUint64ToAsciiDecimal(uint64_t value, char *pBuffer, size_t bufferLength, size_t &dataLength);

	}

	// -------------------------------------------------------------------------------------------------------------------------

	template<>
	constexpr size_t Utils::MaxAsciiCharacters<uint64_t>()
	{
		return 20; // TODO : return log10(numeric_limits<uint64_t>::max()) (requires constexpr log10)
	}
}
