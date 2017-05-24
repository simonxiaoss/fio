#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>

#include "fio.h"

#define LOG_START_SZ		512

size_t log_info_buf(const char *buf, size_t len)
{
	/*
	 * buf could be NULL (not just "").
	 */
	if (!buf)
		return 0;

	if (is_backend) {
		size_t ret = fio_server_text_output(FIO_LOG_INFO, buf, len);
		if (ret != -1)
			return ret;
	}

	if (log_syslog) {
		syslog(LOG_INFO, "%s", buf);
		return len;
	} else
		return fwrite(buf, len, 1, f_out);
}

size_t log_valist(const char *str, va_list args)
{
	size_t len, cur = LOG_START_SZ;
	char *buffer;

	do {
		buffer = calloc(1, cur);

		len = vsnprintf(buffer, cur, str, args);
		if (len <= cur)
			break;

		cur = len;
		free(buffer);
	} while (1);

	cur = log_info_buf(buffer, len);
	free(buffer);

	return cur;
}

size_t log_info(const char *format, ...)
{
	size_t len, cur = LOG_START_SZ;
	char *buffer;
	va_list args;

	do {
		buffer = calloc(1, cur);

		va_start(args, format);
		len = vsnprintf(buffer, cur, format, args);
		va_end(args);

		if (len <= cur)
			break;

		cur = len;
		free(buffer);
	} while (1);

	cur = log_info_buf(buffer, len);
	free(buffer);

	return cur;
}

size_t __log_buf(struct buf_output *buf, const char *format, ...)
{
	size_t len, cur = LOG_START_SZ;
	char *buffer;
	va_list args;

	do {
		buffer = calloc(1, cur);

		va_start(args, format);
		len = vsnprintf(buffer, cur, format, args);
		va_end(args);

		if (len <= cur)
			break;

		cur = len;
		free(buffer);
	} while (1);

	cur = buf_output_add(buf, buffer, len);
	free(buffer);

	return cur;
}

int log_info_flush(void)
{
	if (is_backend || log_syslog)
		return 0;

	return fflush(f_out);
}

size_t log_err(const char *format, ...)
{
	size_t ret, len, cur = LOG_START_SZ;
	char *buffer;
	va_list args;

	do {
		buffer = calloc(1, cur);

		va_start(args, format);
		len = vsnprintf(buffer, cur, format, args);
		va_end(args);

		if (len <= cur)
			break;

		cur = len;
		free(buffer);
	} while (1);


	if (is_backend) {
		ret = fio_server_text_output(FIO_LOG_ERR, buffer, len);
		if (ret != -1)
			goto done;
	}

	if (log_syslog) {
		syslog(LOG_INFO, "%s", buffer);
		ret = len;
	} else {
		if (f_err != stderr)
			ret = fwrite(buffer, len, 1, stderr);

		ret = fwrite(buffer, len, 1, f_err);
	}

done:
	free(buffer);
	return ret;
}

const char *log_get_level(int level)
{
	static const char *levels[] = { "Unknown", "Debug", "Info", "Error",
						"Unknown" };

	if (level >= FIO_LOG_NR)
		level = FIO_LOG_NR;

	return levels[level];
}
