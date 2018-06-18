#pragma once

#include <cstdint>
#include <utility>
#include <gsl/span>

#define NEWPAT_ACT_LAM_WRAP(...) act([](hardware_t &hw, alu_t &alu, word_size_t w, gsl::span<evaluated_arg_t> args) -> void { __VA_ARGS__; })

namespace emu::patterns {
    enum aluflags_t : uint32_t {
        ALUFLAG_NONE = 0,
        ALUFLAG_CARRY = 0x0001,
        ALUFLAG_PARITY = 0x0004,
        ALUFLAG_ADJUST = 0x0010,
        ALUFLAG_ZERO = 0x0040,
        ALUFLAG_SIGN = 0x0080,
        ALUFLAG_TRAP = 0x0100,
        ALUFLAG_INTERRUPT = 0x0200,
        ALUFLAG_DIRECTION = 0x0400,
        ALUFLAG_OVERFLOW = 0x0800
    };

    enum register_t { 
        AX,      CX,      DX,      BX,
        AL = AX, CL = CX, DL = DX, BL = BX,
        SP,      BP,      SI,      DI,
        AH = SP, CH = BP, DH = SI, BH = DI
    };
    enum seg_register_t { ES, CS, SS, DS, FS, GS };

    enum word_size_t {
        BYTE = 1,
        WORD = 2,
        DWORD = 4
    };

    uintptr_t linearFromSegmented(uint16_t segment, uint16_t offset);

    class hardware_t {
    public:
        void poke(uintptr_t ptr, uint32_t value, word_size_t size);
        uint32_t peek(uintptr_t ptr, word_size_t size);
        uint32_t peekSigned(uintptr_t ptr, word_size_t size);

        void out(uint16_t port, uint32_t value, word_size_t size);
        uint32_t in(uint16_t port, word_size_t size);
        uint32_t inSigned(uint16_t port, word_size_t size);

        uint32_t reg(register_t reg, word_size_t size);
        uint32_t regSigned(register_t reg, word_size_t size);
        void setReg(register_t reg, uint32_t value, word_size_t size);
        
        uint16_t sreg(seg_register_t reg);
        void setSreg(seg_register_t reg, uint16_t value);

        uint16_t eip();
        void setEip(uint16_t value);
        void relJmpIf(bool condition, uint32_t value);
        
        aluflags_t eflags();
        void setEflags(aluflags_t value);
        bool hasFlag(aluflags_t flag);

        void push(uint32_t value, word_size_t size);
        uint32_t pop(word_size_t size);

        int interruptDepth();
        void enterInterrupt();
        void leaveInterrupt();
    };

    class pattern_node_t {
    public:
        pattern_node_t() {}
        pattern_node_t(int v) {}
    };

    const pattern_node_t modrm = pattern_node_t();
    const pattern_node_t sizeAndOrderMarked(uint8_t byte) { return pattern_node_t(); }
    const pattern_node_t sizeMarked(uint8_t byte) { return pattern_node_t(); }
    const pattern_node_t aluOpcode(uint8_t byte) { return pattern_node_t(); }
    const pattern_node_t encodedAluOpcode(uint8_t byte) { return pattern_node_t(); }
    const pattern_node_t ranged(uint8_t from, uint8_t to) { return pattern_node_t(); }
    const pattern_node_t strOp(uint8_t byte) { return pattern_node_t(); }

    class pattern_arg_t {
    public:
        pattern_arg_t() {}
        const pattern_arg_t withSize(word_size_t size) const { return pattern_arg_t(); }
        const pattern_arg_t withByteSize() const { return pattern_arg_t(); }
    };

    const pattern_arg_t aluLeft = pattern_arg_t();
    const pattern_arg_t aluRight = pattern_arg_t();
    const pattern_arg_t modrmRegValue = pattern_arg_t();
    const pattern_arg_t modrmRegRegister = pattern_arg_t();
    const pattern_arg_t modrmSegRegister = pattern_arg_t();
    const pattern_arg_t modrmReference = pattern_arg_t();
    const pattern_arg_t immediate = pattern_arg_t();
    const pattern_arg_t memoffset = pattern_arg_t();
    const pattern_arg_t relative = pattern_arg_t();
    const pattern_arg_t rangeIx = pattern_arg_t();
    const pattern_arg_t rangeReg = pattern_arg_t();
    const pattern_arg_t reg(register_t reg) { return pattern_arg_t(); };
    const pattern_arg_t sreg(seg_register_t reg) { return pattern_arg_t(); };
    const pattern_arg_t exact(uint32_t value) { return pattern_arg_t(); };

    class evaluated_arg_t {
    public:
        uint32_t deref();
        void set(uint32_t value);
        operator uint32_t() { return deref(); }
    };

    enum alu_opclass_t { ALU_OP80, ALU_OPC0, ALU_OPF6 };

    class alu_t {
    public:
        uint32_t aAdd(uint32_t lhs, uint32_t rhs, bool noModFlags = false);
        uint32_t aOr(uint32_t lhs, uint32_t rhs, bool noModFlags = false);
        uint32_t aAdc(uint32_t lhs, uint32_t rhs, bool noModFlags = false);
        uint32_t aSbb(uint32_t lhs, uint32_t rhs, bool noModFlags = false);
        uint32_t aAnd(uint32_t lhs, uint32_t rhs, bool noModFlags = false);
        uint32_t aSub(uint32_t lhs, uint32_t rhs, bool noModFlags = false);
        uint32_t aXor(uint32_t lhs, uint32_t rhs, bool noModFlags = false);
        uint32_t aCmp(uint32_t lhs, uint32_t rhs, bool noModFlags = false); /* equ to sub, maybe remove? */

        uint32_t dispatch(alu_opclass_t opClass, int opIndex, uint32_t lhs, uint32_t rhs, bool noModFlags = false);
    };

    using pattern_action_t = void(*)(hardware_t &hw, alu_t &alu, word_size_t w, gsl::span<evaluated_arg_t> args);
    
    class pattern_name_t {
    public:
        pattern_name_t() {}
        pattern_name_t(const char *singleName);

        const char *getName(...);
    };

    const pattern_name_t selectByRangeIx(std::initializer_list<const char*> names) { return pattern_name_t(); }
    const pattern_name_t selectByRegIx(std::initializer_list<const char*> names) { return pattern_name_t(); }
    const pattern_name_t withSizePostfix(std::initializer_list<const char*> names) { return pattern_name_t(); }

    class pattern_t {
    private:
        static const int MAX_NODES = 8;
        pattern_node_t m_nodes[MAX_NODES];
    public:
        pattern_t(pattern_name_t name, std::initializer_list<pattern_node_t> nodes) {
            // TODO fill   
        }

        const pattern_t withArgs(std::initializer_list<pattern_arg_t> args) const {
            return pattern_t(pattern_name_t(), {});
        }

        const pattern_t allByteSized() const { return pattern_t(pattern_name_t(), {}); }
        const pattern_t isUnsigned(bool isUnsigned) const { return pattern_t(pattern_name_t(), {}); }
        const pattern_t isUnsigned() const { return pattern_t(pattern_name_t(), {}); }

        const pattern_t repAllowed() const { return pattern_t(pattern_name_t(), {}); }
        const pattern_t repeAllowed() const { return pattern_t(pattern_name_t(), {}); }
        const pattern_t repneAllowed() const { return pattern_t(pattern_name_t(), {}); }
        
        const pattern_t onlyOnReg(int regv) const { return pattern_t(pattern_name_t(), {}); }
        const pattern_t onlyOnRegInRange(int from, int to) const { return pattern_t(pattern_name_t(), {}); }

        const pattern_t act(pattern_action_t action) const { return pattern_t(pattern_name_t(), {}); }
    };

    const pattern_t last_pattern = pattern_t(pattern_name_t(), {});
}