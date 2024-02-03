#include "InputUtils.h"

#include "Core/Input.h"

namespace Limnova
{

	namespace
	{

		template<typename NUMERIC>
		ResultCode AsciiDecimalToUIntImpl(const char* pData, size_t length, NUMERIC& value)
		{
			size_t i = 0;

			// Trim leading whitespace
			while (i < length && pData[i] == ' ')
				++i;

			NUMERIC tempValue = 0;
			static constexpr NUMERIC overflowLimit = std::numeric_limits<NUMERIC>::max();

			while (i < length && (pData[i] != ' ' && pData[i] != '\0' && pData[i] != '\n'))
			{
				if (pData[i] < LV_ASCII_0 || LV_ASCII_0 + 9 < pData[i])
					return RESULT_CODE_INVALID_FORMAT;

				NUMERIC nextDigit = pData[i] - LV_ASCII_0;

				if ((tempValue > overflowLimit / 10) ||
					(tempValue == overflowLimit / 10 && nextDigit > overflowLimit % 10))
				{
					return RESULT_CODE_OVERFLOW;
				}

				tempValue *= 10;
				tempValue += nextDigit;

				++i;
			}

			value = tempValue;

			return RESULT_CODE_SUCCESS;
		}

		// -------------------------------------------------------------------------------------------------------------------------

		template<typename NUMERIC>
		ResultCode UIntToAsciiDecimalImpl(NUMERIC value, char* pBuffer, size_t bufferLength, size_t& dataLength)
		{
			char tempBuffer[Utils::MaxAsciiCharacters<NUMERIC>()];
			memset(tempBuffer, ' ', sizeof(tempBuffer));

			size_t i = sizeof(tempBuffer);

			do
			{
				tempBuffer[--i] = LV_ASCII_0 + (value % 10);
				value /= 10;
			} while (value != 0);

			dataLength = sizeof(tempBuffer) - i;

			if (dataLength > bufferLength)
				return RESULT_CODE_OVERFLOW;

			memcpy(pBuffer, &tempBuffer[i], dataLength);

			return RESULT_CODE_SUCCESS;
		}

	}

	namespace Utils
	{

		template<>
		ResultCode AsciiDecimalToUInt<uint64_t>(const char* pData, size_t length, uint64_t& value)
		{
			return AsciiDecimalToUIntImpl(pData, length, value);
		}

		// -------------------------------------------------------------------------------------------------------------------------

		template<>
		ResultCode AsciiDecimalToUInt<uint32_t>(const char* pData, size_t length, uint32_t& value)
		{
			return AsciiDecimalToUIntImpl(pData, length, value);
		}

		// -------------------------------------------------------------------------------------------------------------------------

		template<> ResultCode UIntToAsciiDecimal<uint64_t>(uint64_t value, char* pBuffer, size_t bufferLength, size_t& dataLength)
		{
			return UIntToAsciiDecimalImpl(value, pBuffer, bufferLength, dataLength);
		}

		// -------------------------------------------------------------------------------------------------------------------------

		template<> ResultCode UIntToAsciiDecimal<uint32_t>(uint32_t value, char* pBuffer, size_t bufferLength, size_t& dataLength)
		{
			return UIntToAsciiDecimalImpl(value, pBuffer, bufferLength, dataLength);
		}

	}
}
