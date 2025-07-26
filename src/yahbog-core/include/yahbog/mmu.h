#pragma once

#include <cstdint>
#include <memory>
#include <array>
#include <span>

#include <format>
#include <stdexcept>

namespace yahbog {

	template<typename T>
	struct address_range_t {
		uint16_t start;
		uint16_t end;

		using read_fn_t = uint8_t (T::*)(uint16_t);
		using write_fn_t = void (T::*)(uint16_t, uint8_t);

		read_fn_t read_fn;
		write_fn_t write_fn;

		consteval address_range_t(uint16_t start, uint16_t end, read_fn_t read_fn, write_fn_t write_fn) : start(start), end(end), read_fn(read_fn), write_fn(write_fn) {}
	};

	template<typename T>
	struct memory_helpers {
		template<auto MemberPtr>
		constexpr uint8_t read_member_byte([[maybe_unused]] uint16_t addr) {
			return static_cast<T*>(this)->*MemberPtr;
		}

		template<auto MemberPtr>
		constexpr void write_member_byte([[maybe_unused]] uint16_t addr, uint8_t value) {
			static_cast<T*>(this)->*MemberPtr = value;
		}
	};

	template<typename T>
	concept addressable_concept = requires(T) {
		T::address_range().begin(); T::address_range().end();
		{ *T::address_range().begin() } -> std::convertible_to<address_range_t<T>>;
	};

	template<std::size_t AddressStart, std::size_t AddressEnd>
	struct simple_memory {
		consteval static auto address_range() {
			return std::array{ address_range_t<simple_memory>{ AddressStart, AddressEnd, &simple_memory::read, &simple_memory::write } };
		}

		std::array<uint8_t, AddressEnd - AddressStart + 1> memory{};

		constexpr uint8_t read(uint16_t addr) {
			return memory[addr - AddressStart];
		}

		constexpr void write(uint16_t addr, uint8_t data) {
			memory[addr - AddressStart] = data;
		}
	};

	namespace detail {
		template<typename T, typename... Ts>
		consteval std::size_t index_of() {
			std::size_t found{}, count{};
			((!found ? (++count, found = std::is_same_v<T, Ts>) : 0), ...);
			return found ? count - 1 : count;
		}

		
	}

	template<std::size_t NumAddresses, addressable_concept... Handlers>
	class memory_dispatcher {

		using storage_t = std::tuple<Handlers*...>;
		storage_t m_handlers{(Handlers*)nullptr...};

		struct jump_pair {
			using read_fn_t = uint8_t (*)(storage_t&, uint16_t);
			using write_fn_t = void (*)(storage_t&, uint16_t, uint8_t);

			read_fn_t read;
			write_fn_t write;
		};

		template<typename Handler, std::size_t Index>
		constexpr static auto make_jump_pair_for_range() {
			constexpr auto info = Handler::address_range()[Index];
			constexpr auto idx = detail::index_of<Handler, Handlers...>();
			return jump_pair{
				[](storage_t& handlers, uint16_t addr) constexpr -> uint8_t {
					constexpr auto range_info = Handler::address_range()[Index];
					return (std::get<idx>(handlers)->*(range_info.read_fn))(addr);
				},
				[](storage_t& handlers, uint16_t addr, uint8_t data) constexpr -> void {
					constexpr auto range_info = Handler::address_range()[Index];
					(std::get<idx>(handlers)->*(range_info.write_fn))(addr, data);
				}
			};
		}

		template<typename Handler, std::size_t... Indices>
    	constexpr static void fill_table_for_handler(auto& table, std::index_sequence<Indices...>) {
			([&table]() constexpr {
				constexpr auto info = Handler::address_range()[Indices];
				constexpr auto jump_pair_value = make_jump_pair_for_range<Handler, Indices>();
				std::fill(table.begin() + info.start, table.begin() + info.end + 1, jump_pair_value);
			}(), ...);
    	}

		constexpr static std::array<jump_pair, NumAddresses> jump_table = [] {
			std::array<jump_pair, NumAddresses> table{};

			table.fill({nullptr, nullptr});

			([]<typename Handler>(auto& table) constexpr {
				constexpr auto addr_range = Handler::address_range();
                fill_table_for_handler<Handler>(table, std::make_index_sequence<addr_range.size()>{});
			}.template operator()<Handlers>(table), ...);

			return table;
		}();

	public:

		constexpr static auto table_size_bytes = sizeof(jump_table);

		constexpr bool all_valid() const {
			return ((std::get<detail::index_of<Handlers, Handlers...>>(m_handlers) != nullptr) && ...);
		}

		template<typename T>
		constexpr void set_handler(T* handler) {
			constexpr auto idx = detail::index_of<T, Handlers...>();
			static_assert(idx < sizeof...(Handlers), "Handler not found");

			std::get<idx>(m_handlers) = handler;
		}

		constexpr uint8_t read(uint16_t addr) {
			auto handler = jump_table[addr].read;
			if (handler) {
				return handler(m_handlers, addr);
			}
			else throw std::out_of_range(std::format("No read handler found for address: {:04X}", addr));
		}

		constexpr void write(uint16_t addr, uint8_t data) {
			auto handler = jump_table[addr].write;
			if (handler) {
				handler(m_handlers, addr, data);
			}
			else throw std::out_of_range(std::format("No write handler found for address: {:04X}", addr));
		}
	};

}