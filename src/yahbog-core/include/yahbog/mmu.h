#pragma once

#include <cstdint>
#include <memory>
#include <array>
#include <span>

namespace yahbog {
	struct addressable {
		virtual uint8_t read(uint16_t addr) = 0;
		virtual void write(uint16_t addr, uint8_t data) = 0;

		virtual ~addressable() = default;
	};

	struct address_range_t {
		uint16_t start;
		uint16_t end;
	};

	template<typename T>
	concept addressable_concept = std::is_base_of_v<addressable, T> && (
		requires(T) {
			{ T::address_range } -> std::convertible_to<address_range_t>;
		} 
		|| requires(T) {
			T::address_range.begin(); T::address_range.end();
		}
	);

	template<std::size_t AddressStart, std::size_t AddressEnd>
	struct simple_memory : addressable {
		constexpr static address_range_t address_range = { AddressStart, AddressEnd };

		std::array<uint8_t, AddressEnd - AddressStart + 1> memory{};

		uint8_t read(uint16_t addr) {
			return memory[addr - AddressStart];
		}

		void write(uint16_t addr, uint8_t data) {
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
	class memory_dispatcher : public addressable {

		using storage_t = std::array<addressable*, sizeof...(Handlers)>;
		storage_t m_handlers{};

		using jump_fn = addressable * (*)(const storage_t&, uint16_t);

		constexpr static std::array<jump_fn, sizeof...(Handlers)> jump_handlers = {
			[](const storage_t& handlers, uint16_t addr) {
				return handlers[detail::index_of<Handlers, Handlers...>()];
			}...
		};

		constexpr static std::array<jump_fn, NumAddresses> jump_table = [] {
			std::array<jump_fn, NumAddresses> table{};

			table.fill([](const storage_t& handlers, uint16_t addr) -> addressable* { return nullptr; });

			([](auto& table) {

				constexpr auto idx = detail::index_of<Handlers, Handlers...>();
				
				if constexpr (std::is_same_v<decltype(Handlers::address_range), const address_range_t>) {
					const auto range = Handlers::address_range;
					std::fill(table.begin() + range.start, table.begin() + range.end + 1, jump_handlers[idx]);
				}
				else {
					for (const auto& range : Handlers::address_range) {
						std::fill(table.begin() + range.start, table.begin() + range.end + 1, jump_handlers[idx]);
					}
				}

			}(table), ...);

			return table;
		}();

	public:
		bool all_valid() const {
			return (m_handlers[detail::index_of<Handlers, Handlers...>].get() && ...);
		}

		template<typename T>
		void set_handler(T* handler) {
			constexpr auto idx = detail::index_of<T, Handlers...>();
			static_assert(idx < sizeof...(Handlers), "Handler not found");

			m_handlers[idx] = handler;
		}

		uint8_t read(uint16_t addr) override {
			auto handler = jump_table[addr](m_handlers, addr);
			if (handler) {
				return handler->read(addr);
			}
			else throw std::out_of_range(std::format("No handler found for address: {:04X}", addr));
		}

		void write(uint16_t addr, uint8_t data) override {
			auto handler = jump_table[addr](m_handlers, addr);
			if (handler) {
				handler->write(addr, data);
			}
			else throw std::out_of_range(std::format("No handler found for address: {:04X}", addr));
		}
	};

}