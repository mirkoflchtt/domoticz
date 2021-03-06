#include "stdafx.h"
#include "localtime_r.h"

time_t m_lasttime=time(NULL);
boost::mutex TimeMutex_;

#ifdef __APPLE__
	#define localtime_r
#endif

#ifndef localtime_r
struct tm *localtime_r(const time_t *timep, struct tm *result)
{
#ifdef localtime_s
	localtime_s(timep, result);
#else
	boost::mutex::scoped_lock lock(TimeMutex_);
	struct tm *s = localtime(timep);
	if (s == NULL)
		return NULL;
	memcpy(result, s, sizeof(struct tm));
#endif
	return result;
}
#endif

time_t mytime(time_t * _Time)
{
	time_t acttime=time(_Time);
	if (acttime<m_lasttime)
		return m_lasttime;
	m_lasttime=acttime;
	return acttime;
}

// GB3
/* ParseSQLdatetime()
 * Sets time value and corresponding tm struct to match a localized datetime string in 
 * SQL format (YYYY-MM-DD HH:mm:dd). Corrects for DST jump by verifying if mktime
 * returned a different value of tm_isdst than assumed.
 *
 * Note that if the SQL datetime input string falls within the DST jump range this
 * method may still produce an incorrect result. This is because default assumption for
 * the DST flag is to use the one from current time. To defeat this limitation you may
 * use the optional parameter isdst to set either Summer (1) or Winter (0) time as the 
 * initial value, or set to -1 to accept either one.
 *
 * Returns false if no time can be created
 */
bool ParseSQLdatetime(time_t &time, struct tm &result, const std::string szSQLdate) {
	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	return ParseSQLdatetime(time, result, szSQLdate, ltime.tm_isdst);
}

bool ParseSQLdatetime(time_t &time, struct tm &result, const std::string szSQLdate, int isdst) {
	if (szSQLdate.length() != 19) {
		return false;
	}

	bool goodtime = false;
	while (!goodtime) {
		result.tm_isdst = isdst;
		result.tm_year = atoi(szSQLdate.substr(0, 4).c_str()) - 1900;
		result.tm_mon = atoi(szSQLdate.substr(5, 2).c_str()) - 1;
		result.tm_mday = atoi(szSQLdate.substr(8, 2).c_str());
		result.tm_hour = atoi(szSQLdate.substr(11, 2).c_str());
		result.tm_min = atoi(szSQLdate.substr(14, 2).c_str());
		result.tm_sec = atoi(szSQLdate.substr(17, 2).c_str());
		time = mktime(&result);
		if (time == -1) {
			if (isdst == 0) { return false; }
			isdst = 0;
		} else {
			goodtime = ((result.tm_isdst == isdst) || (isdst == -1));
			isdst = result.tm_isdst;
		}
	}
	return true;
}


/* constructTime()
 * Updates a time_t value and corresponding tm struct with given datetime components
 *
 * Returns false if no time can be created
 */

bool constructTime(time_t &time, struct tm &result, int year, int month, int day, int hour, int minute, int second) {
	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	return constructTime(time, result, year, month, day, hour, minute, second, ltime.tm_isdst);
}

bool constructTime(time_t &time, struct tm &result, int year, int month, int day, int hour, int minute, int second, int isdst) {
	bool goodtime = false;
	while (!goodtime) {
		result.tm_isdst = isdst;
		result.tm_year = year - 1900;
		result.tm_mon = month - 1;
		result.tm_mday = day;
		result.tm_hour = hour;
		result.tm_min = minute;
		result.tm_sec = second;
		time = mktime(&result);
		if (time == -1) {
			if (isdst == 0) { return false; }
			isdst = 0;
		} else {
			goodtime = ((result.tm_isdst == isdst) || (isdst == -1));
			isdst = result.tm_isdst;
		}
	}
	return true;
}

/* getMidnight()
 * Shorthand time construct to retrieve the ctime value and corresponding tm struct for midnight
 * Accepts date components for specifying another date than today.
 */

bool getMidnight(time_t &time, struct tm &result) {
	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	return constructTime(time, result, ltime.tm_year, ltime.tm_mon, ltime.tm_mday, 0, 0, 0, ltime.tm_isdst);
}

bool getMidnight(time_t &time, struct tm &result, int year, int month, int day) {
	return constructTime(time, result, year, month, day, 0, 0, 0);
}

/* getNoon()
 * Shorthand time construct to retrieve the ctime value and corresponding tm struct for noon.
 * Accepts date components for specifying another date than today.
 *
 * While noon has no special meaning in Domoticz, you may use this function to save CPU cycles
 * if you are only interested in the (corrected) date.
 */
bool getNoon(time_t &time, struct tm &result) {
	time_t now = mytime(NULL);
	struct tm ltime;
	localtime_r(&now,&ltime);
	return constructTime(time, result, ltime.tm_year, ltime.tm_mon, ltime.tm_mday, 12, 0, 0, ltime.tm_isdst);
}

bool getNoon(time_t &time, struct tm &result, int year, int month, int day) {
	return constructTime(time, result, year, month, day, 12, 0, 0, -1);
}
