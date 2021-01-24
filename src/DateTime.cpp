// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DateTime.h"
#include "CheckResult.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace convert::datetime {

std::string to_string(const FILETIME& ft)
{
	SYSTEMTIME systime{};

	// Convert to UTC SYSTEMTIME.
	CFRt(FileTimeToSystemTime(&ft, &systime));

	// Convert to UTC std::tm.
	std::tm tm{};

	tm.tm_year = systime.wYear - 1900;
	tm.tm_mon = systime.wMonth - 1;
	tm.tm_mday = systime.wDay;
	tm.tm_wday = systime.wDayOfWeek;
	tm.tm_hour = systime.wHour;
	tm.tm_min = systime.wMinute;
	tm.tm_sec = systime.wSecond;
	tm.tm_isdst = 0;

	// Format the std::tm struct.
	std::ostringstream oss;

	oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");

	if (systime.wMilliseconds > 0
		&& systime.wMilliseconds < 1000)
	{
		// Insert the milliseconds for more precision.
		oss << "." << std::setw(3) << std::setfill('0')
			<< systime.wMilliseconds;
	}

	// Add the UTC timezone specifier.
	oss << 'Z';

	return oss.str();
}

FILETIME from_string(const std::string& value)
{
	FILETIME result{};
	std::istringstream iss{ value };
	std::tm tm{};
	std::uint16_t msec = 0;
	char last = '\0';

	iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S")
		>> last;

	CFRt(!iss.fail());

	switch (last)
	{
		case '.':
		case ',':
		{
			iss >> std::setw(3) >> msec
				>> last;

			CFRt(!iss.fail());
			CFRt(last == 'Z');
			CFRt(msec >= 0);
			CFRt(msec < 1000);
			break;
		}
		case 'Z':
			break;

		default:
		{
			constexpr bool Bad_DateTimeString = true;
			CFRt(!Bad_DateTimeString);
		}
	}

	// Make sure we reached the end of the string.
	iss >> last;
	CFRt(iss.eof());

	// Convert to UTC SYSTEMTIME.
	SYSTEMTIME systime{};

	systime.wYear = tm.tm_year + 1900;
	systime.wMonth = tm.tm_mon + 1;
	systime.wDay = tm.tm_mday;
	systime.wDayOfWeek = tm.tm_wday;
	systime.wHour = tm.tm_hour;
	systime.wMinute = tm.tm_min;
	systime.wSecond = tm.tm_sec;
	systime.wMilliseconds = msec;

	// Convert to UTC FILETIME.
	CFRt(SystemTimeToFileTime(&systime, &result));

	return result;
}

} // namespace convert::datetime
