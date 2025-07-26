#pragma once

#include <cstdint>
#include <memory>
#include <array>
#include <span>

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
	concept addressable_concept = (
		requires(T) {
			{ T::address_range() } -> std::convertible_to<address_range_t<T>>;
		} 
		|| requires(T) {
			T::address_range().begin(); T::address_range().end();
			{ *T::address_range().begin() } -> std::convertible_to<address_range_t<T>>;
		}
	);

	template<std::size_t AddressStart, std::size_t AddressEnd>
	struct simple_memory {
		consteval static auto address_range() {
			return address_range_t<simple_memory>{ AddressStart, AddressEnd, &simple_memory::read, &simple_memory::write };
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

		template<typename Handler, std::size_t... Indices>
    	constexpr static void fill_table_for_handler(auto& table, std::index_sequence<Indices...>) {
			constexpr auto idx = detail::index_of<Handler, Handlers...>();
			
			([&table] {
				constexpr static auto info = Handler::address_range()[Indices];
				std::fill(table.begin() + info.start, table.begin() + info.end + 1, jump_pair{
					[](storage_t& handlers, uint16_t addr) -> uint8_t {
						return (std::get<idx>(handlers)->*(info.read_fn))(addr);
					},
					[](storage_t& handlers, uint16_t addr, uint8_t data) -> void {
						(std::get<idx>(handlers)->*(info.write_fn))(addr, data);
					}
				});
			}(), ...);
    	}

		constexpr static std::array<jump_pair, NumAddresses> jump_table = [] {
			std::array<jump_pair, NumAddresses> table{};

			table.fill({nullptr, nullptr});

			([](auto& table) {

				constexpr auto idx = detail::index_of<Handlers, Handlers...>();
				
				constexpr static auto addr_range = Handlers::address_range();

				if constexpr (std::is_same_v<decltype(addr_range), const address_range_t<Handlers>>) {
					std::fill(table.begin() + addr_range.start, table.begin() + addr_range.end + 1, jump_pair{
						[](storage_t& handlers, uint16_t addr) {
							return (std::get<idx>(handlers)->*(addr_range.read_fn))(addr);
						},
						[](storage_t& handlers, uint16_t addr, uint8_t data) {
							(std::get<idx>(handlers)->*(addr_range.write_fn))(addr, data);
						}
					});
				}
				else {
                	fill_table_for_handler<Handlers>(table, std::make_index_sequence<addr_range.size()>{});
				}

			}(table), ...);

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