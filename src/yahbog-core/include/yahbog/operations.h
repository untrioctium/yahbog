#pragma once

#include <string_view>
#include <array>
#include <bit>
#include <utility>

#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/utility/constexpr_function.h>

// warning: copius macro (ab)use ahead

#define OPCODE_ARGS yahbog::registers& reg, [[maybe_unused]] yahbog::mem_fns_t* mem
#define OPCODE_CALL reg, mem

#define DEFINE_OPCODE(name) struct name
#define DEFINE_STAGE(num) constexpr static next_stage_t stage##num(OPCODE_ARGS) noexcept
#define BEGIN_STAGE_TABLE constexpr static next_stage_t stage_table(std::size_t stage) noexcept { switch(stage) {
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
	
#define END_SIMPLE_OPCODE return common::prefetch(OPCODE_CALL); } };

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

	template<jump_condition cc>
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

	using opcode_fn = void(*)(OPCODE_ARGS)noexcept;
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

	struct next_stage_t;
	using continuation_fn_t = next_stage_t(*)(OPCODE_ARGS);

	struct next_stage_t {
		continuation_fn_t fn = nullptr;

		constexpr next_stage_t() = default;
		constexpr next_stage_t(continuation_fn_t fn) : fn(fn) {}

		constexpr next_stage_t(const next_stage_t&) = default;
		constexpr next_stage_t(next_stage_t&&) = default;

		constexpr next_stage_t& operator=(const next_stage_t&) = default;
		constexpr next_stage_t& operator=(next_stage_t&&) = default;

		constexpr next_stage_t operator()(OPCODE_ARGS) noexcept {
			return fn(OPCODE_CALL);
		}

		explicit constexpr operator bool() const noexcept {
			return fn != nullptr;
		}
	};

	namespace opcodes {
		namespace common {
			constexpr next_stage_t prefetch(OPCODE_ARGS) noexcept;
			constexpr next_stage_t prefetch_and_reset(OPCODE_ARGS) noexcept;
			constexpr next_stage_t fetch_opcode(OPCODE_ARGS) noexcept;
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

		template<int op1, regpair_t op2>
		DEFINE_OPCODE(bit_r16P) {
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

		template<int op1, regpair_t op2>
		DEFINE_OPCODE(res_r16P) {
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

		template<int op1, regpair_t op2>
		DEFINE_OPCODE(set_r16P) {
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

		template<int op1, r8_ptr op2>
		DEFINE_SIMPLE_OPCODE(bit_r8)
			auto val = reg.*op2;
			auto bit = 1 << op1;

			reg.FZ(!(val & bit));
			reg.FN(0); reg.FH(1);
		END_SIMPLE_OPCODE;

		template<int op1, r8_ptr op2>
		DEFINE_SIMPLE_OPCODE(res_r8)
			auto val = reg.*op2;
			auto bit = 1 << op1;

			val &= ~bit;
			reg.*op2 = val;
		END_SIMPLE_OPCODE;

		template<int op1, r8_ptr op2>
		DEFINE_SIMPLE_OPCODE(set_r8)
			auto val = reg.*op2;
			auto bit = 1 << op1;

			val |= bit;
			reg.*op2 = val;
		END_SIMPLE_OPCODE;

		template<int op1>
		DEFINE_OPCODE(rst_n8) {
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

		template<jump_condition op1>
		DEFINE_OPCODE(call_cc_n16) {
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

		template<jump_condition op1>
		DEFINE_OPCODE(jp_cc_n16) {
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

		template<jump_condition op1>
		DEFINE_OPCODE(jr_cc_n8) {
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

		template<jump_condition op1>
		DEFINE_OPCODE(ret_cc) {
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

		template<regpair_t op1, regpair_t op2>
		DEFINE_OPCODE(add_r16_r16) {
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

		template<regpair_t op1, r8_ptr op2>
		DEFINE_OPCODE(ld_r16PD_r8) {
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

		template<regpair_t op1, r8_ptr op2>
		DEFINE_OPCODE(ld_r16PI_r8) {
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

		template<regpair_t op1, r8_ptr op2>
		DEFINE_OPCODE(ld_r16P_r8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				mem->write(op1(reg), reg.*op2);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		template<regpair_t op1>
		DEFINE_OPCODE(adc_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(add_r16P) {
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
			
		template<regpair_t op1>
		DEFINE_OPCODE(and_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(cp_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(dec_r16) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(dec_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(inc_r16) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(inc_r16P) {
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

		template<r16_ptr op1>
		DEFINE_OPCODE(ld_n16P_r16) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(ld_r16P_n8) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(ld_r16_n16) {
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
		
		template<regpair_t op1>
		DEFINE_OPCODE(or_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(pop_r16) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(push_r16) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(rl_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(rlc_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(rr_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(rrc_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(sbc_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(sla_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(sra_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(srl_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(sub_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(swap_r16P) {
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

		template<regpair_t op1>
		DEFINE_OPCODE(xor_r16P) {
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

		template<r8_ptr op1, regpair_t op2>
		DEFINE_OPCODE(ld_r8_r16P) {
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

		template<r8_ptr op1, regpair_t op2>
		DEFINE_OPCODE(ld_r8_r16PD) {
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

		template<r8_ptr op1, r8_ptr op2>
		DEFINE_SIMPLE_OPCODE(ld_r8_r8)
			reg.*op1 = reg.*op2;
		END_SIMPLE_OPCODE;

		template<r8_ptr op1, r8_ptr op2>
		DEFINE_OPCODE(ldh_r8P_r8) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				mem->write(0xFF00 + reg.*op1, reg.*op2);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		template<r8_ptr op1, r8_ptr op2>
		DEFINE_OPCODE(ldh_r8_r8P) {
			DECLARE_STAGES_2;

			DEFINE_STAGE(0) {
				reg.*op1 = mem->read(0xFF00 + reg.*op2);
				return MOVE_TO_STAGE(1);
			}

			DEFINE_STAGE(1) {
				return common::prefetch_and_reset(OPCODE_CALL);
			}
		};

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(adc_r8)
			std::uint8_t op = reg.FC();

			reg.FH((reg.a & 0xF) + (reg.*op1 & 0xF) + reg.FC() > 0xF);
			reg.FC(int(reg.a) + int(reg.*op1) + reg.FC() > 0xFF);
			reg.a += reg.*op1;
			reg.a += op;

			reg.FZ(reg.a == 0);

			reg.FN(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(add_r8)
			reg.FH(half_carries_add(reg.a, reg.*op1));
			reg.FC(carries_add(reg.a, reg.*op1));
			reg.a += reg.*op1;
			reg.FZ(reg.a == 0);

			reg.FN(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(and_r8)
			reg.a &= reg.*op1;

			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(1); reg.FC(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(cp_r8)
			auto op = reg.*op1;

			reg.FZ(reg.a == op);
			reg.FH(half_carries_sub(reg.a, op));
			reg.FC(carries_sub(reg.a, op));

			reg.FN(1);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(dec_r8)
			reg.FH(half_carries_sub(reg.*op1, 1));
			(reg.*op1)--;
			reg.FZ(reg.*op1 == 0);

			reg.FN(1);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(inc_r8)
			reg.FH(half_carries_add(reg.*op1, 1));
			(reg.*op1)++;
			reg.FZ(reg.*op1 == 0);
			reg.FN(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_OPCODE(ld_n16P_r8) {
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

		template<r8_ptr op1>
		DEFINE_OPCODE(ld_r8_n16P) {
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

		template<r8_ptr op1>
		DEFINE_OPCODE(ld_r8_n8) {
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

		template<r8_ptr op1>
		DEFINE_OPCODE(ldh_n8P_r8) {
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

		template<r8_ptr op1>
		DEFINE_OPCODE(ldh_r8_n8P) {
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

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(or_r8)
			reg.a |= reg.*op1;

			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(rl_r8)
			auto val = reg.*op1;
			auto carry = reg.FC();

			reg.FC(val >> 7);
			val = (val << 1) | carry;

			reg.*op1 = val;

			reg.FZ(val == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;
		
		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(rlc_r8)
			reg.FC(reg.*op1 >> 7);
			reg.*op1 = (reg.*op1 << 1) | reg.FC();

			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(rr_r8)
			auto carry = reg.FC();

			reg.FC(reg.*op1 & 1);
			reg.*op1 = (reg.*op1 >> 1) | (carry << 7);

			reg.FZ(reg.*op1 == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(rrc_r8)
			reg.FC(reg.*op1 & 1);
			reg.*op1 = (reg.*op1 >> 1) | (reg.FC() << 7);

			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(sbc_r8)
			int res = (reg.a & 0x0F) - (reg.*op1 & 0x0F) - reg.FC();
			int op = reg.*op1 + reg.FC();

			reg.FH(res < 0);
			reg.FC(op > reg.a);
			reg.FN(1);

			reg.a -= op;

			reg.FZ(reg.a == 0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(sla_r8)
			reg.FC(reg.*op1 >> 7);
			reg.*op1 <<= 1;
			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(sra_r8)
			reg.FC(reg.*op1 & 1);
			reg.*op1 = (reg.*op1 >> 1) | (reg.*op1 & 0x80);

			reg.FZ(reg.*op1 == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(srl_r8)
			reg.FC(reg.*op1 & 1);
			reg.*op1 >>= 1;

			reg.FZ(reg.*op1 == 0);
			reg.FN(0); reg.FH(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(sub_r8)
			auto op = reg.*op1;

			reg.FZ(reg.a == op);
			reg.FH(half_carries_sub(reg.a, op));
			reg.FC(carries_sub(reg.a, op));

			reg.a -= op;

			reg.FN(1);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(swap_r8)
			auto upper = reg.*op1 >> 4;
			auto lower = reg.*op1 & 0xF;

			reg.*op1 = (lower << 4) | upper;
			reg.FZ(reg.*op1 == 0);

			reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		template<r8_ptr op1>
		DEFINE_SIMPLE_OPCODE(xor_r8)
			reg.a ^= reg.*op1;

			reg.FZ(reg.a == 0);
			reg.FN(0); reg.FH(0); reg.FC(0);
		END_SIMPLE_OPCODE;

		template<std::uint8_t addr>
		DEFINE_OPCODE(isr) {
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

		struct opcode_traits {
			using table_fn_t = next_stage_t(*)(std::size_t);

			continuation_fn_t entry;
			table_fn_t table;

			template<typename T>
			consteval static auto make() noexcept {
				return opcode_traits{.entry = &T::stage0, .table = &T::stage_table};
			}
		};

		constexpr auto map = []() {
			std::array<opcode_traits, 512> ops{};

			ops[0x00] = opcode_traits::make<nop>();
			ops[0x01] = opcode_traits::make<ld_r16_n16<regpair_bc>>();
			ops[0x02] = opcode_traits::make<ld_r16P_r8<regpair_bc, &registers::a>>();
			ops[0x03] = opcode_traits::make<inc_r16<regpair_bc>>();
			ops[0x04] = opcode_traits::make<inc_r8<&registers::b>>();
			ops[0x05] = opcode_traits::make<dec_r8<&registers::b>>();
			ops[0x06] = opcode_traits::make<ld_r8_n8<&registers::b>>();
			ops[0x07] = opcode_traits::make<rlca>();
			ops[0x08] = opcode_traits::make<ld_n16P_r16<&registers::sp>>();
			ops[0x09] = opcode_traits::make<add_r16_r16<regpair_hl, regpair_bc>>();
			ops[0x0A] = opcode_traits::make<ld_r8_r16P<&registers::a, regpair_bc>>();
			ops[0x0B] = opcode_traits::make<dec_r16<regpair_bc>>();
			ops[0x0C] = opcode_traits::make<inc_r8<&registers::c>>();
			ops[0x0D] = opcode_traits::make<dec_r8<&registers::c>>();
			ops[0x0E] = opcode_traits::make<ld_r8_n8<&registers::c>>();
			ops[0x0F] = opcode_traits::make<rrca>();
			ops[0x10] = opcode_traits::make<stop_n8>();
			ops[0x11] = opcode_traits::make<ld_r16_n16<regpair_de>>();
			ops[0x12] = opcode_traits::make<ld_r16P_r8<regpair_de, &registers::a>>();
			ops[0x13] = opcode_traits::make<inc_r16<regpair_de>>();
			ops[0x14] = opcode_traits::make<inc_r8<&registers::d>>();
			ops[0x15] = opcode_traits::make<dec_r8<&registers::d>>();
			ops[0x16] = opcode_traits::make<ld_r8_n8<&registers::d>>();
			ops[0x17] = opcode_traits::make<rla>();
			ops[0x18] = opcode_traits::make<jr_n8>();
			ops[0x19] = opcode_traits::make<add_r16_r16<regpair_hl, regpair_de>>();
			ops[0x1A] = opcode_traits::make<ld_r8_r16P<&registers::a, regpair_de>>();
			ops[0x1B] = opcode_traits::make<dec_r16<regpair_de>>();
			ops[0x1C] = opcode_traits::make<inc_r8<&registers::e>>();
			ops[0x1D] = opcode_traits::make<dec_r8<&registers::e>>();
			ops[0x1E] = opcode_traits::make<ld_r8_n8<&registers::e>>();
			ops[0x1F] = opcode_traits::make<rra>();
			ops[0x20] = opcode_traits::make<jr_cc_n8<jump_condition::NZ>>();
			ops[0x21] = opcode_traits::make<ld_r16_n16<regpair_hl>>();
			ops[0x22] = opcode_traits::make<ld_r16PI_r8<regpair_hl, &registers::a>>();
			ops[0x23] = opcode_traits::make<inc_r16<regpair_hl>>();
			ops[0x24] = opcode_traits::make<inc_r8<&registers::h>>();
			ops[0x25] = opcode_traits::make<dec_r8<&registers::h>>();
			ops[0x26] = opcode_traits::make<ld_r8_n8<&registers::h>>();
			ops[0x27] = opcode_traits::make<daa>();
			ops[0x28] = opcode_traits::make<jr_cc_n8<jump_condition::Z>>();
			ops[0x29] = opcode_traits::make<add_r16_r16<regpair_hl, regpair_hl>>();
			ops[0x2A] = opcode_traits::make<ld_a_hlp>();
			ops[0x2B] = opcode_traits::make<dec_r16<regpair_hl>>();
			ops[0x2C] = opcode_traits::make<inc_r8<&registers::l>>();
			ops[0x2D] = opcode_traits::make<dec_r8<&registers::l>>();
			ops[0x2E] = opcode_traits::make<ld_r8_n8<&registers::l>>();
			ops[0x2F] = opcode_traits::make<cpl>();
			ops[0x30] = opcode_traits::make<jr_cc_n8<jump_condition::NC>>();
			ops[0x31] = opcode_traits::make<ld_sp_n16>();
			ops[0x32] = opcode_traits::make<ld_r16PD_r8<regpair_hl, &registers::a>>();
			ops[0x33] = opcode_traits::make<inc_sp>();
			ops[0x34] = opcode_traits::make<inc_r16P<regpair_hl>>();
			ops[0x35] = opcode_traits::make<dec_r16P<regpair_hl>>();
			ops[0x36] = opcode_traits::make<ld_r16P_n8<regpair_hl>>();
			ops[0x37] = opcode_traits::make<scf>();
			ops[0x38] = opcode_traits::make<jr_cc_n8<jump_condition::C>>();
			ops[0x39] = opcode_traits::make<add_hl_sp>();
			ops[0x3A] = opcode_traits::make<ld_r8_r16PD<&registers::a, regpair_hl>>();
			ops[0x3B] = opcode_traits::make<dec_sp>();
			ops[0x3C] = opcode_traits::make<inc_r8<&registers::a>>();
			ops[0x3D] = opcode_traits::make<dec_r8<&registers::a>>();
			ops[0x3E] = opcode_traits::make<ld_r8_n8<&registers::a>>();
			ops[0x3F] = opcode_traits::make<ccf>();
			ops[0x40] = opcode_traits::make<ld_r8_r8<&registers::b, &registers::b>>();
			ops[0x41] = opcode_traits::make<ld_r8_r8<&registers::b, &registers::c>>();
			ops[0x42] = opcode_traits::make<ld_r8_r8<&registers::b, &registers::d>>();
			ops[0x43] = opcode_traits::make<ld_r8_r8<&registers::b, &registers::e>>();
			ops[0x44] = opcode_traits::make<ld_r8_r8<&registers::b, &registers::h>>();
			ops[0x45] = opcode_traits::make<ld_r8_r8<&registers::b, &registers::l>>();
			ops[0x46] = opcode_traits::make<ld_r8_r16P<&registers::b, regpair_hl>>();
			ops[0x47] = opcode_traits::make<ld_r8_r8<&registers::b, &registers::a>>();
			ops[0x48] = opcode_traits::make<ld_r8_r8<&registers::c, &registers::b>>();
			ops[0x49] = opcode_traits::make<ld_r8_r8<&registers::c, &registers::c>>();
			ops[0x4A] = opcode_traits::make<ld_r8_r8<&registers::c, &registers::d>>();
			ops[0x4B] = opcode_traits::make<ld_r8_r8<&registers::c, &registers::e>>();
			ops[0x4C] = opcode_traits::make<ld_r8_r8<&registers::c, &registers::h>>();
			ops[0x4D] = opcode_traits::make<ld_r8_r8<&registers::c, &registers::l>>();
			ops[0x4E] = opcode_traits::make<ld_r8_r16P<&registers::c, regpair_hl>>();
			ops[0x4F] = opcode_traits::make<ld_r8_r8<&registers::c, &registers::a>>();
			ops[0x50] = opcode_traits::make<ld_r8_r8<&registers::d, &registers::b>>();
			ops[0x51] = opcode_traits::make<ld_r8_r8<&registers::d, &registers::c>>();
			ops[0x52] = opcode_traits::make<ld_r8_r8<&registers::d, &registers::d>>();
			ops[0x53] = opcode_traits::make<ld_r8_r8<&registers::d, &registers::e>>();
			ops[0x54] = opcode_traits::make<ld_r8_r8<&registers::d, &registers::h>>();
			ops[0x55] = opcode_traits::make<ld_r8_r8<&registers::d, &registers::l>>();
			ops[0x56] = opcode_traits::make<ld_r8_r16P<&registers::d, regpair_hl>>();
			ops[0x57] = opcode_traits::make<ld_r8_r8<&registers::d, &registers::a>>();
			ops[0x58] = opcode_traits::make<ld_r8_r8<&registers::e, &registers::b>>();
			ops[0x59] = opcode_traits::make<ld_r8_r8<&registers::e, &registers::c>>();
			ops[0x5A] = opcode_traits::make<ld_r8_r8<&registers::e, &registers::d>>();
			ops[0x5B] = opcode_traits::make<ld_r8_r8<&registers::e, &registers::e>>();
			ops[0x5C] = opcode_traits::make<ld_r8_r8<&registers::e, &registers::h>>();
			ops[0x5D] = opcode_traits::make<ld_r8_r8<&registers::e, &registers::l>>();
			ops[0x5E] = opcode_traits::make<ld_r8_r16P<&registers::e, regpair_hl>>();
			ops[0x5F] = opcode_traits::make<ld_r8_r8<&registers::e, &registers::a>>();
			ops[0x60] = opcode_traits::make<ld_r8_r8<&registers::h, &registers::b>>();
			ops[0x61] = opcode_traits::make<ld_r8_r8<&registers::h, &registers::c>>();
			ops[0x62] = opcode_traits::make<ld_r8_r8<&registers::h, &registers::d>>();
			ops[0x63] = opcode_traits::make<ld_r8_r8<&registers::h, &registers::e>>();
			ops[0x64] = opcode_traits::make<ld_r8_r8<&registers::h, &registers::h>>();
			ops[0x65] = opcode_traits::make<ld_r8_r8<&registers::h, &registers::l>>();
			ops[0x66] = opcode_traits::make<ld_r8_r16P<&registers::h, regpair_hl>>();
			ops[0x67] = opcode_traits::make<ld_r8_r8<&registers::h, &registers::a>>();
			ops[0x68] = opcode_traits::make<ld_r8_r8<&registers::l, &registers::b>>();
			ops[0x69] = opcode_traits::make<ld_r8_r8<&registers::l, &registers::c>>();
			ops[0x6A] = opcode_traits::make<ld_r8_r8<&registers::l, &registers::d>>();
			ops[0x6B] = opcode_traits::make<ld_r8_r8<&registers::l, &registers::e>>();
			ops[0x6C] = opcode_traits::make<ld_r8_r8<&registers::l, &registers::h>>();
			ops[0x6D] = opcode_traits::make<ld_r8_r8<&registers::l, &registers::l>>();
			ops[0x6E] = opcode_traits::make<ld_r8_r16P<&registers::l, regpair_hl>>();
			ops[0x6F] = opcode_traits::make<ld_r8_r8<&registers::l, &registers::a>>();
			ops[0x70] = opcode_traits::make<ld_r16P_r8<regpair_hl, &registers::b>>();
			ops[0x71] = opcode_traits::make<ld_r16P_r8<regpair_hl, &registers::c>>();
			ops[0x72] = opcode_traits::make<ld_r16P_r8<regpair_hl, &registers::d>>();
			ops[0x73] = opcode_traits::make<ld_r16P_r8<regpair_hl, &registers::e>>();
			ops[0x74] = opcode_traits::make<ld_r16P_r8<regpair_hl, &registers::h>>();
			ops[0x75] = opcode_traits::make<ld_r16P_r8<regpair_hl, &registers::l>>();
			ops[0x76] = opcode_traits::make<halt>();
			ops[0x77] = opcode_traits::make<ld_r16P_r8<regpair_hl, &registers::a>>();
			ops[0x78] = opcode_traits::make<ld_r8_r8<&registers::a, &registers::b>>();
			ops[0x79] = opcode_traits::make<ld_r8_r8<&registers::a, &registers::c>>();
			ops[0x7A] = opcode_traits::make<ld_r8_r8<&registers::a, &registers::d>>();
			ops[0x7B] = opcode_traits::make<ld_r8_r8<&registers::a, &registers::e>>();
			ops[0x7C] = opcode_traits::make<ld_r8_r8<&registers::a, &registers::h>>();
			ops[0x7D] = opcode_traits::make<ld_r8_r8<&registers::a, &registers::l>>();
			ops[0x7E] = opcode_traits::make<ld_r8_r16P<&registers::a, regpair_hl>>();
			ops[0x7F] = opcode_traits::make<ld_r8_r8<&registers::a, &registers::a>>();
			ops[0x80] = opcode_traits::make<add_r8<&registers::b>>();
			ops[0x81] = opcode_traits::make<add_r8<&registers::c>>();
			ops[0x82] = opcode_traits::make<add_r8<&registers::d>>();
			ops[0x83] = opcode_traits::make<add_r8<&registers::e>>();
			ops[0x84] = opcode_traits::make<add_r8<&registers::h>>();
			ops[0x85] = opcode_traits::make<add_r8<&registers::l>>();
			ops[0x86] = opcode_traits::make<add_r16P<regpair_hl>>();
			ops[0x87] = opcode_traits::make<add_aa>();
			ops[0x88] = opcode_traits::make<adc_r8<&registers::b>>();
			ops[0x89] = opcode_traits::make<adc_r8<&registers::c>>();
			ops[0x8A] = opcode_traits::make<adc_r8<&registers::d>>();
			ops[0x8B] = opcode_traits::make<adc_r8<&registers::e>>();
			ops[0x8C] = opcode_traits::make<adc_r8<&registers::h>>();
			ops[0x8D] = opcode_traits::make<adc_r8<&registers::l>>();
			ops[0x8E] = opcode_traits::make<adc_r16P<regpair_hl>>();
			ops[0x8F] = opcode_traits::make<adc_aa>();
			ops[0x90] = opcode_traits::make<sub_r8<&registers::b>>();
			ops[0x91] = opcode_traits::make<sub_r8<&registers::c>>();
			ops[0x92] = opcode_traits::make<sub_r8<&registers::d>>();
			ops[0x93] = opcode_traits::make<sub_r8<&registers::e>>();
			ops[0x94] = opcode_traits::make<sub_r8<&registers::h>>();
			ops[0x95] = opcode_traits::make<sub_r8<&registers::l>>();
			ops[0x96] = opcode_traits::make<sub_r16P<regpair_hl>>();
			ops[0x97] = opcode_traits::make<sub_aa>();
			ops[0x98] = opcode_traits::make<sbc_r8<&registers::b>>();
			ops[0x99] = opcode_traits::make<sbc_r8<&registers::c>>();
			ops[0x9A] = opcode_traits::make<sbc_r8<&registers::d>>();
			ops[0x9B] = opcode_traits::make<sbc_r8<&registers::e>>();
			ops[0x9C] = opcode_traits::make<sbc_r8<&registers::h>>();
			ops[0x9D] = opcode_traits::make<sbc_r8<&registers::l>>();
			ops[0x9E] = opcode_traits::make<sbc_r16P<regpair_hl>>();
			ops[0x9F] = opcode_traits::make<sbc_aa>();
			ops[0xA0] = opcode_traits::make<and_r8<&registers::b>>();
			ops[0xA1] = opcode_traits::make<and_r8<&registers::c>>();
			ops[0xA2] = opcode_traits::make<and_r8<&registers::d>>();
			ops[0xA3] = opcode_traits::make<and_r8<&registers::e>>();
			ops[0xA4] = opcode_traits::make<and_r8<&registers::h>>();
			ops[0xA5] = opcode_traits::make<and_r8<&registers::l>>();
			ops[0xA6] = opcode_traits::make<and_r16P<regpair_hl>>();
			ops[0xA7] = opcode_traits::make<and_aa>();
			ops[0xA8] = opcode_traits::make<xor_r8<&registers::b>>();
			ops[0xA9] = opcode_traits::make<xor_r8<&registers::c>>();
			ops[0xAA] = opcode_traits::make<xor_r8<&registers::d>>();
			ops[0xAB] = opcode_traits::make<xor_r8<&registers::e>>();
			ops[0xAC] = opcode_traits::make<xor_r8<&registers::h>>();
			ops[0xAD] = opcode_traits::make<xor_r8<&registers::l>>();
			ops[0xAE] = opcode_traits::make<xor_r16P<regpair_hl>>();
			ops[0xAF] = opcode_traits::make<xor_aa>();
			ops[0xB0] = opcode_traits::make<or_r8<&registers::b>>();
			ops[0xB1] = opcode_traits::make<or_r8<&registers::c>>();
			ops[0xB2] = opcode_traits::make<or_r8<&registers::d>>();
			ops[0xB3] = opcode_traits::make<or_r8<&registers::e>>();
			ops[0xB4] = opcode_traits::make<or_r8<&registers::h>>();
			ops[0xB5] = opcode_traits::make<or_r8<&registers::l>>();
			ops[0xB6] = opcode_traits::make<or_r16P<regpair_hl>>();
			ops[0xB7] = opcode_traits::make<or_aa>();
			ops[0xB8] = opcode_traits::make<cp_r8<&registers::b>>();
			ops[0xB9] = opcode_traits::make<cp_r8<&registers::c>>();
			ops[0xBA] = opcode_traits::make<cp_r8<&registers::d>>();
			ops[0xBB] = opcode_traits::make<cp_r8<&registers::e>>();
			ops[0xBC] = opcode_traits::make<cp_r8<&registers::h>>();
			ops[0xBD] = opcode_traits::make<cp_r8<&registers::l>>();
			ops[0xBE] = opcode_traits::make<cp_r16P<regpair_hl>>();
			ops[0xBF] = opcode_traits::make<cp_aa>();
			ops[0xC0] = opcode_traits::make<ret_cc<jump_condition::NZ>>();
			ops[0xC1] = opcode_traits::make<pop_r16<regpair_bc>>();
			ops[0xC2] = opcode_traits::make<jp_cc_n16<jump_condition::NZ>>();
			ops[0xC3] = opcode_traits::make<jp_n16>();
			ops[0xC4] = opcode_traits::make<call_cc_n16<jump_condition::NZ>>();
			ops[0xC5] = opcode_traits::make<push_r16<regpair_bc>>();
			ops[0xC6] = opcode_traits::make<add_n8>();
			ops[0xC7] = opcode_traits::make<rst_n8<0x00>>();
			ops[0xC8] = opcode_traits::make<ret_cc<jump_condition::Z>>();
			ops[0xC9] = opcode_traits::make<ret>();
			ops[0xCA] = opcode_traits::make<jp_cc_n16<jump_condition::Z>>();
			ops[0xCB] = opcode_traits::make<prefix>();
			ops[0xCC] = opcode_traits::make<call_cc_n16<jump_condition::Z>>();
			ops[0xCD] = opcode_traits::make<call_n16>();
			ops[0xCE] = opcode_traits::make<adc_n8>();
			ops[0xCF] = opcode_traits::make<rst_n8<0x08>>();
			ops[0xD0] = opcode_traits::make<ret_cc<jump_condition::NC>>();
			ops[0xD1] = opcode_traits::make<pop_r16<regpair_de>>();
			ops[0xD2] = opcode_traits::make<jp_cc_n16<jump_condition::NC>>();
			ops[0xD3] = opcode_traits::make<illegal>();
			ops[0xD4] = opcode_traits::make<call_cc_n16<jump_condition::NC>>();
			ops[0xD5] = opcode_traits::make<push_r16<regpair_de>>();
			ops[0xD6] = opcode_traits::make<sub_n8>();
			ops[0xD7] = opcode_traits::make<rst_n8<0x10>>();
			ops[0xD8] = opcode_traits::make<ret_cc<jump_condition::C>>();
			ops[0xD9] = opcode_traits::make<reti>();
			ops[0xDA] = opcode_traits::make<jp_cc_n16<jump_condition::C>>();
			ops[0xDB] = opcode_traits::make<illegal>();
			ops[0xDC] = opcode_traits::make<call_cc_n16<jump_condition::C>>();
			ops[0xDD] = opcode_traits::make<illegal>();
			ops[0xDE] = opcode_traits::make<sbc_n8>();
			ops[0xDF] = opcode_traits::make<rst_n8<0x18>>();
			ops[0xE0] = opcode_traits::make<ldh_n8P_r8<&registers::a>>();
			ops[0xE1] = opcode_traits::make<pop_r16<regpair_hl>>();
			ops[0xE2] = opcode_traits::make<ldh_r8P_r8<&registers::c, &registers::a>>();
			ops[0xE3] = opcode_traits::make<isr<0x40>>();
			ops[0xE4] = opcode_traits::make<isr<0x48>>();
			ops[0xE5] = opcode_traits::make<push_r16<regpair_hl>>();
			ops[0xE6] = opcode_traits::make<and_n8>();
			ops[0xE7] = opcode_traits::make<rst_n8<0x20>>();
			ops[0xE8] = opcode_traits::make<add_sp_n8>();
			ops[0xE9] = opcode_traits::make<jp_hl>();
			ops[0xEA] = opcode_traits::make<ld_n16P_r8<&registers::a>>();
			ops[0xEB] = opcode_traits::make<isr<0x50>>();
			ops[0xEC] = opcode_traits::make<isr<0x58>>();
			ops[0xED] = opcode_traits::make<isr<0x60>>();
			ops[0xEE] = opcode_traits::make<xor_n8>();
			ops[0xEF] = opcode_traits::make<rst_n8<0x28>>();
			ops[0xF0] = opcode_traits::make<ldh_r8_n8P<&registers::a>>();
			ops[0xF1] = opcode_traits::make<pop_r16<regpair_af>>();
			ops[0xF2] = opcode_traits::make<ldh_r8_r8P<&registers::a, &registers::c>>();
			ops[0xF3] = opcode_traits::make<di>();
			ops[0xF4] = opcode_traits::make<illegal>();
			ops[0xF5] = opcode_traits::make<push_r16<regpair_af>>();
			ops[0xF6] = opcode_traits::make<or_n8>();
			ops[0xF7] = opcode_traits::make<rst_n8<0x30>>();
			ops[0xF8] = opcode_traits::make<ld_hl_sp_n8>();
			ops[0xF9] = opcode_traits::make<ld_sp_hl>();
			ops[0xFA] = opcode_traits::make<ld_r8_n16P<&registers::a>>();
			ops[0xFB] = opcode_traits::make<ei>();
			ops[0xFC] = opcode_traits::make<illegal>();
			ops[0xFD] = opcode_traits::make<illegal>();
			ops[0xFE] = opcode_traits::make<cp_n8>();
			ops[0xFF] = opcode_traits::make<rst_n8<0x38>>();
			ops[0x100] = opcode_traits::make<rlc_r8<&registers::b>>();
			ops[0x101] = opcode_traits::make<rlc_r8<&registers::c>>();
			ops[0x102] = opcode_traits::make<rlc_r8<&registers::d>>();
			ops[0x103] = opcode_traits::make<rlc_r8<&registers::e>>();
			ops[0x104] = opcode_traits::make<rlc_r8<&registers::h>>();
			ops[0x105] = opcode_traits::make<rlc_r8<&registers::l>>();
			ops[0x106] = opcode_traits::make<rlc_r16P<regpair_hl>>();
			ops[0x107] = opcode_traits::make<rlc_r8<&registers::a>>();
			ops[0x108] = opcode_traits::make<rrc_r8<&registers::b>>();
			ops[0x109] = opcode_traits::make<rrc_r8<&registers::c>>();
			ops[0x10A] = opcode_traits::make<rrc_r8<&registers::d>>();
			ops[0x10B] = opcode_traits::make<rrc_r8<&registers::e>>();
			ops[0x10C] = opcode_traits::make<rrc_r8<&registers::h>>();
			ops[0x10D] = opcode_traits::make<rrc_r8<&registers::l>>();
			ops[0x10E] = opcode_traits::make<rrc_r16P<regpair_hl>>();
			ops[0x10F] = opcode_traits::make<rrc_r8<&registers::a>>();
			ops[0x110] = opcode_traits::make<rl_r8<&registers::b>>();
			ops[0x111] = opcode_traits::make<rl_r8<&registers::c>>();
			ops[0x112] = opcode_traits::make<rl_r8<&registers::d>>();
			ops[0x113] = opcode_traits::make<rl_r8<&registers::e>>();
			ops[0x114] = opcode_traits::make<rl_r8<&registers::h>>();
			ops[0x115] = opcode_traits::make<rl_r8<&registers::l>>();
			ops[0x116] = opcode_traits::make<rl_r16P<regpair_hl>>();
			ops[0x117] = opcode_traits::make<rl_r8<&registers::a>>();
			ops[0x118] = opcode_traits::make<rr_r8<&registers::b>>();
			ops[0x119] = opcode_traits::make<rr_r8<&registers::c>>();
			ops[0x11A] = opcode_traits::make<rr_r8<&registers::d>>();
			ops[0x11B] = opcode_traits::make<rr_r8<&registers::e>>();
			ops[0x11C] = opcode_traits::make<rr_r8<&registers::h>>();
			ops[0x11D] = opcode_traits::make<rr_r8<&registers::l>>();
			ops[0x11E] = opcode_traits::make<rr_r16P<regpair_hl>>();
			ops[0x11F] = opcode_traits::make<rr_r8<&registers::a>>();
			ops[0x120] = opcode_traits::make<sla_r8<&registers::b>>();
			ops[0x121] = opcode_traits::make<sla_r8<&registers::c>>();
			ops[0x122] = opcode_traits::make<sla_r8<&registers::d>>();
			ops[0x123] = opcode_traits::make<sla_r8<&registers::e>>();
			ops[0x124] = opcode_traits::make<sla_r8<&registers::h>>();
			ops[0x125] = opcode_traits::make<sla_r8<&registers::l>>();
			ops[0x126] = opcode_traits::make<sla_r16P<regpair_hl>>();
			ops[0x127] = opcode_traits::make<sla_r8<&registers::a>>();
			ops[0x128] = opcode_traits::make<sra_r8<&registers::b>>();
			ops[0x129] = opcode_traits::make<sra_r8<&registers::c>>();
			ops[0x12A] = opcode_traits::make<sra_r8<&registers::d>>();
			ops[0x12B] = opcode_traits::make<sra_r8<&registers::e>>();
			ops[0x12C] = opcode_traits::make<sra_r8<&registers::h>>();
			ops[0x12D] = opcode_traits::make<sra_r8<&registers::l>>();
			ops[0x12E] = opcode_traits::make<sra_r16P<regpair_hl>>();
			ops[0x12F] = opcode_traits::make<sra_r8<&registers::a>>();
			ops[0x130] = opcode_traits::make<swap_r8<&registers::b>>();
			ops[0x131] = opcode_traits::make<swap_r8<&registers::c>>();
			ops[0x132] = opcode_traits::make<swap_r8<&registers::d>>();
			ops[0x133] = opcode_traits::make<swap_r8<&registers::e>>();
			ops[0x134] = opcode_traits::make<swap_r8<&registers::h>>();
			ops[0x135] = opcode_traits::make<swap_r8<&registers::l>>();
			ops[0x136] = opcode_traits::make<swap_r16P<regpair_hl>>();
			ops[0x137] = opcode_traits::make<swap_r8<&registers::a>>();
			ops[0x138] = opcode_traits::make<srl_r8<&registers::b>>();
			ops[0x139] = opcode_traits::make<srl_r8<&registers::c>>();
			ops[0x13A] = opcode_traits::make<srl_r8<&registers::d>>();
			ops[0x13B] = opcode_traits::make<srl_r8<&registers::e>>();
			ops[0x13C] = opcode_traits::make<srl_r8<&registers::h>>();
			ops[0x13D] = opcode_traits::make<srl_r8<&registers::l>>();
			ops[0x13E] = opcode_traits::make<srl_r16P<regpair_hl>>();
			ops[0x13F] = opcode_traits::make<srl_r8<&registers::a>>();
			ops[0x140] = opcode_traits::make<bit_r8<0, &registers::b>>();
			ops[0x141] = opcode_traits::make<bit_r8<0, &registers::c>>();
			ops[0x142] = opcode_traits::make<bit_r8<0, &registers::d>>();
			ops[0x143] = opcode_traits::make<bit_r8<0, &registers::e>>();
			ops[0x144] = opcode_traits::make<bit_r8<0, &registers::h>>();
			ops[0x145] = opcode_traits::make<bit_r8<0, &registers::l>>();
			ops[0x146] = opcode_traits::make<bit_r16P<0, regpair_hl>>();
			ops[0x147] = opcode_traits::make<bit_r8<0, &registers::a>>();
			ops[0x148] = opcode_traits::make<bit_r8<1, &registers::b>>();
			ops[0x149] = opcode_traits::make<bit_r8<1, &registers::c>>();
			ops[0x14A] = opcode_traits::make<bit_r8<1, &registers::d>>();
			ops[0x14B] = opcode_traits::make<bit_r8<1, &registers::e>>();
			ops[0x14C] = opcode_traits::make<bit_r8<1, &registers::h>>();
			ops[0x14D] = opcode_traits::make<bit_r8<1, &registers::l>>();
			ops[0x14E] = opcode_traits::make<bit_r16P<1, regpair_hl>>();
			ops[0x14F] = opcode_traits::make<bit_r8<1, &registers::a>>();
			ops[0x150] = opcode_traits::make<bit_r8<2, &registers::b>>();
			ops[0x151] = opcode_traits::make<bit_r8<2, &registers::c>>();
			ops[0x152] = opcode_traits::make<bit_r8<2, &registers::d>>();
			ops[0x153] = opcode_traits::make<bit_r8<2, &registers::e>>();
			ops[0x154] = opcode_traits::make<bit_r8<2, &registers::h>>();
			ops[0x155] = opcode_traits::make<bit_r8<2, &registers::l>>();
			ops[0x156] = opcode_traits::make<bit_r16P<2, regpair_hl>>();
			ops[0x157] = opcode_traits::make<bit_r8<2, &registers::a>>();
			ops[0x158] = opcode_traits::make<bit_r8<3, &registers::b>>();
			ops[0x159] = opcode_traits::make<bit_r8<3, &registers::c>>();
			ops[0x15A] = opcode_traits::make<bit_r8<3, &registers::d>>();
			ops[0x15B] = opcode_traits::make<bit_r8<3, &registers::e>>();
			ops[0x15C] = opcode_traits::make<bit_r8<3, &registers::h>>();
			ops[0x15D] = opcode_traits::make<bit_r8<3, &registers::l>>();
			ops[0x15E] = opcode_traits::make<bit_r16P<3, regpair_hl>>();
			ops[0x15F] = opcode_traits::make<bit_r8<3, &registers::a>>();
			ops[0x160] = opcode_traits::make<bit_r8<4, &registers::b>>();
			ops[0x161] = opcode_traits::make<bit_r8<4, &registers::c>>();
			ops[0x162] = opcode_traits::make<bit_r8<4, &registers::d>>();
			ops[0x163] = opcode_traits::make<bit_r8<4, &registers::e>>();
			ops[0x164] = opcode_traits::make<bit_r8<4, &registers::h>>();
			ops[0x165] = opcode_traits::make<bit_r8<4, &registers::l>>();
			ops[0x166] = opcode_traits::make<bit_r16P<4, regpair_hl>>();
			ops[0x167] = opcode_traits::make<bit_r8<4, &registers::a>>();
			ops[0x168] = opcode_traits::make<bit_r8<5, &registers::b>>();
			ops[0x169] = opcode_traits::make<bit_r8<5, &registers::c>>();
			ops[0x16A] = opcode_traits::make<bit_r8<5, &registers::d>>();
			ops[0x16B] = opcode_traits::make<bit_r8<5, &registers::e>>();
			ops[0x16C] = opcode_traits::make<bit_r8<5, &registers::h>>();
			ops[0x16D] = opcode_traits::make<bit_r8<5, &registers::l>>();
			ops[0x16E] = opcode_traits::make<bit_r16P<5, regpair_hl>>();
			ops[0x16F] = opcode_traits::make<bit_r8<5, &registers::a>>();
			ops[0x170] = opcode_traits::make<bit_r8<6, &registers::b>>();
			ops[0x171] = opcode_traits::make<bit_r8<6, &registers::c>>();
			ops[0x172] = opcode_traits::make<bit_r8<6, &registers::d>>();
			ops[0x173] = opcode_traits::make<bit_r8<6, &registers::e>>();
			ops[0x174] = opcode_traits::make<bit_r8<6, &registers::h>>();
			ops[0x175] = opcode_traits::make<bit_r8<6, &registers::l>>();
			ops[0x176] = opcode_traits::make<bit_r16P<6, regpair_hl>>();
			ops[0x177] = opcode_traits::make<bit_r8<6, &registers::a>>();
			ops[0x178] = opcode_traits::make<bit_r8<7, &registers::b>>();
			ops[0x179] = opcode_traits::make<bit_r8<7, &registers::c>>();
			ops[0x17A] = opcode_traits::make<bit_r8<7, &registers::d>>();
			ops[0x17B] = opcode_traits::make<bit_r8<7, &registers::e>>();
			ops[0x17C] = opcode_traits::make<bit_r8<7, &registers::h>>();
			ops[0x17D] = opcode_traits::make<bit_r8<7, &registers::l>>();
			ops[0x17E] = opcode_traits::make<bit_r16P<7, regpair_hl>>();
			ops[0x17F] = opcode_traits::make<bit_r8<7, &registers::a>>();
			ops[0x180] = opcode_traits::make<res_r8<0, &registers::b>>();
			ops[0x181] = opcode_traits::make<res_r8<0, &registers::c>>();
			ops[0x182] = opcode_traits::make<res_r8<0, &registers::d>>();
			ops[0x183] = opcode_traits::make<res_r8<0, &registers::e>>();
			ops[0x184] = opcode_traits::make<res_r8<0, &registers::h>>();
			ops[0x185] = opcode_traits::make<res_r8<0, &registers::l>>();
			ops[0x186] = opcode_traits::make<res_r16P<0, regpair_hl>>();
			ops[0x187] = opcode_traits::make<res_r8<0, &registers::a>>();
			ops[0x188] = opcode_traits::make<res_r8<1, &registers::b>>();
			ops[0x189] = opcode_traits::make<res_r8<1, &registers::c>>();
			ops[0x18A] = opcode_traits::make<res_r8<1, &registers::d>>();
			ops[0x18B] = opcode_traits::make<res_r8<1, &registers::e>>();
			ops[0x18C] = opcode_traits::make<res_r8<1, &registers::h>>();
			ops[0x18D] = opcode_traits::make<res_r8<1, &registers::l>>();
			ops[0x18E] = opcode_traits::make<res_r16P<1, regpair_hl>>();
			ops[0x18F] = opcode_traits::make<res_r8<1, &registers::a>>();
			ops[0x190] = opcode_traits::make<res_r8<2, &registers::b>>();
			ops[0x191] = opcode_traits::make<res_r8<2, &registers::c>>();
			ops[0x192] = opcode_traits::make<res_r8<2, &registers::d>>();
			ops[0x193] = opcode_traits::make<res_r8<2, &registers::e>>();
			ops[0x194] = opcode_traits::make<res_r8<2, &registers::h>>();
			ops[0x195] = opcode_traits::make<res_r8<2, &registers::l>>();
			ops[0x196] = opcode_traits::make<res_r16P<2, regpair_hl>>();
			ops[0x197] = opcode_traits::make<res_r8<2, &registers::a>>();
			ops[0x198] = opcode_traits::make<res_r8<3, &registers::b>>();
			ops[0x199] = opcode_traits::make<res_r8<3, &registers::c>>();
			ops[0x19A] = opcode_traits::make<res_r8<3, &registers::d>>();
			ops[0x19B] = opcode_traits::make<res_r8<3, &registers::e>>();
			ops[0x19C] = opcode_traits::make<res_r8<3, &registers::h>>();
			ops[0x19D] = opcode_traits::make<res_r8<3, &registers::l>>();
			ops[0x19E] = opcode_traits::make<res_r16P<3, regpair_hl>>();
			ops[0x19F] = opcode_traits::make<res_r8<3, &registers::a>>();
			ops[0x1A0] = opcode_traits::make<res_r8<4, &registers::b>>();
			ops[0x1A1] = opcode_traits::make<res_r8<4, &registers::c>>();
			ops[0x1A2] = opcode_traits::make<res_r8<4, &registers::d>>();
			ops[0x1A3] = opcode_traits::make<res_r8<4, &registers::e>>();
			ops[0x1A4] = opcode_traits::make<res_r8<4, &registers::h>>();
			ops[0x1A5] = opcode_traits::make<res_r8<4, &registers::l>>();
			ops[0x1A6] = opcode_traits::make<res_r16P<4, regpair_hl>>();
			ops[0x1A7] = opcode_traits::make<res_r8<4, &registers::a>>();
			ops[0x1A8] = opcode_traits::make<res_r8<5, &registers::b>>();
			ops[0x1A9] = opcode_traits::make<res_r8<5, &registers::c>>();
			ops[0x1AA] = opcode_traits::make<res_r8<5, &registers::d>>();
			ops[0x1AB] = opcode_traits::make<res_r8<5, &registers::e>>();
			ops[0x1AC] = opcode_traits::make<res_r8<5, &registers::h>>();
			ops[0x1AD] = opcode_traits::make<res_r8<5, &registers::l>>();
			ops[0x1AE] = opcode_traits::make<res_r16P<5, regpair_hl>>();
			ops[0x1AF] = opcode_traits::make<res_r8<5, &registers::a>>();
			ops[0x1B0] = opcode_traits::make<res_r8<6, &registers::b>>();
			ops[0x1B1] = opcode_traits::make<res_r8<6, &registers::c>>();
			ops[0x1B2] = opcode_traits::make<res_r8<6, &registers::d>>();
			ops[0x1B3] = opcode_traits::make<res_r8<6, &registers::e>>();
			ops[0x1B4] = opcode_traits::make<res_r8<6, &registers::h>>();
			ops[0x1B5] = opcode_traits::make<res_r8<6, &registers::l>>();
			ops[0x1B6] = opcode_traits::make<res_r16P<6, regpair_hl>>();
			ops[0x1B7] = opcode_traits::make<res_r8<6, &registers::a>>();
			ops[0x1B8] = opcode_traits::make<res_r8<7, &registers::b>>();
			ops[0x1B9] = opcode_traits::make<res_r8<7, &registers::c>>();
			ops[0x1BA] = opcode_traits::make<res_r8<7, &registers::d>>();
			ops[0x1BB] = opcode_traits::make<res_r8<7, &registers::e>>();
			ops[0x1BC] = opcode_traits::make<res_r8<7, &registers::h>>();
			ops[0x1BD] = opcode_traits::make<res_r8<7, &registers::l>>();
			ops[0x1BE] = opcode_traits::make<res_r16P<7, regpair_hl>>();
			ops[0x1BF] = opcode_traits::make<res_r8<7, &registers::a>>();
			ops[0x1C0] = opcode_traits::make<set_r8<0, &registers::b>>();
			ops[0x1C1] = opcode_traits::make<set_r8<0, &registers::c>>();
			ops[0x1C2] = opcode_traits::make<set_r8<0, &registers::d>>();
			ops[0x1C3] = opcode_traits::make<set_r8<0, &registers::e>>();
			ops[0x1C4] = opcode_traits::make<set_r8<0, &registers::h>>();
			ops[0x1C5] = opcode_traits::make<set_r8<0, &registers::l>>();
			ops[0x1C6] = opcode_traits::make<set_r16P<0, regpair_hl>>();
			ops[0x1C7] = opcode_traits::make<set_r8<0, &registers::a>>();
			ops[0x1C8] = opcode_traits::make<set_r8<1, &registers::b>>();
			ops[0x1C9] = opcode_traits::make<set_r8<1, &registers::c>>();
			ops[0x1CA] = opcode_traits::make<set_r8<1, &registers::d>>();
			ops[0x1CB] = opcode_traits::make<set_r8<1, &registers::e>>();
			ops[0x1CC] = opcode_traits::make<set_r8<1, &registers::h>>();
			ops[0x1CD] = opcode_traits::make<set_r8<1, &registers::l>>();
			ops[0x1CE] = opcode_traits::make<set_r16P<1, regpair_hl>>();
			ops[0x1CF] = opcode_traits::make<set_r8<1, &registers::a>>();
			ops[0x1D0] = opcode_traits::make<set_r8<2, &registers::b>>();
			ops[0x1D1] = opcode_traits::make<set_r8<2, &registers::c>>();
			ops[0x1D2] = opcode_traits::make<set_r8<2, &registers::d>>();
			ops[0x1D3] = opcode_traits::make<set_r8<2, &registers::e>>();
			ops[0x1D4] = opcode_traits::make<set_r8<2, &registers::h>>();
			ops[0x1D5] = opcode_traits::make<set_r8<2, &registers::l>>();
			ops[0x1D6] = opcode_traits::make<set_r16P<2, regpair_hl>>();
			ops[0x1D7] = opcode_traits::make<set_r8<2, &registers::a>>();
			ops[0x1D8] = opcode_traits::make<set_r8<3, &registers::b>>();
			ops[0x1D9] = opcode_traits::make<set_r8<3, &registers::c>>();
			ops[0x1DA] = opcode_traits::make<set_r8<3, &registers::d>>();
			ops[0x1DB] = opcode_traits::make<set_r8<3, &registers::e>>();
			ops[0x1DC] = opcode_traits::make<set_r8<3, &registers::h>>();
			ops[0x1DD] = opcode_traits::make<set_r8<3, &registers::l>>();
			ops[0x1DE] = opcode_traits::make<set_r16P<3, regpair_hl>>();
			ops[0x1DF] = opcode_traits::make<set_r8<3, &registers::a>>();
			ops[0x1E0] = opcode_traits::make<set_r8<4, &registers::b>>();
			ops[0x1E1] = opcode_traits::make<set_r8<4, &registers::c>>();
			ops[0x1E2] = opcode_traits::make<set_r8<4, &registers::d>>();
			ops[0x1E3] = opcode_traits::make<set_r8<4, &registers::e>>();
			ops[0x1E4] = opcode_traits::make<set_r8<4, &registers::h>>();
			ops[0x1E5] = opcode_traits::make<set_r8<4, &registers::l>>();
			ops[0x1E6] = opcode_traits::make<set_r16P<4, regpair_hl>>();
			ops[0x1E7] = opcode_traits::make<set_r8<4, &registers::a>>();
			ops[0x1E8] = opcode_traits::make<set_r8<5, &registers::b>>();
			ops[0x1E9] = opcode_traits::make<set_r8<5, &registers::c>>();
			ops[0x1EA] = opcode_traits::make<set_r8<5, &registers::d>>();
			ops[0x1EB] = opcode_traits::make<set_r8<5, &registers::e>>();
			ops[0x1EC] = opcode_traits::make<set_r8<5, &registers::h>>();
			ops[0x1ED] = opcode_traits::make<set_r8<5, &registers::l>>();
			ops[0x1EE] = opcode_traits::make<set_r16P<5, regpair_hl>>();
			ops[0x1EF] = opcode_traits::make<set_r8<5, &registers::a>>();
			ops[0x1F0] = opcode_traits::make<set_r8<6, &registers::b>>();
			ops[0x1F1] = opcode_traits::make<set_r8<6, &registers::c>>();
			ops[0x1F2] = opcode_traits::make<set_r8<6, &registers::d>>();
			ops[0x1F3] = opcode_traits::make<set_r8<6, &registers::e>>();
			ops[0x1F4] = opcode_traits::make<set_r8<6, &registers::h>>();
			ops[0x1F5] = opcode_traits::make<set_r8<6, &registers::l>>();
			ops[0x1F6] = opcode_traits::make<set_r16P<6, regpair_hl>>();
			ops[0x1F7] = opcode_traits::make<set_r8<6, &registers::a>>();
			ops[0x1F8] = opcode_traits::make<set_r8<7, &registers::b>>();
			ops[0x1F9] = opcode_traits::make<set_r8<7, &registers::c>>();
			ops[0x1FA] = opcode_traits::make<set_r8<7, &registers::d>>();
			ops[0x1FB] = opcode_traits::make<set_r8<7, &registers::e>>();
			ops[0x1FC] = opcode_traits::make<set_r8<7, &registers::h>>();
			ops[0x1FD] = opcode_traits::make<set_r8<7, &registers::l>>();
			ops[0x1FE] = opcode_traits::make<set_r16P<7, regpair_hl>>();
			ops[0x1FF] = opcode_traits::make<set_r8<7, &registers::a>>();

			return ops;
		}();

		namespace common {
			constexpr next_stage_t prefetch(OPCODE_ARGS) noexcept {
				reg.ir = mem->read(reg.pc);
				reg.pc++;
				return fetch_opcode(reg, mem);
			}

			constexpr next_stage_t prefetch_and_reset(OPCODE_ARGS) noexcept {
				reg.mupc = 0;
				return prefetch(reg, mem);
			}

			constexpr next_stage_t fetch_opcode(OPCODE_ARGS) noexcept {
				return opcodes::map[reg.ir].entry;
			}
		}
	}
}