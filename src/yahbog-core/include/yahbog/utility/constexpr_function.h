#pragma once

#include <array>
#include <memory>

namespace yahbog {

    template<typename>
    class constexpr_function;

    template<typename ReturnValue, typename... Args>
    class constexpr_function<ReturnValue(Args...)> {
    public:
        template<typename T>
        constexpr constexpr_function& operator=(T&& f) {
            callable_ptr = std::make_unique<callable_impl<T>>(std::forward<T>(f));
            return *this;
        }

        template<typename T>
        constexpr constexpr_function(T&& f): callable_ptr(std::make_unique<callable_impl<T>>(std::forward<T>(f))) {}

        constexpr constexpr_function() = default;

        constexpr constexpr_function(const constexpr_function& other) = delete;
        constexpr constexpr_function(constexpr_function&& other) noexcept = default;
        constexpr constexpr_function& operator=(const constexpr_function& other) = delete;
        constexpr constexpr_function& operator=(constexpr_function&& other) noexcept = default;

        constexpr ReturnValue operator()(Args... args) const {
            return callable_ptr->operator()(std::forward<Args>(args)...);
        }
    private:
        struct callable {
            constexpr virtual ~callable() = default;
            constexpr virtual ReturnValue operator()(Args... args) = 0;
        };

        template<typename F>
        struct callable_impl : public callable {
            F f;

            constexpr callable_impl(F&& f) : f(std::move(f)) {}

            constexpr ReturnValue operator()(Args... args) override {
                return f(std::forward<Args>(args)...);
            }
        };

        std::unique_ptr<callable> callable_ptr;
    };

    using test_t = constexpr_function<int()>;
    static_assert(test_t{[](){ return 42;}}() == 42);
}