#include "InputUtils.h"

#include "Core/Input.h"

namespace Limnova
{
	namespace Utils
	{

		ResultCode ConvertAsciiDecimalToUint64(const char *pData, size_t length, uint64_t &value)
		{
			size_t i = 0;

			// Trim leading whitespace
			while (i < length && pData[i] == ' ')
				++i;

			uint64_t tempValue = 0;
			static constexpr uint64_t overflowLimit = std::numeric_limits<uint64_t>::max(); // 18446744073709551615

			while (i < length && (pData[i] != ' ' && pData[i] != '\0' && pData[i] != '\n'))
			{
				if (pData[i] < LV_ASCII_0 || LV_ASCII_0 + 9 < pData[i])
					return RESULT_CODE_INVALID_FORMAT;

				uint64_t nextDigit = pData[i] - LV_ASCII_0;

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

		ResultCode ConvertUint64ToAsciiDecimal(uint64_t value, char *pBuffer, size_t bufferLength, size_t &dataLength)
		{
			char tempBuffer[MaxAsciiCharacters<uint64_t>()];
			memset(tempBuffer, ' ', sizeof(tempBuffer));

			size_t i = sizeof(tempBuffer);

			do
			{
				tempBuffer[--i] = LV_ASCII_0 + (value % 10);
				value /= 10;
			}
			while (value != 0);

			dataLength = sizeof(tempBuffer) - i;

			if (dataLength > bufferLength)
				return RESULT_CODE_OVERFLOW;

			memcpy(pBuffer, &tempBuffer[i], dataLength);

			return RESULT_CODE_SUCCESS;
		}
	}
}
