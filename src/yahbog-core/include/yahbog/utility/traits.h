#pragma once

#include <type_traits>

namespace yahbog::traits {

	namespace detail {

		template<typename T>
		struct member_helper;

		template<typename C, typename M>
		struct member_helper<M C::*> {
			using class_type = C;
			using member_type = M;
		};

		template<typename C, typename M>
		struct member_helper<M C::* const> {
			using class_type = C;
			using member_type = M;
		};
	}

	template<typename T>
	concept member_pointer = std::is_member_pointer_v<T>;

	template<member_pointer MemberPtrT>
	using class_type_of = typename detail::member_helper<MemberPtrT>::class_type;

	template<member_pointer MemberPtrT>
	using member_type_of = typename detail::member_helper<MemberPtrT>::member_type;

}