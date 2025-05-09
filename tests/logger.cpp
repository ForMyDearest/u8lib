#include <u8lib/log.hpp>
#include <u8lib/string.hpp>

#define U8LIB_STRINGIZING(...)			#__VA_ARGS__
#define U8LIB_MAKE_STRING(...)			U8LIB_STRINGIZING(__VA_ARGS__)
#define U8LIB_FILE_LINE					__FILE__ ":" U8LIB_MAKE_STRING(__LINE__)

#define U8LIB_LOG(level, format, ...)	\
	do {	\
		if (!u8lib::log::check_log_level(u8lib::log::LogLevel::level)) break;	\
		u8lib::log::log(reinterpret_cast<const char8_t*>(U8LIB_FILE_LINE), reinterpret_cast<const char8_t*>(__FUNCTION__), u8lib::log::LogLevel::level, format, ##__VA_ARGS__);	\
	} while (0);

#define LOG_TRACE(format, ...)	U8LIB_LOG(trace, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)	U8LIB_LOG(debug, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)	U8LIB_LOG(info, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)	U8LIB_LOG(warn, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)	U8LIB_LOG(error, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)	U8LIB_LOG(fatal, format, ##__VA_ARGS__)

void runBenchmark();

#include <print>
#include <chrono>

void logcb(int64_t ns, u8lib::log::LogLevel level, std::u8string_view location, uint32_t tid, std::u8string_view threadName, std::u8string_view msg) {
	std::println("callback full msg: {}", (const char*)msg.data());
}

void logQFullCB(void* userData) {
	std::println("log q full");
}

int main() {
	// char randomString[] = "Hello World";
	// LOG_INFO(u8"A string, pointer, number, and float: '{}', {}, {}, {}", randomString, (void*) &randomString, 512, 3.14159);
	//
	// char strarr[10] = "111";
	// char* cstr = strarr;
	// std::string str = "aaa";
	// LOG_INFO(u8"str: {}, pstr: {}, strarr: {}, pstrarr: {}, cstr: {}, pcstr: {}", str, (void*)&str, strarr, (void*)&strarr, cstr, (void*)&cstr);
	// str = "bbb";
	// strcpy(cstr, "222");
	//
	// LOG_DEBUG(u8"This message wont be logged since it is lower than the current log level.");
	// u8lib::log::set_log_level(u8lib::log::LogLevel::debug);
	// LOG_DEBUG(u8"Now debug msg is shown");
	//
	// u8lib::log::poll();
	//
	// u8lib::log::set_thread_name(u8"main");
	// LOG_INFO(u8"Thread name changed");
	//
	// u8lib::log::poll();
	//
	// u8lib::log::set_header_pattern(u8"{m} {l}[{t}] {M}");
	// u8lib::log::set_timestamp_precision(u8lib::log::TimestampPrecision::ns);
	// LOG_INFO(u8"Header pattern is changed, full date time info is shown");
	//
	// u8lib::log::poll();
	//
	// u8lib::log::set_log_callback(logcb, u8lib::log::LogLevel::warn);
	// LOG_WARN(u8"This msg will be called back");
	//
	// u8lib::log::set_log_file(u8"/tmp/wow", true);
	// for (int i = 0; i < 10; i++) {
	// 	LOG_WARN(u8"test logfilepos: {}.", i);
	// }
	//
	// u8lib::log::set_log_queue_full_callback(logQFullCB, nullptr);
	// for (int i = 0; i < 1024; i++) {
	// 	std::string str(1000, ' ');
	// 	LOG_INFO(u8"log q full cb test: {}", str);
	// }

	// u8lib::log::poll();
	runBenchmark();

	return 0;
}

void runBenchmark() {
	constexpr int RECORDS = 10000000;
	u8lib::log::close_log_file();
	u8lib::log::set_log_callback(nullptr, u8lib::log::LogLevel::warn);

	const auto t0 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < RECORDS; ++i) {
		LOG_INFO(u8"Simple log message with one parameters, {}", i);
	}
	const auto t1 = std::chrono::high_resolution_clock::now();

	double span = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
	std::println("benchmark, front latency: {:.1f} ns/msg average", (span / RECORDS) * 1e9);
}
