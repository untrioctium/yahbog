#pragma once

#include <span>
#include <cstdint>
#include <array>
#include <bit>

#include <nameof.hpp>

#include <yahbog/utility/sha1.h>
#include <yahbog/registers.h>
#include <yahbog/utility/traits.h>

namespace yahbog {

	using crc32_t = std::uint32_t;

	constexpr static std::array<crc32_t, 256> crc32_table = []() {
		std::uint8_t index = 0;

		std::array<crc32_t, 256> table;

		do {
			table[index] = index;
			for(std::size_t i = 0; i < 8; i++) {
				if(table[index] & 1) {
					table[index] = (table[index] >> 1) ^ 0xEDB88320;
				}
				else {
					table[index] >>= 1;
				}
			}
		} while(++index);

		return table;
	}();

	constexpr crc32_t crc32(std::span<const std::uint8_t> data) {
		crc32_t crc = 0xFFFFFFFF;
		for(auto byte : data) {
			crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
		}
		return crc ^ 0xFFFFFFFF;
	}

	namespace typeinfo {

		// byte 0: type id
		// byte 1: container id (if applicable)
		// bytes 2-7: any type-specific data (e.g. array size)
		template<typename T>
		constexpr inline std::size_t id_v = 0;

		template<>
		constexpr inline std::size_t id_v<std::uint8_t> = 0x01;

		template<>
		constexpr inline std::size_t id_v<std::uint16_t> = 0x02;

		template<>
		constexpr inline std::size_t id_v<std::uint32_t> = 0x03;

		template<>
		constexpr inline std::size_t id_v<std::uint64_t> = 0x04;

		template<>
		constexpr inline std::size_t id_v<std::int8_t> = 0x05;

		template<>
		constexpr inline std::size_t id_v<std::int16_t> = 0x06;

		template<>
		constexpr inline std::size_t id_v<std::int32_t> = 0x07;

		template<>
		constexpr inline std::size_t id_v<std::int64_t> = 0x08;

		template<>
		constexpr inline std::size_t id_v<bool> = 0x09;

		template<>
		constexpr inline std::size_t id_v<float> = 0x0A;

		template<>
		constexpr inline std::size_t id_v<double> = 0x0B;

		template<typename T>
		constexpr inline std::size_t id_v<std::vector<T>> = 0x01 << 8 | id_v<T>;

		template<typename T, std::size_t N>
		constexpr inline std::size_t id_v<std::array<T, N>> = N << 16 | 0x02 << 8 | id_v<T>;

		template<typename T>
		constexpr inline std::size_t id_v<io_register<T>> = 0x0C;

		template<>
		constexpr inline std::size_t id_v<registers> = sizeof(registers) << 16 | 0x0D;
	}

	/*static_assert(
		crc32(
			std::array<std::uint8_t, 38>
			{0x74, 0x68, 0x65, 0x79, 0x27, 0x72, 0x65, 
			0x20, 0x74, 0x61, 0x6b, 0x69, 0x6e, 0x67, 
			0x20, 0x74, 0x68, 0x65, 0x20, 0x68, 0x6f, 
			0x62, 0x62, 0x69, 0x74, 0x73, 0x20, 0x74, 
			0x6f, 0x20, 0x69, 0x73, 0x65, 0x6e, 0x67, 
			0x61, 0x72, 0x64}) == 0x35588bdc, "CRC32 test failed");*/

	template<typename T>
	concept trivially_copyable = std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

	template<trivially_copyable T>
	constexpr std::array<std::uint8_t, sizeof(T)> to_bytes(T&& value) {
		return std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(std::forward<T>(value));
	}

	template<trivially_copyable T>
	constexpr T from_bytes(std::span<const std::uint8_t> data) {
		auto intermediate = std::array<std::uint8_t, sizeof(T)>{};
		std::copy(data.begin(), data.end(), intermediate.begin());
		return std::bit_cast<T>(intermediate);
	}

	template<trivially_copyable T>
	constexpr std::span<std::uint8_t> write_to_span(std::span<std::uint8_t> data, const T& value) {
		return write_to_span(data, to_bytes(value));
	}

	constexpr std::span<std::uint8_t> write_to_span(std::span<std::uint8_t> data, const std::vector<std::uint8_t>& value) {
		std::copy(value.begin(), value.end(), data.begin());
		return data.subspan(value.size());
	}

	constexpr std::span<std::uint8_t> write_to_span(std::span<std::uint8_t> data, std::uint8_t value) {
		data[0] = value;
		return data.subspan(1);
	}

	template<typename T>
	constexpr std::span<const std::uint8_t> read_from_span(std::span<const std::uint8_t>& data, T& value) {
		value = from_bytes<T>(data);
		return data.subspan(sizeof(T));
	}

	template<>
	constexpr std::span<const std::uint8_t> read_from_span(std::span<const std::uint8_t>& data, std::uint8_t& value) {
		value = data[0];
		return data.subspan(1);
	}

	template<>
	constexpr std::span<const std::uint8_t> read_from_span(std::span<const std::uint8_t>& data, std::vector<std::uint8_t>& value) {
		std::copy(data.begin(), data.end(), value.begin());
		return data.subspan(value.size());
	}

	template<std::size_t N>
	constexpr std::span<const std::uint8_t> read_from_span(std::span<const std::uint8_t>& data, std::array<std::uint8_t, N>& value) {
		std::copy(data.begin(), data.end(), value.begin());
		return data.subspan(N);
	}

	template<trivially_copyable T>
	constexpr std::size_t size_of(const T& value) {
		return sizeof(T);
	}

	constexpr std::size_t size_of(const std::vector<std::uint8_t>& value) {
		return value.size();
	}

	namespace detail {
		template<typename T> struct remove_rvalue_reference {
			using type = T;
		};

		template<typename T> struct remove_rvalue_reference<T&&> {
			using type = T;
		};

		template<typename T>
		using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;
	}

	template<typename Derived>
	struct serializable {

		constexpr static void add_version_signature(sha1& hasher) {
			constexpr auto members = Derived::serializable_members();
			constexpr auto n_members = std::tuple_size_v<decltype(members)>;

			[&members, &hasher]<std::size_t... I>(std::index_sequence<I...>) {
				([&members, &hasher](){
					constexpr auto ptr = std::get<I>(members);
					using base_type = detail::traits::member_type_of<ptr>;
					static_assert(typeinfo::id_v<base_type> != 0, "Unknown serialized type");

					hasher.process_bytes(nameof::nameof_member<ptr>());
					hasher.process_bytes(typeinfo::id_v<base_type>);
				}(), ...);
			}(std::make_index_sequence<n_members>{});
			
		}

		constexpr std::size_t serialized_size() const {
			auto self = static_cast<const Derived*>(this);
			// members is a tuple of member pointers
			constexpr auto members = Derived::serializable_members();
			constexpr auto n_members = std::tuple_size_v<decltype(members)>;

			return [&members, &self]<std::size_t... I>(std::index_sequence<I...>) {
				return (size_of(self->*std::get<I>(members)) + ...);
			}(std::make_index_sequence<n_members>{});
		}

		constexpr std::span<std::uint8_t> serialize(std::span<std::uint8_t> data) const {
			auto self = static_cast<const Derived*>(this);
			constexpr auto members = Derived::serializable_members();
			constexpr auto n_members = std::tuple_size_v<decltype(members)>;

			[&members, &self, &data]<std::size_t... I>(std::index_sequence<I...>) {
				([&data, &self, &members]{data = write_to_span(data, self->*std::get<I>(members));}(), ...);
			}(std::make_index_sequence<n_members>{});

			return data;
		}

		constexpr std::span<const std::uint8_t> deserialize(std::span<const std::uint8_t> data) {
			auto self = static_cast<Derived*>(this);
			constexpr auto members = Derived::serializable_members();
			constexpr auto n_members = std::tuple_size_v<decltype(members)>;

			[&members, &self, &data]<std::size_t... I>(std::index_sequence<I...>) {
				([&data, &self, &members]{data = read_from_span(data, self->*std::get<I>(members));}(), ...);
			}(std::make_index_sequence<n_members>{});

			return data;
		}
	};
}