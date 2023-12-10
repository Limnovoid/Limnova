#pragma once

namespace Limnova
{
	namespace Utils
	{
		/// <summary>
		/// Convert an ASCII encoding of a decimal number to uint64_t.
		/// Note: 'value' is unchanged if conversion fails.
		/// </summary>
		/// <param name="pData">Source of ASCII data</param>
		/// <param name="length">Length of ASCII data (number of characters)</param>
		/// <param name="value">Storage for output value</param>
		/// <returns>True if successfully converted, otherwise false</returns>
		bool ConvertAsciiDecimalToUint64(const char *pData, size_t length, uint64_t &value);

		/// <summary>
		/// Encode a uint64_t as a decimal number into ASCII representation.
		/// Note: 'pBuffer' and 'dataLength' are unchanged if conversion fails.
		/// </summary>
		/// <param name="value">Value to encode</param>
		/// <param name="pBuffer">Destination of ASCII data</param>
		/// <param name="bufferLength">Length of destination buffer (max number of characters)</param>
		/// <param name="dataLength">Length of encoded representation (number of characters)</param>
		/// <returns></returns>
		bool ConvertUint64ToAsciiDecimal(uint64_t value, char *pBuffer, size_t bufferLength, size_t &dataLength);

	}
}
