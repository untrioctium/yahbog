#include <yahbog/opinfo.h>

namespace yahbog {

	constinit const std::array<opinfo_t, 512> opinfo = []() {
		std::array<opinfo_t, 512> ops;

        ops[0x00] = {
            .name = "NOP"
        };

        ops[0x01] = {
            .name = "LD BC, n16"
        };

        ops[0x02] = {
            .name = "LD [BC], A"
        };

        ops[0x03] = {
            .name = "INC BC"
        };

        ops[0x04] = {
            .name = "INC B"
        };

        ops[0x05] = {
            .name = "DEC B"
        };

        ops[0x06] = {
            .name = "LD B, n8"
        };

        ops[0x07] = {
            .name = "RLCA"
        };

        ops[0x08] = {
            .name = "LD [a16], SP"
        };

        ops[0x09] = {
            .name = "ADD HL, BC"
        };

        ops[0x0A] = {
            .name = "LD A, [BC]"
        };

        ops[0x0B] = {
            .name = "DEC BC"
        };

        ops[0x0C] = {
            .name = "INC C"
        };

        ops[0x0D] = {
            .name = "DEC C"
        };

        ops[0x0E] = {
            .name = "LD C, n8"
        };

        ops[0x0F] = {
            .name = "RRCA"
        };

        ops[0x10] = {
            .name = "STOP n8"
        };

        ops[0x11] = {
            .name = "LD DE, n16"
        };

        ops[0x12] = {
            .name = "LD [DE], A"
        };

        ops[0x13] = {
            .name = "INC DE"
        };

        ops[0x14] = {
            .name = "INC D"
        };

        ops[0x15] = {
            .name = "DEC D"
        };

        ops[0x16] = {
            .name = "LD D, n8"
        };

        ops[0x17] = {
            .name = "RLA"
        };

        ops[0x18] = {
            .name = "JR e8"
        };

        ops[0x19] = {
            .name = "ADD HL, DE"
        };

        ops[0x1A] = {
            .name = "LD A, [DE]"
        };

        ops[0x1B] = {
            .name = "DEC DE"
        };

        ops[0x1C] = {
            .name = "INC E"
        };

        ops[0x1D] = {
            .name = "DEC E"
        };

        ops[0x1E] = {
            .name = "LD E, n8"
        };

        ops[0x1F] = {
            .name = "RRA"
        };

        ops[0x20] = {
            .name = "JR NZ, e8"
        };

        ops[0x21] = {
            .name = "LD HL, n16"
        };

        ops[0x22] = {
            .name = "LD [HL+], A"
        };

        ops[0x23] = {
            .name = "INC HL"
        };

        ops[0x24] = {
            .name = "INC H"
        };

        ops[0x25] = {
            .name = "DEC H"
        };

        ops[0x26] = {
            .name = "LD H, n8"
        };

        ops[0x27] = {
            .name = "DAA"
        };

        ops[0x28] = {
            .name = "JR Z, e8"
        };

        ops[0x29] = {
            .name = "ADD HL, HL"
        };

        ops[0x2A] = {
            .name = "LD A, [HL+]"
        };

        ops[0x2B] = {
            .name = "DEC HL"
        };

        ops[0x2C] = {
            .name = "INC L"
        };

        ops[0x2D] = {
            .name = "DEC L"
        };

        ops[0x2E] = {
            .name = "LD L, n8"
        };

        ops[0x2F] = {
            .name = "CPL"
        };

        ops[0x30] = {
            .name = "JR NC, e8"
        };

        ops[0x31] = {
            .name = "LD SP, n16"
        };

        ops[0x32] = {
            .name = "LD [HL-], A"
        };

        ops[0x33] = {
            .name = "INC SP"
        };

        ops[0x34] = {
            .name = "INC [HL]"
        };

        ops[0x35] = {
            .name = "DEC [HL]"
        };

        ops[0x36] = {
            .name = "LD [HL], n8"
        };

        ops[0x37] = {
            .name = "SCF"
        };

        ops[0x38] = {
            .name = "JR C, e8"
        };

        ops[0x39] = {
            .name = "ADD HL, SP"
        };

        ops[0x3A] = {
            .name = "LD A, [HL-]"
        };

        ops[0x3B] = {
            .name = "DEC SP"
        };

        ops[0x3C] = {
            .name = "INC A"
        };

        ops[0x3D] = {
            .name = "DEC A"
        };

        ops[0x3E] = {
            .name = "LD A, n8"
        };

        ops[0x3F] = {
            .name = "CCF"
        };

        ops[0x40] = {
            .name = "LD B, B"
        };

        ops[0x41] = {
            .name = "LD B, C"
        };

        ops[0x42] = {
            .name = "LD B, D"
        };

        ops[0x43] = {
            .name = "LD B, E"
        };

        ops[0x44] = {
            .name = "LD B, H"
        };

        ops[0x45] = {
            .name = "LD B, L"
        };

        ops[0x46] = {
            .name = "LD B, [HL]"
        };

        ops[0x47] = {
            .name = "LD B, A"
        };

        ops[0x48] = {
            .name = "LD C, B"
        };

        ops[0x49] = {
            .name = "LD C, C"
        };

        ops[0x4A] = {
            .name = "LD C, D"
        };

        ops[0x4B] = {
            .name = "LD C, E"
        };

        ops[0x4C] = {
            .name = "LD C, H"
        };

        ops[0x4D] = {
            .name = "LD C, L"
        };

        ops[0x4E] = {
            .name = "LD C, [HL]"
        };

        ops[0x4F] = {
            .name = "LD C, A"
        };

        ops[0x50] = {
            .name = "LD D, B"
        };

        ops[0x51] = {
            .name = "LD D, C"
        };

        ops[0x52] = {
            .name = "LD D, D"
        };

        ops[0x53] = {
            .name = "LD D, E"
        };

        ops[0x54] = {
            .name = "LD D, H"
        };

        ops[0x55] = {
            .name = "LD D, L"
        };

        ops[0x56] = {
            .name = "LD D, [HL]"
        };

        ops[0x57] = {
            .name = "LD D, A"
        };

        ops[0x58] = {
            .name = "LD E, B"
        };

        ops[0x59] = {
            .name = "LD E, C"
        };

        ops[0x5A] = {
            .name = "LD E, D"
        };

        ops[0x5B] = {
            .name = "LD E, E"
        };

        ops[0x5C] = {
            .name = "LD E, H"
        };

        ops[0x5D] = {
            .name = "LD E, L"
        };

        ops[0x5E] = {
            .name = "LD E, [HL]"
        };

        ops[0x5F] = {
            .name = "LD E, A"
        };

        ops[0x60] = {
            .name = "LD H, B"
        };

        ops[0x61] = {
            .name = "LD H, C"
        };

        ops[0x62] = {
            .name = "LD H, D"
        };

        ops[0x63] = {
            .name = "LD H, E"
        };

        ops[0x64] = {
            .name = "LD H, H"
        };

        ops[0x65] = {
            .name = "LD H, L"
        };

        ops[0x66] = {
            .name = "LD H, [HL]"
        };

        ops[0x67] = {
            .name = "LD H, A"
        };

        ops[0x68] = {
            .name = "LD L, B"
        };

        ops[0x69] = {
            .name = "LD L, C"
        };

        ops[0x6A] = {
            .name = "LD L, D"
        };

        ops[0x6B] = {
            .name = "LD L, E"
        };

        ops[0x6C] = {
            .name = "LD L, H"
        };

        ops[0x6D] = {
            .name = "LD L, L"
        };

        ops[0x6E] = {
            .name = "LD L, [HL]"
        };

        ops[0x6F] = {
            .name = "LD L, A"
        };

        ops[0x70] = {
            .name = "LD [HL], B"
        };

        ops[0x71] = {
            .name = "LD [HL], C"
        };

        ops[0x72] = {
            .name = "LD [HL], D"
        };

        ops[0x73] = {
            .name = "LD [HL], E"
        };

        ops[0x74] = {
            .name = "LD [HL], H"
        };

        ops[0x75] = {
            .name = "LD [HL], L"
        };

        ops[0x76] = {
            .name = "HALT"
        };

        ops[0x77] = {
            .name = "LD [HL], A"
        };

        ops[0x78] = {
            .name = "LD A, B"
        };

        ops[0x79] = {
            .name = "LD A, C"
        };

        ops[0x7A] = {
            .name = "LD A, D"
        };

        ops[0x7B] = {
            .name = "LD A, E"
        };

        ops[0x7C] = {
            .name = "LD A, H"
        };

        ops[0x7D] = {
            .name = "LD A, L"
        };

        ops[0x7E] = {
            .name = "LD A, [HL]"
        };

        ops[0x7F] = {
            .name = "LD A, A"
        };

        ops[0x80] = {
            .name = "ADD A, B"
        };

        ops[0x81] = {
            .name = "ADD A, C"
        };

        ops[0x82] = {
            .name = "ADD A, D"
        };

        ops[0x83] = {
            .name = "ADD A, E"
        };

        ops[0x84] = {
            .name = "ADD A, H"
        };

        ops[0x85] = {
            .name = "ADD A, L"
        };

        ops[0x86] = {
            .name = "ADD A, [HL]"
        };

        ops[0x87] = {
            .name = "ADD A, A"
        };

        ops[0x88] = {
            .name = "ADC A, B"
        };

        ops[0x89] = {
            .name = "ADC A, C"
        };

        ops[0x8A] = {
            .name = "ADC A, D"
        };

        ops[0x8B] = {
            .name = "ADC A, E"
        };

        ops[0x8C] = {
            .name = "ADC A, H"
        };

        ops[0x8D] = {
            .name = "ADC A, L"
        };

        ops[0x8E] = {
            .name = "ADC A, [HL]"
        };

        ops[0x8F] = {
            .name = "ADC A, A"
        };

        ops[0x90] = {
            .name = "SUB A, B"
        };

        ops[0x91] = {
            .name = "SUB A, C"
        };

        ops[0x92] = {
            .name = "SUB A, D"
        };

        ops[0x93] = {
            .name = "SUB A, E"
        };

        ops[0x94] = {
            .name = "SUB A, H"
        };

        ops[0x95] = {
            .name = "SUB A, L"
        };

        ops[0x96] = {
            .name = "SUB A, [HL]"
        };

        ops[0x97] = {
            .name = "SUB A, A"
        };

        ops[0x98] = {
            .name = "SBC A, B"
        };

        ops[0x99] = {
            .name = "SBC A, C"
        };

        ops[0x9A] = {
            .name = "SBC A, D"
        };

        ops[0x9B] = {
            .name = "SBC A, E"
        };

        ops[0x9C] = {
            .name = "SBC A, H"
        };

        ops[0x9D] = {
            .name = "SBC A, L"
        };

        ops[0x9E] = {
            .name = "SBC A, [HL]"
        };

        ops[0x9F] = {
            .name = "SBC A, A"
        };

        ops[0xA0] = {
            .name = "AND A, B"
        };

        ops[0xA1] = {
            .name = "AND A, C"
        };

        ops[0xA2] = {
            .name = "AND A, D"
        };

        ops[0xA3] = {
            .name = "AND A, E"
        };

        ops[0xA4] = {
            .name = "AND A, H"
        };

        ops[0xA5] = {
            .name = "AND A, L"
        };

        ops[0xA6] = {
            .name = "AND A, [HL]"
        };

        ops[0xA7] = {
            .name = "AND A, A"
        };

        ops[0xA8] = {
            .name = "XOR A, B"
        };

        ops[0xA9] = {
            .name = "XOR A, C"
        };

        ops[0xAA] = {
            .name = "XOR A, D"
        };

        ops[0xAB] = {
            .name = "XOR A, E"
        };

        ops[0xAC] = {
            .name = "XOR A, H"
        };

        ops[0xAD] = {
            .name = "XOR A, L"
        };

        ops[0xAE] = {
            .name = "XOR A, [HL]"
        };

        ops[0xAF] = {
            .name = "XOR A, A"
        };

        ops[0xB0] = {
            .name = "OR A, B"
        };

        ops[0xB1] = {
            .name = "OR A, C"
        };

        ops[0xB2] = {
            .name = "OR A, D"
        };

        ops[0xB3] = {
            .name = "OR A, E"
        };

        ops[0xB4] = {
            .name = "OR A, H"
        };

        ops[0xB5] = {
            .name = "OR A, L"
        };

        ops[0xB6] = {
            .name = "OR A, [HL]"
        };

        ops[0xB7] = {
            .name = "OR A, A"
        };

        ops[0xB8] = {
            .name = "CP A, B"
        };

        ops[0xB9] = {
            .name = "CP A, C"
        };

        ops[0xBA] = {
            .name = "CP A, D"
        };

        ops[0xBB] = {
            .name = "CP A, E"
        };

        ops[0xBC] = {
            .name = "CP A, H"
        };

        ops[0xBD] = {
            .name = "CP A, L"
        };

        ops[0xBE] = {
            .name = "CP A, [HL]"
        };

        ops[0xBF] = {
            .name = "CP A, A"
        };

        ops[0xC0] = {
            .name = "RET NZ"
        };

        ops[0xC1] = {
            .name = "POP BC"
        };

        ops[0xC2] = {
            .name = "JP NZ, a16"
        };

        ops[0xC3] = {
            .name = "JP a16"
        };

        ops[0xC4] = {
            .name = "CALL NZ, a16"
        };

        ops[0xC5] = {
            .name = "PUSH BC"
        };

        ops[0xC6] = {
            .name = "ADD A, n8"
        };

        ops[0xC7] = {
            .name = "RST $00"
        };

        ops[0xC8] = {
            .name = "RET Z"
        };

        ops[0xC9] = {
            .name = "RET"
        };

        ops[0xCA] = {
            .name = "JP Z, a16"
        };

        ops[0xCB] = {
            .name = "PREFIX"
        };

        ops[0xCC] = {
            .name = "CALL Z, a16"
        };

        ops[0xCD] = {
            .name = "CALL a16"
        };

        ops[0xCE] = {
            .name = "ADC A, n8"
        };

        ops[0xCF] = {
            .name = "RST $08"
        };

        ops[0xD0] = {
            .name = "RET NC"
        };

        ops[0xD1] = {
            .name = "POP DE"
        };

        ops[0xD2] = {
            .name = "JP NC, a16"
        };

        ops[0xD3] = {
            .name = "ILLEGAL_D3"
        };

        ops[0xD4] = {
            .name = "CALL NC, a16"
        };

        ops[0xD5] = {
            .name = "PUSH DE"
        };

        ops[0xD6] = {
            .name = "SUB A, n8"
        };

        ops[0xD7] = {
            .name = "RST $10"
        };

        ops[0xD8] = {
            .name = "RET C"
        };

        ops[0xD9] = {
            .name = "RETI"
        };

        ops[0xDA] = {
            .name = "JP C, a16"
        };

        ops[0xDB] = {
            .name = "ILLEGAL_DB"
        };

        ops[0xDC] = {
            .name = "CALL C, a16"
        };

        ops[0xDD] = {
            .name = "ILLEGAL_DD"
        };

        ops[0xDE] = {
            .name = "SBC A, n8"
        };

        ops[0xDF] = {
            .name = "RST $18"
        };

        ops[0xE0] = {
            .name = "LDH [a8], A"
        };

        ops[0xE1] = {
            .name = "POP HL"
        };

        ops[0xE2] = {
            .name = "LDH [C], A"
        };

        ops[0xE3] = {
            .name = "ILLEGAL_E3"
        };

        ops[0xE4] = {
            .name = "ILLEGAL_E4"
        };

        ops[0xE5] = {
            .name = "PUSH HL"
        };

        ops[0xE6] = {
            .name = "AND A, n8"
        };

        ops[0xE7] = {
            .name = "RST $20"
        };

        ops[0xE8] = {
            .name = "ADD SP, e8"
        };

        ops[0xE9] = {
            .name = "JP HL"
        };

        ops[0xEA] = {
            .name = "LD [a16], A"
        };

        ops[0xEB] = {
            .name = "ILLEGAL_EB"
        };

        ops[0xEC] = {
            .name = "ILLEGAL_EC"
        };

        ops[0xED] = {
            .name = "ILLEGAL_ED"
        };

        ops[0xEE] = {
            .name = "XOR A, n8"
        };

        ops[0xEF] = {
            .name = "RST $28"
        };

        ops[0xF0] = {
            .name = "LDH A, [a8]"
        };

        ops[0xF1] = {
            .name = "POP AF"
        };

        ops[0xF2] = {
            .name = "LDH A, [C]"
        };

        ops[0xF3] = {
            .name = "DI"
        };

        ops[0xF4] = {
            .name = "ILLEGAL_F4"
        };

        ops[0xF5] = {
            .name = "PUSH AF"
        };

        ops[0xF6] = {
            .name = "OR A, n8"
        };

        ops[0xF7] = {
            .name = "RST $30"
        };

        ops[0xF8] = {
            .name = "LD HL, SP+e8"
        };

        ops[0xF9] = {
            .name = "LD SP, HL"
        };

        ops[0xFA] = {
            .name = "LD A, [a16]"
        };

        ops[0xFB] = {
            .name = "EI"
        };

        ops[0xFC] = {
            .name = "ILLEGAL_FC"
        };

        ops[0xFD] = {
            .name = "ILLEGAL_FD"
        };

        ops[0xFE] = {
            .name = "CP A, n8"
        };

        ops[0xFF] = {
            .name = "RST $38"
        };

        ops[0x100] = {
            .name = "RLC B"
        };

        ops[0x101] = {
            .name = "RLC C"
        };

        ops[0x102] = {
            .name = "RLC D"
        };

        ops[0x103] = {
            .name = "RLC E"
        };

        ops[0x104] = {
            .name = "RLC H"
        };

        ops[0x105] = {
            .name = "RLC L"
        };

        ops[0x106] = {
            .name = "RLC [HL]"
        };

        ops[0x107] = {
            .name = "RLC A"
        };

        ops[0x108] = {
            .name = "RRC B"
        };

        ops[0x109] = {
            .name = "RRC C"
        };

        ops[0x10A] = {
            .name = "RRC D"
        };

        ops[0x10B] = {
            .name = "RRC E"
        };

        ops[0x10C] = {
            .name = "RRC H"
        };

        ops[0x10D] = {
            .name = "RRC L"
        };

        ops[0x10E] = {
            .name = "RRC [HL]"
        };

        ops[0x10F] = {
            .name = "RRC A"
        };

        ops[0x110] = {
            .name = "RL B"
        };

        ops[0x111] = {
            .name = "RL C"
        };

        ops[0x112] = {
            .name = "RL D"
        };

        ops[0x113] = {
            .name = "RL E"
        };

        ops[0x114] = {
            .name = "RL H"
        };

        ops[0x115] = {
            .name = "RL L"
        };

        ops[0x116] = {
            .name = "RL [HL]"
        };

        ops[0x117] = {
            .name = "RL A"
        };

        ops[0x118] = {
            .name = "RR B"
        };

        ops[0x119] = {
            .name = "RR C"
        };

        ops[0x11A] = {
            .name = "RR D"
        };

        ops[0x11B] = {
            .name = "RR E"
        };

        ops[0x11C] = {
            .name = "RR H"
        };

        ops[0x11D] = {
            .name = "RR L"
        };

        ops[0x11E] = {
            .name = "RR [HL]"
        };

        ops[0x11F] = {
            .name = "RR A"
        };

        ops[0x120] = {
            .name = "SLA B"
        };

        ops[0x121] = {
            .name = "SLA C"
        };

        ops[0x122] = {
            .name = "SLA D"
        };

        ops[0x123] = {
            .name = "SLA E"
        };

        ops[0x124] = {
            .name = "SLA H"
        };

        ops[0x125] = {
            .name = "SLA L"
        };

        ops[0x126] = {
            .name = "SLA [HL]"
        };

        ops[0x127] = {
            .name = "SLA A"
        };

        ops[0x128] = {
            .name = "SRA B"
        };

        ops[0x129] = {
            .name = "SRA C"
        };

        ops[0x12A] = {
            .name = "SRA D"
        };

        ops[0x12B] = {
            .name = "SRA E"
        };

        ops[0x12C] = {
            .name = "SRA H"
        };

        ops[0x12D] = {
            .name = "SRA L"
        };

        ops[0x12E] = {
            .name = "SRA [HL]"
        };

        ops[0x12F] = {
            .name = "SRA A"
        };

        ops[0x130] = {
            .name = "SWAP B"
        };

        ops[0x131] = {
            .name = "SWAP C"
        };

        ops[0x132] = {
            .name = "SWAP D"
        };

        ops[0x133] = {
            .name = "SWAP E"
        };

        ops[0x134] = {
            .name = "SWAP H"
        };

        ops[0x135] = {
            .name = "SWAP L"
        };

        ops[0x136] = {
            .name = "SWAP [HL]"
        };

        ops[0x137] = {
            .name = "SWAP A"
        };

        ops[0x138] = {
            .name = "SRL B"
        };

        ops[0x139] = {
            .name = "SRL C"
        };

        ops[0x13A] = {
            .name = "SRL D"
        };

        ops[0x13B] = {
            .name = "SRL E"
        };

        ops[0x13C] = {
            .name = "SRL H"
        };

        ops[0x13D] = {
            .name = "SRL L"
        };

        ops[0x13E] = {
            .name = "SRL [HL]"
        };

        ops[0x13F] = {
            .name = "SRL A"
        };

        ops[0x140] = {
            .name = "BIT 0, B"
        };

        ops[0x141] = {
            .name = "BIT 0, C"
        };

        ops[0x142] = {
            .name = "BIT 0, D"
        };

        ops[0x143] = {
            .name = "BIT 0, E"
        };

        ops[0x144] = {
            .name = "BIT 0, H"
        };

        ops[0x145] = {
            .name = "BIT 0, L"
        };

        ops[0x146] = {
            .name = "BIT 0, [HL]"
        };

        ops[0x147] = {
            .name = "BIT 0, A"
        };

        ops[0x148] = {
            .name = "BIT 1, B"
        };

        ops[0x149] = {
            .name = "BIT 1, C"
        };

        ops[0x14A] = {
            .name = "BIT 1, D"
        };

        ops[0x14B] = {
            .name = "BIT 1, E"
        };

        ops[0x14C] = {
            .name = "BIT 1, H"
        };

        ops[0x14D] = {
            .name = "BIT 1, L"
        };

        ops[0x14E] = {
            .name = "BIT 1, [HL]"
        };

        ops[0x14F] = {
            .name = "BIT 1, A"
        };

        ops[0x150] = {
            .name = "BIT 2, B"
        };

        ops[0x151] = {
            .name = "BIT 2, C"
        };

        ops[0x152] = {
            .name = "BIT 2, D"
        };

        ops[0x153] = {
            .name = "BIT 2, E"
        };

        ops[0x154] = {
            .name = "BIT 2, H"
        };

        ops[0x155] = {
            .name = "BIT 2, L"
        };

        ops[0x156] = {
            .name = "BIT 2, [HL]"
        };

        ops[0x157] = {
            .name = "BIT 2, A"
        };

        ops[0x158] = {
            .name = "BIT 3, B"
        };

        ops[0x159] = {
            .name = "BIT 3, C"
        };

        ops[0x15A] = {
            .name = "BIT 3, D"
        };

        ops[0x15B] = {
            .name = "BIT 3, E"
        };

        ops[0x15C] = {
            .name = "BIT 3, H"
        };

        ops[0x15D] = {
            .name = "BIT 3, L"
        };

        ops[0x15E] = {
            .name = "BIT 3, [HL]"
        };

        ops[0x15F] = {
            .name = "BIT 3, A"
        };

        ops[0x160] = {
            .name = "BIT 4, B"
        };

        ops[0x161] = {
            .name = "BIT 4, C"
        };

        ops[0x162] = {
            .name = "BIT 4, D"
        };

        ops[0x163] = {
            .name = "BIT 4, E"
        };

        ops[0x164] = {
            .name = "BIT 4, H"
        };

        ops[0x165] = {
            .name = "BIT 4, L"
        };

        ops[0x166] = {
            .name = "BIT 4, [HL]"
        };

        ops[0x167] = {
            .name = "BIT 4, A"
        };

        ops[0x168] = {
            .name = "BIT 5, B"
        };

        ops[0x169] = {
            .name = "BIT 5, C"
        };

        ops[0x16A] = {
            .name = "BIT 5, D"
        };

        ops[0x16B] = {
            .name = "BIT 5, E"
        };

        ops[0x16C] = {
            .name = "BIT 5, H"
        };

        ops[0x16D] = {
            .name = "BIT 5, L"
        };

        ops[0x16E] = {
            .name = "BIT 5, [HL]"
        };

        ops[0x16F] = {
            .name = "BIT 5, A"
        };

        ops[0x170] = {
            .name = "BIT 6, B"
        };

        ops[0x171] = {
            .name = "BIT 6, C"
        };

        ops[0x172] = {
            .name = "BIT 6, D"
        };

        ops[0x173] = {
            .name = "BIT 6, E"
        };

        ops[0x174] = {
            .name = "BIT 6, H"
        };

        ops[0x175] = {
            .name = "BIT 6, L"
        };

        ops[0x176] = {
            .name = "BIT 6, [HL]"
        };

        ops[0x177] = {
            .name = "BIT 6, A"
        };

        ops[0x178] = {
            .name = "BIT 7, B"
        };

        ops[0x179] = {
            .name = "BIT 7, C"
        };

        ops[0x17A] = {
            .name = "BIT 7, D"
        };

        ops[0x17B] = {
            .name = "BIT 7, E"
        };

        ops[0x17C] = {
            .name = "BIT 7, H"
        };

        ops[0x17D] = {
            .name = "BIT 7, L"
        };

        ops[0x17E] = {
            .name = "BIT 7, [HL]"
        };

        ops[0x17F] = {
            .name = "BIT 7, A"
        };

        ops[0x180] = {
            .name = "RES 0, B"
        };

        ops[0x181] = {
            .name = "RES 0, C"
        };

        ops[0x182] = {
            .name = "RES 0, D"
        };

        ops[0x183] = {
            .name = "RES 0, E"
        };

        ops[0x184] = {
            .name = "RES 0, H"
        };

        ops[0x185] = {
            .name = "RES 0, L"
        };

        ops[0x186] = {
            .name = "RES 0, [HL]"
        };

        ops[0x187] = {
            .name = "RES 0, A"
        };

        ops[0x188] = {
            .name = "RES 1, B"
        };

        ops[0x189] = {
            .name = "RES 1, C"
        };

        ops[0x18A] = {
            .name = "RES 1, D"
        };

        ops[0x18B] = {
            .name = "RES 1, E"
        };

        ops[0x18C] = {
            .name = "RES 1, H"
        };

        ops[0x18D] = {
            .name = "RES 1, L"
        };

        ops[0x18E] = {
            .name = "RES 1, [HL]"
        };

        ops[0x18F] = {
            .name = "RES 1, A"
        };

        ops[0x190] = {
            .name = "RES 2, B"
        };

        ops[0x191] = {
            .name = "RES 2, C"
        };

        ops[0x192] = {
            .name = "RES 2, D"
        };

        ops[0x193] = {
            .name = "RES 2, E"
        };

        ops[0x194] = {
            .name = "RES 2, H"
        };

        ops[0x195] = {
            .name = "RES 2, L"
        };

        ops[0x196] = {
            .name = "RES 2, [HL]"
        };

        ops[0x197] = {
            .name = "RES 2, A"
        };

        ops[0x198] = {
            .name = "RES 3, B"
        };

        ops[0x199] = {
            .name = "RES 3, C"
        };

        ops[0x19A] = {
            .name = "RES 3, D"
        };

        ops[0x19B] = {
            .name = "RES 3, E"
        };

        ops[0x19C] = {
            .name = "RES 3, H"
        };

        ops[0x19D] = {
            .name = "RES 3, L"
        };

        ops[0x19E] = {
            .name = "RES 3, [HL]"
        };

        ops[0x19F] = {
            .name = "RES 3, A"
        };

        ops[0x1A0] = {
            .name = "RES 4, B"
        };

        ops[0x1A1] = {
            .name = "RES 4, C"
        };

        ops[0x1A2] = {
            .name = "RES 4, D"
        };

        ops[0x1A3] = {
            .name = "RES 4, E"
        };

        ops[0x1A4] = {
            .name = "RES 4, H"
        };

        ops[0x1A5] = {
            .name = "RES 4, L"
        };

        ops[0x1A6] = {
            .name = "RES 4, [HL]"
        };

        ops[0x1A7] = {
            .name = "RES 4, A"
        };

        ops[0x1A8] = {
            .name = "RES 5, B"
        };

        ops[0x1A9] = {
            .name = "RES 5, C"
        };

        ops[0x1AA] = {
            .name = "RES 5, D"
        };

        ops[0x1AB] = {
            .name = "RES 5, E"
        };

        ops[0x1AC] = {
            .name = "RES 5, H"
        };

        ops[0x1AD] = {
            .name = "RES 5, L"
        };

        ops[0x1AE] = {
            .name = "RES 5, [HL]"
        };

        ops[0x1AF] = {
            .name = "RES 5, A"
        };

        ops[0x1B0] = {
            .name = "RES 6, B"
        };

        ops[0x1B1] = {
            .name = "RES 6, C"
        };

        ops[0x1B2] = {
            .name = "RES 6, D"
        };

        ops[0x1B3] = {
            .name = "RES 6, E"
        };

        ops[0x1B4] = {
            .name = "RES 6, H"
        };

        ops[0x1B5] = {
            .name = "RES 6, L"
        };

        ops[0x1B6] = {
            .name = "RES 6, [HL]"
        };

        ops[0x1B7] = {
            .name = "RES 6, A"
        };

        ops[0x1B8] = {
            .name = "RES 7, B"
        };

        ops[0x1B9] = {
            .name = "RES 7, C"
        };

        ops[0x1BA] = {
            .name = "RES 7, D"
        };

        ops[0x1BB] = {
            .name = "RES 7, E"
        };

        ops[0x1BC] = {
            .name = "RES 7, H"
        };

        ops[0x1BD] = {
            .name = "RES 7, L"
        };

        ops[0x1BE] = {
            .name = "RES 7, [HL]"
        };

        ops[0x1BF] = {
            .name = "RES 7, A"
        };

        ops[0x1C0] = {
            .name = "SET 0, B"
        };

        ops[0x1C1] = {
            .name = "SET 0, C"
        };

        ops[0x1C2] = {
            .name = "SET 0, D"
        };

        ops[0x1C3] = {
            .name = "SET 0, E"
        };

        ops[0x1C4] = {
            .name = "SET 0, H"
        };

        ops[0x1C5] = {
            .name = "SET 0, L"
        };

        ops[0x1C6] = {
            .name = "SET 0, [HL]"
        };

        ops[0x1C7] = {
            .name = "SET 0, A"
        };

        ops[0x1C8] = {
            .name = "SET 1, B"
        };

        ops[0x1C9] = {
            .name = "SET 1, C"
        };

        ops[0x1CA] = {
            .name = "SET 1, D"
        };

        ops[0x1CB] = {
            .name = "SET 1, E"
        };

        ops[0x1CC] = {
            .name = "SET 1, H"
        };

        ops[0x1CD] = {
            .name = "SET 1, L"
        };

        ops[0x1CE] = {
            .name = "SET 1, [HL]"
        };

        ops[0x1CF] = {
            .name = "SET 1, A"
        };

        ops[0x1D0] = {
            .name = "SET 2, B"
        };

        ops[0x1D1] = {
            .name = "SET 2, C"
        };

        ops[0x1D2] = {
            .name = "SET 2, D"
        };

        ops[0x1D3] = {
            .name = "SET 2, E"
        };

        ops[0x1D4] = {
            .name = "SET 2, H"
        };

        ops[0x1D5] = {
            .name = "SET 2, L"
        };

        ops[0x1D6] = {
            .name = "SET 2, [HL]"
        };

        ops[0x1D7] = {
            .name = "SET 2, A"
        };

        ops[0x1D8] = {
            .name = "SET 3, B"
        };

        ops[0x1D9] = {
            .name = "SET 3, C"
        };

        ops[0x1DA] = {
            .name = "SET 3, D"
        };

        ops[0x1DB] = {
            .name = "SET 3, E"
        };

        ops[0x1DC] = {
            .name = "SET 3, H"
        };

        ops[0x1DD] = {
            .name = "SET 3, L"
        };

        ops[0x1DE] = {
            .name = "SET 3, [HL]"
        };

        ops[0x1DF] = {
            .name = "SET 3, A"
        };

        ops[0x1E0] = {
            .name = "SET 4, B"
        };

        ops[0x1E1] = {
            .name = "SET 4, C"
        };

        ops[0x1E2] = {
            .name = "SET 4, D"
        };

        ops[0x1E3] = {
            .name = "SET 4, E"
        };

        ops[0x1E4] = {
            .name = "SET 4, H"
        };

        ops[0x1E5] = {
            .name = "SET 4, L"
        };

        ops[0x1E6] = {
            .name = "SET 4, [HL]"
        };

        ops[0x1E7] = {
            .name = "SET 4, A"
        };

        ops[0x1E8] = {
            .name = "SET 5, B"
        };

        ops[0x1E9] = {
            .name = "SET 5, C"
        };

        ops[0x1EA] = {
            .name = "SET 5, D"
        };

        ops[0x1EB] = {
            .name = "SET 5, E"
        };

        ops[0x1EC] = {
            .name = "SET 5, H"
        };

        ops[0x1ED] = {
            .name = "SET 5, L"
        };

        ops[0x1EE] = {
            .name = "SET 5, [HL]"
        };

        ops[0x1EF] = {
            .name = "SET 5, A"
        };

        ops[0x1F0] = {
            .name = "SET 6, B"
        };

        ops[0x1F1] = {
            .name = "SET 6, C"
        };

        ops[0x1F2] = {
            .name = "SET 6, D"
        };

        ops[0x1F3] = {
            .name = "SET 6, E"
        };

        ops[0x1F4] = {
            .name = "SET 6, H"
        };

        ops[0x1F5] = {
            .name = "SET 6, L"
        };

        ops[0x1F6] = {
            .name = "SET 6, [HL]"
        };

        ops[0x1F7] = {
            .name = "SET 6, A"
        };

        ops[0x1F8] = {
            .name = "SET 7, B"
        };

        ops[0x1F9] = {
            .name = "SET 7, C"
        };

        ops[0x1FA] = {
            .name = "SET 7, D"
        };

        ops[0x1FB] = {
            .name = "SET 7, E"
        };

        ops[0x1FC] = {
            .name = "SET 7, H"
        };

        ops[0x1FD] = {
            .name = "SET 7, L"
        };

        ops[0x1FE] = {
            .name = "SET 7, [HL]"
        };

        ops[0x1FF] = {
            .name = "SET 7, A"
        };

        return ops;
	}();
}