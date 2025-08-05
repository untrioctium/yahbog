#pragma once

namespace yahbog::detail::traits {

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

	template<auto MemberPtr>
	using class_type_of = typename member_helper<decltype(MemberPtr)>::class_type;

	template<auto MemberPtr>
	using member_type_of = typename member_helper<decltype(MemberPtr)>::member_type;

}