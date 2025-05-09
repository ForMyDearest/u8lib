#include "pch.hpp"

#include <u8lib/guid.hpp>

#if defined(_WIN32)
#	include <objbase.h>
#elif defined(__linux__) || defined(__unix__)
#	include <uuid/uuid.h>
#elif defined(__APPLE__)
#	include <CoreFoundation/CFUUID.h>
#else

#include <memory>
#include <random>

namespace u8lib
{
	template<typename UniformRandomNumberGenerator>
	class basic_guid_random_generator {
	public:
		using engine_type = UniformRandomNumberGenerator;

		explicit basic_guid_random_generator(engine_type& gen) : generator(&gen, [](auto) {}) {}
		explicit basic_guid_random_generator(engine_type* gen) : generator(gen, [](auto) {}) {}

		[[nodiscard]] guid_t operator()() {
			alignas(uint32_t) uint8_t bytes[16];
			for (int i = 0; i < 16; i += 4) {
				*reinterpret_cast<uint32_t*>(bytes + i) = distribution(*generator);
			}

			// variant must be 10xxxxxx
			bytes[8] &= 0xBF;
			bytes[8] |= 0x80;

			// version must be 0100xxxx
			bytes[6] &= 0x4F;
			bytes[6] |= 0x40;

			return guid_t{std::begin(bytes), std::end(bytes)};
		}

	private:
		std::uniform_int_distribution<uint32_t> distribution;
		std::shared_ptr<UniformRandomNumberGenerator> generator;
	};

	using guid_random_generator = basic_guid_random_generator<std::mt19937>;
}

#endif

namespace u8lib
{
	std::optional<guid_t> guid_t::create() noexcept {
#if defined(_WIN32)

		GUID newId;
		HRESULT hr = CoCreateGuid(&newId);

		if (FAILED(hr)) {
			return {};
		}

		std::array<uint8_t, 16> bytes =
		{{
			static_cast<unsigned char>((newId.Data1 >> 24) & 0xFF),
			static_cast<unsigned char>((newId.Data1 >> 16) & 0xFF),
			static_cast<unsigned char>((newId.Data1 >> 8) & 0xFF),
			static_cast<unsigned char>((newId.Data1) & 0xFF),

			static_cast<unsigned char>((newId.Data2 >> 8) & 0xFF),
			static_cast<unsigned char>((newId.Data2) & 0xFF),

			static_cast<unsigned char>((newId.Data3 >> 8) & 0xFF),
			static_cast<unsigned char>((newId.Data3) & 0xFF),

			newId.Data4[0],
			newId.Data4[1],
			newId.Data4[2],
			newId.Data4[3],
			newId.Data4[4],
			newId.Data4[5],
			newId.Data4[6],
			newId.Data4[7]
		}};

		return guid_t{std::begin(bytes), std::end(bytes)};

#elif defined(__APPLE__)

		auto newId = CFUUIDCreate(NULL);
		auto bytes = CFUUIDGetUUIDBytes(newId);
		CFRelease(newId);

		std::array<uint8_t, 16> arrbytes =
		{{
			bytes.byte0,
			bytes.byte1,
			bytes.byte2,
			bytes.byte3,
			bytes.byte4,
			bytes.byte5,
			bytes.byte6,
			bytes.byte7,
			bytes.byte8,
			bytes.byte9,
			bytes.byte10,
			bytes.byte11,
			bytes.byte12,
			bytes.byte13,
			bytes.byte14,
			bytes.byte15
		}};
		return guid_t{std::begin(arrbytes), std::end(arrbytes)};

#elif defined(__linux__) || defined(__unix__)

		guid_t id;
		uuid_generate(id);

		std::array<uint8_t, 16> bytes =
		{{
			id[0],
			id[1],
			id[2],
			id[3],
			id[4],
			id[5],
			id[6],
			id[7],
			id[8],
			id[9],
			id[10],
			id[11],
			id[12],
			id[13],
			id[14],
			id[15]
		}};

		return guid_t{std::begin(bytes), std::end(bytes)};

#else

		static auto gen = [] {
			std::random_device rd;
			auto seed_data = std::array<int, std::mt19937::state_size>{};
			std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
			std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
			std::mt19937 generator(seq);
			return guid_random_generator{generator};
		}();

		return gen();
#endif
	}

	u8string guid_t::to_string() const noexcept {
		static constexpr char8_t guid_encoder[17] = u8"0123456789abcdef";
		static constexpr char8_t empty_guid[37] = u8"00000000-0000-0000-0000-000000000000";

		u8string uustr = empty_guid;

		for (size_t i = 0, index = 0; i < 36; ++i) {
			if (i == 8 || i == 13 || i == 18 || i == 23) {
				continue;
			}
			uustr[i] = guid_encoder[data[index] >> 4 & 0x0f];
			uustr[++i] = guid_encoder[data[index] & 0x0f];
			index++;
		}

		return uustr;
	}
}
