#pragma once

#include "Core/Typedefs.h"

namespace Limnova
{
	namespace Utils
	{

		template<typename NUMERIC>
		constexpr size_t MaxAsciiCharacters()
		{
			LV_CORE_ERROR("MaxAsciiCharacters(): unsupported type");
			return 0;
		}

		template<typename NUMERIC>
		ResultCode AsciiDecimalToUInt(const char* pData, size_t length, NUMERIC& value)
		{
			LV_CORE_ERROR("ConvertAsciiDecimalToNumeric(): unsupported type");
			return 0;
		}

		template<typename NUMERIC>
		ResultCode UIntToAsciiDecimal(NUMERIC value, char* pBuffer, size_t bufferLength, size_t& dataLength)
		{
			LV_CORE_ERROR("ConvertNumericToAsciiDecimal(): unsupported type");
			return 0;
		}

	}

	// -------------------------------------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------------------------------------

	template<>
	constexpr size_t Utils::MaxAsciiCharacters<uint64_t>()
	{
		return 20; // TODO : return log10(numeric_limits<uint64_t>::max()) (requires constexpr log10)
	}

	// -------------------------------------------------------------------------------------------------------------------------

	template<>
	constexpr size_t Utils::MaxAsciiCharacters<uint32_t>()
	{
		return 10; // TODO : return log10(numeric_limits<uint64_t>::max()) (requires constexpr log10)
	}

	// -------------------------------------------------------------------------------------------------------------------------

	template<> ResultCode Utils::AsciiDecimalToUInt<uint64_t>(const char* pData, size_t length, uint64_t& value);
	template<> ResultCode Utils::AsciiDecimalToUInt<uint32_t>(const char* pData, size_t length, uint32_t& value);

	template<> ResultCode Utils::UIntToAsciiDecimal<uint64_t>(uint64_t value, char* pBuffer, size_t bufferLength, size_t& dataLength);
	template<> ResultCode Utils::UIntToAsciiDecimal<uint32_t>(uint32_t value, char* pBuffer, size_t bufferLength, size_t& dataLength);

}
