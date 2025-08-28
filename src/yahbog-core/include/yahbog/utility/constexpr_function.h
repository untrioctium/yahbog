#pragma once

#include <memory>

namespace yahbog {

	template<typename>
	class constexpr_function;

	template<typename ReturnValue, typename... Args>
	class constexpr_function<ReturnValue(Args...)> {
	public:
		template<typename T>
		constexpr constexpr_function& operator=(T&& f) noexcept {
			callable_ptr = std::make_unique<callable_impl<T>>(std::forward<T>(f));
			return *this;
		}

		template<typename T>
		constexpr constexpr_function(T&& f) noexcept: callable_ptr(std::make_unique<callable_impl<T>>(std::forward<T>(f))) {}

		constexpr constexpr_function() noexcept = default;

		constexpr constexpr_function(const constexpr_function& other) noexcept: callable_ptr(other.callable_ptr->clone()) {}
		constexpr constexpr_function(constexpr_function&& other) noexcept = default;
		constexpr constexpr_function& operator=(const constexpr_function& other) noexcept {
			callable_ptr = other.callable_ptr->clone();
			return *this;
		}
		constexpr constexpr_function& operator=(constexpr_function&& other) noexcept = default;

		constexpr ReturnValue operator()(Args... args) const noexcept {
			return callable_ptr->call(std::forward<Args>(args)...);
		}
	private:
		struct callable {
			constexpr virtual ~callable() = default;
			constexpr virtual ReturnValue call(Args... args) const noexcept = 0;
			constexpr virtual std::unique_ptr<callable> clone() const noexcept = 0;
		};

		template<typename F>
			requires std::is_invocable_r_v<ReturnValue, F, Args...>
		struct callable_impl : public callable {
			F f;

			constexpr ~callable_impl() noexcept = default;
			constexpr callable_impl(F&& f) noexcept : f(std::forward<F>(f)) {}
			constexpr callable_impl(const F& f) noexcept : f(f) {}

			constexpr ReturnValue call(Args... args) const noexcept override {
				return f(std::forward<Args>(args)...);
			}

			constexpr std::unique_ptr<callable> clone() const noexcept override {
				return std::make_unique<callable_impl>(f);
			}
		};

		std::unique_ptr<callable> callable_ptr;
	};

	template<typename F, typename Context>
	struct function_ref;

	template<typename R, typename... Args, typename Context>
	struct function_ref<R(Args...), Context> {
		R (*fn)(Context*, Args...);
		Context* ctx;

		constexpr function_ref(R (*fn)(Context*, Args...), Context* ctx) : fn(fn), ctx(ctx) {}

		constexpr R operator()(Args... args) const noexcept {
			return fn(ctx, std::forward<Args>(args)...);
		}
	};

	namespace static_test {
		using test_t = constexpr_function<int()>;
		static_assert(test_t{[](){ return 42;}}() == 42, "constexpr_function is not constexpr");
		static_assert([]() {
			int x = 0;
			test_t f = [&x]() {
				return x++;
			};
			return f() == 0 && f() == 1 && f() == 2;
		}(), "capturing constexpr_function is not constexpr");
	}
}