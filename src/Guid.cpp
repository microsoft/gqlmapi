// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

namespace convert::guid {

std::string to_string(const GUID& guid)
{
	std::ostringstream oss;

	const std::uint32_t data1{ guid.Data1 };
	const std::uint16_t data2{ guid.Data2 };
	const std::uint16_t data3{ guid.Data3 };
	const std::array<std::uint16_t, 4> data4{
		static_cast<std::uint16_t>((guid.Data4[0] << 8) | guid.Data4[1]),
		static_cast<std::uint16_t>((guid.Data4[2] << 8) | guid.Data4[3]),
		static_cast<std::uint16_t>((guid.Data4[4] << 8) | guid.Data4[5]),
		static_cast<std::uint16_t>((guid.Data4[6] << 8) | guid.Data4[7]),
	};

	static_assert(sizeof(data1) == sizeof(guid.Data1));
	static_assert(sizeof(data2) == sizeof(guid.Data2));
	static_assert(sizeof(data3) == sizeof(guid.Data3));
	static_assert(sizeof(data4) == sizeof(guid.Data4));

	oss << std::hex << std::uppercase << std::setfill('0')
		<< std::setw(8) << data1;
	oss << "-" << std::hex << std::uppercase << std::setfill('0')
		<< std::setw(4) << data2;
	oss << "-" << std::hex << std::uppercase << std::setfill('0')
		<< std::setw(4) << data3;
	oss << "-" << std::hex << std::uppercase << std::setfill('0')
		<< std::setw(4) << data4[0];
	oss << "-" << std::hex << std::uppercase << std::setfill('0')
		<< std::setw(4) << data4[1]
		<< data4[2]
		<< data4[3];

	return oss.str();
}

GUID from_string(const std::string& value)
{
	GUID result{};
	std::istringstream iss{ value };

	char separator;
	std::uint32_t data1;
	std::uint16_t data2;
	std::uint16_t data3;
	std::uint16_t data4a;
	std::uint64_t data4b;

	static_assert(sizeof(data1) == sizeof(result.Data1));
	static_assert(sizeof(data2) == sizeof(result.Data2));
	static_assert(sizeof(data3) == sizeof(result.Data3));
	static_assert(sizeof(data4b) == sizeof(result.Data4));

	iss >> std::hex >> std::setw(8) >> data1;
	result.Data1 = data1;

	iss >> separator >> std::hex >> std::setw(4) >> data2;
	result.Data2 = data2;

	iss >> separator >> std::hex >> std::setw(4) >> data3;
	result.Data3 = data3;

	iss >> separator >> std::hex >> std::setw(4) >> data4a;
	result.Data4[0] = static_cast<unsigned char>((data4a & 0xFF00) >> 8);
	result.Data4[1] = static_cast<unsigned char>(data4a & 0xFF);

	iss >> separator >> std::hex >> std::setw(12) >> data4b;
	result.Data4[2] = static_cast<unsigned char>((data4b & 0xFF0000000000) >> 40);
	result.Data4[3] = static_cast<unsigned char>((data4b & 0xFF00000000) >> 32);
	result.Data4[4] = static_cast<unsigned char>((data4b & 0xFF000000) >> 24);
	result.Data4[5] = static_cast<unsigned char>((data4b & 0xFF0000) >> 16);
	result.Data4[6] = static_cast<unsigned char>((data4b & 0xFF00) >> 8);
	result.Data4[7] = static_cast<unsigned char>(data4b & 0xFF);

	return result;
}

} // namespace convert::utf8
