#pragma once

#include <string_view>
#include <array>
#include <bit>
#include <utility>

#include <yahbog/registers.h>
#include <yahbog/mmu.h>
#include <yahbog/utility/constexpr_function.h>

namespace yahbog {

    using read_fn_t = yahbog::constexpr_function<uint8_t(uint16_t)>;
    using write_fn_t = yahbog::constexpr_function<void(uint16_t, uint8_t)>;

    struct mem_fns_t {
        read_fn_t* read_ = nullptr;
        write_fn_t* write_ = nullptr;

        constexpr auto read(std::uint16_t addr) const noexcept {
            return (*read_)(addr);
        }

        constexpr void write(std::uint16_t addr, std::uint8_t value) const noexcept {
            (*write_)(addr, value);
        }
    }; 

    #define OPCODE_ARGS yahbog::registers& reg, [[maybe_unused]] yahbog::mem_fns_t* mem

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

        constexpr std::uint16_t operator()(yahbog::registers& reg) const {
            return (reg.*r1 << 8) | reg.*r2;
        }
        constexpr void operator()(yahbog::registers& reg, std::uint16_t val) const {
            reg.*r1 = val >> 8;
            reg.*r2 = val & 0xFF;
        }

        constexpr bool operator==(const regpair_t& other) const {
            return r1 == other.r1 && r2 == other.r2;
        }
    };

    constexpr auto regpair_af = regpair_t{ &yahbog::registers::a, &yahbog::registers::f };
    constexpr auto regpair_bc = regpair_t{ &yahbog::registers::b, &yahbog::registers::c };
    constexpr auto regpair_de = regpair_t{ &yahbog::registers::d, &yahbog::registers::e };
    constexpr auto regpair_hl = regpair_t{ &yahbog::registers::h, &yahbog::registers::l };

    namespace opcodes {

        struct info {
            opcode_fn fn;
            std::string_view name;
        };

        constexpr void adc_aa(OPCODE_ARGS) noexcept {
            auto fc = reg.FC();

            reg.FH((reg.a & 0xF) + (reg.a & 0xF) + reg.FC() > 0xF);
            reg.FC(int(reg.a) + int(reg.a) + reg.FC() > 0xFF);
            reg.a += reg.a + fc;
            reg.FZ(reg.a == 0);
            reg.FN(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void adc_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0: {
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            }
            case 1: {
                auto fc = reg.FC();

                reg.FH(half_carries_add(reg.a, reg.z) | half_carries_add(reg.a + reg.z, reg.FC()));
                reg.FC(carries_add(reg.a, reg.z) | carries_add(reg.a + reg.z, reg.FC()));
                reg.a += reg.z + fc;

                reg.FZ(reg.a == 0);
                reg.FN(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void add_aa(OPCODE_ARGS) noexcept {
            reg.FH(half_carries_add(reg.a, reg.a));
            reg.FC(carries_add(reg.a, reg.a));
            reg.a += reg.a;
            reg.FZ(reg.a == 0);

            reg.FN(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void add_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.FH(half_carries_add(reg.a, reg.z));
                reg.FC(carries_add(reg.a, reg.z));
                reg.a += reg.z;
                reg.FZ(reg.a == 0);
                reg.FN(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void and_aa(OPCODE_ARGS) noexcept {
            reg.FZ(reg.a == 0);
            reg.FN(0); reg.FH(1); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void and_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.a &= reg.z;
                reg.FZ(reg.a == 0);
                reg.FN(0); reg.FH(1); reg.FC(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void call_n16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 2;
                return;
            case 2:
                reg.sp -= 1;
                reg.mupc = 3;
                return;
            case 3:
                mem->write(reg.sp, reg.pc >> 8);
                reg.sp--;
                reg.mupc = 4;
                return;
            case 4:
                mem->write(reg.sp, reg.pc & 0xFF);
                reg.pc = reg.wz();
                reg.mupc = 5;
                return;
            case 5:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void ccf(OPCODE_ARGS) noexcept {
            reg.FC(!reg.FC());
            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void cp_aa(OPCODE_ARGS) noexcept {
            reg.FZ(1); reg.FN(1); reg.FH(0); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void cp_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.FZ(reg.a == reg.z);
                reg.FH(half_carries_sub(reg.a, reg.z));
                reg.FC(carries_sub(reg.a, reg.z));
                reg.FN(1);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void cpl(OPCODE_ARGS) noexcept {
            reg.a = ~reg.a;
            reg.FN(1); reg.FH(1);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void daa(OPCODE_ARGS) noexcept {
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

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void di(OPCODE_ARGS) noexcept {
            reg.ime = 0;

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void ei(OPCODE_ARGS) noexcept {
            reg.ie = 1;

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void halt(OPCODE_ARGS) noexcept {
            reg.halted = 1;

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void illegal(OPCODE_ARGS) noexcept {
            // no-op

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void jp_n16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 2;
                return;
            case 2:
                reg.pc = reg.wz();
                reg.mupc = 3;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void jr_n8(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.wz(reg.pc + std::bit_cast<std::int8_t>(reg.z));
                reg.mupc = 2;
                return;
            case 2:
                reg.ir = mem->read(reg.wz());
                reg.pc = reg.wz() + 1;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void nop(OPCODE_ARGS) noexcept {
            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void or_aa(OPCODE_ARGS) noexcept {
            reg.FZ(reg.a == 0);
            reg.FN(0); reg.FH(0); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void or_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.a |= reg.z;
                reg.FZ(reg.a == 0);
                reg.FN(0); reg.FH(0); reg.FC(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void prefix(OPCODE_ARGS) noexcept {
            reg.ir = mem->read(reg.pc) + 0x100;
            reg.pc++;
        }

        constexpr void ret(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.sp);
                reg.sp++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.sp);
                reg.sp++;
                reg.mupc = 2;
                return;
            case 2:
                reg.pc = reg.wz();
                reg.mupc = 3;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void reti(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.sp);
                reg.sp++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.sp);
                reg.sp++;
                reg.mupc = 2;
                return;
            case 2:
                reg.pc = reg.wz();
                reg.ime = 1;
                reg.mupc = 3;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void rla(OPCODE_ARGS) noexcept {
            auto carry = reg.FC();
            reg.FC(reg.a >> 7);
            reg.a = (reg.a << 1) | carry;

            reg.FZ(0); reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void rlca(OPCODE_ARGS) noexcept {
            reg.FC(reg.a >> 7);
            reg.a = (reg.a << 1) | reg.FC();

            reg.FZ(0); reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void rra(OPCODE_ARGS) noexcept {
            auto carry = reg.FC();
            reg.FC(reg.a & 1);
            reg.a = (reg.a >> 1) | (carry << 7);

            reg.FZ(0); reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void rrca(OPCODE_ARGS) noexcept {
            reg.FC(reg.a & 1);
            reg.a = (reg.a >> 1) | (reg.FC() << 7);

            reg.FZ(0); reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void sbc_aa(OPCODE_ARGS) noexcept {
            int res = (reg.a & 0x0F) - (reg.a & 0x0F) - reg.FC();
            int op = reg.a + reg.FC();

            reg.FH(res < 0);
            reg.FC(op > reg.a);
            reg.FN(1);

            reg.a -= op;

            reg.FZ(reg.a == 0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void sbc_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0: {
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            }
            case 1: {
                int res = (reg.a & 0x0F) - (reg.z & 0x0F) - reg.FC();
                int op = reg.z + reg.FC();

                reg.FH(res < 0);
                reg.FC(op > reg.a);
                reg.FN(1);

                reg.a -= op;

                reg.FZ(reg.a == 0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void scf(OPCODE_ARGS) noexcept {
            reg.FN(0); reg.FH(0); reg.FC(1);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void stop_n8(OPCODE_ARGS) noexcept {
            reg.pc++;
        }

        constexpr void sub_aa(OPCODE_ARGS) noexcept {
            reg.a = 0;
            reg.FZ(1); reg.FN(1); reg.FH(0); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void sub_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0: {
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            }
            case 1: {
                auto op = reg.z;
                reg.FZ(reg.a == op);
                reg.FH(half_carries_sub(reg.a, op));
                reg.FC(carries_sub(reg.a, op));
                reg.FN(1);
                reg.a -= op;

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void xor_aa(OPCODE_ARGS) noexcept {
            reg.a = 0;
            reg.FZ(1); reg.FN(0); reg.FH(0); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        constexpr void xor_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.a ^= reg.z;
                reg.FZ(reg.a == 0);
                reg.FN(0); reg.FH(0); reg.FC(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<int op1, regpair_t op2>
        constexpr void bit_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op2(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto bit = 1 << op1;

                reg.FZ(!(reg.z & bit));
                reg.FN(0); reg.FH(1);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<int op1, regpair_t op2>
        constexpr void res_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op2(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto bit = 1 << op1;
                reg.z &= ~bit;
                mem->write(op2(reg), reg.z);

                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<int op1, regpair_t op2>
        constexpr void set_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op2(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto bit = 1 << op1;
                reg.z |= bit;
                mem->write(op2(reg), reg.z);

                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<int op1, r8_ptr op2>
        constexpr void bit_r8(OPCODE_ARGS) noexcept {
            auto val = reg.*op2;
            auto bit = 1 << op1;

            reg.FZ(!(val & bit));
            reg.FN(0); reg.FH(1);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<int op1, r8_ptr op2>
        constexpr void res_r8(OPCODE_ARGS) noexcept {
            auto val = reg.*op2;
            auto bit = 1 << op1;

            val &= ~bit;
            reg.*op2 = val;

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<int op1, r8_ptr op2>
        constexpr void set_r8(OPCODE_ARGS) noexcept {
            auto val = reg.*op2;
            auto bit = 1 << op1;

            val |= bit;
            reg.*op2 = val;

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<int op1>
        constexpr void rst_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.sp--;
                reg.mupc = 1;
                return;
            case 1:
                mem->write(reg.sp, reg.pc >> 8);
                reg.sp--;
                reg.mupc = 2;
                return;
            case 2:
                mem->write(reg.sp, reg.pc & 0xFF);
                reg.pc = op1;
                reg.mupc = 3;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<jump_condition op1>
        constexpr void call_cc_n16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;

                if (!jump_condition_met<op1>(reg, mem)) {
                    reg.mupc = 2;
                }
                else {
                    reg.mupc = 3;
                }
                return;
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            case 3:
                reg.sp -= 1;
                reg.mupc = 4;
                return;
            case 4:
                mem->write(reg.sp, reg.pc >> 8);
                reg.sp--;
                reg.mupc = 5;
                return;
            case 5:
                mem->write(reg.sp, reg.pc & 0xFF);
                reg.pc = reg.wz();
                reg.mupc = 2;
                return;
            default:
                std::unreachable();
            }

            std::unreachable();
        }

        template<jump_condition op1>
        constexpr void jp_cc_n16(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;

                if (!jump_condition_met<op1>(reg, mem)) {
                    reg.mupc = 3;
                }
                else {
                    reg.mupc = 2;
                }

                return;
            case 2:
                reg.pc = reg.wz();
                reg.mupc = 3;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<jump_condition op1>
        constexpr void jr_cc_n8(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;

                if (!jump_condition_met<op1>(reg, mem)) {
                    reg.mupc = 3;
                }
                else {
                    reg.mupc = 1;
                }

                return;
            case 1:
                // no-op
                reg.mupc = 2;
                return;
            case 2:
                reg.wz(reg.pc + std::bit_cast<std::int8_t>(reg.z));
                reg.ir = mem->read(reg.wz());
                reg.pc = reg.wz() + 1;
                reg.mupc = 0;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<jump_condition op1>
        constexpr void ret_cc(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                if (!jump_condition_met<op1>(reg, mem)) {
                    reg.mupc = 4;
                }
                else {
                    reg.mupc = 1;
                }
                return;

            case 1:
                reg.z = mem->read(reg.sp);
                reg.sp++;
                reg.mupc = 2;
                return;
            case 2:
                reg.w = mem->read(reg.sp);
                reg.sp++;
                reg.mupc = 3;
                return;
            case 3:
                reg.pc = reg.wz();
                reg.mupc = 4;
                return;
            case 4:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();

        }

        template<regpair_t op1, regpair_t op2>
        constexpr void add_r16_r16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                // no-op
                reg.mupc = 1;
                return;
            case 1: {
                auto v1 = op1(reg);
                auto v2 = op2(reg);

                reg.FH(half_carries_add(v1, v2));
                reg.FC(carries_add(v1, v2));

                op1(reg, op1(reg) + v2);

                reg.FN(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void add_hl_sp(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                // no-op
                reg.mupc = 1;
                return;
            case 1: {
                auto v1 = reg.hl();
                auto v2 = reg.sp;

                reg.FH(half_carries_add(v1, v2));
                reg.FC(carries_add(v1, v2));

                reg.hl(reg.hl() + v2);

                reg.FN(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void ld_sp_hl(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                // no-op
                reg.mupc = 1;
                return;
            case 1:
                reg.sp = reg.hl();

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void ld_hl_sp_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                // no-op
                reg.mupc = 2;
                return;
            case 2: {
                int offset = std::bit_cast<std::int8_t>(reg.z);

                reg.FC((reg.sp & 0xFF) + (offset & 0xFF) > 0xFF);
                reg.FH((reg.sp & 0xF) + (offset & 0xF) > 0xF);

                reg.hl(reg.sp + offset);

                reg.FZ(0); reg.FN(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1, r8_ptr op2>
        constexpr void ld_r16PD_r8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                mem->write(op1(reg), reg.*op2);
                op1(reg, op1(reg) - 1);
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1, r8_ptr op2>
        constexpr void ld_r16PI_r8(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                mem->write(op1(reg), reg.*op2);
                op1(reg, op1(reg) + 1);
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1, r8_ptr op2>
        constexpr void ld_r16P_r8(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                mem->write(op1(reg), reg.*op2);
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void adc_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto fc = reg.FC();

                reg.FH((reg.a & 0xF) + (reg.z & 0xF) + reg.FC() > 0xF);
                reg.FC(int(reg.a) + int(reg.z) + reg.FC() > 0xFF);

                reg.a += reg.z + fc;

                reg.FZ(reg.a == 0);
                reg.FN(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void add_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1:

                reg.FH(half_carries_add(reg.a, reg.z));
                reg.FC(carries_add(reg.a, reg.z));

                reg.a += reg.z;
                reg.FZ(reg.a == 0);

                reg.FN(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void add_sp_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                // no-op
                reg.mupc = 2;
                return;
            case 2: {

                int val = std::bit_cast<std::int8_t>(reg.z);
                int regval = reg.sp;

                reg.FH((regval & 0xF) + (val & 0xF) > 0xF);
                reg.FC((regval & 0xFF) + (val & 0xFF) > 0xFF);

                reg.sp += val;

                reg.FZ(0); reg.FN(0);

                reg.mupc = 3;
                return;
            }
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void and_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.a &= reg.z;
                reg.FZ(reg.a == 0);
                reg.FN(0); reg.FH(1); reg.FC(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void cp_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.FZ(reg.a == reg.z);
                reg.FH(half_carries_sub(reg.a, reg.z));
                reg.FC(carries_sub(reg.a, reg.z));
                reg.FN(1);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void dec_r16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                op1(reg, op1(reg) - 1);
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void dec_sp(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                --reg.sp;
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void dec_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.FH(half_carries_sub(reg.z, 1));
                reg.FZ(reg.z == 1);
                reg.FN(1);

                mem->write(op1(reg), reg.z - 1);
                reg.mupc = 2;
                return;
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void inc_r16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                op1(reg, op1(reg) + 1);
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void inc_sp(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                ++reg.sp;
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void inc_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.FH(half_carries_add(reg.z, 1));
                reg.FZ(reg.z == 0xFF);

                mem->write(op1(reg), reg.z + 1);
                reg.FN(0);

                reg.mupc = 2;
                return;
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void jp_hl(OPCODE_ARGS) noexcept {
            reg.ir = mem->read(reg.hl());
            reg.pc = reg.hl() + 1;
        }

        template<r16_ptr op1>
        constexpr void ld_n16P_r16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 2;
                return;
            case 2:
                mem->write(reg.wz(), reg.*op1 & 0xFF);
                reg.wz(reg.wz() + 1);
                reg.mupc = 3;
                return;
            case 3:
                mem->write(reg.wz(), reg.*op1 >> 8);
                reg.mupc = 4;
                return;
            case 4:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void ld_r16P_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                mem->write(op1(reg), reg.z);
                reg.mupc = 2;
                return;
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void ld_r16_n16(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 2;
                return;
            case 2:
                op1(reg, reg.wz());
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void ld_sp_n16(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 2;
                return;
            case 2:
                reg.sp = reg.wz();
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void or_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.a |= reg.z;

                reg.FZ(reg.a == 0);
                reg.FN(0); reg.FH(0); reg.FC(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void pop_r16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                if constexpr(op1 == regpair_af) {
                    reg.z = mem->read(reg.sp) & 0xF0;
                } else {
                    reg.z = mem->read(reg.sp);
                }

                reg.sp++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.sp);
                reg.sp++;
                reg.mupc = 2;
                return;
            case 2:
                op1(reg, reg.wz());
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void push_r16(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.sp--;
                reg.mupc = 1;
                return;
            case 1:
                mem->write(reg.sp, op1(reg) >> 8);
                reg.sp--;
                reg.mupc = 2;
                return;
            case 2:
                mem->write(reg.sp, op1(reg) & 0xFF);
                reg.mupc = 3;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void rl_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;
                auto carry = reg.FC();

                reg.FC(val >> 7);
                val = (val << 1) | carry;

                mem->write(op1(reg), val);

                reg.FZ(val == 0);
                reg.FN(0); reg.FH(0);
                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void rlc_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;

                reg.FC(val >> 7);
                val = (val << 1) | reg.FC();

                reg.FZ(val == 0);
                reg.FN(0); reg.FH(0);

                mem->write(op1(reg), val);
                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void rr_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto carry = reg.FC();
                auto val = reg.z;

                reg.FC(val & 1);
                val = (val >> 1) | (carry << 7);

                mem->write(op1(reg), val);

                reg.FZ(val == 0);
                reg.FN(0); reg.FH(0);
                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void rrc_r16P(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                reg.FC(reg.z & 1);
                reg.z = (reg.z >> 1) | (reg.FC() << 7);
                reg.FZ(reg.z == 0);
                reg.FN(0); reg.FH(0);

                mem->write(op1(reg), reg.z);

                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }
        }

        template<regpair_t op1>
        constexpr void sbc_r16P(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;

                int res = (reg.a & 0x0F) - (val & 0x0F) - reg.FC();
                int op = val + reg.FC();

                reg.FH(res < 0);
                reg.FC(op > reg.a);
                reg.FN(1);

                reg.a -= op;

                reg.FZ(reg.a == 0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }

            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void sla_r16P(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;

                reg.FC(val >> 7);
                val <<= 1;
                reg.FZ(val == 0);
                reg.FN(0); reg.FH(0);
                mem->write(op1(reg), val);

                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void sra_r16P(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;

                reg.FC(val & 1);
                val = (val & 0x80) | (val >> 1);

                mem->write(op1(reg), val);

                reg.FZ(val == 0);
                reg.FN(0); reg.FH(0);

                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void srl_r16P(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;

                reg.FC(val & 1);
                val >>= 1;

                mem->write(op1(reg), val);

                reg.FZ(val == 0);
                reg.FN(0); reg.FH(0);

                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void sub_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;
                reg.FZ(reg.a == val);
                reg.FN(1);
                reg.FH(half_carries_sub(reg.a, val));
                reg.FC(carries_sub(reg.a, val));

                reg.a -= val;

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            }
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void swap_r16P(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1: {
                auto val = reg.z;

                val = (val >> 4) | (val << 4);

                mem->write(op1(reg), val);


                reg.FZ(val == 0);
                reg.FN(0); reg.FH(0); reg.FC(0);

                reg.mupc = 2;
                return;
            }
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<regpair_t op1>
        constexpr void xor_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op1(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.a ^= reg.z;
                reg.FZ(reg.a == 0);
                reg.FN(0); reg.FH(0); reg.FC(0);

                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1, regpair_t op2>
        constexpr void ld_r8_r16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op2(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.*op1 = reg.z;
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1, regpair_t op2>
        constexpr void ld_r8_r16PD(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(op2(reg));
                reg.mupc = 1;
                return;
            case 1:
                reg.*op1 = reg.z;
                op2(reg, op2(reg) - 1);
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr void ld_a_hlp(OPCODE_ARGS) noexcept {
            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.hl());
                reg.hl(reg.hl() + 1);
                reg.mupc = 1;
                return;
            case 1:
                reg.a = reg.z;
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1, r8_ptr op2>
        constexpr void ld_r8_r8(OPCODE_ARGS) noexcept {
            reg.*op1 = reg.*op2;

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1, r8_ptr op2>
        constexpr void ldh_r8P_r8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                mem->write(0xFF00 + reg.*op1, reg.*op2);
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1, r8_ptr op2>
        constexpr void ldh_r8_r8P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.*op1 = mem->read(0xFF00 + reg.*op2);
                reg.mupc = 1;
                return;
            case 1:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1>
        constexpr void adc_r8(OPCODE_ARGS) noexcept {
            std::uint8_t op = reg.FC();

            reg.FH((reg.a & 0xF) + (reg.*op1 & 0xF) + reg.FC() > 0xF);
            reg.FC(int(reg.a) + int(reg.*op1) + reg.FC() > 0xFF);
            reg.a += reg.*op1;
            reg.a += op;

            reg.FZ(reg.a == 0);

            reg.FN(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void add_r8(OPCODE_ARGS) noexcept {
            reg.FH(half_carries_add(reg.a, reg.*op1));
            reg.FC(carries_add(reg.a, reg.*op1));
            reg.a += reg.*op1;
            reg.FZ(reg.a == 0);

            reg.FN(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void and_r8(OPCODE_ARGS) noexcept {
            reg.a &= reg.*op1;

            reg.FZ(reg.a == 0);
            reg.FN(0); reg.FH(1); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void cp_r8(OPCODE_ARGS) noexcept {
            auto op = reg.*op1;

            reg.FZ(reg.a == op);
            reg.FH(half_carries_sub(reg.a, op));
            reg.FC(carries_sub(reg.a, op));

            reg.FN(1);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void dec_r8(OPCODE_ARGS) noexcept {
            reg.FH(half_carries_sub(reg.*op1, 1));
            (reg.*op1)--;
            reg.FZ(reg.*op1 == 0);

            reg.FN(1);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void inc_r8(OPCODE_ARGS) noexcept {
            reg.FH(half_carries_add(reg.*op1, 1));
            (reg.*op1)++;
            reg.FZ(reg.*op1 == 0);
            reg.FN(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void jr_r8_n8(OPCODE_ARGS) noexcept {
            reg.mupc = 255;
            return;

        }

        template<r8_ptr op1>
        constexpr void ld_n16P_r8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 2;
                return;
            case 2:
                mem->write(reg.wz(), reg.*op1);
                reg.mupc = 3;
                return;
            case 3:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1>
        constexpr void ld_r8_n16P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.w = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 2;
                return;
            case 2:
                reg.z = mem->read(reg.wz());
                reg.mupc = 3;
                return;
            case 3:
                reg.*op1 = reg.z;
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1>
        constexpr void ld_r8_n8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.*op1 = reg.z;
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1>
        constexpr void ldh_n8P_r8(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                mem->write(0xFF00 + reg.z, reg.*op1);
                reg.mupc = 2;
                return;
            case 2:
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1>
        constexpr void ldh_r8_n8P(OPCODE_ARGS) noexcept {

            switch (reg.mupc) {
            case 0:
                reg.z = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 1;
                return;
            case 1:
                reg.z = mem->read(0xFF00 + reg.z);
                reg.mupc = 2;
                return;
            case 2:
                reg.*op1 = reg.z;
                reg.ir = mem->read(reg.pc);
                reg.pc++;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        template<r8_ptr op1>
        constexpr void or_r8(OPCODE_ARGS) noexcept {
            reg.a |= reg.*op1;

            reg.FZ(reg.a == 0);
            reg.FN(0); reg.FH(0); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void rl_r8(OPCODE_ARGS) noexcept {
            auto val = reg.*op1;
            auto carry = reg.FC();

            reg.FC(val >> 7);
            val = (val << 1) | carry;

            reg.*op1 = val;

            reg.FZ(val == 0);
            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void rlc_r8(OPCODE_ARGS) noexcept {
            reg.FC(reg.*op1 >> 7);
            reg.*op1 = (reg.*op1 << 1) | reg.FC();

            reg.FZ(reg.*op1 == 0);

            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void rr_r8(OPCODE_ARGS) noexcept {
            auto carry = reg.FC();

            reg.FC(reg.*op1 & 1);
            reg.*op1 = (reg.*op1 >> 1) | (carry << 7);

            reg.FZ(reg.*op1 == 0);
            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void rrc_r8(OPCODE_ARGS) noexcept {
            reg.FC(reg.*op1 & 1);
            reg.*op1 = (reg.*op1 >> 1) | (reg.FC() << 7);

            reg.FZ(reg.*op1 == 0);

            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void sbc_r8(OPCODE_ARGS) noexcept {
            int res = (reg.a & 0x0F) - (reg.*op1 & 0x0F) - reg.FC();
            int op = reg.*op1 + reg.FC();

            reg.FH(res < 0);
            reg.FC(op > reg.a);
            reg.FN(1);

            reg.a -= op;

            reg.FZ(reg.a == 0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void sla_r8(OPCODE_ARGS) noexcept {
            reg.FC(reg.*op1 >> 7);
            reg.*op1 <<= 1;
            reg.FZ(reg.*op1 == 0);

            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void sra_r8(OPCODE_ARGS) noexcept {
            reg.FC(reg.*op1 & 1);
            reg.*op1 = (reg.*op1 >> 1) | (reg.*op1 & 0x80);

            reg.FZ(reg.*op1 == 0);
            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void srl_r8(OPCODE_ARGS) noexcept {
            reg.FC(reg.*op1 & 1);
            reg.*op1 >>= 1;

            reg.FZ(reg.*op1 == 0);
            reg.FN(0); reg.FH(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void sub_r8(OPCODE_ARGS) noexcept {
            auto op = reg.*op1;

            reg.FZ(reg.a == op);
            reg.FH(half_carries_sub(reg.a, op));
            reg.FC(carries_sub(reg.a, op));

            reg.a -= op;

            reg.FN(1);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void swap_r8(OPCODE_ARGS) noexcept {
            auto upper = reg.*op1 >> 4;
            auto lower = reg.*op1 & 0xF;

            reg.*op1 = (lower << 4) | upper;
            reg.FZ(reg.*op1 == 0);

            reg.FN(0); reg.FH(0); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        template<r8_ptr op1>
        constexpr void xor_r8(OPCODE_ARGS) noexcept {
            reg.a ^= reg.*op1;

            reg.FZ(reg.a == 0);
            reg.FN(0); reg.FH(0); reg.FC(0);

            reg.ir = mem->read(reg.pc);
            reg.pc++;
        }

        // pseudo instruction for handling interrupts
        template<std::uint8_t addr>
        constexpr void isr(OPCODE_ARGS) noexcept {
            switch(reg.mupc) {
            case 0:
                reg.ime = 0;
                reg.pc--;
                reg.mupc = 1;
                return;
            case 1:
                reg.sp--;
                reg.mupc = 2;
                return;
            case 2:
                mem->write(reg.sp, reg.pc >> 8);
                reg.sp--;
                reg.mupc = 3;
                return;
            case 3:
                mem->write(reg.sp, reg.pc & 0xFF);
                reg.mupc = 4;
                return;
            case 4:
                reg.ir = mem->read(addr);
                reg.pc = addr + 1;
                reg.mupc = 0;
                return;
            default: std::unreachable();
            }

            std::unreachable();
        }

        constexpr auto map = []() {
            std::array<opcode_fn, 512> ops{};

            ops[0x00] = &nop;
            ops[0x01] = &ld_r16_n16<regpair_bc>;
            ops[0x02] = &ld_r16P_r8<regpair_bc, &registers::a>;
            ops[0x03] = &inc_r16<regpair_bc>;
            ops[0x04] = &inc_r8<&registers::b>;
            ops[0x05] = &dec_r8<&registers::b>;
            ops[0x06] = &ld_r8_n8<&registers::b>;
            ops[0x07] = &rlca;
            ops[0x08] = &ld_n16P_r16<&registers::sp>;
            ops[0x09] = &add_r16_r16<regpair_hl, regpair_bc>;
            ops[0x0A] = &ld_r8_r16P<&registers::a, regpair_bc>;
            ops[0x0B] = &dec_r16<regpair_bc>;
            ops[0x0C] = &inc_r8<&registers::c>;
            ops[0x0D] = &dec_r8<&registers::c>;
            ops[0x0E] = &ld_r8_n8<&registers::c>;
            ops[0x0F] = &rrca;
            ops[0x10] = &stop_n8;
            ops[0x11] = &ld_r16_n16<regpair_de>;
            ops[0x12] = &ld_r16P_r8<regpair_de, &registers::a>;
            ops[0x13] = &inc_r16<regpair_de>;
            ops[0x14] = &inc_r8<&registers::d>;
            ops[0x15] = &dec_r8<&registers::d>;
            ops[0x16] = &ld_r8_n8<&registers::d>;
            ops[0x17] = &rla;
            ops[0x18] = &jr_n8;
            ops[0x19] = &add_r16_r16<regpair_hl, regpair_de>;
            ops[0x1A] = &ld_r8_r16P<&registers::a, regpair_de>;
            ops[0x1B] = &dec_r16<regpair_de>;
            ops[0x1C] = &inc_r8<&registers::e>;
            ops[0x1D] = &dec_r8<&registers::e>;
            ops[0x1E] = &ld_r8_n8<&registers::e>;
            ops[0x1F] = &rra;
            ops[0x20] = &jr_cc_n8<jump_condition::NZ>;
            ops[0x21] = &ld_r16_n16<regpair_hl>;
            ops[0x22] = &ld_r16PI_r8<regpair_hl, &registers::a>;
            ops[0x23] = &inc_r16<regpair_hl>;
            ops[0x24] = &inc_r8<&registers::h>;
            ops[0x25] = &dec_r8<&registers::h>;
            ops[0x26] = &ld_r8_n8<&registers::h>;
            ops[0x27] = &daa;
            ops[0x28] = &jr_cc_n8<jump_condition::Z>;
            ops[0x29] = &add_r16_r16<regpair_hl, regpair_hl>;
            ops[0x2A] = &ld_a_hlp;
            ops[0x2B] = &dec_r16<regpair_hl>;
            ops[0x2C] = &inc_r8<&registers::l>;
            ops[0x2D] = &dec_r8<&registers::l>;
            ops[0x2E] = &ld_r8_n8<&registers::l>;
            ops[0x2F] = &cpl;
            ops[0x30] = &jr_cc_n8<jump_condition::NC>;
            ops[0x31] = &ld_sp_n16;
            ops[0x32] = &ld_r16PD_r8<regpair_hl, &registers::a>;
            ops[0x33] = &inc_sp;
            ops[0x34] = &inc_r16P<regpair_hl>;
            ops[0x35] = &dec_r16P<regpair_hl>;
            ops[0x36] = &ld_r16P_n8<regpair_hl>;
            ops[0x37] = &scf;
            ops[0x38] = &jr_cc_n8<jump_condition::C>;
            ops[0x39] = &add_hl_sp;
            ops[0x3A] = &ld_r8_r16PD<&registers::a, regpair_hl>;
            ops[0x3B] = &dec_sp;
            ops[0x3C] = &inc_r8<&registers::a>;
            ops[0x3D] = &dec_r8<&registers::a>;
            ops[0x3E] = &ld_r8_n8<&registers::a>;
            ops[0x3F] = &ccf;
            ops[0x40] = &ld_r8_r8<&registers::b, &registers::b>;
            ops[0x41] = &ld_r8_r8<&registers::b, &registers::c>;
            ops[0x42] = &ld_r8_r8<&registers::b, &registers::d>;
            ops[0x43] = &ld_r8_r8<&registers::b, &registers::e>;
            ops[0x44] = &ld_r8_r8<&registers::b, &registers::h>;
            ops[0x45] = &ld_r8_r8<&registers::b, &registers::l>;
            ops[0x46] = &ld_r8_r16P<&registers::b, regpair_hl>;
            ops[0x47] = &ld_r8_r8<&registers::b, &registers::a>;
            ops[0x48] = &ld_r8_r8<&registers::c, &registers::b>;
            ops[0x49] = &ld_r8_r8<&registers::c, &registers::c>;
            ops[0x4A] = &ld_r8_r8<&registers::c, &registers::d>;
            ops[0x4B] = &ld_r8_r8<&registers::c, &registers::e>;
            ops[0x4C] = &ld_r8_r8<&registers::c, &registers::h>;
            ops[0x4D] = &ld_r8_r8<&registers::c, &registers::l>;
            ops[0x4E] = &ld_r8_r16P<&registers::c, regpair_hl>;
            ops[0x4F] = &ld_r8_r8<&registers::c, &registers::a>;
            ops[0x50] = &ld_r8_r8<&registers::d, &registers::b>;
            ops[0x51] = &ld_r8_r8<&registers::d, &registers::c>;
            ops[0x52] = &ld_r8_r8<&registers::d, &registers::d>;
            ops[0x53] = &ld_r8_r8<&registers::d, &registers::e>;
            ops[0x54] = &ld_r8_r8<&registers::d, &registers::h>;
            ops[0x55] = &ld_r8_r8<&registers::d, &registers::l>;
            ops[0x56] = &ld_r8_r16P<&registers::d, regpair_hl>;
            ops[0x57] = &ld_r8_r8<&registers::d, &registers::a>;
            ops[0x58] = &ld_r8_r8<&registers::e, &registers::b>;
            ops[0x59] = &ld_r8_r8<&registers::e, &registers::c>;
            ops[0x5A] = &ld_r8_r8<&registers::e, &registers::d>;
            ops[0x5B] = &ld_r8_r8<&registers::e, &registers::e>;
            ops[0x5C] = &ld_r8_r8<&registers::e, &registers::h>;
            ops[0x5D] = &ld_r8_r8<&registers::e, &registers::l>;
            ops[0x5E] = &ld_r8_r16P<&registers::e, regpair_hl>;
            ops[0x5F] = &ld_r8_r8<&registers::e, &registers::a>;
            ops[0x60] = &ld_r8_r8<&registers::h, &registers::b>;
            ops[0x61] = &ld_r8_r8<&registers::h, &registers::c>;
            ops[0x62] = &ld_r8_r8<&registers::h, &registers::d>;
            ops[0x63] = &ld_r8_r8<&registers::h, &registers::e>;
            ops[0x64] = &ld_r8_r8<&registers::h, &registers::h>;
            ops[0x65] = &ld_r8_r8<&registers::h, &registers::l>;
            ops[0x66] = &ld_r8_r16P<&registers::h, regpair_hl>;
            ops[0x67] = &ld_r8_r8<&registers::h, &registers::a>;
            ops[0x68] = &ld_r8_r8<&registers::l, &registers::b>;
            ops[0x69] = &ld_r8_r8<&registers::l, &registers::c>;
            ops[0x6A] = &ld_r8_r8<&registers::l, &registers::d>;
            ops[0x6B] = &ld_r8_r8<&registers::l, &registers::e>;
            ops[0x6C] = &ld_r8_r8<&registers::l, &registers::h>;
            ops[0x6D] = &ld_r8_r8<&registers::l, &registers::l>;
            ops[0x6E] = &ld_r8_r16P<&registers::l, regpair_hl>;
            ops[0x6F] = &ld_r8_r8<&registers::l, &registers::a>;
            ops[0x70] = &ld_r16P_r8<regpair_hl, &registers::b>;
            ops[0x71] = &ld_r16P_r8<regpair_hl, &registers::c>;
            ops[0x72] = &ld_r16P_r8<regpair_hl, &registers::d>;
            ops[0x73] = &ld_r16P_r8<regpair_hl, &registers::e>;
            ops[0x74] = &ld_r16P_r8<regpair_hl, &registers::h>;
            ops[0x75] = &ld_r16P_r8<regpair_hl, &registers::l>;
            ops[0x76] = &halt;
            ops[0x77] = &ld_r16P_r8<regpair_hl, &registers::a>;
            ops[0x78] = &ld_r8_r8<&registers::a, &registers::b>;
            ops[0x79] = &ld_r8_r8<&registers::a, &registers::c>;
            ops[0x7A] = &ld_r8_r8<&registers::a, &registers::d>;
            ops[0x7B] = &ld_r8_r8<&registers::a, &registers::e>;
            ops[0x7C] = &ld_r8_r8<&registers::a, &registers::h>;
            ops[0x7D] = &ld_r8_r8<&registers::a, &registers::l>;
            ops[0x7E] = &ld_r8_r16P<&registers::a, regpair_hl>;
            ops[0x7F] = &ld_r8_r8<&registers::a, &registers::a>;
            ops[0x80] = &add_r8<&registers::b>;
            ops[0x81] = &add_r8<&registers::c>;
            ops[0x82] = &add_r8<&registers::d>;
            ops[0x83] = &add_r8<&registers::e>;
            ops[0x84] = &add_r8<&registers::h>;
            ops[0x85] = &add_r8<&registers::l>;
            ops[0x86] = &add_r16P<regpair_hl>;
            ops[0x87] = &add_aa;
            ops[0x88] = &adc_r8<&registers::b>;
            ops[0x89] = &adc_r8<&registers::c>;
            ops[0x8A] = &adc_r8<&registers::d>;
            ops[0x8B] = &adc_r8<&registers::e>;
            ops[0x8C] = &adc_r8<&registers::h>;
            ops[0x8D] = &adc_r8<&registers::l>;
            ops[0x8E] = &adc_r16P<regpair_hl>;
            ops[0x8F] = &adc_aa;
            ops[0x90] = &sub_r8<&registers::b>;
            ops[0x91] = &sub_r8<&registers::c>;
            ops[0x92] = &sub_r8<&registers::d>;
            ops[0x93] = &sub_r8<&registers::e>;
            ops[0x94] = &sub_r8<&registers::h>;
            ops[0x95] = &sub_r8<&registers::l>;
            ops[0x96] = &sub_r16P<regpair_hl>;
            ops[0x97] = &sub_aa;
            ops[0x98] = &sbc_r8<&registers::b>;
            ops[0x99] = &sbc_r8<&registers::c>;
            ops[0x9A] = &sbc_r8<&registers::d>;
            ops[0x9B] = &sbc_r8<&registers::e>;
            ops[0x9C] = &sbc_r8<&registers::h>;
            ops[0x9D] = &sbc_r8<&registers::l>;
            ops[0x9E] = &sbc_r16P<regpair_hl>;
            ops[0x9F] = &sbc_aa;
            ops[0xA0] = &and_r8<&registers::b>;
            ops[0xA1] = &and_r8<&registers::c>;
            ops[0xA2] = &and_r8<&registers::d>;
            ops[0xA3] = &and_r8<&registers::e>;
            ops[0xA4] = &and_r8<&registers::h>;
            ops[0xA5] = &and_r8<&registers::l>;
            ops[0xA6] = &and_r16P<regpair_hl>;
            ops[0xA7] = &and_aa;
            ops[0xA8] = &xor_r8<&registers::b>;
            ops[0xA9] = &xor_r8<&registers::c>;
            ops[0xAA] = &xor_r8<&registers::d>;
            ops[0xAB] = &xor_r8<&registers::e>;
            ops[0xAC] = &xor_r8<&registers::h>;
            ops[0xAD] = &xor_r8<&registers::l>;
            ops[0xAE] = &xor_r16P<regpair_hl>;
            ops[0xAF] = &xor_aa;
            ops[0xB0] = &or_r8<&registers::b>;
            ops[0xB1] = &or_r8<&registers::c>;
            ops[0xB2] = &or_r8<&registers::d>;
            ops[0xB3] = &or_r8<&registers::e>;
            ops[0xB4] = &or_r8<&registers::h>;
            ops[0xB5] = &or_r8<&registers::l>;
            ops[0xB6] = &or_r16P<regpair_hl>;
            ops[0xB7] = &or_aa;
            ops[0xB8] = &cp_r8<&registers::b>;
            ops[0xB9] = &cp_r8<&registers::c>;
            ops[0xBA] = &cp_r8<&registers::d>;
            ops[0xBB] = &cp_r8<&registers::e>;
            ops[0xBC] = &cp_r8<&registers::h>;
            ops[0xBD] = &cp_r8<&registers::l>;
            ops[0xBE] = &cp_r16P<regpair_hl>;
            ops[0xBF] = &cp_aa;
            ops[0xC0] = &ret_cc<jump_condition::NZ>;
            ops[0xC1] = &pop_r16<regpair_bc>;
            ops[0xC2] = &jp_cc_n16<jump_condition::NZ>;
            ops[0xC3] = &jp_n16;
            ops[0xC4] = &call_cc_n16<jump_condition::NZ>;
            ops[0xC5] = &push_r16<regpair_bc>;
            ops[0xC6] = &add_n8;
            ops[0xC7] = &rst_n8<0x00>;
            ops[0xC8] = &ret_cc<jump_condition::Z>;
            ops[0xC9] = &ret;
            ops[0xCA] = &jp_cc_n16<jump_condition::Z>;
            ops[0xCB] = &prefix;
            ops[0xCC] = &call_cc_n16<jump_condition::Z>;
            ops[0xCD] = &call_n16;
            ops[0xCE] = &adc_n8;
            ops[0xCF] = &rst_n8<0x08>;
            ops[0xD0] = &ret_cc<jump_condition::NC>;
            ops[0xD1] = &pop_r16<regpair_de>;
            ops[0xD2] = &jp_cc_n16<jump_condition::NC>;
            ops[0xD3] = &illegal;
            ops[0xD4] = &call_cc_n16<jump_condition::NC>;
            ops[0xD5] = &push_r16<regpair_de>;
            ops[0xD6] = &sub_n8;
            ops[0xD7] = &rst_n8<0x10>;
            ops[0xD8] = &ret_cc<jump_condition::C>;
            ops[0xD9] = &reti;
            ops[0xDA] = &jp_cc_n16<jump_condition::C>;
            ops[0xDB] = &illegal;
            ops[0xDC] = &call_cc_n16<jump_condition::C>;
            ops[0xDD] = &illegal;
            ops[0xDE] = &sbc_n8;
            ops[0xDF] = &rst_n8<0x18>;
            ops[0xE0] = &ldh_n8P_r8<&registers::a>;
            ops[0xE1] = &pop_r16<regpair_hl>;
            ops[0xE2] = &ldh_r8P_r8<&registers::c, &registers::a>;
            ops[0xE3] = &isr<0x40>;
            ops[0xE4] = &isr<0x48>;
            ops[0xE5] = &push_r16<regpair_hl>;
            ops[0xE6] = &and_n8;
            ops[0xE7] = &rst_n8<0x20>;
            ops[0xE8] = &add_sp_n8;
            ops[0xE9] = &jp_hl;
            ops[0xEA] = &ld_n16P_r8<&registers::a>;
            ops[0xEB] = &isr<0x50>;
            ops[0xEC] = &isr<0x58>;
            ops[0xED] = &isr<0x60>;
            ops[0xEE] = &xor_n8;
            ops[0xEF] = &rst_n8<0x28>;
            ops[0xF0] = &ldh_r8_n8P<&registers::a>;
            ops[0xF1] = &pop_r16<regpair_af>;
            ops[0xF2] = &ldh_r8_r8P<&registers::a, &registers::c>;
            ops[0xF3] = &di;
            ops[0xF4] = &illegal;
            ops[0xF5] = &push_r16<regpair_af>;
            ops[0xF6] = &or_n8;
            ops[0xF7] = &rst_n8<0x30>;
            ops[0xF8] = &ld_hl_sp_n8;
            ops[0xF9] = &ld_sp_hl;
            ops[0xFA] = &ld_r8_n16P<&registers::a>;
            ops[0xFB] = &ei;
            ops[0xFC] = &illegal;
            ops[0xFD] = &illegal;
            ops[0xFE] = &cp_n8;
            ops[0xFF] = &rst_n8<0x38>;
            ops[0x100] = &rlc_r8<&registers::b>;
            ops[0x101] = &rlc_r8<&registers::c>;
            ops[0x102] = &rlc_r8<&registers::d>;
            ops[0x103] = &rlc_r8<&registers::e>;
            ops[0x104] = &rlc_r8<&registers::h>;
            ops[0x105] = &rlc_r8<&registers::l>;
            ops[0x106] = &rlc_r16P<regpair_hl>;
            ops[0x107] = &rlc_r8<&registers::a>;
            ops[0x108] = &rrc_r8<&registers::b>;
            ops[0x109] = &rrc_r8<&registers::c>;
            ops[0x10A] = &rrc_r8<&registers::d>;
            ops[0x10B] = &rrc_r8<&registers::e>;
            ops[0x10C] = &rrc_r8<&registers::h>;
            ops[0x10D] = &rrc_r8<&registers::l>;
            ops[0x10E] = &rrc_r16P<regpair_hl>;
            ops[0x10F] = &rrc_r8<&registers::a>;
            ops[0x110] = &rl_r8<&registers::b>;
            ops[0x111] = &rl_r8<&registers::c>;
            ops[0x112] = &rl_r8<&registers::d>;
            ops[0x113] = &rl_r8<&registers::e>;
            ops[0x114] = &rl_r8<&registers::h>;
            ops[0x115] = &rl_r8<&registers::l>;
            ops[0x116] = &rl_r16P<regpair_hl>;
            ops[0x117] = &rl_r8<&registers::a>;
            ops[0x118] = &rr_r8<&registers::b>;
            ops[0x119] = &rr_r8<&registers::c>;
            ops[0x11A] = &rr_r8<&registers::d>;
            ops[0x11B] = &rr_r8<&registers::e>;
            ops[0x11C] = &rr_r8<&registers::h>;
            ops[0x11D] = &rr_r8<&registers::l>;
            ops[0x11E] = &rr_r16P<regpair_hl>;
            ops[0x11F] = &rr_r8<&registers::a>;
            ops[0x120] = &sla_r8<&registers::b>;
            ops[0x121] = &sla_r8<&registers::c>;
            ops[0x122] = &sla_r8<&registers::d>;
            ops[0x123] = &sla_r8<&registers::e>;
            ops[0x124] = &sla_r8<&registers::h>;
            ops[0x125] = &sla_r8<&registers::l>;
            ops[0x126] = &sla_r16P<regpair_hl>;
            ops[0x127] = &sla_r8<&registers::a>;
            ops[0x128] = &sra_r8<&registers::b>;
            ops[0x129] = &sra_r8<&registers::c>;
            ops[0x12A] = &sra_r8<&registers::d>;
            ops[0x12B] = &sra_r8<&registers::e>;
            ops[0x12C] = &sra_r8<&registers::h>;
            ops[0x12D] = &sra_r8<&registers::l>;
            ops[0x12E] = &sra_r16P<regpair_hl>;
            ops[0x12F] = &sra_r8<&registers::a>;
            ops[0x130] = &swap_r8<&registers::b>;
            ops[0x131] = &swap_r8<&registers::c>;
            ops[0x132] = &swap_r8<&registers::d>;
            ops[0x133] = &swap_r8<&registers::e>;
            ops[0x134] = &swap_r8<&registers::h>;
            ops[0x135] = &swap_r8<&registers::l>;
            ops[0x136] = &swap_r16P<regpair_hl>;
            ops[0x137] = &swap_r8<&registers::a>;
            ops[0x138] = &srl_r8<&registers::b>;
            ops[0x139] = &srl_r8<&registers::c>;
            ops[0x13A] = &srl_r8<&registers::d>;
            ops[0x13B] = &srl_r8<&registers::e>;
            ops[0x13C] = &srl_r8<&registers::h>;
            ops[0x13D] = &srl_r8<&registers::l>;
            ops[0x13E] = &srl_r16P<regpair_hl>;
            ops[0x13F] = &srl_r8<&registers::a>;
            ops[0x140] = &bit_r8<0, &registers::b>;
            ops[0x141] = &bit_r8<0, &registers::c>;
            ops[0x142] = &bit_r8<0, &registers::d>;
            ops[0x143] = &bit_r8<0, &registers::e>;
            ops[0x144] = &bit_r8<0, &registers::h>;
            ops[0x145] = &bit_r8<0, &registers::l>;
            ops[0x146] = &bit_r16P<0, regpair_hl>;
            ops[0x147] = &bit_r8<0, &registers::a>;
            ops[0x148] = &bit_r8<1, &registers::b>;
            ops[0x149] = &bit_r8<1, &registers::c>;
            ops[0x14A] = &bit_r8<1, &registers::d>;
            ops[0x14B] = &bit_r8<1, &registers::e>;
            ops[0x14C] = &bit_r8<1, &registers::h>;
            ops[0x14D] = &bit_r8<1, &registers::l>;
            ops[0x14E] = &bit_r16P<1, regpair_hl>;
            ops[0x14F] = &bit_r8<1, &registers::a>;
            ops[0x150] = &bit_r8<2, &registers::b>;
            ops[0x151] = &bit_r8<2, &registers::c>;
            ops[0x152] = &bit_r8<2, &registers::d>;
            ops[0x153] = &bit_r8<2, &registers::e>;
            ops[0x154] = &bit_r8<2, &registers::h>;
            ops[0x155] = &bit_r8<2, &registers::l>;
            ops[0x156] = &bit_r16P<2, regpair_hl>;
            ops[0x157] = &bit_r8<2, &registers::a>;
            ops[0x158] = &bit_r8<3, &registers::b>;
            ops[0x159] = &bit_r8<3, &registers::c>;
            ops[0x15A] = &bit_r8<3, &registers::d>;
            ops[0x15B] = &bit_r8<3, &registers::e>;
            ops[0x15C] = &bit_r8<3, &registers::h>;
            ops[0x15D] = &bit_r8<3, &registers::l>;
            ops[0x15E] = &bit_r16P<3, regpair_hl>;
            ops[0x15F] = &bit_r8<3, &registers::a>;
            ops[0x160] = &bit_r8<4, &registers::b>;
            ops[0x161] = &bit_r8<4, &registers::c>;
            ops[0x162] = &bit_r8<4, &registers::d>;
            ops[0x163] = &bit_r8<4, &registers::e>;
            ops[0x164] = &bit_r8<4, &registers::h>;
            ops[0x165] = &bit_r8<4, &registers::l>;
            ops[0x166] = &bit_r16P<4, regpair_hl>;
            ops[0x167] = &bit_r8<4, &registers::a>;
            ops[0x168] = &bit_r8<5, &registers::b>;
            ops[0x169] = &bit_r8<5, &registers::c>;
            ops[0x16A] = &bit_r8<5, &registers::d>;
            ops[0x16B] = &bit_r8<5, &registers::e>;
            ops[0x16C] = &bit_r8<5, &registers::h>;
            ops[0x16D] = &bit_r8<5, &registers::l>;
            ops[0x16E] = &bit_r16P<5, regpair_hl>;
            ops[0x16F] = &bit_r8<5, &registers::a>;
            ops[0x170] = &bit_r8<6, &registers::b>;
            ops[0x171] = &bit_r8<6, &registers::c>;
            ops[0x172] = &bit_r8<6, &registers::d>;
            ops[0x173] = &bit_r8<6, &registers::e>;
            ops[0x174] = &bit_r8<6, &registers::h>;
            ops[0x175] = &bit_r8<6, &registers::l>;
            ops[0x176] = &bit_r16P<6, regpair_hl>;
            ops[0x177] = &bit_r8<6, &registers::a>;
            ops[0x178] = &bit_r8<7, &registers::b>;
            ops[0x179] = &bit_r8<7, &registers::c>;
            ops[0x17A] = &bit_r8<7, &registers::d>;
            ops[0x17B] = &bit_r8<7, &registers::e>;
            ops[0x17C] = &bit_r8<7, &registers::h>;
            ops[0x17D] = &bit_r8<7, &registers::l>;
            ops[0x17E] = &bit_r16P<7, regpair_hl>;
            ops[0x17F] = &bit_r8<7, &registers::a>;
            ops[0x180] = &res_r8<0, &registers::b>;
            ops[0x181] = &res_r8<0, &registers::c>;
            ops[0x182] = &res_r8<0, &registers::d>;
            ops[0x183] = &res_r8<0, &registers::e>;
            ops[0x184] = &res_r8<0, &registers::h>;
            ops[0x185] = &res_r8<0, &registers::l>;
            ops[0x186] = &res_r16P<0, regpair_hl>;
            ops[0x187] = &res_r8<0, &registers::a>;
            ops[0x188] = &res_r8<1, &registers::b>;
            ops[0x189] = &res_r8<1, &registers::c>;
            ops[0x18A] = &res_r8<1, &registers::d>;
            ops[0x18B] = &res_r8<1, &registers::e>;
            ops[0x18C] = &res_r8<1, &registers::h>;
            ops[0x18D] = &res_r8<1, &registers::l>;
            ops[0x18E] = &res_r16P<1, regpair_hl>;
            ops[0x18F] = &res_r8<1, &registers::a>;
            ops[0x190] = &res_r8<2, &registers::b>;
            ops[0x191] = &res_r8<2, &registers::c>;
            ops[0x192] = &res_r8<2, &registers::d>;
            ops[0x193] = &res_r8<2, &registers::e>;
            ops[0x194] = &res_r8<2, &registers::h>;
            ops[0x195] = &res_r8<2, &registers::l>;
            ops[0x196] = &res_r16P<2, regpair_hl>;
            ops[0x197] = &res_r8<2, &registers::a>;
            ops[0x198] = &res_r8<3, &registers::b>;
            ops[0x199] = &res_r8<3, &registers::c>;
            ops[0x19A] = &res_r8<3, &registers::d>;
            ops[0x19B] = &res_r8<3, &registers::e>;
            ops[0x19C] = &res_r8<3, &registers::h>;
            ops[0x19D] = &res_r8<3, &registers::l>;
            ops[0x19E] = &res_r16P<3, regpair_hl>;
            ops[0x19F] = &res_r8<3, &registers::a>;
            ops[0x1A0] = &res_r8<4, &registers::b>;
            ops[0x1A1] = &res_r8<4, &registers::c>;
            ops[0x1A2] = &res_r8<4, &registers::d>;
            ops[0x1A3] = &res_r8<4, &registers::e>;
            ops[0x1A4] = &res_r8<4, &registers::h>;
            ops[0x1A5] = &res_r8<4, &registers::l>;
            ops[0x1A6] = &res_r16P<4, regpair_hl>;
            ops[0x1A7] = &res_r8<4, &registers::a>;
            ops[0x1A8] = &res_r8<5, &registers::b>;
            ops[0x1A9] = &res_r8<5, &registers::c>;
            ops[0x1AA] = &res_r8<5, &registers::d>;
            ops[0x1AB] = &res_r8<5, &registers::e>;
            ops[0x1AC] = &res_r8<5, &registers::h>;
            ops[0x1AD] = &res_r8<5, &registers::l>;
            ops[0x1AE] = &res_r16P<5, regpair_hl>;
            ops[0x1AF] = &res_r8<5, &registers::a>;
            ops[0x1B0] = &res_r8<6, &registers::b>;
            ops[0x1B1] = &res_r8<6, &registers::c>;
            ops[0x1B2] = &res_r8<6, &registers::d>;
            ops[0x1B3] = &res_r8<6, &registers::e>;
            ops[0x1B4] = &res_r8<6, &registers::h>;
            ops[0x1B5] = &res_r8<6, &registers::l>;
            ops[0x1B6] = &res_r16P<6, regpair_hl>;
            ops[0x1B7] = &res_r8<6, &registers::a>;
            ops[0x1B8] = &res_r8<7, &registers::b>;
            ops[0x1B9] = &res_r8<7, &registers::c>;
            ops[0x1BA] = &res_r8<7, &registers::d>;
            ops[0x1BB] = &res_r8<7, &registers::e>;
            ops[0x1BC] = &res_r8<7, &registers::h>;
            ops[0x1BD] = &res_r8<7, &registers::l>;
            ops[0x1BE] = &res_r16P<7, regpair_hl>;
            ops[0x1BF] = &res_r8<7, &registers::a>;
            ops[0x1C0] = &set_r8<0, &registers::b>;
            ops[0x1C1] = &set_r8<0, &registers::c>;
            ops[0x1C2] = &set_r8<0, &registers::d>;
            ops[0x1C3] = &set_r8<0, &registers::e>;
            ops[0x1C4] = &set_r8<0, &registers::h>;
            ops[0x1C5] = &set_r8<0, &registers::l>;
            ops[0x1C6] = &set_r16P<0, regpair_hl>;
            ops[0x1C7] = &set_r8<0, &registers::a>;
            ops[0x1C8] = &set_r8<1, &registers::b>;
            ops[0x1C9] = &set_r8<1, &registers::c>;
            ops[0x1CA] = &set_r8<1, &registers::d>;
            ops[0x1CB] = &set_r8<1, &registers::e>;
            ops[0x1CC] = &set_r8<1, &registers::h>;
            ops[0x1CD] = &set_r8<1, &registers::l>;
            ops[0x1CE] = &set_r16P<1, regpair_hl>;
            ops[0x1CF] = &set_r8<1, &registers::a>;
            ops[0x1D0] = &set_r8<2, &registers::b>;
            ops[0x1D1] = &set_r8<2, &registers::c>;
            ops[0x1D2] = &set_r8<2, &registers::d>;
            ops[0x1D3] = &set_r8<2, &registers::e>;
            ops[0x1D4] = &set_r8<2, &registers::h>;
            ops[0x1D5] = &set_r8<2, &registers::l>;
            ops[0x1D6] = &set_r16P<2, regpair_hl>;
            ops[0x1D7] = &set_r8<2, &registers::a>;
            ops[0x1D8] = &set_r8<3, &registers::b>;
            ops[0x1D9] = &set_r8<3, &registers::c>;
            ops[0x1DA] = &set_r8<3, &registers::d>;
            ops[0x1DB] = &set_r8<3, &registers::e>;
            ops[0x1DC] = &set_r8<3, &registers::h>;
            ops[0x1DD] = &set_r8<3, &registers::l>;
            ops[0x1DE] = &set_r16P<3, regpair_hl>;
            ops[0x1DF] = &set_r8<3, &registers::a>;
            ops[0x1E0] = &set_r8<4, &registers::b>;
            ops[0x1E1] = &set_r8<4, &registers::c>;
            ops[0x1E2] = &set_r8<4, &registers::d>;
            ops[0x1E3] = &set_r8<4, &registers::e>;
            ops[0x1E4] = &set_r8<4, &registers::h>;
            ops[0x1E5] = &set_r8<4, &registers::l>;
            ops[0x1E6] = &set_r16P<4, regpair_hl>;
            ops[0x1E7] = &set_r8<4, &registers::a>;
            ops[0x1E8] = &set_r8<5, &registers::b>;
            ops[0x1E9] = &set_r8<5, &registers::c>;
            ops[0x1EA] = &set_r8<5, &registers::d>;
            ops[0x1EB] = &set_r8<5, &registers::e>;
            ops[0x1EC] = &set_r8<5, &registers::h>;
            ops[0x1ED] = &set_r8<5, &registers::l>;
            ops[0x1EE] = &set_r16P<5, regpair_hl>;
            ops[0x1EF] = &set_r8<5, &registers::a>;
            ops[0x1F0] = &set_r8<6, &registers::b>;
            ops[0x1F1] = &set_r8<6, &registers::c>;
            ops[0x1F2] = &set_r8<6, &registers::d>;
            ops[0x1F3] = &set_r8<6, &registers::e>;
            ops[0x1F4] = &set_r8<6, &registers::h>;
            ops[0x1F5] = &set_r8<6, &registers::l>;
            ops[0x1F6] = &set_r16P<6, regpair_hl>;
            ops[0x1F7] = &set_r8<6, &registers::a>;
            ops[0x1F8] = &set_r8<7, &registers::b>;
            ops[0x1F9] = &set_r8<7, &registers::c>;
            ops[0x1FA] = &set_r8<7, &registers::d>;
            ops[0x1FB] = &set_r8<7, &registers::e>;
            ops[0x1FC] = &set_r8<7, &registers::h>;
            ops[0x1FD] = &set_r8<7, &registers::l>;
            ops[0x1FE] = &set_r16P<7, regpair_hl>;
            ops[0x1FF] = &set_r8<7, &registers::a>;

            return ops;
        }();
    }
}