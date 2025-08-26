#pragma once

#include <cstdint>
#include <memory>
#include <array>
#include <span>
#include <variant>

#include <format>
#include <stdexcept>

#include <yahbog/registers.h>
#include <yahbog/common.h>
namespace yahbog {

	namespace mem_helpers {
		namespace detail {

			template<typename T>
			struct is_io_register_helper {
				static constexpr bool value = false;
			};

			template<typename T>
			struct is_io_register_helper<io_register<T>> {
				static constexpr bool value = true;
			};
			
			template<typename T>
			concept is_io_register = is_io_register_helper<T>::value;

		}

		template<traits::member_pointer auto MemberPtr>
		requires std::is_same_v<traits::member_type_of<decltype(MemberPtr)>, uint8_t>
		constexpr uint8_t read_byte(traits::class_type_of<decltype(MemberPtr)>* obj, [[maybe_unused]] uint16_t addr) {
			return obj->*MemberPtr;
		}

		template<traits::member_pointer auto MemberPtr>
		requires std::is_same_v<traits::member_type_of<decltype(MemberPtr)>, uint8_t>
		constexpr void write_byte(traits::class_type_of<decltype(MemberPtr)>* obj, [[maybe_unused]] uint16_t addr, uint8_t data) {
			obj->*MemberPtr = data;
		}

		template<traits::member_pointer auto MemberPtr>
		requires detail::is_io_register<traits::member_type_of<decltype(MemberPtr)>>
		constexpr std::uint8_t read_io_register(traits::class_type_of<decltype(MemberPtr)>* obj, [[maybe_unused]] uint16_t addr) {
			return (obj->*MemberPtr).read();
		}

		template<traits::member_pointer auto MemberPtr>
		requires detail::is_io_register<traits::member_type_of<decltype(MemberPtr)>>
		constexpr void write_io_register(traits::class_type_of<decltype(MemberPtr)>* obj, [[maybe_unused]] uint16_t addr, uint8_t data) {
			(obj->*MemberPtr).write(data);
		}

		template<std::size_t Location, traits::member_pointer auto MemberPtr>
		requires std::is_same_v<traits::member_type_of<decltype(MemberPtr)>, uint8_t>
		consteval auto make_member_accessor() -> address_range_t<traits::class_type_of<decltype(MemberPtr)>> {
			return {
				Location,
				Location,
				&mem_helpers::read_byte<MemberPtr>,
				&mem_helpers::write_byte<MemberPtr>
			};
		}

		template<std::size_t Location, traits::member_pointer auto MemberPtr>
		requires detail::is_io_register<traits::member_type_of<decltype(MemberPtr)>>
		consteval auto make_member_accessor() -> address_range_t<traits::class_type_of<decltype(MemberPtr)>> {
			return {
				Location,
				Location,
				&mem_helpers::read_io_register<MemberPtr>,
				&mem_helpers::write_io_register<MemberPtr>
			};
		}
	}

	template<typename T>
	concept addressable_concept = requires(T) {
		T::address_range().begin(); T::address_range().end();
		{ *T::address_range().begin() } -> std::convertible_to<address_range_t<T>>;
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
		consteval static auto make_jump_pair_for_range() {
			constexpr auto handler_idx = detail::index_of<Handler, Handlers...>();

			using addr_range_t = address_range_t<Handler>;

			return jump_pair{
				[](storage_t& handlers, uint16_t addr) constexpr -> uint8_t {

					constexpr auto range_info = Handler::address_range()[Index];
					if constexpr (std::holds_alternative<typename addr_range_t::read_fn_member_t>(range_info.read_fn)) {
						return (std::get<handler_idx>(handlers)->*(std::get<typename addr_range_t::read_fn_member_t>(range_info.read_fn)))(addr);
					}
					else {
						return std::get<typename addr_range_t::read_fn_ext_t>(range_info.read_fn)(std::get<handler_idx>(handlers), addr);
					}
				},
				[](storage_t& handlers, uint16_t addr, uint8_t data) constexpr -> void {
					constexpr auto range_info = Handler::address_range()[Index];
					if constexpr (std::holds_alternative<typename addr_range_t::write_fn_member_t>(range_info.write_fn)) {
						(std::get<handler_idx>(handlers)->*(std::get<typename addr_range_t::write_fn_member_t>(range_info.write_fn)))(addr, data);
					}
					else {
						std::get<typename addr_range_t::write_fn_ext_t>(range_info.write_fn)(std::get<handler_idx>(handlers), addr, data);
					}
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

		constexpr static std::array<jump_pair, NumAddresses> jump_table = []() consteval {
			std::array<jump_pair, NumAddresses> table{};

			table.fill({
				[](storage_t&, uint16_t) constexpr -> uint8_t { return 0xFF; }, 
				[](storage_t&, uint16_t, uint8_t) constexpr -> void {}
			});

			([](auto& table) constexpr {
				constexpr auto addr_range = Handlers::address_range();
				fill_table_for_handler<Handlers>(table, std::make_index_sequence<addr_range.size()>{});
			}(table), ...);

			return table;
		}();

	public:

		constexpr static auto table_size_bytes = sizeof(jump_table);

		constexpr bool all_valid() const noexcept {
			return ((std::get<detail::index_of<Handlers, Handlers...>>(m_handlers) != nullptr) && ...);
		}

		template<typename T>
		constexpr void set_handler(T* handler) noexcept {
			constexpr auto idx = detail::index_of<T, Handlers...>();
			static_assert(idx < sizeof...(Handlers), "Handler not found");

			std::get<idx>(m_handlers) = handler;
		}

		constexpr uint8_t read(uint16_t addr) noexcept {
			return jump_table[addr].read(m_handlers, addr);
		}

		constexpr void write(uint16_t addr, uint8_t data) noexcept {
			jump_table[addr].write(m_handlers, addr, data);
		}
	};

}