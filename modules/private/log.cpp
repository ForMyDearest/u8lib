#include "pch.hpp"

#include <u8lib/log.hpp>

#include <array>
#include <mutex>
#include <thread>
#include <vector>


#ifdef _MSC_VER
#	include <intrin.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <processthreadsapi.h>
#else
#	include <sys/syscall.h>
#	include <unistd.h>
#endif

#ifndef U8LOG_QUEUE_SIZE
#	define U8LOG_QUEUE_SIZE (1 << 20)
#endif

#ifndef U8LOG_BLOCK
#	define U8LOG_BLOCK 0
#endif

namespace u8lib::log
{
	static constexpr std::u8string_view default_pattern =
			u8"[{m:<25}] [{T}(tid:{t})] [{l}: {M}]\n"
			u8"    [In {f}] [At {L}]";

	static constexpr TimestampPrecision default_precision = TimestampPrecision::ms;

	static constexpr std::u8string_view default_thread_name = u8"unnamed thread";
}

namespace u8lib::log
{
	// https://github.com/MengRao/str
	template<size_t SIZE>
	struct Str {
		static const int Size = SIZE;
		char8_t s[SIZE];

		constexpr Str() = default;
		Str(const char8_t* p) { *this = *(const Str*) p; }

		char8_t& operator[](int i) { return s[i]; }
		char8_t operator[](int i) const { return s[i]; }

		template<typename T>
		void fromi(T num) {
			if constexpr (Size & 1) {
				s[Size - 1] = '0' + (num % 10);
				num /= 10;
			}
			switch (Size & -2) {
				case 18: *(uint16_t*) (s + 16) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 16: *(uint16_t*) (s + 14) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 14: *(uint16_t*) (s + 12) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 12: *(uint16_t*) (s + 10) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 10: *(uint16_t*) (s + 8) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 8: *(uint16_t*) (s + 6) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 6: *(uint16_t*) (s + 4) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 4: *(uint16_t*) (s + 2) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
				case 2: *(uint16_t*) (s + 0) = *(uint16_t*) (digit_pairs + ((num % 100) << 1));
					num /= 100;
			}
		}

		static constexpr const char8_t* digit_pairs =
				u8"00010203040506070809"
				"10111213141516171819"
				"20212223242526272829"
				"30313233343536373839"
				"40414243444546474849"
				"50515253545556575859"
				"60616263646566676869"
				"70717273747576777879"
				"80818283848586878889"
				"90919293949596979899";
	};

	struct MsgHeader {
		void push(uint32_t sz) {
			*reinterpret_cast<volatile uint32_t*>(&size) = sz + sizeof(MsgHeader);
		}

		uint32_t size;
		uint32_t logId;
	};

	// https://github.com/MengRao/SPSC_Queue
	class SPSCVarQueueOPT {
	public:
		static constexpr uint32_t BLK_CNT = U8LOG_QUEUE_SIZE / sizeof(MsgHeader);

		MsgHeader* alloc(uint32_t size) {
			size += sizeof(MsgHeader);
			const uint32_t blk_sz = (size + sizeof(MsgHeader) - 1) / sizeof(MsgHeader);
			if (blk_sz >= free_write_cnt) {
				const uint32_t read_idx_cache = *reinterpret_cast<volatile uint32_t*>(&read_idx);
				if (read_idx_cache <= write_idx) {
					free_write_cnt = BLK_CNT - write_idx;
					if (blk_sz >= free_write_cnt && read_idx_cache != 0) {
						// wrap around
						blk[0].size = 0;
						blk[write_idx].size = 1;
						write_idx = 0;
						free_write_cnt = read_idx_cache;
					}
				} else {
					free_write_cnt = read_idx_cache - write_idx;
				}
				if (free_write_cnt <= blk_sz) {
					return nullptr;
				}
			}
			MsgHeader* ret = &blk[write_idx];
			write_idx += blk_sz;
			free_write_cnt -= blk_sz;
			blk[write_idx].size = 0;
			return ret;
		}

		const MsgHeader* front() {
			uint32_t size = blk[read_idx].size;
			if (size == 1) {
				// wrap around
				read_idx = 0;
				size = blk[0].size;
			}
			if (size == 0) return nullptr;
			return &blk[read_idx];
		}

		void pop() {
			const uint32_t blk_sz = (blk[read_idx].size + sizeof(MsgHeader) - 1) / sizeof(MsgHeader);
			*reinterpret_cast<volatile uint32_t*>(&read_idx) = read_idx + blk_sz;
		}

	private:
		alignas(64) MsgHeader blk[BLK_CNT] = {};
		uint32_t write_idx = 0;
		uint32_t free_write_cnt = BLK_CNT;
		alignas(128) uint32_t read_idx = 0;
	};

	// https://github.com/MengRao/tscns
	struct TSCNS {
		static constexpr int64_t NsPerSec = 1000000000;

		void init(int64_t init_calibrate_ns = 20000000, int64_t calibrate_interval_ns = 3 * NsPerSec) {
			calibate_interval_ns_ = calibrate_interval_ns;
			int64_t base_tsc, base_ns;
			syncTime(base_tsc, base_ns);
			const int64_t expire_ns = base_ns + init_calibrate_ns;
			while (rdsysns() < expire_ns) std::this_thread::yield();
			int64_t delayed_tsc, delayed_ns;
			syncTime(delayed_tsc, delayed_ns);
			const double init_ns_per_tsc = static_cast<double>(delayed_ns - base_ns) / static_cast<double>(delayed_tsc - base_tsc);
			saveParam(base_tsc, base_ns, 0, init_ns_per_tsc);
		}

		void calibrate() {
			if (rdtsc() < next_calibrate_tsc_) return;
			int64_t tsc, ns;
			syncTime(tsc, ns);
			int64_t ns_err = tsc2ns(tsc) - ns;
			if (ns_err > 1000000) ns_err = 1000000;
			if (ns_err < -1000000) ns_err = -1000000;
			const double new_ns_per_tsc =
					ns_per_tsc_ * (1.0 - static_cast<double>(ns_err + ns_err - base_ns_err_) / (static_cast<double>(tsc - base_tsc_) * ns_per_tsc_));
			saveParam(tsc, ns, ns_err, new_ns_per_tsc);
		}

		static int64_t rdtsc() {
			return static_cast<int64_t>(
#ifdef _MSC_VER
				__rdtsc()
#elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
				__builtin_ia32_rdtsc()
#else
				rdsysns()
#endif
			);
		}

		int64_t tsc2ns(int64_t tsc) const {
			while (true) {
				const uint32_t before_seq = param_seq_.load(std::memory_order_acquire) & ~1;
				std::atomic_signal_fence(std::memory_order_acq_rel);
				const int64_t ns = base_ns_ + static_cast<int64_t>(static_cast<double>(tsc - base_tsc_) * ns_per_tsc_);
				std::atomic_signal_fence(std::memory_order_acq_rel);
				const uint32_t after_seq = param_seq_.load(std::memory_order_acquire);
				if (before_seq == after_seq) return ns;
			}
		}

		int64_t rdns() const { return tsc2ns(rdtsc()); }

		static int64_t rdsysns() {
			using namespace std::chrono;
			return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
		}

		double getTscGhz() const { return 1.0 / ns_per_tsc_; }

		// Linux kernel sync time by finding the first trial with tsc diff < 50000
		// We try several times and return the one with the mininum tsc diff.
		// Note that MSVC has a 100ns resolution clock, so we need to combine those ns with the same
		// value, and drop the first and the last value as they may not scan a full 100ns range
		static void syncTime(int64_t& tsc_out, int64_t& ns_out) {
#ifdef _MSC_VER
			constexpr int N = 15;
#else
      const int N = 3;
#endif
			int64_t tsc[N + 1];
			int64_t ns[N + 1];

			tsc[0] = rdtsc();
			for (int i = 1; i <= N; i++) {
				ns[i] = rdsysns();
				tsc[i] = rdtsc();
			}

#ifdef _MSC_VER
			int j = 1;
			for (int i = 2; i <= N; i++) {
				if (ns[i] == ns[i - 1]) continue;
				tsc[j - 1] = tsc[i - 1];
				ns[j++] = ns[i];
			}
			j--;
#else
      int j = N + 1;
#endif

			int best = 1;
			for (int i = 2; i < j; i++) {
				if (tsc[i] - tsc[i - 1] < tsc[best] - tsc[best - 1]) best = i;
			}
			tsc_out = (tsc[best] + tsc[best - 1]) >> 1;
			ns_out = ns[best];
		}

		void saveParam(int64_t base_tsc, int64_t sys_ns, int64_t base_ns_err, double new_ns_per_tsc) {
			base_ns_err_ = base_ns_err;
			next_calibrate_tsc_ = base_tsc + static_cast<int64_t>(static_cast<double>(calibate_interval_ns_ - 1000) / new_ns_per_tsc);
			uint32_t seq = param_seq_.load(std::memory_order_relaxed);
			param_seq_.store(++seq, std::memory_order_release);
			std::atomic_signal_fence(std::memory_order_acq_rel);
			base_tsc_ = base_tsc;
			base_ns_ = sys_ns + base_ns_err;
			ns_per_tsc_ = new_ns_per_tsc;
			std::atomic_signal_fence(std::memory_order_acq_rel);
			param_seq_.store(++seq, std::memory_order_release);
		}

		alignas(64) std::atomic<uint32_t> param_seq_ = 0;
		double ns_per_tsc_;
		int64_t base_tsc_;
		int64_t base_ns_;
		int64_t calibate_interval_ns_;
		int64_t base_ns_err_;
		int64_t next_calibrate_tsc_;
	};

	struct ThreadBuffer {
		SPSCVarQueueOPT varq;
		bool shouldDeallocate = false;
		uint32_t tid;
		std::u8string_view name = default_thread_name;
	};

	template<size_t SIZE = 1000, typename Allocator = std::allocator<char8_t>>
	class memory_buffer : public internal::buffer {
	public:
		using value_type = char8_t;
		using const_reference = const char8_t&;

		explicit memory_buffer(const Allocator& alloc = Allocator()): buffer(grow), alloc_(alloc) {
			this->set(store_, SIZE);
		}

		memory_buffer(memory_buffer&& other) noexcept : buffer(grow) {
			this->move(other);
		}

		~memory_buffer() { deallocate(); }

		// Moves the content of the other `memory_buffer` object to this one.
		memory_buffer& operator=(memory_buffer&& other) noexcept {
			assert(this != &other);
			deallocate();
			this->move(other);
			return *this;
		}

		// Returns a copy of the allocator associated with this buffer.
		Allocator get_allocator() const { return alloc_; }

		/// Resizes the buffer to contain `count` elements. If T is a POD type new
		/// elements may not be initialized.
		void resize(size_t count) { this->try_resize(count); }

		/// Increases the buffer capacity to `new_capacity`.
		void reserve(size_t new_capacity) { this->try_reserve(new_capacity); }

		using buffer::append;

		template<typename ContiguousRange>
		void append(const ContiguousRange& range) {
			this->append(range.data(), range.data() + range.size());
		}

	private:
		// Move data from other to this buffer.
		void move(memory_buffer& other) {
			alloc_ = std::move(other.alloc_);
			char8_t* data = other.data();
			const size_t size = other.size();
			const size_t capacity = other.capacity();
			if (data == other.store_) {
				this->set(store_, capacity);
				std::copy(other.store_, other.store_ + size, store_);
			} else {
				this->set(data, capacity);
				// Set pointer to the inline array so that delete is not called
				// when deallocating.
				other.set(other.store_, 0);
				other.clear();
			}
			this->resize(size);
		}

		char8_t store_[SIZE];

		// Don't inherit from Allocator to avoid generating type_info for it.
		[[no_unique_address]] Allocator alloc_;

		// Deallocate memory allocated by the buffer.
		void deallocate() {
			char8_t* data = this->data();
			if (data != store_) alloc_.deallocate(data, this->capacity());
		}

		static void grow(buffer* buf, size_t size) {
			// assert(size <= 5000);
			auto& self = *reinterpret_cast<memory_buffer*>(buf);
			const size_t max_size = std::allocator_traits<Allocator>::max_size(self.alloc_);
			size_t old_capacity = buf->capacity();
			size_t new_capacity = old_capacity + old_capacity / 2;
			if (size > new_capacity)
				new_capacity = size;
			else if (new_capacity > max_size)
				new_capacity = std::max(size, max_size);
			char8_t* old_data = buf->data();
			char8_t* new_data = self.alloc_.allocate(new_capacity);
			// Suppress a bogus -Wstringop-overflow in gcc 13.1 (#3481).
			[[assume(buf.size() <= new_capacity)]];
			// The following code doesn't throw, so the raw pointer above doesn't leak.
			memcpy(new_data, old_data, buf->size());
			self.set(new_data, new_capacity);
			// deallocate must not throw according to the standard, but even if it does,
			// the buffer already uses the new storage and will deallocate it in
			// destructor.
			if (old_data != self.store_) self.alloc_.deallocate(old_data, old_capacity);
		}
	};

	struct HeapNode {
		HeapNode(ThreadBuffer* buffer): tb(buffer) {}

		ThreadBuffer* tb;
		const MsgHeader* header = nullptr;
	};

	struct ThreadBufferDestroyer {
		constexpr ThreadBufferDestroyer() = default;

		~ThreadBufferDestroyer();
	};
}

namespace u8lib::log
{

	struct Logger {
#pragma region pattern

		static constexpr auto kPatternCount = std::size(kPatternParam);

		// 0: log level
		// 1: file location
		// 2: thread id
		// 3: thread name
		// 4: function name
		// 5: timestamp
		// 6: message
		int8_t arg_n;
		std::array<int8_t, kPatternCount> reorderIdx;
		std::u8string pattern;

		void setHeaderPattern(std::u8string_view pattern_) {
			reorderIdx.fill(-1);
			pattern.clear();
			pattern.reserve(pattern_.size());
			auto begin = pattern_.begin();
			const auto end = pattern_.end();

			arg_n = 0;
			while (true) {
				auto pos = std::find(begin, end, '{');
				if (pos == end) {
					pattern.append(begin, end);
					return;
				}
				++pos;
				pattern.append(begin, pos);
				const char8_t ch = *pos;
				begin = ++pos;

				[&] {
					for (auto i = 0; i < kPatternCount; i++) {
						if (ch == kPatternParam[i]) {
							reorderIdx[i] = arg_n++;
							return;
						}
					}
					internal::report_error(u8"invalid format string");
				}();
			}
		}

#pragma endregion pattern

#pragma region logFile

		bool manageFp = false;
		FILE* outputFp = nullptr;
		size_t fpos = 0; // file position of membuf, used only when manageFp == true

		void setLogFile(const char8_t* filename, bool truncate = false) {
			FILE* newFp = fopen(reinterpret_cast<const char*>(filename), truncate ? "w" : "a");
			if (!newFp) {
				std::u8string err;
				format_to(std::back_inserter(err), u8"unable to open file: {}: {}", filename, strerror(errno));
				internal::report_error(err.c_str());
			}
			setbuf(newFp, nullptr);
			fpos = ftell(newFp);

			closeLogFile();
			outputFp = newFp;
			manageFp = true;
		}

		void setLogFile(FILE* fp, bool manageFp_ = false) {
			closeLogFile();
			if (manageFp_) {
				setbuf(fp, nullptr);
				fpos = ftell(fp);
			} else {
				fpos = 0;
			}

			outputFp = fp;
			manageFp = manageFp_;
		}

		void closeLogFile() {
			if (membuf.size()) flushLogFile();
			if (manageFp) fclose(outputFp);
			outputFp = nullptr;
			manageFp = false;
		}

#pragma endregion logFile

#pragma region flush

		LogLevel flushLogLevel = LogLevel::off;
		uint32_t flushBufSize = 8 * 1024;
		int64_t flushDelay = 3000000000;
		int64_t nextFlushTime = std::numeric_limits<int64_t>::max();

		void setFlushDelay(int64_t ns) {
			flushDelay = ns;
		}

		void flushOn(LogLevel flushLogLevel_) {
			flushLogLevel = flushLogLevel_;
		}

		void setFlushBufSize(uint32_t bytes) {
			flushBufSize = bytes;
		}

		void flushLogFile() {
			if (outputFp) {
				fwrite(membuf.data(), 1, membuf.size(), outputFp);
				if (!manageFp) {
					fflush(outputFp);
				} else {
					fpos += membuf.size();
				}
			}
			membuf.clear();
			nextFlushTime = std::numeric_limits<int64_t>::max();
		}

#pragma endregion flush

#pragma region logCB

		LogQFullCBFn logQFullCB = [](void*) {};
		void* logQFullCBArg = nullptr;
		LogCBFn logCB = nullptr;
		LogLevel minCBLogLevel;

		void setLogCB(LogCBFn cb, LogLevel minCBLogLevel_) {
			logCB = cb;
			minCBLogLevel = minCBLogLevel_;
		}

		void setLogQFullCB(LogQFullCBFn cb, void* userData) {
			logQFullCB = cb;
			logQFullCBArg = userData;
		}

#pragma endregion logCB

#pragma region logLevel

		volatile LogLevel currentLogLevel;

		static constexpr std::u8string_view LogLevelNameLUT[] = {
			u8"TRACE",
			u8"DEBUG",
			u8"INFO",
			u8"WARN",
			u8"ERROR",
			u8"FATAL",
			u8"OFF"
		};

		void setLogLevel(LogLevel logLevel) {
			currentLogLevel = logLevel;
		}

		LogLevel getLogLevel() const {
			return currentLogLevel;
		}

		bool checkLogLevel(LogLevel logLevel) const {
			return logLevel >= currentLogLevel;
		}

#pragma endregion logLevel

#pragma region pollingThread

		volatile bool threadRunning = false;
		std::thread thr;

		void startPollingThread(int64_t pollInterval) {
			stopPollingThread();
			threadRunning = true;
			thr = std::thread([pollInterval, this]() {
				while (threadRunning) {
					const int64_t before = tscns.rdns();
					poll(false);
					const int64_t delay = tscns.rdns() - before;
					if (delay < pollInterval) {
						std::this_thread::sleep_for(std::chrono::nanoseconds(pollInterval - delay));
					}
				}
				poll(true);
			});
		}

		void stopPollingThread() {
			if (!threadRunning) return;
			threadRunning = false;
			if (thr.joinable()) thr.join();
		}

#pragma endregion pollingThread

#pragma region timestamp

		struct X {
			Str<4> year;
			char8_t dash1 = '-';
			Str<2> month;
			char8_t dash2 = '-';
			Str<2> day;
			char8_t space = ' ';
			Str<2> hour;
			char8_t colon1 = ':';
			Str<2> minute;
			char8_t colon2 = ':';
			Str<2> second;
		};
		Str<4> year;
		char8_t dash1 = '-';
		Str<2> month;
		char8_t dash2 = '-';
		Str<2> day;
		char8_t space = ' ';
		Str<2> hour;
		char8_t colon1 = ':';
		Str<2> minute;
		char8_t colon2 = ':';
		Str<2> second;
		char8_t dot1 = '.';
		Str<9> nanosecond;

		int64_t midnightNs;
		TSCNS tscns;

		void resetDate() {
			const time_t rawtime = tscns.rdns() / 1000000000;
			tm* timeinfo = localtime(&rawtime);
			timeinfo->tm_sec = timeinfo->tm_min = timeinfo->tm_hour = 0;
			midnightNs = mktime(timeinfo) * 1000000000;
			year.fromi(1900 + timeinfo->tm_year);
			month.fromi(1 + timeinfo->tm_mon);
			day.fromi(timeinfo->tm_mday);
		}

		static int length_for_precision(TimestampPrecision precision) {
			using enum TimestampPrecision;
			switch (precision) {
				case none:
					return 0;
				case ms:
					return 4;
				case us:
					return 7;
				case ns:
					return 10;
				default:
					std::unreachable();
			}
		}

		void setTimestampPrecision(TimestampPrecision precision) {
			setArg<5>(std::u8string_view(year.s, 19 + length_for_precision(precision)));
		}

#pragma endregion timestamp

#pragma region threadBuffer

		static thread_local ThreadBufferDestroyer sbc;
		static thread_local ThreadBuffer* threadBuffer;

		std::vector<ThreadBuffer*> threadBuffers;
		std::vector<HeapNode> bgThreadBuffers;
		std::mutex bufferMutex;
		memory_buffer<> membuf;

		void preallocate() {
			if (threadBuffer) return;
			threadBuffer = new ThreadBuffer();
#ifdef _WIN32
			threadBuffer->tid = static_cast<uint32_t>(::GetCurrentThreadId());
#else
			threadBuffer->tid = static_cast<uint32_t>(::syscall(SYS_gettid));
#endif

			std::lock_guard guard(bufferMutex);
			threadBuffers.push_back(threadBuffer);
		}

		void setThreadName(std::u8string_view name) {
			preallocate();
			threadBuffer->name = name;
		}

		MsgHeader* allocMsg(uint32_t size, bool q_full_cb) {
			if (threadBuffer == nullptr) preallocate();
			const auto ret = threadBuffer->varq.alloc(size);
			if (!ret && q_full_cb) logQFullCB(logQFullCBArg);
			return ret;
		}

#pragma endregion threadBuffer

		// sort by LogLevel
		void adjustHeap(size_t i) {
			while (true) {
				size_t min_i = i;
				for (size_t ch = i * 2 + 1, end = std::min(ch + 2, bgThreadBuffers.size()); ch < end; ch++) {
					auto h_ch = bgThreadBuffers[ch].header;
					auto h_min = bgThreadBuffers[min_i].header;
					if (h_ch && (!h_min || *(int64_t*) (h_ch + 1) < *(int64_t*) (h_min + 1))) min_i = ch;
				}
				if (min_i == i) break;
				std::swap(bgThreadBuffers[i], bgThreadBuffers[min_i]);
				i = min_i;
			}
		}

		format_arg args[kPatternCount];

		template<size_t I, typename T>
		void setArg(const T& arg) {
			args[reorderIdx[I]] = arg;
		}

		template<size_t I, typename T>
		void setArgVal(const T& arg) {
			args[reorderIdx[I]].value_ = arg;
		}

		static Logger& instance() {
			static Logger logger;
			return logger;
		}

		Logger() {
			tscns.init();
			currentLogLevel = LogLevel::info;

			resetDate();
			setLogFile(stdout);
			setHeaderPattern(default_pattern);
			setTimestampPrecision(default_precision);

			threadBuffers.reserve(8);
			bgThreadBuffers.reserve(8);
			memset(membuf.data(), 0, membuf.capacity());
		}

		~Logger() {
			stopPollingThread();
			poll(true);
			closeLogFile();
		}

		void vlog(const char8_t* location, const char8_t* function, LogLevel level, std::u8string_view fmt, format_args args) {
			uint32_t fmt_size = vformatted_size(fmt, args);
			uint32_t alloc_size = 8 + 8 + 8 + fmt_size;
			bool q_full_cb = true;
			do {
				if (auto header = allocMsg(alloc_size, q_full_cb)) {
					header->logId = static_cast<uint32_t>(level);
					auto* out = (char8_t*) (header + 1);
					*(int64_t*) out = TSCNS::rdtsc();
					out += 8;
					*(const char8_t**) out = location;
					out += 8;
					*(const char8_t**) out = function;
					out += 8;
					vformat_to(out, fmt, args);
					header->push(alloc_size);
					break;
				}
				q_full_cb = false;
			} while (U8LOG_BLOCK);
		}

		void poll(bool forceFlush) {
			tscns.calibrate();
			int64_t tsc = TSCNS::rdtsc();
			if (!threadBuffers.empty()) {
				std::lock_guard lock(bufferMutex);
				for (auto tb: threadBuffers) {
					bgThreadBuffers.emplace_back(tb);
				}
				threadBuffers.clear();
			}

			for (size_t i = 0; i < bgThreadBuffers.size(); i++) {
				auto& node = bgThreadBuffers[i];
				if (node.header) continue;
				node.header = node.tb->varq.front();
				if (!node.header && node.tb->shouldDeallocate) {
					delete node.tb;
					node = bgThreadBuffers.back();
					bgThreadBuffers.pop_back();
					i--;
				}
			}

			if (bgThreadBuffers.empty()) return;

			// build heap
			for (int i = (int) bgThreadBuffers.size() / 2; i >= 0; i--) {
				adjustHeap(i);
			}

			while (true) {
				auto h = bgThreadBuffers[0].header;
				if (!h || *(int64_t*) (h + 1) >= tsc) break;
				auto tb = bgThreadBuffers[0].tb;
				handleLog(tb->tid, tb->name, h);
				tb->varq.pop();
				bgThreadBuffers[0].header = tb->varq.front();
				adjustHeap(0);
			}

			if (membuf.size() == 0) return;
			if (!manageFp || forceFlush) {
				flushLogFile();
				return;
			}
			const int64_t now = tscns.tsc2ns(tsc);
			if (now > nextFlushTime) {
				flushLogFile();
			} else if (nextFlushTime == std::numeric_limits<int64_t>::max()) {
				nextFlushTime = now + flushDelay;
			}
		}

		void handleLog(uint32_t tid, std::u8string_view threadName, const MsgHeader* header) {
			auto lod_level = header->logId;
			const char8_t* data = (const char8_t*) (header + 1);
			const char8_t* end = (const char8_t*) header + header->size;
			int64_t tsc = *(int64_t*) data;
			data += 8;
			std::u8string_view location = *(const char8_t**) data;
			data += 8;
			std::u8string_view function = *(const char8_t**) data;
			data += 8;
			std::u8string_view message{data, static_cast<size_t>(end - data)};

			int64_t ts = tscns.tsc2ns(tsc);
			// the date could go back when polling different threads
			uint64_t t = (ts > midnightNs) ? (ts - midnightNs) : 0;
			nanosecond.fromi(t % 1000000000);
			t /= 1000000000;
			second.fromi(t % 60);
			t /= 60;
			minute.fromi(t % 60);
			t /= 60;
			uint32_t h = t; // hour
			if (h > 23) {
				h %= 24;
				resetDate();
			}
			hour.fromi(h);

			setArg<0>(LogLevelNameLUT[lod_level]);
			setArg<1>(location);
			setArg<2>(tid);
			setArg<3>(threadName);
			setArg<4>(function);
			setArg<6>(message);

			size_t headerPos = membuf.size();
			vformat_to(membuf, pattern, format_args(args, arg_n));

			if (logCB && lod_level >= static_cast<uint32_t>(minCBLogLevel)) {
				logCB(
					ts,
					static_cast<LogLevel>(lod_level),
					location,
					tid,
					threadName,
					std::u8string_view(membuf.data() + headerPos, membuf.size() - headerPos)
				);
			}

			membuf.push_back('\n');
			if (membuf.size() >= flushBufSize || lod_level >= static_cast<uint32_t>(flushLogLevel)) {
				flushLogFile();
			}
		}
	};

	ThreadBufferDestroyer::~ThreadBufferDestroyer() {
		if (Logger::instance().threadBuffer != nullptr) {
			Logger::instance().threadBuffer->shouldDeallocate = true;
			Logger::instance().threadBuffer = nullptr;
		}
	}

	thread_local ThreadBufferDestroyer Logger::sbc;
	thread_local ThreadBuffer* Logger::threadBuffer;
}

namespace u8lib::log
{
	void preallocate() {
		Logger::instance().preallocate();
	}

	void set_log_file(const char8_t* filename, bool truncate) {
		Logger::instance().setLogFile(filename, truncate);
	}

	void set_log_file(FILE* fp, bool manageFp) {
		Logger::instance().setLogFile(fp, manageFp);
	}

	void close_log_file() {
		Logger::instance().closeLogFile();
	}

	void set_log_level(LogLevel logLevel) {
		Logger::instance().setLogLevel(logLevel);
	}

	LogLevel get_log_level() {
		return Logger::instance().getLogLevel();
	}

	bool check_log_level(LogLevel logLevel) {
		return Logger::instance().checkLogLevel(logLevel);
	}

	void set_flush_delay(int64_t ns) {
		Logger::instance().setFlushDelay(ns);
	}

	void set_flush_log_level(LogLevel flushLogLevel) {
		Logger::instance().flushOn(flushLogLevel);
	}

	void set_flush_buffer_size(uint32_t bytes) {
		Logger::instance().setFlushBufSize(bytes);
	}

	void set_log_callback(LogCBFn cb, LogLevel minCBLogLevel) {
		Logger::instance().setLogCB(cb, minCBLogLevel);
	}

	void set_log_queue_full_callback(LogQFullCBFn cb, void* userData) {
		Logger::instance().setLogQFullCB(cb, userData);
	}

	void set_header_pattern(std::u8string_view pattern) {
		Logger::instance().setHeaderPattern(pattern);
	}

	void set_thread_name(std::u8string_view name) {
		Logger::instance().setThreadName(name);
	}

	void set_timestamp_precision(TimestampPrecision precision) {
		Logger::instance().setTimestampPrecision(precision);
	}

	void vlog(const char8_t* location, const char8_t* function, LogLevel level, std::u8string_view fmt, format_args args) {
		Logger::instance().vlog(location, function, level, fmt, args);
	}

	void poll(bool forceFlush) {
		Logger::instance().poll(forceFlush);
	}

	void start_polling_thread(int64_t pollInterval) {
		Logger::instance().startPollingThread(pollInterval);
	}

	void stop_polling_thread() {
		Logger::instance().stopPollingThread();
	}
}
