#include "InputUtils.h"

#include "Core/Input.h"

namespace Limnova
{
	namespace Utils
	{
		bool ConvertAsciiDecimalToUint64(const char *pData, size_t length, uint64_t &value)
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
					return false;

				uint64_t nextDigit = pData[i] - LV_ASCII_0;

				if ((tempValue > overflowLimit / 10) ||
					(tempValue == overflowLimit / 10 && nextDigit > overflowLimit % 10))
				{
					return false;
				}

				tempValue *= 10;
				tempValue += nextDigit;

				++i;
			}

			value = tempValue;

			return true;
		}

		// -------------------------------------------------------------------------------------------------------------------------

		bool ConvertUint64ToAsciiDecimal(uint64_t value, char *pBuffer, size_t bufferLength, size_t &dataLength)
		{
			char tempBuffer[20];
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
				return false;

			memcpy(pBuffer, &tempBuffer[i], dataLength);

			return true;
		}
	}
}
