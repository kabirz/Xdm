#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>

static char buf[1024];

static struct {
	uint64_t user;
	uint64_t nice;
	uint64_t system;
	uint64_t idle;
	uint64_t iowait;
	uint64_t irq;
	uint64_t softirq;
	uint64_t steal;
	uint64_t guest;

	uint64_t total;
} old_stat, stat;

static void get_cpu_used(void)
{
	char *s;
	FILE *fp = fopen("/proc/stat", "r");
	if (fp == NULL)
		return;

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		s = strstr(buf, "cpu ");
		if (s && s == buf) {
			sscanf(buf, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu",
			       &stat.user, &stat.nice, &stat.system, &stat.idle,
			       &stat.iowait, &stat.irq, &stat.softirq,
			       &stat.steal, &stat.guest);
			stat.total = stat.user + stat.nice + stat.system +
				     stat.idle + stat.iowait + stat.irq +
				     stat.softirq + stat.steal + stat.guest;
			break;
		}
	}
	fclose(fp);

}

static void get_disk(uint64_t *total, uint64_t *all_used)
{
	char *	 s;
	uint64_t size, used;

	FILE *fp = popen("df", "r");
	*total	  = 0;
	*all_used = 0;
	if (fp == NULL)
		return;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		s = strstr(buf, "/dev/");
		if (s && s == buf) {
			sscanf(s, "%*s %lu %lu", &size, &used);
			*total += size;
			*all_used += used;
		}
	}
	fclose(fp);
}

static void get_mem(uint64_t *total, uint64_t *used, uint64_t *shmem)
{
	char *	 s;
	uint64_t reserve;

	FILE *fp = popen("free", "r");
	if (fp == NULL)
		return;

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		s = strstr(buf, "Mem:");
		if (s && s == buf) {
			sscanf(s, "%*s %lu %lu %lu %lu", total, used, &reserve,
			       shmem);
			break;
		}
	}
	fclose(fp);
}

static void get_rates(uint64_t *up, uint64_t *down)
{
	char *	 s;
	int	 reserve;
	uint64_t _down, _up;

	FILE *fp = fopen("/proc/net/dev", "r");
	if (fp == NULL)
		return;

	*up = 0;
	*down = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		s = strstr(buf, ":");
		if (s && strncmp(buf, "lo", 2)) {
			sscanf(s + 1, "%lu %d %d %d %d %d %d %d %lu", &_down,
			       &reserve, &reserve, &reserve, &reserve, &reserve,
			       &reserve, &reserve, &_up);
			*down += _down;
			*up += _up;
		}
	}
	fclose(fp);
}

static bool get_battery(bool *full, bool *charging, int *percent)
{
	char *s;
	bool  ret = false;
	FILE *fp  = popen("acpi -b", "r");
	if (fp == NULL)
		return false;
	if (fgets(buf, sizeof(buf), fp) != NULL) {
		s	  = strstr(buf, "Full");
		*full	  = s ? true : false;
		s	  = strstr(buf, "Discharging");
		*charging = !s ? true : false;
		s	  = strstr(buf, "%");
		if (s) {
			*s	 = '\0';
			*percent = atoi(s - 3);
		}
		ret = true;
	}
	fclose(fp);
	return ret;
}

int get_hard_info(char *buffer, size_t size)
{
#define MB 1024
#define GB (1024 * 1024)

	time_t	   now;
	bool	   full = false, charging = false;
	int	   percent = 0;
	struct tm *tm_now;
	uint64_t   total, used, shmem;
	int	   count = 0;

	static struct timeval old_tv, now_tv;
	static uint64_t	      old_down = 0, old_up = 0;
	uint64_t down = 0, up = 0;

	/* 1. cpu info */
	get_cpu_used();
	if (old_stat.total != 0) {
		uint64_t maxval = stat.total - old_stat.total;
		if (maxval != 0)
			percent = 100 * (maxval - (stat.idle - old_stat.idle)) /
				  maxval;
		if (percent < 0)
			percent = 0;
	}
	memcpy(&old_stat, &stat, sizeof(stat));
	count += snprintf(buffer + count, size - count, "CPU:%d%%", percent);

	/* 2. disk info */
	get_disk(&total, &used);
	if (total > 10000ul * GB) {
		total = used = 0;
	}
	count += snprintf(buffer + count, size - count, " ⛁%lu/%luGB",
			  used / GB, total / GB);

	/* 3. memory info */
	get_mem(&total, &used, &shmem);
	if (total > 150 * GB) {
		total = used = shmem = 0;
	}
	count += snprintf(buffer + count, size - count, " %lu/%luMB",
			  (used + shmem) / MB, total / MB);

	/* 4. battery info */
	if (get_battery(&full, &charging, &percent)) {
		char *flags = full ? "☻ " : charging ? " " : " ";
		count += snprintf(buffer + count, size - count, " %s%d%%",
				  flags, percent);
	}

	/* 5. network rates info */
	gettimeofday(&now_tv, NULL);
	get_rates(&up, &down);
	if (old_tv.tv_sec != 0 && old_tv.tv_usec != 0) {
		uint64_t us = (now_tv.tv_sec * 1000000 + now_tv.tv_usec) -
			      (old_tv.tv_sec * 1000000 + old_tv.tv_usec);
		float down_kb = (down - old_down) / 1024.0 / (us / 1000000);
		float up_kb   = (up - old_up) / 1024.0 / (us / 1000000);

		if (down_kb > 1024.0) {
			count += snprintf(buffer + count, size - count,
					  " ⬇%.1fMB/s", down_kb / 1024);
		} else {
			count += snprintf(buffer + count, size - count,
					  " ⬇%.1fKB/s", down_kb);
		}

		if (up_kb > 1024.0) {
			count += snprintf(buffer + count, size - count,
					  " ⬆%.1fMB/s", up_kb / 1024);
		} else {
			count += snprintf(buffer + count, size - count,
					  " ⬆%.1fKB/s", up_kb);
		}
	} else {
		count += snprintf(buffer + count, size - count,
				  "  ⬇0KB/s ⬆0KB/s");
	}

	memcpy(&old_tv, &now_tv, sizeof(now_tv));
	old_up	 = up;
	old_down = down;

	/* 6. time update */
	time(&now);
	tm_now = localtime(&now);
	count += snprintf(buffer + count, size - count,
			  " %04d-%02d-%02d %02d:%02d:%02d",
			  tm_now->tm_year + 1900, tm_now->tm_mon + 1,
			  tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min,
			  tm_now->tm_sec);

	return count;
}
