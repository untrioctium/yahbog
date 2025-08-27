#pragma once

#include <string_view>
#include <array>
#include <bit>
#include <utility>

#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/utility/constexpr_function.h>

// warning: copius macro (ab)use ahead

#define OPCODE_ARGS yahbog::registers& reg, [[maybe_unused]] MemProvider* mem
#define OPCODE_CALL reg, mem

#define DEFINE_OPCODE(name) template<typename MemProvider> struct name
#define DEFINE_TEMPLATE_OPCODE(name, ...) template<typename MemProvider, __VA_ARGS__> struct name
#define DEFINE_STAGE(num) constexpr static next_stage_t<MemProvider> stage##num(OPCODE_ARGS) noexcept
#define BEGIN_STAGE_TABLE constexpr static next_stage_t<MemProvider> stage_table(std::size_t stage) noexcept { switch(stage) {
#define STAGE_ENTRY(num) case num: return &stage##num;
#define END_STAGE_TABLE default: return nullptr; } std::unreachable(); }
#define MOVE_TO_STAGE(num) (reg.mupc = num, &stage##num)

#define DECLARE_STAGE_COUNT(num) constexpr static std::size_t stage_count = num;

#define DECLARE_STAGES_1 DECLARE_STAGE_COUNT(1) BEGIN_STAGE_TABLE STAGE_ENTRY(0) END_STAGE_TABLE
#define DECLARE_STAGES_2 DECLARE_STAGE_COUNT(2) BEGIN_STAGE_TABLE STAGE_ENTRY(0) STAGE_ENTRY(1) END_STAGE_TABLE
#define DECLARE_STAGES_3 DECLARE_STAGE_COUNT(3) BEGIN_STAGE_TABLE STAGE_ENTRY(0) STAGE_ENTRY(1) STAGE_ENTRY(2) END_STAGE_TABLE
#define DECLARE_STAGES_4 DECLARE_STAGE_COUNT(4) BEGIN_STAGE_TABLE STAGE_ENTRY(0) STAGE_ENTRY(1) STAGE_ENTRY(2) STAGE_ENTRY(3) END_STAGE_TABLE
#define DECLARE_STAGES_5 DECLARE_STAGE_COUNT(5) BEGIN_STAGE_TABLE STAGE_ENTRY(0) STAGE_ENTRY(1) STAGE_ENTRY(2) STAGE_ENTRY(3) STAGE_ENTRY(4) END_STAGE_TABLE
#define DECLARE_STAGES_6 DECLARE_STAGE_COUNT(6) BEGIN_STAGE_TABLE STAGE_ENTRY(0) STAGE_ENTRY(1) STAGE_ENTRY(2) STAGE_ENTRY(3) STAGE_ENTRY(4) STAGE_ENTRY(5) END_STAGE_TABLE
#define DECLARE_STAGES_7 DECLARE_STAGE_COUNT(7) BEGIN_STAGE_TABLE STAGE_ENTRY(0) STAGE_ENTRY(1) STAGE_ENTRY(2) STAGE_ENTRY(3) STAGE_ENTRY(4) STAGE_ENTRY(5) STAGE_ENTRY(6) END_STAGE_TABLE
#define DECLARE_STAGES_8 DECLARE_STAGE_COUNT(8) BEGIN_STAGE_TABLE STAGE_ENTRY(0) STAGE_ENTRY(1) STAGE_ENTRY(2) STAGE_ENTRY(3) STAGE_ENTRY(4) STAGE_ENTRY(5) STAGE_ENTRY(6) STAGE_ENTRY(7) END_STAGE_TABLE

#define DEFINE_SIMPLE_OPCODE(name) DEFINE_OPCODE(name) { DECLARE_STAGES_1; DEFINE_STAGE(0) { 
#define DEFINE_SIMPLE_TEMPLATE_OPCODE(name, ...) DEFINE_TEMPLATE_OPCODE(name, __VA_ARGS__) { DECLARE_STAGES_1; DEFINE_STAGE(0) { 
	
#define END_SIMPLE_OPCODE return common::prefetch<MemProvider>(OPCODE_CALL); } };

namespace yahbog {

	constexpr bool half_carries_add(std::uint8_t a, std::uint8_t b) {
		return (((a & 0xF) + (b & 0xF)) & 0x10) == 0x10;
	}

	constexpr bool half_carries_sub(std::uint8_t a, std::uint8_t b) {
		return (((a & 0xF) - (b & 0xF)) & 0x10) == 0x10;
	}

	constexpr bool carries_add(std::uint8_t a, std::uint8_t b) {
		return (std::uint16_t(a) + std::uint16_t(b)) > 0xFF;
	}

	constexpr bool carries_sub(std::uint8_t a, std::uint8_t b) {
		return a < b;
	}

	constexpr bool half_carries_add(std::uint16_t a, std::uint16_t b) {
		return (((a & 0xFFF) + (b & 0xFFF)) & 0x1000) == 0x1000;
	}

	constexpr bool half_carries_sub(std::uint16_t a, std::uint16_t b) {
		return (((a & 0xFFF) - (b & 0xFFF)) & 0x1000) == 0x1000;
	}

	constexpr bool carries_add(std::uint16_t a, std::uint16_t b) {
		return (a + b) > 0xFFFF;
	}

	constexpr bool carries_sub(std::uint16_t a, std::uint16_t b) {
		return a < b;
	}

	enum class jump_condition {
		NZ,
		Z,
		NC,
		C
	};

	template<jump_condition cc, typename MemProvider>
	constexpr bool jump_condition_met(OPCODE_ARGS) {
		if constexpr (cc == jump_condition::NZ) {
			return !reg.FZ();
		}
		else if constexpr (cc == jump_condition::Z) {
			return reg.FZ();
		}
		else if constexpr (cc == jump_condition::NC) {
			return !reg.FC();
		}
		else if constexpr (cc == jump_condition::C) {
			return reg.FC();
		}

		std::unreachable();
	}

	using r8_ptr = std::uint8_t yahbog::registers::*;
	using r16_ptr = std::uint16_t yahbog::registers::*;

	struct regpair_t {
		r8_ptr r1;
		r8_ptr r2;

		consteval regpair_t(r8_ptr r1, r8_ptr r2) : r1(r1), r2(r2) {}

		constexpr std::uint16_t operator()(yahbog::registers& reg) const {
			return (reg.*r1 << 8) | reg.*r2;
		}
		constexpr void operator()(yahbog::registers& reg, std::uint16_t val) const {
			reg.*r1 = val >> 8;
			reg.*r2 = val & 0xFF;
		}

		consteval bool operator==(const regpair_t& other) const {
			return r1 == other.r1 && r2 == other.r2;
		}
	};

	constexpr auto regpair_af = regpair_t{ &yahbog::registers::a, &yahbog::registers::f };
	constexpr auto regpair_bc = regpair_t{ &yahbog::registers::b, &yahbog::registers::c };
	constexpr auto regpair_de = regpair_t{ &yahbog::registers::d, &yahbog::registers::e };
	constexpr auto regpair_hl = regpair_t{ &yahbog::registers::h, &yahbog::registers::l };

	template<typename MemProvider>
	struct next_stage_t;

	template<typename MemProvider>
	using continuation_fn_t = next_stage_t<MemProvider>(*)(OPCODE_ARGS);

	template<typename MemProvider>
	struct next_stage_t {
		continuation_fn_t<MemProvider> fn = nullptr;

		constexpr next_stage_t() = default;
		constexpr next_stage_t(continuation_fn_t<MemProvider> fn) : fn(fn) {}

		constexpr next_stage_t(const next_stage_t&) = default;
		constexpr next_stage_t(next_stage_t&&) = default;

		constexpr next_stage_t& operator=(const next_stage_t&) = default;
		constexpr next_stage_t& operator=(next_stage_t&&) = default;

		constexpr next_stage_t<MemProvider> operator()(OPCODE_ARGS) noexcept {
			return fn(OPCODE_CALL);
		}

		explicit constexpr operator bool() const noexcept {
			return fn != nullptr;
		}
	};

	namespace opcodes {
		namespace common {
			template<typename MemProvider>
			constexpr next_stage_t<MemProvider> prefetch(OPCODE_ARGS) noexcept;

			template<typename MemProvider>
			constexpr next_stage_t<MemProvider> prefetch_and_reset(OPCODE_ARGS) noexcept;

			template<typename MemProvider>
			constexpr next_stage_t<MemProvider> fetch_opcode(OPCODE_ARGS) noexcept;
		}

		DEFINE_SIMPLE_OPCODE(adc_aa)
			auto fc = reg.FC();

			reg.FH((reg.a & 0xF) + (reg.a & 0xF) + reg.FC() > 0xF);
			reg.FC(int(reg.a) + int(reg.a) + reg.FC() > 0xFF);
			reg.a += reg.a + fc;
			reg.FZ(reg.a == 0);
			reg.FN(0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(adc_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto fc = reg.FC();

				reg.FH(half_carries_add(reg.a, reg.z) | half_carries_add(reg.a + reg.z, reg.FC()));
				reg.FC(carries_add(reg.a, reg.z) | carries_add(reg.a + reg.z, reg.FC()));
				reg.a += reg.z + fc;

				reg.FZ(reg.a == 0);
				reg.FN(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_OPCODE(add_aa)
			reg.FH(half_carries_add(reg.a, reg.a));
			reg.FC(carries_add(reg.a, reg.a));
			reg.a += reg.a;
			reg.FZ(reg.a == 0);
			reg.FN(0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(add_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.FH(half_carries_add(reg.a, reg.z));
				reg.FC(carries_add(reg.a, reg.z));
				reg.a += reg.z;
				reg.FZ(reg.a == 0);
				reg.FN(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_OPCODE(and_aa)
			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(1); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(and_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.a &= reg.z;
				reg.FZ(reg.a == 0);
				reg.FN(0); reg.FH(1); reg.FC(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(call_n16) {
			DECLARE_STAGES_6;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.sp -= 1;
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				mem->write(reg.sp, reg.pc >> 8);
				reg.sp--;
				return MOVE_TO_STAGE(4);
			}

			DEFINE_STAGE(4) {
				mem->write(reg.sp, reg.pc & 0xFF);
				reg.pc = reg.wz();
				return MOVE_TO_STAGE(5);
			}

			DEFINE_STAGE(5) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_OPCODE(ccf)
			reg.FC(!reg.FC());
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(cp_aa)
			reg.FZ(1); reg.FN(1); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(cp_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.FZ(reg.a == reg.z);
				reg.FH(half_carries_sub(reg.a, reg.z));
				reg.FC(carries_sub(reg.a, reg.z));
				reg.FN(1);
				
				return common::prefetch_and_reset(OPCODE_CALL);
			}
			
		};

		DEFINE_SIMPLE_OPCODE(cpl)
			reg.a = ~reg.a;
			reg.FH(1); reg.FN(1);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(daa)
			auto adjust = 0;

			if (reg.FN()) {
				if (reg.FH()) {
					adjust |= 0x06;
				}
				if (reg.FC()) {
					adjust |= 0x60;
				}
				reg.a -= adjust;
			}
			else {
				if ((reg.a & 0xF) > 9 || reg.FH()) {
					adjust |= 0x06;
				}
				if (reg.a > 0x99 || reg.FC()) {
					adjust |= 0x60;
					reg.FC(1);
				}
				reg.a += adjust;
			}

			reg.FZ(reg.a == 0);
			reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(di)
			reg.ime = 0;
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(ei)
			reg.ie = 1;
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(halt)
			reg.halted = 1;
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(illegal)
			// no-op, but should technically crash the CPU
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(jp_n16) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(2);
			}
			
			DEFINE_STAGE(2) {
				reg.pc = reg.wz();
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(jr_n8) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}
			
			DEFINE_STAGE(1) {
				reg.wz(reg.pc + std::bit_cast<std::int8_t>(reg.z));
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.pc = reg.wz();
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_OPCODE(nop)
			// no-op, obviously
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(or_aa)
			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(or_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.a |= reg.z;
				reg.FZ(reg.a == 0);
				reg.FN(0); reg.FH(0); reg.FC(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(prefix) {
			DECLARE_STAGES_1;

			DEFINE_STAGE(0) {
				reg.ir = mem->read(reg.pc) + 0x100;
				reg.pc++;
				return common::fetch_opcode(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(ret) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.sp);
				reg.sp++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.sp);
				reg.sp++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.pc = reg.wz();
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};
		
		DEFINE_OPCODE(reti) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.sp);
				reg.sp++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.sp);
				reg.sp++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.pc = reg.wz();
				reg.ime = 1;
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_OPCODE(rla)
			auto carry = reg.FC();
			reg.FC(reg.a >> 7);
			reg.a = (reg.a << 1) | carry;

			reg.FZ(0); reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(rlca)
			reg.FC(reg.a >> 7);
			reg.a = (reg.a << 1) | reg.FC();

			reg.FZ(0); reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(rra)
			auto carry = reg.FC();
			reg.FC(reg.a & 1);
			reg.a = (reg.a >> 1) | (carry << 7);

			reg.FZ(0); reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(rrca)
			reg.FC(reg.a & 1);
			reg.a = (reg.a >> 1) | (reg.FC() << 7);

			reg.FZ(0); reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(sbc_aa)
			int res = (reg.a & 0x0F) - (reg.a & 0x0F) - reg.FC();
			int op = reg.a + reg.FC();

			reg.FH(res < 0);
			reg.FC(op > reg.a);
			reg.FN(1);

			reg.a -= op;

			reg.FZ(reg.a == 0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(sbc_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				int res = (reg.a & 0x0F) - (reg.z & 0x0F) - reg.FC();
				int op = reg.z + reg.FC();

				reg.FH(res < 0);
				reg.FC(op > reg.a);
				reg.FN(1);

				reg.a -= op;

				reg.FZ(reg.a == 0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_OPCODE(scf)
			reg.FN(0); reg.FH(0); reg.FC(1);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(stop_n8)
			// no-op, but should end all execution
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_OPCODE(sub_aa)
			reg.a = 0;
			reg.FZ(1); reg.FN(1); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(sub_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto op = reg.z;
				reg.FZ(reg.a == op);
				reg.FH(half_carries_sub(reg.a, op));
				reg.FC(carries_sub(reg.a, op));
				reg.FN(1);
				reg.a -= op;

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_OPCODE(xor_aa)
			reg.a = 0;
			reg.FZ(1); reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_OPCODE(xor_n8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.a ^= reg.z;
				reg.FZ(reg.a == 0);
				reg.FN(0); reg.FH(0); reg.FC(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(bit_r16P, int op1, regpair_t op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op2(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto bit = 1 << op1;

				reg.FZ(!(reg.z & bit));
				reg.FN(0); reg.FH(1);

				return common::prefetch_and_reset(reg, mem);
			}
		};

		DEFINE_TEMPLATE_OPCODE(res_r16P, int op1, regpair_t op2) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op2(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto bit = 1 << op1;
				reg.z &= ~bit;
				mem->write(op2(reg), reg.z);
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(reg, mem);
			}
		};

		DEFINE_TEMPLATE_OPCODE(set_r16P, int op1, regpair_t op2) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op2(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto bit = 1 << op1;
				reg.z |= bit;
				mem->write(op2(reg), reg.z);
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(reg, mem);
			}
		};

		DEFINE_SIMPLE_TEMPLATE_OPCODE(bit_r8, int op1, r8_ptr op2) 
			auto val = reg.*op2;
			auto bit = 1 << op1;

			reg.FZ(!(val & bit));
			reg.FN(0); reg.FH(1);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(res_r8, int op1, r8_ptr op2)
			auto val = reg.*op2;
			auto bit = 1 << op1;

			val &= ~bit;
			reg.*op2 = val;
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(set_r8, int op1, r8_ptr op2)
			auto val = reg.*op2;
			auto bit = 1 << op1;

			val |= bit;
			reg.*op2 = val;
		END_SIMPLE_OPCODE;

		DEFINE_TEMPLATE_OPCODE(rst_n8, int op1) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.sp--;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				mem->write(reg.sp, reg.pc >> 8);
				reg.sp--;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				mem->write(reg.sp, reg.pc & 0xFF);
				reg.pc = op1;
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(call_cc_n16, jump_condition op1) {
			DECLARE_STAGES_6;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}
			
			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				
				if(jump_condition_met<op1>(OPCODE_CALL)) {
					return MOVE_TO_STAGE(3);
				} else {
					return MOVE_TO_STAGE(2);
				}
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}

			DEFINE_STAGE(3) {
				reg.sp--;
				return MOVE_TO_STAGE(4);
			}

			DEFINE_STAGE(4) {
				mem->write(reg.sp, reg.pc >> 8);
				reg.sp--;
				return MOVE_TO_STAGE(5);
			}

			DEFINE_STAGE(5) {
				mem->write(reg.sp, reg.pc & 0xFF);
				reg.pc = reg.wz();
				return MOVE_TO_STAGE(2);
			}
		};

		DEFINE_TEMPLATE_OPCODE(jp_cc_n16, jump_condition op1) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;

				if(jump_condition_met<op1>(OPCODE_CALL)) {
					return MOVE_TO_STAGE(2);
				} else {
					return MOVE_TO_STAGE(3);
				}
			}

			DEFINE_STAGE(2) {
				reg.pc = reg.wz();
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(jr_cc_n8, jump_condition op1) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;

				if(jump_condition_met<op1>(OPCODE_CALL)) {
					return MOVE_TO_STAGE(1);
				} else {
					return MOVE_TO_STAGE(3);
				}
			}

			DEFINE_STAGE(1) {
				// FIXME: something should happen here
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.wz(reg.pc + std::bit_cast<std::int8_t>(reg.z));
				reg.ir = mem->read(reg.wz());
				reg.pc = reg.wz() + 1;
				reg.mupc = 0;
				return common::fetch_opcode(OPCODE_CALL);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ret_cc, jump_condition op1) {
			DECLARE_STAGES_5;

			DEFINE_STAGE(0) {
				if(jump_condition_met<op1>(OPCODE_CALL)) {
					return MOVE_TO_STAGE(1);
				} else {
					return MOVE_TO_STAGE(4);
				}
			}

			DEFINE_STAGE(1) {
				reg.z = mem->read(reg.sp);
				reg.sp++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.w = mem->read(reg.sp);
				reg.sp++;
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				reg.pc = reg.wz();
				return MOVE_TO_STAGE(4);
			}

			DEFINE_STAGE(4) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(add_r16_r16, regpair_t op1, regpair_t op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				// FIXME: something should happen here
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto v1 = op1(reg);
				auto v2 = op2(reg);

				reg.FH(half_carries_add(v1, v2));
				reg.FC(carries_add(v1, v2));

				op1(reg, op1(reg) + v2);

				reg.FN(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(add_hl_sp) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				// FIXME: something should happen here
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto v1 = reg.hl();
				auto v2 = reg.sp;

				reg.FH(half_carries_add(v1, v2));
				reg.FC(carries_add(v1, v2));

				reg.hl(reg.hl() + v2);

				reg.FN(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
			
		};

		DEFINE_OPCODE(ld_sp_hl) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.sp = reg.hl();
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(ld_hl_sp_n8) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				// FIXME: something should happen here
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				int offset = std::bit_cast<std::int8_t>(reg.z);

				reg.FC((reg.sp & 0xFF) + (offset & 0xFF) > 0xFF);
				reg.FH((reg.sp & 0xF) + (offset & 0xF) > 0xF);

				reg.hl(reg.sp + offset);

				reg.FZ(0); reg.FN(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
				
		};

		DEFINE_TEMPLATE_OPCODE(ld_r16PD_r8, regpair_t op1, r8_ptr op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				mem->write(op1(reg), reg.*op2);
				op1(reg, op1(reg) - 1);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_r16PI_r8, regpair_t op1, r8_ptr op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				mem->write(op1(reg), reg.*op2);
				op1(reg, op1(reg) + 1);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_r16P_r8, regpair_t op1, r8_ptr op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				mem->write(op1(reg), reg.*op2);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(adc_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto fc = reg.FC();

				reg.FH((reg.a & 0xF) + (reg.z & 0xF) + reg.FC() > 0xF);
				reg.FC(int(reg.a) + int(reg.z) + reg.FC() > 0xFF);

				reg.a += reg.z + fc;

				reg.FZ(reg.a == 0);
				reg.FN(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(add_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.FH(half_carries_add(reg.a, reg.z));
				reg.FC(carries_add(reg.a, reg.z));

				reg.a += reg.z;
				reg.FZ(reg.a == 0);

				reg.FN(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(add_sp_n8) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				// FIXME: something should happen here
				return MOVE_TO_STAGE(2);
			}
			
			DEFINE_STAGE(2) {
				int val = std::bit_cast<std::int8_t>(reg.z);
				int regval = reg.sp;

				reg.FH((regval & 0xF) + (val & 0xF) > 0xF);
				reg.FC((regval & 0xFF) + (val & 0xFF) > 0xFF);

				reg.sp += val;

				reg.FZ(0); reg.FN(0);
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
			
		};
			
		DEFINE_TEMPLATE_OPCODE(and_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.a &= reg.z;
				reg.FZ(reg.a == 0);
				reg.FN(0); reg.FH(1); reg.FC(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(cp_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.FZ(reg.a == reg.z);
				reg.FH(half_carries_sub(reg.a, reg.z));
				reg.FC(carries_sub(reg.a, reg.z));
				reg.FN(1);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(dec_r16, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				op1(reg, op1(reg) - 1);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(dec_sp) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				--reg.sp;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(dec_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.FH(half_carries_sub(reg.z, 1));
				reg.FZ(reg.z == 1);
				reg.FN(1);

				mem->write(op1(reg), reg.z - 1);
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(inc_r16, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				op1(reg, op1(reg) + 1);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(inc_sp) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				++reg.sp;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
			
		};

		DEFINE_TEMPLATE_OPCODE(inc_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.FH(half_carries_add(reg.z, 1));
				reg.FZ(reg.z == 0xFF);

				mem->write(op1(reg), reg.z + 1);
				reg.FN(0);
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(jp_hl) {
			DECLARE_STAGES_1;

			DEFINE_STAGE(0) {
				reg.ir = mem->read(reg.hl());
				reg.pc = reg.hl() + 1;
				return common::fetch_opcode(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_n16P_r16, r16_ptr op1) {
			DECLARE_STAGES_5;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				mem->write(reg.wz(), reg.*op1 & 0xFF);
				reg.wz(reg.wz() + 1);
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				mem->write(reg.wz(), reg.*op1 >> 8);
				return MOVE_TO_STAGE(4);
			}

			DEFINE_STAGE(4) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
			
		};

		DEFINE_TEMPLATE_OPCODE(ld_r16P_n8, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				mem->write(op1(reg), reg.z);
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_r16_n16, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				op1(reg, reg.wz());
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(ld_sp_n16) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.sp = reg.wz();
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};
		
		DEFINE_TEMPLATE_OPCODE(or_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.a |= reg.z;

				reg.FZ(reg.a == 0);
				reg.FN(0); reg.FH(0); reg.FC(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(pop_r16, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				if constexpr(op1 == regpair_af) {
					reg.z = mem->read(reg.sp) & 0xF0;
				} else {
					reg.z = mem->read(reg.sp);
				}

				reg.sp++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.sp);
				reg.sp++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				op1(reg, reg.wz());
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(push_r16, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.sp--;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				mem->write(reg.sp, op1(reg) >> 8);
				reg.sp--;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				mem->write(reg.sp, op1(reg) & 0xFF);
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(rl_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;
				auto carry = reg.FC();

				reg.FC(val >> 7);
				val = (val << 1) | carry;

				mem->write(op1(reg), val);

				reg.FZ(val == 0);
				reg.FN(0); reg.FH(0);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(rlc_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;

				reg.FC(val >> 7);
				val = (val << 1) | reg.FC();

				reg.FZ(val == 0);
				reg.FN(0); reg.FH(0);

				mem->write(op1(reg), val);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(rr_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto carry = reg.FC();
				auto val = reg.z;

				reg.FC(val & 1);
				val = (val >> 1) | (carry << 7);

				mem->write(op1(reg), val);

				reg.FZ(val == 0);
				reg.FN(0); reg.FH(0);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(rrc_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.FC(reg.z & 1);
				reg.z = (reg.z >> 1) | (reg.FC() << 7);
				reg.FZ(reg.z == 0);
				reg.FN(0); reg.FH(0);

				mem->write(op1(reg), reg.z);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(sbc_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;

				int res = (reg.a & 0x0F) - (val & 0x0F) - reg.FC();
				int op = val + reg.FC();

				reg.FH(res < 0);
				reg.FC(op > reg.a);
				reg.FN(1);

				reg.a -= op;

				reg.FZ(reg.a == 0);
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(sla_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;

				reg.FC(val >> 7);
				val <<= 1;
				reg.FZ(val == 0);
				reg.FN(0); reg.FH(0);
				mem->write(op1(reg), val);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(sra_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;

				reg.FC(val & 1);
				val = (val & 0x80) | (val >> 1);

				mem->write(op1(reg), val);

				reg.FZ(val == 0);
				reg.FN(0); reg.FH(0);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(srl_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;

				reg.FC(val & 1);
				val >>= 1;

				mem->write(op1(reg), val);

				reg.FZ(val == 0);
				reg.FN(0); reg.FH(0);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(sub_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;
				reg.FZ(reg.a == val);
				reg.FN(1);
				reg.FH(half_carries_sub(reg.a, val));
				reg.FC(carries_sub(reg.a, val));

				reg.a -= val;

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(swap_r16P, regpair_t op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				auto val = reg.z;

				val = (val >> 4) | (val << 4);

				mem->write(op1(reg), val);


				reg.FZ(val == 0);
				reg.FN(0); reg.FH(0); reg.FC(0);

				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(xor_r16P, regpair_t op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op1(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.a ^= reg.z;
				reg.FZ(reg.a == 0);
				reg.FN(0); reg.FH(0); reg.FC(0);

				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_r8_r16P, r8_ptr op1, regpair_t op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op2(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.*op1 = reg.z;
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_r8_r16PD, r8_ptr op1, regpair_t op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(op2(reg));
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.*op1 = reg.z;
				op2(reg, op2(reg) - 1);
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_OPCODE(ld_a_hlp) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.hl());
				reg.hl(reg.hl() + 1);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.a = reg.z;
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_TEMPLATE_OPCODE(ld_r8_r8, r8_ptr op1, r8_ptr op2)
			reg.*op1 = reg.*op2;
		END_SIMPLE_OPCODE;

		DEFINE_TEMPLATE_OPCODE(ldh_r8P_r8, r8_ptr op1, r8_ptr op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				mem->write(0xFF00 + reg.*op1, reg.*op2);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ldh_r8_r8P, r8_ptr op1, r8_ptr op2) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.*op1 = mem->read(0xFF00 + reg.*op2);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_TEMPLATE_OPCODE(adc_r8, r8_ptr op1)
			std::uint8_t op = reg.FC();

			reg.FH((reg.a & 0xF) + (reg.*op1 & 0xF) + reg.FC() > 0xF);
			reg.FC(int(reg.a) + int(reg.*op1) + reg.FC() > 0xFF);
			reg.a += reg.*op1;
			reg.a += op;

			reg.FZ(reg.a == 0);

			reg.FN(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(add_r8, r8_ptr op1)
			reg.FH(half_carries_add(reg.a, reg.*op1));
			reg.FC(carries_add(reg.a, reg.*op1));
			reg.a += reg.*op1;
			reg.FZ(reg.a == 0);

			reg.FN(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(and_r8, r8_ptr op1)
			reg.a &= reg.*op1;

			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(1); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(cp_r8, r8_ptr op1)
			auto op = reg.*op1;

			reg.FZ(reg.a == op);
			reg.FH(half_carries_sub(reg.a, op));
			reg.FC(carries_sub(reg.a, op));

			reg.FN(1);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(dec_r8, r8_ptr op1)
			reg.FH(half_carries_sub(reg.*op1, 1));
			(reg.*op1)--;
			reg.FZ(reg.*op1 == 0);

			reg.FN(1);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(inc_r8, r8_ptr op1)
			reg.FH(half_carries_add(reg.*op1, 1));
			(reg.*op1)++;
			reg.FZ(reg.*op1 == 0);
			reg.FN(0);
		END_SIMPLE_OPCODE;

		DEFINE_TEMPLATE_OPCODE(ld_n16P_r8, r8_ptr op1) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				mem->write(reg.wz(), reg.*op1);
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_r8_n16P, r8_ptr op1) {
			DECLARE_STAGES_4;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.w = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.z = mem->read(reg.wz());
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				reg.*op1 = reg.z;
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ld_r8_n8, r8_ptr op1) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.*op1 = reg.z;
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ldh_n8P_r8, r8_ptr op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				mem->write(0xFF00 + reg.z, reg.*op1);
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_TEMPLATE_OPCODE(ldh_r8_n8P, r8_ptr op1) {
			DECLARE_STAGES_3;

			DEFINE_STAGE(0) {
				reg.z = mem->read(reg.pc);
				reg.pc++;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.z = mem->read(0xFF00 + reg.z);
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				reg.*op1 = reg.z;
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		DEFINE_SIMPLE_TEMPLATE_OPCODE(or_r8, r8_ptr op1)
			reg.a |= reg.*op1;

			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(rl_r8, r8_ptr op1)
			auto val = reg.*op1;
			auto carry = reg.FC();

			reg.FC(val >> 7);
			val = (val << 1) | carry;

			reg.*op1 = val;

			reg.FZ(val == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;
		
		DEFINE_SIMPLE_TEMPLATE_OPCODE(rlc_r8, r8_ptr op1)
			reg.FC(reg.*op1 >> 7);
			reg.*op1 = (reg.*op1 << 1) | reg.FC();

			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(rr_r8, r8_ptr op1)
			auto carry = reg.FC();

			reg.FC(reg.*op1 & 1);
			reg.*op1 = (reg.*op1 >> 1) | (carry << 7);

			reg.FZ(reg.*op1 == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(rrc_r8, r8_ptr op1)
			reg.FC(reg.*op1 & 1);
			reg.*op1 = (reg.*op1 >> 1) | (reg.FC() << 7);

			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(sbc_r8, r8_ptr op1)
			int res = (reg.a & 0x0F) - (reg.*op1 & 0x0F) - reg.FC();
			int op = reg.*op1 + reg.FC();

			reg.FH(res < 0);
			reg.FC(op > reg.a);
			reg.FN(1);

			reg.a -= op;

			reg.FZ(reg.a == 0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(sla_r8, r8_ptr op1)
			reg.FC(reg.*op1 >> 7);
			reg.*op1 <<= 1;
			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(sra_r8, r8_ptr op1)
			reg.FC(reg.*op1 & 1);
			reg.*op1 = (reg.*op1 >> 1) | (reg.*op1 & 0x80);

			reg.FZ(reg.*op1 == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(srl_r8, r8_ptr op1)
			reg.FC(reg.*op1 & 1);
			reg.*op1 >>= 1;

			reg.FZ(reg.*op1 == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(sub_r8, r8_ptr op1)
			auto op = reg.*op1;

			reg.FZ(reg.a == op);
			reg.FH(half_carries_sub(reg.a, op));
			reg.FC(carries_sub(reg.a, op));

			reg.a -= op;

			reg.FN(1);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(swap_r8, r8_ptr op1)
			auto upper = reg.*op1 >> 4;
			auto lower = reg.*op1 & 0xF;

			reg.*op1 = (lower << 4) | upper;
			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_SIMPLE_TEMPLATE_OPCODE(xor_r8, r8_ptr op1)
			reg.a ^= reg.*op1;

			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		DEFINE_TEMPLATE_OPCODE(isr, std::uint8_t addr) {
			DECLARE_STAGES_5;

			DEFINE_STAGE(0) {
				reg.ime = 0;
				reg.pc--;
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				reg.sp--;
				return MOVE_TO_STAGE(2);
			}

			DEFINE_STAGE(2) {
				mem->write(reg.sp, reg.pc >> 8);
				reg.sp--;
				return MOVE_TO_STAGE(3);
			}

			DEFINE_STAGE(3) {
				mem->write(reg.sp, reg.pc & 0xFF);
				return MOVE_TO_STAGE(4);
			}

			DEFINE_STAGE(4) {
				reg.ir = mem->read(addr);
				reg.pc = addr + 1;
				reg.mupc = 0;
				return common::fetch_opcode(reg, mem);
			}
		};

		template<typename MemProvider>
		struct opcode_traits {
			using table_fn_t = next_stage_t<MemProvider>(*)(std::size_t);

			continuation_fn_t<MemProvider> entry;
			table_fn_t table;

			template<typename T>
			constexpr static opcode_traits<MemProvider> make() noexcept {
				return opcode_traits<MemProvider>{.entry = &T::stage0, .table = &T::stage_table};
			}
		};

		template<typename MemProvider>
		constexpr const std::array<opcode_traits<MemProvider>, 512>& map();

		namespace common {
			template<typename MemProvider>
			constexpr next_stage_t<MemProvider> prefetch(OPCODE_ARGS) noexcept {
				reg.ir = mem->read(reg.pc);
				reg.pc++;
				return fetch_opcode<MemProvider>(reg, mem);
			}

			template<typename MemProvider>
			constexpr next_stage_t<MemProvider> prefetch_and_reset(OPCODE_ARGS) noexcept {
				reg.mupc = 0;
				return prefetch<MemProvider>(reg, mem);
			}

			template<typename MemProvider>
			constexpr next_stage_t<MemProvider> fetch_opcode(OPCODE_ARGS) noexcept {
				return opcodes::map<MemProvider>()[reg.ir].entry;
			}
		}

		template<typename MemProvider>
		constexpr const std::array<opcode_traits<MemProvider>, 512>& map() {
			constexpr static std::array<opcode_traits<MemProvider>, 512> ops = []() constexpr {
				std::array<opcode_traits<MemProvider>, 512> ops{};

				using traits_t = opcode_traits<MemProvider>;

				#define MAKE_TRAITS_SIMPLE(name) traits_t::template make<name<MemProvider>>()
				#define MAKE_TRAITS(name, ...) traits_t::template make<name<MemProvider, __VA_ARGS__>>()

				ops[0x00] = MAKE_TRAITS_SIMPLE(nop);
				ops[0x01] = MAKE_TRAITS(ld_r16_n16, regpair_bc);
				ops[0x02] = MAKE_TRAITS(ld_r16P_r8, regpair_bc, &registers::a);
				ops[0x03] = MAKE_TRAITS(inc_r16, regpair_bc);
				ops[0x04] = MAKE_TRAITS(inc_r8, &registers::b);
				ops[0x05] = MAKE_TRAITS(dec_r8, &registers::b);
				ops[0x06] = MAKE_TRAITS(ld_r8_n8, &registers::b);
				ops[0x07] = MAKE_TRAITS_SIMPLE(rlca);
				ops[0x08] = MAKE_TRAITS(ld_n16P_r16, &registers::sp);
				ops[0x09] = MAKE_TRAITS(add_r16_r16, regpair_hl, regpair_bc);
				ops[0x0A] = MAKE_TRAITS(ld_r8_r16P, &registers::a, regpair_bc);
				ops[0x0B] = MAKE_TRAITS(dec_r16, regpair_bc);
				ops[0x0C] = MAKE_TRAITS(inc_r8, &registers::c);
				ops[0x0D] = MAKE_TRAITS(dec_r8, &registers::c);
				ops[0x0E] = MAKE_TRAITS(ld_r8_n8, &registers::c);
				ops[0x0F] = MAKE_TRAITS_SIMPLE(rrca);
				ops[0x10] = MAKE_TRAITS_SIMPLE(stop_n8);
				ops[0x11] = MAKE_TRAITS(ld_r16_n16, regpair_de);
				ops[0x12] = MAKE_TRAITS(ld_r16P_r8, regpair_de, &registers::a);
				ops[0x13] = MAKE_TRAITS(inc_r16, regpair_de);
				ops[0x14] = MAKE_TRAITS(inc_r8, &registers::d);
				ops[0x15] = MAKE_TRAITS(dec_r8, &registers::d);
				ops[0x16] = MAKE_TRAITS(ld_r8_n8, &registers::d);
				ops[0x17] = MAKE_TRAITS_SIMPLE(rla);
				ops[0x18] = MAKE_TRAITS_SIMPLE(jr_n8);
				ops[0x19] = MAKE_TRAITS(add_r16_r16, regpair_hl, regpair_de);
				ops[0x1A] = MAKE_TRAITS(ld_r8_r16P, &registers::a, regpair_de);
				ops[0x1B] = MAKE_TRAITS(dec_r16, regpair_de);
				ops[0x1C] = MAKE_TRAITS(inc_r8, &registers::e);
				ops[0x1D] = MAKE_TRAITS(dec_r8, &registers::e);
				ops[0x1E] = MAKE_TRAITS(ld_r8_n8, &registers::e);
				ops[0x1F] = MAKE_TRAITS_SIMPLE(rra);
				ops[0x20] = MAKE_TRAITS(jr_cc_n8, jump_condition::NZ);
				ops[0x21] = MAKE_TRAITS(ld_r16_n16, regpair_hl);
				ops[0x22] = MAKE_TRAITS(ld_r16PI_r8, regpair_hl, &registers::a);
				ops[0x23] = MAKE_TRAITS(inc_r16, regpair_hl);
				ops[0x24] = MAKE_TRAITS(inc_r8, &registers::h);
				ops[0x25] = MAKE_TRAITS(dec_r8, &registers::h);
				ops[0x26] = MAKE_TRAITS(ld_r8_n8, &registers::h);
				ops[0x27] = MAKE_TRAITS_SIMPLE(daa);
				ops[0x28] = MAKE_TRAITS(jr_cc_n8, jump_condition::Z);
				ops[0x29] = MAKE_TRAITS(add_r16_r16, regpair_hl, regpair_hl);
				ops[0x2A] = MAKE_TRAITS_SIMPLE(ld_a_hlp);
				ops[0x2B] = MAKE_TRAITS(dec_r16, regpair_hl);
				ops[0x2C] = MAKE_TRAITS(inc_r8, &registers::l);
				ops[0x2D] = MAKE_TRAITS(dec_r8, &registers::l);
				ops[0x2E] = MAKE_TRAITS(ld_r8_n8, &registers::l);
				ops[0x2F] = MAKE_TRAITS_SIMPLE(cpl);
				ops[0x30] = MAKE_TRAITS(jr_cc_n8, jump_condition::NC);
				ops[0x31] = MAKE_TRAITS_SIMPLE(ld_sp_n16);
				ops[0x32] = MAKE_TRAITS(ld_r16PD_r8, regpair_hl, &registers::a);
				ops[0x33] = MAKE_TRAITS_SIMPLE(inc_sp);
				ops[0x34] = MAKE_TRAITS(inc_r16P, regpair_hl);
				ops[0x35] = MAKE_TRAITS(dec_r16P, regpair_hl);
				ops[0x36] = MAKE_TRAITS(ld_r16P_n8, regpair_hl);
				ops[0x37] = MAKE_TRAITS_SIMPLE(scf);
				ops[0x38] = MAKE_TRAITS(jr_cc_n8, jump_condition::C);
				ops[0x39] = MAKE_TRAITS_SIMPLE(add_hl_sp);
				ops[0x3A] = MAKE_TRAITS(ld_r8_r16PD, &registers::a, regpair_hl);
				ops[0x3B] = MAKE_TRAITS_SIMPLE(dec_sp);
				ops[0x3C] = MAKE_TRAITS(inc_r8, &registers::a);
				ops[0x3D] = MAKE_TRAITS(dec_r8, &registers::a);
				ops[0x3E] = MAKE_TRAITS(ld_r8_n8, &registers::a);
				ops[0x3F] = MAKE_TRAITS_SIMPLE(ccf);
				ops[0x40] = MAKE_TRAITS(ld_r8_r8, &registers::b, &registers::b);
				ops[0x41] = MAKE_TRAITS(ld_r8_r8, &registers::b, &registers::c);
				ops[0x42] = MAKE_TRAITS(ld_r8_r8, &registers::b, &registers::d);
				ops[0x43] = MAKE_TRAITS(ld_r8_r8, &registers::b, &registers::e);
				ops[0x44] = MAKE_TRAITS(ld_r8_r8, &registers::b, &registers::h);
				ops[0x45] = MAKE_TRAITS(ld_r8_r8, &registers::b, &registers::l);
				ops[0x46] = MAKE_TRAITS(ld_r8_r16P, &registers::b, regpair_hl);
				ops[0x47] = MAKE_TRAITS(ld_r8_r8, &registers::b, &registers::a);
				ops[0x48] = MAKE_TRAITS(ld_r8_r8, &registers::c, &registers::b);
				ops[0x49] = MAKE_TRAITS(ld_r8_r8, &registers::c, &registers::c);
				ops[0x4A] = MAKE_TRAITS(ld_r8_r8, &registers::c, &registers::d);
				ops[0x4B] = MAKE_TRAITS(ld_r8_r8, &registers::c, &registers::e);
				ops[0x4C] = MAKE_TRAITS(ld_r8_r8, &registers::c, &registers::h);
				ops[0x4D] = MAKE_TRAITS(ld_r8_r8, &registers::c, &registers::l);
				ops[0x4E] = MAKE_TRAITS(ld_r8_r16P, &registers::c, regpair_hl);
				ops[0x4F] = MAKE_TRAITS(ld_r8_r8, &registers::c, &registers::a);
				ops[0x50] = MAKE_TRAITS(ld_r8_r8, &registers::d, &registers::b);
				ops[0x51] = MAKE_TRAITS(ld_r8_r8, &registers::d, &registers::c);
				ops[0x52] = MAKE_TRAITS(ld_r8_r8, &registers::d, &registers::d);
				ops[0x53] = MAKE_TRAITS(ld_r8_r8, &registers::d, &registers::e);
				ops[0x54] = MAKE_TRAITS(ld_r8_r8, &registers::d, &registers::h);
				ops[0x55] = MAKE_TRAITS(ld_r8_r8, &registers::d, &registers::l);
				ops[0x56] = MAKE_TRAITS(ld_r8_r16P, &registers::d, regpair_hl);
				ops[0x57] = MAKE_TRAITS(ld_r8_r8, &registers::d, &registers::a);
				ops[0x58] = MAKE_TRAITS(ld_r8_r8, &registers::e, &registers::b);
				ops[0x59] = MAKE_TRAITS(ld_r8_r8, &registers::e, &registers::c);
				ops[0x5A] = MAKE_TRAITS(ld_r8_r8, &registers::e, &registers::d);
				ops[0x5B] = MAKE_TRAITS(ld_r8_r8, &registers::e, &registers::e);
				ops[0x5C] = MAKE_TRAITS(ld_r8_r8, &registers::e, &registers::h);
				ops[0x5D] = MAKE_TRAITS(ld_r8_r8, &registers::e, &registers::l);
				ops[0x5E] = MAKE_TRAITS(ld_r8_r16P, &registers::e, regpair_hl);
				ops[0x5F] = MAKE_TRAITS(ld_r8_r8, &registers::e, &registers::a);
				ops[0x60] = MAKE_TRAITS(ld_r8_r8, &registers::h, &registers::b);
				ops[0x61] = MAKE_TRAITS(ld_r8_r8, &registers::h, &registers::c);
				ops[0x62] = MAKE_TRAITS(ld_r8_r8, &registers::h, &registers::d);
				ops[0x63] = MAKE_TRAITS(ld_r8_r8, &registers::h, &registers::e);
				ops[0x64] = MAKE_TRAITS(ld_r8_r8, &registers::h, &registers::h);
				ops[0x65] = MAKE_TRAITS(ld_r8_r8, &registers::h, &registers::l);
				ops[0x66] = MAKE_TRAITS(ld_r8_r16P, &registers::h, regpair_hl);
				ops[0x67] = MAKE_TRAITS(ld_r8_r8, &registers::h, &registers::a);
				ops[0x68] = MAKE_TRAITS(ld_r8_r8, &registers::l, &registers::b);
				ops[0x69] = MAKE_TRAITS(ld_r8_r8, &registers::l, &registers::c);
				ops[0x6A] = MAKE_TRAITS(ld_r8_r8, &registers::l, &registers::d);
				ops[0x6B] = MAKE_TRAITS(ld_r8_r8, &registers::l, &registers::e);
				ops[0x6C] = MAKE_TRAITS(ld_r8_r8, &registers::l, &registers::h);
				ops[0x6D] = MAKE_TRAITS(ld_r8_r8, &registers::l, &registers::l);
				ops[0x6E] = MAKE_TRAITS(ld_r8_r16P, &registers::l, regpair_hl);
				ops[0x6F] = MAKE_TRAITS(ld_r8_r8, &registers::l, &registers::a);
				ops[0x70] = MAKE_TRAITS(ld_r16P_r8, regpair_hl, &registers::b);
				ops[0x71] = MAKE_TRAITS(ld_r16P_r8, regpair_hl, &registers::c);
				ops[0x72] = MAKE_TRAITS(ld_r16P_r8, regpair_hl, &registers::d);
				ops[0x73] = MAKE_TRAITS(ld_r16P_r8, regpair_hl, &registers::e);
				ops[0x74] = MAKE_TRAITS(ld_r16P_r8, regpair_hl, &registers::h);
				ops[0x75] = MAKE_TRAITS(ld_r16P_r8, regpair_hl, &registers::l);
				ops[0x76] = MAKE_TRAITS_SIMPLE(halt);
				ops[0x77] = MAKE_TRAITS(ld_r16P_r8, regpair_hl, &registers::a);
				ops[0x78] = MAKE_TRAITS(ld_r8_r8, &registers::a, &registers::b);
				ops[0x79] = MAKE_TRAITS(ld_r8_r8, &registers::a, &registers::c);
				ops[0x7A] = MAKE_TRAITS(ld_r8_r8, &registers::a, &registers::d);
				ops[0x7B] = MAKE_TRAITS(ld_r8_r8, &registers::a, &registers::e);
				ops[0x7C] = MAKE_TRAITS(ld_r8_r8, &registers::a, &registers::h);
				ops[0x7D] = MAKE_TRAITS(ld_r8_r8, &registers::a, &registers::l);
				ops[0x7E] = MAKE_TRAITS(ld_r8_r16P, &registers::a, regpair_hl);
				ops[0x7F] = MAKE_TRAITS(ld_r8_r8, &registers::a, &registers::a);
				ops[0x80] = MAKE_TRAITS(add_r8, &registers::b);
				ops[0x81] = MAKE_TRAITS(add_r8, &registers::c);
				ops[0x82] = MAKE_TRAITS(add_r8, &registers::d);
				ops[0x83] = MAKE_TRAITS(add_r8, &registers::e);
				ops[0x84] = MAKE_TRAITS(add_r8, &registers::h);
				ops[0x85] = MAKE_TRAITS(add_r8, &registers::l);
				ops[0x86] = MAKE_TRAITS(add_r16P, regpair_hl);
				ops[0x87] = MAKE_TRAITS_SIMPLE(add_aa);
				ops[0x88] = MAKE_TRAITS(adc_r8, &registers::b);
				ops[0x89] = MAKE_TRAITS(adc_r8, &registers::c);
				ops[0x8A] = MAKE_TRAITS(adc_r8, &registers::d);
				ops[0x8B] = MAKE_TRAITS(adc_r8, &registers::e);
				ops[0x8C] = MAKE_TRAITS(adc_r8, &registers::h);
				ops[0x8D] = MAKE_TRAITS(adc_r8, &registers::l);
				ops[0x8E] = MAKE_TRAITS(adc_r16P, regpair_hl);
				ops[0x8F] = MAKE_TRAITS_SIMPLE(adc_aa);
				ops[0x90] = MAKE_TRAITS(sub_r8, &registers::b);
				ops[0x91] = MAKE_TRAITS(sub_r8, &registers::c);
				ops[0x92] = MAKE_TRAITS(sub_r8, &registers::d);
				ops[0x93] = MAKE_TRAITS(sub_r8, &registers::e);
				ops[0x94] = MAKE_TRAITS(sub_r8, &registers::h);
				ops[0x95] = MAKE_TRAITS(sub_r8, &registers::l);
				ops[0x96] = MAKE_TRAITS(sub_r16P, regpair_hl);
				ops[0x97] = MAKE_TRAITS_SIMPLE(sub_aa);
				ops[0x98] = MAKE_TRAITS(sbc_r8, &registers::b);
				ops[0x99] = MAKE_TRAITS(sbc_r8, &registers::c);
				ops[0x9A] = MAKE_TRAITS(sbc_r8, &registers::d);
				ops[0x9B] = MAKE_TRAITS(sbc_r8, &registers::e);
				ops[0x9C] = MAKE_TRAITS(sbc_r8, &registers::h);
				ops[0x9D] = MAKE_TRAITS(sbc_r8, &registers::l);
				ops[0x9E] = MAKE_TRAITS(sbc_r16P, regpair_hl);
				ops[0x9F] = MAKE_TRAITS_SIMPLE(sbc_aa);
				ops[0xA0] = MAKE_TRAITS(and_r8, &registers::b);
				ops[0xA1] = MAKE_TRAITS(and_r8, &registers::c);
				ops[0xA2] = MAKE_TRAITS(and_r8, &registers::d);
				ops[0xA3] = MAKE_TRAITS(and_r8, &registers::e);
				ops[0xA4] = MAKE_TRAITS(and_r8, &registers::h);
				ops[0xA5] = MAKE_TRAITS(and_r8, &registers::l);
				ops[0xA6] = MAKE_TRAITS(and_r16P, regpair_hl);
				ops[0xA7] = MAKE_TRAITS_SIMPLE(and_aa);
				ops[0xA8] = MAKE_TRAITS(xor_r8, &registers::b);
				ops[0xA9] = MAKE_TRAITS(xor_r8, &registers::c);
				ops[0xAA] = MAKE_TRAITS(xor_r8, &registers::d);
				ops[0xAB] = MAKE_TRAITS(xor_r8, &registers::e);
				ops[0xAC] = MAKE_TRAITS(xor_r8, &registers::h);
				ops[0xAD] = MAKE_TRAITS(xor_r8, &registers::l);
				ops[0xAE] = MAKE_TRAITS(xor_r16P, regpair_hl);
				ops[0xAF] = MAKE_TRAITS_SIMPLE(xor_aa);
				ops[0xB0] = MAKE_TRAITS(or_r8, &registers::b);
				ops[0xB1] = MAKE_TRAITS(or_r8, &registers::c);
				ops[0xB2] = MAKE_TRAITS(or_r8, &registers::d);
				ops[0xB3] = MAKE_TRAITS(or_r8, &registers::e);
				ops[0xB4] = MAKE_TRAITS(or_r8, &registers::h);
				ops[0xB5] = MAKE_TRAITS(or_r8, &registers::l);
				ops[0xB6] = MAKE_TRAITS(or_r16P, regpair_hl);
				ops[0xB7] = MAKE_TRAITS_SIMPLE(or_aa);
				ops[0xB8] = MAKE_TRAITS(cp_r8, &registers::b);
				ops[0xB9] = MAKE_TRAITS(cp_r8, &registers::c);
				ops[0xBA] = MAKE_TRAITS(cp_r8, &registers::d);
				ops[0xBB] = MAKE_TRAITS(cp_r8, &registers::e);
				ops[0xBC] = MAKE_TRAITS(cp_r8, &registers::h);
				ops[0xBD] = MAKE_TRAITS(cp_r8, &registers::l);
				ops[0xBE] = MAKE_TRAITS(cp_r16P, regpair_hl);
				ops[0xBF] = MAKE_TRAITS_SIMPLE(cp_aa);
				ops[0xC0] = MAKE_TRAITS(ret_cc, jump_condition::NZ);
				ops[0xC1] = MAKE_TRAITS(pop_r16, regpair_bc);
				ops[0xC2] = MAKE_TRAITS(jp_cc_n16, jump_condition::NZ);
				ops[0xC3] = MAKE_TRAITS_SIMPLE(jp_n16);
				ops[0xC4] = MAKE_TRAITS(call_cc_n16, jump_condition::NZ);
				ops[0xC5] = MAKE_TRAITS(push_r16, regpair_bc);
				ops[0xC6] = MAKE_TRAITS_SIMPLE(add_n8);
				ops[0xC7] = MAKE_TRAITS(rst_n8, 0x00);
				ops[0xC8] = MAKE_TRAITS(ret_cc, jump_condition::Z);
				ops[0xC9] = MAKE_TRAITS_SIMPLE(ret);
				ops[0xCA] = MAKE_TRAITS(jp_cc_n16, jump_condition::Z);
				ops[0xCB] = MAKE_TRAITS_SIMPLE(prefix);
				ops[0xCC] = MAKE_TRAITS(call_cc_n16, jump_condition::Z);
				ops[0xCD] = MAKE_TRAITS_SIMPLE(call_n16);
				ops[0xCE] = MAKE_TRAITS_SIMPLE(adc_n8);
				ops[0xCF] = MAKE_TRAITS(rst_n8, 0x08);
				ops[0xD0] = MAKE_TRAITS(ret_cc, jump_condition::NC);
				ops[0xD1] = MAKE_TRAITS(pop_r16, regpair_de);
				ops[0xD2] = MAKE_TRAITS(jp_cc_n16, jump_condition::NC);
				ops[0xD3] = MAKE_TRAITS_SIMPLE(illegal);
				ops[0xD4] = MAKE_TRAITS(call_cc_n16, jump_condition::NC);
				ops[0xD5] = MAKE_TRAITS(push_r16, regpair_de);
				ops[0xD6] = MAKE_TRAITS_SIMPLE(sub_n8);
				ops[0xD7] = MAKE_TRAITS(rst_n8, 0x10);
				ops[0xD8] = MAKE_TRAITS(ret_cc, jump_condition::C);
				ops[0xD9] = MAKE_TRAITS_SIMPLE(reti);
				ops[0xDA] = MAKE_TRAITS(jp_cc_n16, jump_condition::C);
				ops[0xDB] = MAKE_TRAITS_SIMPLE(illegal);
				ops[0xDC] = MAKE_TRAITS(call_cc_n16, jump_condition::C);
				ops[0xDD] = MAKE_TRAITS_SIMPLE(illegal);
				ops[0xDE] = MAKE_TRAITS_SIMPLE(sbc_n8);
				ops[0xDF] = MAKE_TRAITS(rst_n8, 0x18);
				ops[0xE0] = MAKE_TRAITS(ldh_n8P_r8, &registers::a);
				ops[0xE1] = MAKE_TRAITS(pop_r16, regpair_hl);
				ops[0xE2] = MAKE_TRAITS(ldh_r8P_r8, &registers::c, &registers::a);
				ops[0xE3] = MAKE_TRAITS(isr, 0x40);
				ops[0xE4] = MAKE_TRAITS(isr, 0x48);
				ops[0xE5] = MAKE_TRAITS(push_r16, regpair_hl);
				ops[0xE6] = MAKE_TRAITS_SIMPLE(and_n8);
				ops[0xE7] = MAKE_TRAITS(rst_n8, 0x20);
				ops[0xE8] = MAKE_TRAITS_SIMPLE(add_sp_n8);
				ops[0xE9] = MAKE_TRAITS_SIMPLE(jp_hl);
				ops[0xEA] = MAKE_TRAITS(ld_n16P_r8, &registers::a);
				ops[0xEB] = MAKE_TRAITS(isr, 0x50);
				ops[0xEC] = MAKE_TRAITS(isr, 0x58);
				ops[0xED] = MAKE_TRAITS(isr, 0x60);
				ops[0xEE] = MAKE_TRAITS_SIMPLE(xor_n8);
				ops[0xEF] = MAKE_TRAITS(rst_n8, 0x28);
				ops[0xF0] = MAKE_TRAITS(ldh_r8_n8P, &registers::a);
				ops[0xF1] = MAKE_TRAITS(pop_r16, regpair_af);
				ops[0xF2] = MAKE_TRAITS(ldh_r8_r8P, &registers::a, &registers::c);
				ops[0xF3] = MAKE_TRAITS_SIMPLE(di);
				ops[0xF4] = MAKE_TRAITS_SIMPLE(illegal);
				ops[0xF5] = MAKE_TRAITS(push_r16, regpair_af);
				ops[0xF6] = MAKE_TRAITS_SIMPLE(or_n8);
				ops[0xF7] = MAKE_TRAITS(rst_n8, 0x30);
				ops[0xF8] = MAKE_TRAITS_SIMPLE(ld_hl_sp_n8);
				ops[0xF9] = MAKE_TRAITS_SIMPLE(ld_sp_hl);
				ops[0xFA] = MAKE_TRAITS(ld_r8_n16P, &registers::a);
				ops[0xFB] = MAKE_TRAITS_SIMPLE(ei);
				ops[0xFC] = MAKE_TRAITS_SIMPLE(illegal);
				ops[0xFD] = MAKE_TRAITS_SIMPLE(illegal);
				ops[0xFE] = MAKE_TRAITS_SIMPLE(cp_n8);
				ops[0xFF] = MAKE_TRAITS(rst_n8, 0x38);
				ops[0x100] = MAKE_TRAITS(rlc_r8, &registers::b);
				ops[0x101] = MAKE_TRAITS(rlc_r8, &registers::c);
				ops[0x102] = MAKE_TRAITS(rlc_r8, &registers::d);
				ops[0x103] = MAKE_TRAITS(rlc_r8, &registers::e);
				ops[0x104] = MAKE_TRAITS(rlc_r8, &registers::h);
				ops[0x105] = MAKE_TRAITS(rlc_r8, &registers::l);
				ops[0x106] = MAKE_TRAITS(rlc_r16P, regpair_hl);
				ops[0x107] = MAKE_TRAITS(rlc_r8, &registers::a);
				ops[0x108] = MAKE_TRAITS(rrc_r8, &registers::b);
				ops[0x109] = MAKE_TRAITS(rrc_r8, &registers::c);
				ops[0x10A] = MAKE_TRAITS(rrc_r8, &registers::d);
				ops[0x10B] = MAKE_TRAITS(rrc_r8, &registers::e);
				ops[0x10C] = MAKE_TRAITS(rrc_r8, &registers::h);
				ops[0x10D] = MAKE_TRAITS(rrc_r8, &registers::l);
				ops[0x10E] = MAKE_TRAITS(rrc_r16P, regpair_hl);
				ops[0x10F] = MAKE_TRAITS(rrc_r8, &registers::a);
				ops[0x110] = MAKE_TRAITS(rl_r8, &registers::b);
				ops[0x111] = MAKE_TRAITS(rl_r8, &registers::c);
				ops[0x112] = MAKE_TRAITS(rl_r8, &registers::d);
				ops[0x113] = MAKE_TRAITS(rl_r8, &registers::e);
				ops[0x114] = MAKE_TRAITS(rl_r8, &registers::h);
				ops[0x115] = MAKE_TRAITS(rl_r8, &registers::l);
				ops[0x116] = MAKE_TRAITS(rl_r16P, regpair_hl);
				ops[0x117] = MAKE_TRAITS(rl_r8, &registers::a);
				ops[0x118] = MAKE_TRAITS(rr_r8, &registers::b);
				ops[0x119] = MAKE_TRAITS(rr_r8, &registers::c);
				ops[0x11A] = MAKE_TRAITS(rr_r8, &registers::d);
				ops[0x11B] = MAKE_TRAITS(rr_r8, &registers::e);
				ops[0x11C] = MAKE_TRAITS(rr_r8, &registers::h);
				ops[0x11D] = MAKE_TRAITS(rr_r8, &registers::l);
				ops[0x11E] = MAKE_TRAITS(rr_r16P, regpair_hl);
				ops[0x11F] = MAKE_TRAITS(rr_r8, &registers::a);
				ops[0x120] = MAKE_TRAITS(sla_r8, &registers::b);
				ops[0x121] = MAKE_TRAITS(sla_r8, &registers::c);
				ops[0x122] = MAKE_TRAITS(sla_r8, &registers::d);
				ops[0x123] = MAKE_TRAITS(sla_r8, &registers::e);
				ops[0x124] = MAKE_TRAITS(sla_r8, &registers::h);
				ops[0x125] = MAKE_TRAITS(sla_r8, &registers::l);
				ops[0x126] = MAKE_TRAITS(sla_r16P, regpair_hl);
				ops[0x127] = MAKE_TRAITS(sla_r8, &registers::a);
				ops[0x128] = MAKE_TRAITS(sra_r8, &registers::b);
				ops[0x129] = MAKE_TRAITS(sra_r8, &registers::c);
				ops[0x12A] = MAKE_TRAITS(sra_r8, &registers::d);
				ops[0x12B] = MAKE_TRAITS(sra_r8, &registers::e);
				ops[0x12C] = MAKE_TRAITS(sra_r8, &registers::h);
				ops[0x12D] = MAKE_TRAITS(sra_r8, &registers::l);
				ops[0x12E] = MAKE_TRAITS(sra_r16P, regpair_hl);
				ops[0x12F] = MAKE_TRAITS(sra_r8, &registers::a);
				ops[0x130] = MAKE_TRAITS(swap_r8, &registers::b);
				ops[0x131] = MAKE_TRAITS(swap_r8, &registers::c);
				ops[0x132] = MAKE_TRAITS(swap_r8, &registers::d);
				ops[0x133] = MAKE_TRAITS(swap_r8, &registers::e);
				ops[0x134] = MAKE_TRAITS(swap_r8, &registers::h);
				ops[0x135] = MAKE_TRAITS(swap_r8, &registers::l);
				ops[0x136] = MAKE_TRAITS(swap_r16P, regpair_hl);
				ops[0x137] = MAKE_TRAITS(swap_r8, &registers::a);
				ops[0x138] = MAKE_TRAITS(srl_r8, &registers::b);
				ops[0x139] = MAKE_TRAITS(srl_r8, &registers::c);
				ops[0x13A] = MAKE_TRAITS(srl_r8, &registers::d);
				ops[0x13B] = MAKE_TRAITS(srl_r8, &registers::e);
				ops[0x13C] = MAKE_TRAITS(srl_r8, &registers::h);
				ops[0x13D] = MAKE_TRAITS(srl_r8, &registers::l);
				ops[0x13E] = MAKE_TRAITS(srl_r16P, regpair_hl);
				ops[0x13F] = MAKE_TRAITS(srl_r8, &registers::a);
				ops[0x140] = MAKE_TRAITS(bit_r8, 0, &registers::b);
				ops[0x141] = MAKE_TRAITS(bit_r8, 0, &registers::c);
				ops[0x142] = MAKE_TRAITS(bit_r8, 0, &registers::d);
				ops[0x143] = MAKE_TRAITS(bit_r8, 0, &registers::e);
				ops[0x144] = MAKE_TRAITS(bit_r8, 0, &registers::h);
				ops[0x145] = MAKE_TRAITS(bit_r8, 0, &registers::l);
				ops[0x146] = MAKE_TRAITS(bit_r16P, 0, regpair_hl);
				ops[0x147] = MAKE_TRAITS(bit_r8, 0, &registers::a);
				ops[0x148] = MAKE_TRAITS(bit_r8, 1, &registers::b);
				ops[0x149] = MAKE_TRAITS(bit_r8, 1, &registers::c);
				ops[0x14A] = MAKE_TRAITS(bit_r8, 1, &registers::d);
				ops[0x14B] = MAKE_TRAITS(bit_r8, 1, &registers::e);
				ops[0x14C] = MAKE_TRAITS(bit_r8, 1, &registers::h);
				ops[0x14D] = MAKE_TRAITS(bit_r8, 1, &registers::l);
				ops[0x14E] = MAKE_TRAITS(bit_r16P, 1, regpair_hl);
				ops[0x14F] = MAKE_TRAITS(bit_r8, 1, &registers::a);
				ops[0x150] = MAKE_TRAITS(bit_r8, 2, &registers::b);
				ops[0x151] = MAKE_TRAITS(bit_r8, 2, &registers::c);
				ops[0x152] = MAKE_TRAITS(bit_r8, 2, &registers::d);
				ops[0x153] = MAKE_TRAITS(bit_r8, 2, &registers::e);
				ops[0x154] = MAKE_TRAITS(bit_r8, 2, &registers::h);
				ops[0x155] = MAKE_TRAITS(bit_r8, 2, &registers::l);
				ops[0x156] = MAKE_TRAITS(bit_r16P, 2, regpair_hl);
				ops[0x157] = MAKE_TRAITS(bit_r8, 2, &registers::a);
				ops[0x158] = MAKE_TRAITS(bit_r8, 3, &registers::b);
				ops[0x159] = MAKE_TRAITS(bit_r8, 3, &registers::c);
				ops[0x15A] = MAKE_TRAITS(bit_r8, 3, &registers::d);
				ops[0x15B] = MAKE_TRAITS(bit_r8, 3, &registers::e);
				ops[0x15C] = MAKE_TRAITS(bit_r8, 3, &registers::h);
				ops[0x15D] = MAKE_TRAITS(bit_r8, 3, &registers::l);
				ops[0x15E] = MAKE_TRAITS(bit_r16P, 3, regpair_hl);
				ops[0x15F] = MAKE_TRAITS(bit_r8, 3, &registers::a);
				ops[0x160] = MAKE_TRAITS(bit_r8, 4, &registers::b);
				ops[0x161] = MAKE_TRAITS(bit_r8, 4, &registers::c);
				ops[0x162] = MAKE_TRAITS(bit_r8, 4, &registers::d);
				ops[0x163] = MAKE_TRAITS(bit_r8, 4, &registers::e);
				ops[0x164] = MAKE_TRAITS(bit_r8, 4, &registers::h);
				ops[0x165] = MAKE_TRAITS(bit_r8, 4, &registers::l);
				ops[0x166] = MAKE_TRAITS(bit_r16P, 4, regpair_hl);
				ops[0x167] = MAKE_TRAITS(bit_r8, 4, &registers::a);
				ops[0x168] = MAKE_TRAITS(bit_r8, 5, &registers::b);
				ops[0x169] = MAKE_TRAITS(bit_r8, 5, &registers::c);
				ops[0x16A] = MAKE_TRAITS(bit_r8, 5, &registers::d);
				ops[0x16B] = MAKE_TRAITS(bit_r8, 5, &registers::e);
				ops[0x16C] = MAKE_TRAITS(bit_r8, 5, &registers::h);
				ops[0x16D] = MAKE_TRAITS(bit_r8, 5, &registers::l);
				ops[0x16E] = MAKE_TRAITS(bit_r16P, 5, regpair_hl);
				ops[0x16F] = MAKE_TRAITS(bit_r8, 5, &registers::a);
				ops[0x170] = MAKE_TRAITS(bit_r8, 6, &registers::b);
				ops[0x171] = MAKE_TRAITS(bit_r8, 6, &registers::c);
				ops[0x172] = MAKE_TRAITS(bit_r8, 6, &registers::d);
				ops[0x173] = MAKE_TRAITS(bit_r8, 6, &registers::e);
				ops[0x174] = MAKE_TRAITS(bit_r8, 6, &registers::h);
				ops[0x175] = MAKE_TRAITS(bit_r8, 6, &registers::l);
				ops[0x176] = MAKE_TRAITS(bit_r16P, 6, regpair_hl);
				ops[0x177] = MAKE_TRAITS(bit_r8, 6, &registers::a);
				ops[0x178] = MAKE_TRAITS(bit_r8, 7, &registers::b);
				ops[0x179] = MAKE_TRAITS(bit_r8, 7, &registers::c);
				ops[0x17A] = MAKE_TRAITS(bit_r8, 7, &registers::d);
				ops[0x17B] = MAKE_TRAITS(bit_r8, 7, &registers::e);
				ops[0x17C] = MAKE_TRAITS(bit_r8, 7, &registers::h);
				ops[0x17D] = MAKE_TRAITS(bit_r8, 7, &registers::l);
				ops[0x17E] = MAKE_TRAITS(bit_r16P, 7, regpair_hl);
				ops[0x17F] = MAKE_TRAITS(bit_r8, 7, &registers::a);
				ops[0x180] = MAKE_TRAITS(res_r8, 0, &registers::b);
				ops[0x181] = MAKE_TRAITS(res_r8, 0, &registers::c);
				ops[0x182] = MAKE_TRAITS(res_r8, 0, &registers::d);
				ops[0x183] = MAKE_TRAITS(res_r8, 0, &registers::e);
				ops[0x184] = MAKE_TRAITS(res_r8, 0, &registers::h);
				ops[0x185] = MAKE_TRAITS(res_r8, 0, &registers::l);
				ops[0x186] = MAKE_TRAITS(res_r16P, 0, regpair_hl);
				ops[0x187] = MAKE_TRAITS(res_r8, 0, &registers::a);
				ops[0x188] = MAKE_TRAITS(res_r8, 1, &registers::b);
				ops[0x189] = MAKE_TRAITS(res_r8, 1, &registers::c);
				ops[0x18A] = MAKE_TRAITS(res_r8, 1, &registers::d);
				ops[0x18B] = MAKE_TRAITS(res_r8, 1, &registers::e);
				ops[0x18C] = MAKE_TRAITS(res_r8, 1, &registers::h);
				ops[0x18D] = MAKE_TRAITS(res_r8, 1, &registers::l);
				ops[0x18E] = MAKE_TRAITS(res_r16P, 1, regpair_hl);
				ops[0x18F] = MAKE_TRAITS(res_r8, 1, &registers::a);
				ops[0x190] = MAKE_TRAITS(res_r8, 2, &registers::b);
				ops[0x191] = MAKE_TRAITS(res_r8, 2, &registers::c);
				ops[0x192] = MAKE_TRAITS(res_r8, 2, &registers::d);
				ops[0x193] = MAKE_TRAITS(res_r8, 2, &registers::e);
				ops[0x194] = MAKE_TRAITS(res_r8, 2, &registers::h);
				ops[0x195] = MAKE_TRAITS(res_r8, 2, &registers::l);
				ops[0x196] = MAKE_TRAITS(res_r16P, 2, regpair_hl);
				ops[0x197] = MAKE_TRAITS(res_r8, 2, &registers::a);
				ops[0x198] = MAKE_TRAITS(res_r8, 3, &registers::b);
				ops[0x199] = MAKE_TRAITS(res_r8, 3, &registers::c);
				ops[0x19A] = MAKE_TRAITS(res_r8, 3, &registers::d);
				ops[0x19B] = MAKE_TRAITS(res_r8, 3, &registers::e);
				ops[0x19C] = MAKE_TRAITS(res_r8, 3, &registers::h);
				ops[0x19D] = MAKE_TRAITS(res_r8, 3, &registers::l);
				ops[0x19E] = MAKE_TRAITS(res_r16P, 3, regpair_hl);
				ops[0x19F] = MAKE_TRAITS(res_r8, 3, &registers::a);
				ops[0x1A0] = MAKE_TRAITS(res_r8, 4, &registers::b);
				ops[0x1A1] = MAKE_TRAITS(res_r8, 4, &registers::c);
				ops[0x1A2] = MAKE_TRAITS(res_r8, 4, &registers::d);
				ops[0x1A3] = MAKE_TRAITS(res_r8, 4, &registers::e);
				ops[0x1A4] = MAKE_TRAITS(res_r8, 4, &registers::h);
				ops[0x1A5] = MAKE_TRAITS(res_r8, 4, &registers::l);
				ops[0x1A6] = MAKE_TRAITS(res_r16P, 4, regpair_hl);
				ops[0x1A7] = MAKE_TRAITS(res_r8, 4, &registers::a);
				ops[0x1A8] = MAKE_TRAITS(res_r8, 5, &registers::b);
				ops[0x1A9] = MAKE_TRAITS(res_r8, 5, &registers::c);
				ops[0x1AA] = MAKE_TRAITS(res_r8, 5, &registers::d);
				ops[0x1AB] = MAKE_TRAITS(res_r8, 5, &registers::e);
				ops[0x1AC] = MAKE_TRAITS(res_r8, 5, &registers::h);
				ops[0x1AD] = MAKE_TRAITS(res_r8, 5, &registers::l);
				ops[0x1AE] = MAKE_TRAITS(res_r16P, 5, regpair_hl);
				ops[0x1AF] = MAKE_TRAITS(res_r8, 5, &registers::a);
				ops[0x1B0] = MAKE_TRAITS(res_r8, 6, &registers::b);
				ops[0x1B1] = MAKE_TRAITS(res_r8, 6, &registers::c);
				ops[0x1B2] = MAKE_TRAITS(res_r8, 6, &registers::d);
				ops[0x1B3] = MAKE_TRAITS(res_r8, 6, &registers::e);
				ops[0x1B4] = MAKE_TRAITS(res_r8, 6, &registers::h);
				ops[0x1B5] = MAKE_TRAITS(res_r8, 6, &registers::l);
				ops[0x1B6] = MAKE_TRAITS(res_r16P, 6, regpair_hl);
				ops[0x1B7] = MAKE_TRAITS(res_r8, 6, &registers::a);
				ops[0x1B8] = MAKE_TRAITS(res_r8, 7, &registers::b);
				ops[0x1B9] = MAKE_TRAITS(res_r8, 7, &registers::c);
				ops[0x1BA] = MAKE_TRAITS(res_r8, 7, &registers::d);
				ops[0x1BB] = MAKE_TRAITS(res_r8, 7, &registers::e);
				ops[0x1BC] = MAKE_TRAITS(res_r8, 7, &registers::h);
				ops[0x1BD] = MAKE_TRAITS(res_r8, 7, &registers::l);
				ops[0x1BE] = MAKE_TRAITS(res_r16P, 7, regpair_hl);
				ops[0x1BF] = MAKE_TRAITS(res_r8, 7, &registers::a);
				ops[0x1C0] = MAKE_TRAITS(set_r8, 0, &registers::b);
				ops[0x1C1] = MAKE_TRAITS(set_r8, 0, &registers::c);
				ops[0x1C2] = MAKE_TRAITS(set_r8, 0, &registers::d);
				ops[0x1C3] = MAKE_TRAITS(set_r8, 0, &registers::e);
				ops[0x1C4] = MAKE_TRAITS(set_r8, 0, &registers::h);
				ops[0x1C5] = MAKE_TRAITS(set_r8, 0, &registers::l);
				ops[0x1C6] = MAKE_TRAITS(set_r16P, 0, regpair_hl);
				ops[0x1C7] = MAKE_TRAITS(set_r8, 0, &registers::a);
				ops[0x1C8] = MAKE_TRAITS(set_r8, 1, &registers::b);
				ops[0x1C9] = MAKE_TRAITS(set_r8, 1, &registers::c);
				ops[0x1CA] = MAKE_TRAITS(set_r8, 1, &registers::d);
				ops[0x1CB] = MAKE_TRAITS(set_r8, 1, &registers::e);
				ops[0x1CC] = MAKE_TRAITS(set_r8, 1, &registers::h);
				ops[0x1CD] = MAKE_TRAITS(set_r8, 1, &registers::l);
				ops[0x1CE] = MAKE_TRAITS(set_r16P, 1, regpair_hl);
				ops[0x1CF] = MAKE_TRAITS(set_r8, 1, &registers::a);
				ops[0x1D0] = MAKE_TRAITS(set_r8, 2, &registers::b);
				ops[0x1D1] = MAKE_TRAITS(set_r8, 2, &registers::c);
				ops[0x1D2] = MAKE_TRAITS(set_r8, 2, &registers::d);
				ops[0x1D3] = MAKE_TRAITS(set_r8, 2, &registers::e);
				ops[0x1D4] = MAKE_TRAITS(set_r8, 2, &registers::h);
				ops[0x1D5] = MAKE_TRAITS(set_r8, 2, &registers::l);
				ops[0x1D6] = MAKE_TRAITS(set_r16P, 2, regpair_hl);
				ops[0x1D7] = MAKE_TRAITS(set_r8, 2, &registers::a);
				ops[0x1D8] = MAKE_TRAITS(set_r8, 3, &registers::b);
				ops[0x1D9] = MAKE_TRAITS(set_r8, 3, &registers::c);
				ops[0x1DA] = MAKE_TRAITS(set_r8, 3, &registers::d);
				ops[0x1DB] = MAKE_TRAITS(set_r8, 3, &registers::e);
				ops[0x1DC] = MAKE_TRAITS(set_r8, 3, &registers::h);
				ops[0x1DD] = MAKE_TRAITS(set_r8, 3, &registers::l);
				ops[0x1DE] = MAKE_TRAITS(set_r16P, 3, regpair_hl);
				ops[0x1DF] = MAKE_TRAITS(set_r8, 3, &registers::a);
				ops[0x1E0] = MAKE_TRAITS(set_r8, 4, &registers::b);
				ops[0x1E1] = MAKE_TRAITS(set_r8, 4, &registers::c);
				ops[0x1E2] = MAKE_TRAITS(set_r8, 4, &registers::d);
				ops[0x1E3] = MAKE_TRAITS(set_r8, 4, &registers::e);
				ops[0x1E4] = MAKE_TRAITS(set_r8, 4, &registers::h);
				ops[0x1E5] = MAKE_TRAITS(set_r8, 4, &registers::l);
				ops[0x1E6] = MAKE_TRAITS(set_r16P, 4, regpair_hl);
				ops[0x1E7] = MAKE_TRAITS(set_r8, 4, &registers::a);
				ops[0x1E8] = MAKE_TRAITS(set_r8, 5, &registers::b);
				ops[0x1E9] = MAKE_TRAITS(set_r8, 5, &registers::c);
				ops[0x1EA] = MAKE_TRAITS(set_r8, 5, &registers::d);
				ops[0x1EB] = MAKE_TRAITS(set_r8, 5, &registers::e);
				ops[0x1EC] = MAKE_TRAITS(set_r8, 5, &registers::h);
				ops[0x1ED] = MAKE_TRAITS(set_r8, 5, &registers::l);
				ops[0x1EE] = MAKE_TRAITS(set_r16P, 5, regpair_hl);
				ops[0x1EF] = MAKE_TRAITS(set_r8, 5, &registers::a);
				ops[0x1F0] = MAKE_TRAITS(set_r8, 6, &registers::b);
				ops[0x1F1] = MAKE_TRAITS(set_r8, 6, &registers::c);
				ops[0x1F2] = MAKE_TRAITS(set_r8, 6, &registers::d);
				ops[0x1F3] = MAKE_TRAITS(set_r8, 6, &registers::e);
				ops[0x1F4] = MAKE_TRAITS(set_r8, 6, &registers::h);
				ops[0x1F5] = MAKE_TRAITS(set_r8, 6, &registers::l);
				ops[0x1F6] = MAKE_TRAITS(set_r16P, 6, regpair_hl);
				ops[0x1F7] = MAKE_TRAITS(set_r8, 6, &registers::a);
				ops[0x1F8] = MAKE_TRAITS(set_r8, 7, &registers::b);
				ops[0x1F9] = MAKE_TRAITS(set_r8, 7, &registers::c);
				ops[0x1FA] = MAKE_TRAITS(set_r8, 7, &registers::d);
				ops[0x1FB] = MAKE_TRAITS(set_r8, 7, &registers::e);
				ops[0x1FC] = MAKE_TRAITS(set_r8, 7, &registers::h);
				ops[0x1FD] = MAKE_TRAITS(set_r8, 7, &registers::l);
				ops[0x1FE] = MAKE_TRAITS(set_r16P, 7, regpair_hl);
				ops[0x1FF] = MAKE_TRAITS(set_r8, 7, &registers::a);
				
				return ops;
			}();

			return ops;
		};
	}
}