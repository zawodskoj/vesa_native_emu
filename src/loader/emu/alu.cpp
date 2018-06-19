#include <emu/newpat.h>

using namespace emu::patterns;

aluflags_t calculateFlags(uint64_t result, aluflags_t initial, aluflags_t allowedToChange, aluflags_t needToReset) {
    typename std::underlying_type<aluflags_t>::type newFlags = aluflags_t(initial & ~(needToReset| allowedToChange));
    if (result & 0x80000000) newFlags |= ALUFLAG_SIGN;
    if (!(result & 0xffffffff)) newFlags |= ALUFLAG_ZERO;
    // TODO parity flag
    if (result & 0x100000000) newFlags |= ALUFLAG_CARRY;
    if (!!(result & 0x200000000) ^ !!(result & 0x80000000)) newFlags |= ALUFLAG_OVERFLOW;
    return aluflags_t(newFlags);
}

#define ALU_OPERATOR(name, operator, flagsToChange, flagsToReset, ...) \
uint32_t alu_t::name(uint32_t lhs, uint32_t rhs, bool noModFlags) { \
    auto eflags = m_hw->eflags(); \
    auto result = lhs operator rhs __VA_ARGS__; \
    if (!noModFlags) { \
        eflags = calculateFlags(result, eflags, aluflags_t(flagsToChange), aluflags_t(flagsToReset)); \
        m_hw->setEflags(eflags); \
    } \
    return result; \
}

const aluflags_t OP_ANY_FLAGS = aluflags_t(ALUFLAG_CARRY | ALUFLAG_PARITY | ALUFLAG_OVERFLOW | ALUFLAG_SIGN | ALUFLAG_ZERO | ALUFLAG_ADJUST);
const aluflags_t OP_BIT_FLAGS = aluflags_t(ALUFLAG_SIGN | ALUFLAG_ZERO | ALUFLAG_PARITY);
const aluflags_t OP_BIT_RSFLAGS = aluflags_t(ALUFLAG_OVERFLOW | ALUFLAG_CARRY);

ALU_OPERATOR(aAdd, +, OP_ANY_FLAGS, 0)
ALU_OPERATOR(aOr, |, OP_BIT_FLAGS, OP_BIT_RSFLAGS)
ALU_OPERATOR(aAdc, +, OP_ANY_FLAGS, 0, +(eflags & ALUFLAG_CARRY ? 1 : 0))
ALU_OPERATOR(aSbb, -, OP_ANY_FLAGS, 0, -(eflags & ALUFLAG_CARRY ? 1 : 0))
ALU_OPERATOR(aAnd, &, OP_BIT_FLAGS, OP_BIT_RSFLAGS)
ALU_OPERATOR(aSub, -, OP_ANY_FLAGS, 0)
ALU_OPERATOR(aXor, ^, OP_BIT_FLAGS, OP_BIT_RSFLAGS)
ALU_OPERATOR(aCmp, ^, OP_BIT_FLAGS, OP_BIT_RSFLAGS)