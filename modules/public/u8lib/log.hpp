#pragma once

#include "base.hpp"

namespace u8lib::log
{
	enum class LogLevel : uint8_t {
		trace = 0,
		debug,
		info,
		warn,
		error,
		fatal,
		off
	};

	enum class TimestampPrecision : uint8_t {
		none,
		ms,
		us,
		ns
	};

	static constexpr char8_t kPatternParam[] = {
		'l', // log level
		'L', // file location
		't', // thread id
		'T', // thread name
		'f', // function name
		'm', // timestamp
		'M', // message
	};

	/*!
	 * @brief
	 *		Preallocate thread queue for current thread
	 * @note
	 *		Since it takes some time to allocate the queue, calling preallocate()
	 *		after creating the thread can reduce the latency of the first log
	 */
	U8LIB_API void preallocate();

	// Set the file for logging
	U8LIB_API void set_log_file(const char8_t* filename, bool truncate = false);

	/*!
	 * @brief
	 *		Set an existing FILE* for logging
	 *
	 * @param manageFp
	 *		if manageFp is false logger will not buffer log
	 *		internally and will not close the FILE*
	 */
	U8LIB_API void set_log_file(FILE* fp, bool manageFp = false);

	// Close the log file and subsequent msgs will not be written into the file,
	// but callback function can still be used
	U8LIB_API void close_log_file();

	// Set current log level, lower level log msgs will be discarded
	U8LIB_API void set_log_level(LogLevel logLevel);

	//! @return Current log level
	U8LIB_API LogLevel get_log_level();

	//! @return True if passed log level is not lower than current log level
	U8LIB_API bool check_log_level(LogLevel level);

	// Set flush delay in nanosecond
	// If there's msg older than ns in the buffer, flush will be triggered
	U8LIB_API void set_flush_delay(int64_t ns);

	// If current msg has level >= flush_log_level, flush will be triggered
	U8LIB_API void set_flush_log_level(LogLevel flush_log_level);

	// If file buffer has more than specified bytes, flush will be triggered
	U8LIB_API void set_flush_buffer_size(uint32_t bytes);

	// callback signature user can register
	typedef void (*LogCBFn)(
		int64_t ns,                    // nanosecond timestamp
		LogLevel level,                // logLevel
		std::u8string_view location,   // full file path with line num
		uint32_t tid,                  // thread id
		std::u8string_view threadName, // thread name
		std::u8string_view msg         // full log msg with header
	);

	/*!
	 * @brief
	 *		Set a callback function for all log msgs with a mininum log level
	 * @note
	 *		There is a risk of thread race.
	 *		Please add mutex if nessasery.
	 */
	U8LIB_API void set_log_callback(LogCBFn cb, LogLevel minCBLogLevel);

	typedef void (*LogQFullCBFn)(void* userData);
	U8LIB_API void set_log_queue_full_callback(LogQFullCBFn cb, void* userData);

	// Set log header pattern with fmt named arguments
	// TODO : fix bug ( not support character '{' )
	U8LIB_API void set_header_pattern(std::u8string_view pattern);

	// Set a name for current thread, it'll be shown in {t} part in header pattern
	U8LIB_API void set_thread_name(std::u8string_view name);

	U8LIB_API void set_timestamp_precision(TimestampPrecision precision);

	U8LIB_API void vlog(const char8_t* location, const char8_t* function, LogLevel level, std::u8string_view fmt, format_args args);

	template<typename... Args>
	void log(const char8_t* location, const char8_t* function, LogLevel level, format_string<Args...> fmt, Args&&... args) {
		constexpr auto DESC = internal::make_descriptor<Args...>();
		log::vlog(location, function, level, fmt.get(), format_args(u8lib::make_format_store(args...), DESC));
	}

	/*!
	 * @brief
	 *		Collect log msgs from all threads and write to log file
	 *
	 * @param force_flush
	 *		If true, internal file buffer is flushed
	 *
	 * @note
	 *		User need to call poll() repeatedly if start_polling_thread is not used
	 */
	U8LIB_API void poll(bool force_flush = false);

	/*!
	 * @brief
	 *		Run a polling thread in the background with a polling interval in ns
	 *
	 * @note
	 *		poll() can't be called after start_polling_thread() is called
	 */
	U8LIB_API void start_polling_thread(int64_t poll_interval = 1000000000);

	// Stop the polling thread
	U8LIB_API void stop_polling_thread();
}
