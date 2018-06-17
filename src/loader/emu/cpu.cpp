#include "cpu.h"
#include <cstdio>
#include <cstring>
#include <type_traits>

#define OVERRIDE_OPERAND(x) (x)
//#define NO_TRACE_OUTPUT

uint32_t truncateToSize(uint32_t val, ref_width_t wid) {
    switch (wid)
    {
    case ref_width_t::BYTE: return uint8_t(val);
    case ref_width_t::WORD: return uint16_t(val);
    default: return val;
    }
}

parsed_reference_t cpu_t::parseRef(modrm_t modrm, uint8_t bytes[], size_t &position, ref_width_t refw, cpu_register_t override) {
    parsed_reference_t ref = {};
    ref.exists = true;
    ref.modrmRegField = modrm.reg;

    uint32_t displacement = 0;
    if ((modrm.mod == 0 && modrm.reg == 0b110) || modrm.mod == 2) {
        displacement = signExtendWord(bytes[position] | (bytes[position + 1] << 8));
        position += 2;
    }
    if (modrm.mod == 1) displacement = signExtendByte(bytes[position++]);
    if (modrm.mod == 3) { // register reference
        switch (refw) {
        case ref_width_t::BYTE: strncpy(ref.desc, regNamesByte[modrm.rm], sizeof(ref.desc)); break;
        case ref_width_t::WORD: strncpy(ref.desc, regNamesWord[modrm.rm], sizeof(ref.desc)); break;
        case ref_width_t::DWRD: strncpy(ref.desc, regNamesDwrd[modrm.rm], sizeof(ref.desc)); break;
        }
        ref.isRegisterReference = true;
        ref.ref.referencedRegister = cpu_register_t(modrm.rm);
    }
    else {
        int32_t idisp = int32_t(displacement);
        uint32_t moddisp = idisp < 0 && idisp != int32_t(0x80000000) ? -idisp : idisp;
#define PICKFORMAT(nodisp, withdisp) (modrm.mod == 0 ? nodisp : withdisp)
#define SIG (idisp < 0 ? '-' : '+')
#define SET_VALID_SEG(defaultSeg) if (override == cpu_register_t::REG_NOOVERRIDE)\
    ref.ref.linearAddress.segment = peekw(selseg = defaultSeg);\
else\
    ref.ref.linearAddress.segment = peekw(selseg = override);
#define SEGMENTR (sregNames[ungeneralizeSegmentRegister(selseg)])
        cpu_register_t selseg;
        switch (modrm.rm) {
        case 0:
            SET_VALID_SEG(cpu_register_t::REG_DS_G);
            snprintf(ref.desc, sizeof(ref.desc), PICKFORMAT("%s:[bx + si]", "%s:[bx + si %c 0x%lx]"), SEGMENTR, SIG, moddisp);
            ref.ref.linearAddress.offset = uint16_t(uint16_t(ebx) + uint16_t(esi) + displacement);
            break;
        case 1:
            SET_VALID_SEG(cpu_register_t::REG_DS_G);
            snprintf(ref.desc, sizeof(ref.desc), PICKFORMAT("%s:[bx + di]", "%s:[bx + di %c 0x%lx]"), SEGMENTR, SIG, moddisp);
            ref.ref.linearAddress.offset = uint16_t(uint16_t(ebx) + uint16_t(edi) + displacement);
            break;
        case 2:
            SET_VALID_SEG(cpu_register_t::REG_SS_G);
            snprintf(ref.desc, sizeof(ref.desc), PICKFORMAT("%s:[bp + si]", "%s:[bp + si %c 0x%lx]"), SEGMENTR, SIG, moddisp);
            ref.ref.linearAddress.offset = uint16_t(uint16_t(ebp) + uint16_t(esi) + displacement);
            break;
        case 3:
            SET_VALID_SEG(cpu_register_t::REG_SS_G);
            snprintf(ref.desc, sizeof(ref.desc), PICKFORMAT("%s:[bp + di]", "%s:[bp + di %c 0x%lx]"), SEGMENTR, SIG, moddisp);
            ref.ref.linearAddress.offset = uint16_t(uint16_t(ebp) + uint16_t(edi) + displacement);
            break;
        case 4:
            SET_VALID_SEG(cpu_register_t::REG_DS_G);
            snprintf(ref.desc, sizeof(ref.desc), PICKFORMAT("%s:[si]", "%s:[si %c 0x%lx]"), SEGMENTR, SIG, moddisp);
            ref.ref.linearAddress.offset = uint16_t(uint16_t(esi) + displacement);
            break;
        case 5:
            SET_VALID_SEG(cpu_register_t::REG_DS_G);
            snprintf(ref.desc, sizeof(ref.desc), PICKFORMAT("%s:[di]", "%s:[di %c 0x%lx]"), SEGMENTR, SIG, moddisp);
            ref.ref.linearAddress.offset = uint16_t(uint16_t(edi) + displacement);
            break;
        case 6:
            if (modrm.mod == 0) {
                SET_VALID_SEG(cpu_register_t::REG_DS_G);
                snprintf(ref.desc, sizeof(ref.desc), idisp < 0 ? "%s:[-0x%lx]" : "%s:[0x%lx]", SEGMENTR, moddisp);
                ref.ref.linearAddress.offset = displacement;
            }
            else {
                SET_VALID_SEG(cpu_register_t::REG_SS_G);
                snprintf(ref.desc, sizeof(ref.desc), "%s:[bp %c 0x%lx]", SEGMENTR, SIG, moddisp);
                ref.ref.linearAddress.offset = uint16_t(uint16_t(ebp) + displacement);
            }
            break;
        case 7:
            SET_VALID_SEG(cpu_register_t::REG_DS_G);
            snprintf(ref.desc, sizeof(ref.desc), PICKFORMAT("%s:[bx]", "%s:[bx %c 0x%lx]"), SEGMENTR, SIG, moddisp);
            ref.ref.linearAddress.offset = uint16_t(uint16_t(ebx) + displacement);
            break;
        }
    }

    return ref;
}

const uint8_t OP_ADDRESS_SIZE_PREFIX = 0x67;
const uint8_t OP_OPERAND_SIZE_PREFIX = 0x66;

enum cpu_instruction_prefix_t
{
    NOPREFIX,
    REP_REPE,
    REPNE
};

void cpu_t::executeSingleInstruction() {
    uint8_t instructionBuffer[MAX_INSTRUCTION_SIZE];
    uint32_t matched[MAX_OPCODE_PATTERN_SIZE];

#ifndef NO_TRACE_OUTPUT
    printf("%lu (esp: 0x%04lx, ebp: 0x%04lx) [%04x:%04x]    ", instructionsExecuted, esp, ebp, cs, uint16_t(eip));
#endif

    // reading instruction
    for (size_t i = 0; i < MAX_INSTRUCTION_SIZE; i++) instructionBuffer[i] = peekb(cs, eip + i);
    auto override = cpu_register_t::REG_NOOVERRIDE;

    size_t prefetchedBytes = 0;

    cpu_instruction_prefix_t prefix = NOPREFIX;
    bool overrideOperand = false;

    switch (instructionBuffer[prefetchedBytes++])
    {
    case 0xf3: prefix = REP_REPE; break;
    case 0xf2: prefix = REPNE; break;
    case 0xf0: printf("LOCK PREFIX IS NOT SUPPORTED\n"); while (1);
    default: prefetchedBytes--;
    }

    if (instructionBuffer[prefetchedBytes] == OP_ADDRESS_SIZE_PREFIX)
    {
        printf("ADDRESS SIZE PREFIXES IS NOT SUPPORTED\n"); while (1);
    }

    if (instructionBuffer[prefetchedBytes] == OP_OPERAND_SIZE_PREFIX)
    {
        printf("!OP! ");
        prefetchedBytes++;
        overrideOperand = true;
        //printf("OPERAND SIZE PREFIXES IS NOT SUPPORTED\n"); while (1);
    }

    switch (instructionBuffer[prefetchedBytes++])
    {
    case 0x26: override = cpu_register_t::REG_ES_G; break;
    case 0x2e: override = cpu_register_t::REG_CS_G; break;
    case 0x36: override = cpu_register_t::REG_SS_G; break;
    case 0x3e: override = cpu_register_t::REG_DS_G; break;
    case 0x64: override = cpu_register_t::REG_FS_G; break;
    case 0x65: override = cpu_register_t::REG_GS_G; break;
    default: prefetchedBytes--;
    }

    // matching instruction to pattern
    for (size_t patternIx = 0; patternIx < MAX_OPCODE_PATTERNS; patternIx++) {
        if (m_opcodePatterns[patternIx].pattern[0] == P_LASTPATTERN) break;
        auto pattern = &m_opcodePatterns[patternIx];
        size_t matchedIx = 0, fetchedBytes = prefetchedBytes;
        bool gotModrm = false;
        
        union {
            modrm_t struc;
            uint8_t byte;
        } modrm;

        for (size_t partIx = 0; partIx < MAX_OPCODE_PATTERN_SIZE; partIx++) {
            uint8_t byteToMatch = 0;
            parsed_reference_t ref = {};
            opcode_name_t name = {};

            switch (pattern->pattern[partIx]) {
            case 0: // last part is zero, pattern is completed
                if (pattern->predicate && !pattern->predicate(instructionBuffer + prefetchedBytes))
                    goto failedPattern;
                if (gotModrm)
                    ref = parseRef(modrm.struc, instructionBuffer, fetchedBytes, OVERRIDE_OPERAND(pattern->defWidth), override);

                eip += fetchedBytes;
#ifndef NO_TRACE_OUTPUT
                name = pattern->nameSel(ref);
                switch (prefix)
                {
                case REP_REPE: printf("rep "); break; // TODO determine REP/REPE
                case REPNE: printf("repne "); break;
                case NOPREFIX: break;
                }
                printf(name.name);
                switch (name.lhs) {
                case opcode_arg_t::EXACT:
                    printf(" %s", name.exactLhs(instructionBuffer));
                    break;
                case opcode_arg_t::MODRM_REF:
                    printf(" %s", ref.desc);
                    break;
                case opcode_arg_t::RELATIVE: // какой уебищный формат
                {
                    int32_t iimm = int32_t(matched[name.lhsImmAt]);
                    uint32_t modImm = iimm < 0 && iimm != int32_t(0x80000000) ? -iimm : iimm;
                    printf(iimm < 0 ? " .-0x%lx (%04x)" : " .+0x%lx (%04x)", modImm, uint16_t(eip + iimm));
                }
                break;
                case opcode_arg_t::IMMEDIATE:
                    printf(" 0x%lx", truncateToSize(matched[name.lhsImmAt], pattern->defWidth));
                    break;
                break;
                case opcode_arg_t::NONE: break;
                }
                switch (name.rhs) {
                case opcode_arg_t::EXACT:
                    printf(", %s", name.exactRhs(instructionBuffer));
                    break;
                case opcode_arg_t::MODRM_REF:
                    printf(", %s", ref.desc);
                    break;
                case opcode_arg_t::RELATIVE:
                {
                    int32_t iimm = int32_t(matched[name.rhsImmAt]);
                    uint32_t modImm = iimm < 0 && iimm != int32_t(0x80000000) ? -iimm : iimm;
                    printf(iimm < 0 ? ", .-0x%lx (%04x)" : ", .+0x%lx (%04x)", modImm, uint16_t(eip + iimm));
                }
                break;
                case opcode_arg_t::IMMEDIATE:
                    printf(", 0x%lx", truncateToSize(matched[name.rhsImmAt], pattern->defWidth));
                    break;
                case opcode_arg_t::NONE: break;
                }
                putchar('\n');
#endif
                /* TODO CHECK INSTRUCTION PREFIX VALIDITY */
                switch (prefix)
                {
                case REP_REPE:
                    for (; uint16_t(ecx) > 0; ecx--)
                    {
                        pattern->action(*this, ref, matched, instructionBuffer + prefetchedBytes);
                    }
                    break;
                case REPNE:
                    printf("REPNE/LOCK prefixes are not supported");
                    while (1);
                case NOPREFIX:
                    pattern->action(*this, ref, matched, instructionBuffer + prefetchedBytes);
                    break;
                }
                return;
            case P_MODRM16:
                gotModrm = true;
                modrm.byte = instructionBuffer[fetchedBytes++];
                break;
            case P_MODRM32:
                printf("P_MODRM32 - not implemented");
                while (1); // not implemented
            case P_BYTE_VAL:
                matched[matchedIx++] = signExtendByte(instructionBuffer[fetchedBytes++]);
                break;
            case P_WORD_VAL:
                matched[matchedIx++] = signExtendWord(*reinterpret_cast<uint16_t*>(&instructionBuffer[fetchedBytes]));
                fetchedBytes += 2;
                break;
            case P_DWRD_VAL:
                matched[matchedIx++] = *reinterpret_cast<uint32_t*>(&instructionBuffer[fetchedBytes]);
                fetchedBytes += 4;
                break;
            default:
                byteToMatch = pattern->pattern[partIx] == P_ZERO ? 0 : pattern->pattern[partIx];
                if (instructionBuffer[fetchedBytes++] != byteToMatch) {
                    goto failedPattern;
                }
            }
        }
    failedPattern:;
    }

    printf("unmatched: %02x %02x %02x %02x\n", instructionBuffer[0], instructionBuffer[1], instructionBuffer[2], instructionBuffer[3]);
    while (1);
}

cpu_flags_t calculateFlags(uint64_t result, cpu_flags_t initial, cpu_flags_t allowedToChange, cpu_flags_t needToReset) {
    typename std::underlying_type<cpu_flags_t>::type newFlags = cpu_flags_t(initial & ~(needToReset| allowedToChange));
    if (result & 0x80000000) newFlags |= CPUFLAG_SIGN;
    if (!(result & 0xffffffff)) newFlags |= CPUFLAG_ZERO;
    // TODO parity flag
    if (result & 0x100000000) newFlags |= CPUFLAG_CARRY;
    if (!!(result & 0x200000000) ^ !!(result & 0x80000000)) newFlags |= CPUFLAG_OVERFLOW;
    return cpu_flags_t(newFlags);
}

#define OP_ALU(operator, flagsToChange, flagsToReset, mutateValue, ...) { \
    auto result = signExtendDword(lhsRef.getValue(*this, refWidth)) operator signExtendDword(rhsRef.getValue(*this, refWidth)) __VA_ARGS__; \
    eflags = calculateFlags(result, eflags, cpu_flags_t(flagsToChange), cpu_flags_t(flagsToReset)); \
    if (mutateValue) lhsRef.setValue(*this, refWidth, result); \
} break;

#define OP_ANY_FLAGS (CPUFLAG_CARRY | CPUFLAG_PARITY | CPUFLAG_OVERFLOW | CPUFLAG_SIGN | CPUFLAG_ZERO | CPUFLAG_ADJUST)
#define OP_BIT_FLAGS (CPUFLAG_SIGN | CPUFLAG_ZERO | CPUFLAG_PARITY)
#define OP_BIT_RSFLAGS (CPUFLAG_OVERFLOW | CPUFLAG_CARRY)

void cpu_t::aluOperation(cpu_alu_op_t aluOp, reference_t lhsRef, reference_t rhsRef, ref_width_t refWidth) {
    switch (aluOp) {
    case ALU_OP_ADD: OP_ALU(+, OP_ANY_FLAGS, 0, true)
    case ALU_OP_OR:  OP_ALU(| , OP_BIT_FLAGS, OP_BIT_RSFLAGS, true)
    case ALU_OP_ADC: OP_ALU(+, OP_ANY_FLAGS, 0, true, +(eflags & CPUFLAG_CARRY ? 1 : 0))
    case ALU_OP_SBB: OP_ALU(+, OP_ANY_FLAGS, 0, true, -(eflags & CPUFLAG_CARRY ? 1 : 0))
    case ALU_OP_AND: OP_ALU(&, OP_BIT_FLAGS, OP_BIT_RSFLAGS, true)
    case ALU_OP_SUB: OP_ALU(-, OP_ANY_FLAGS, 0, true)
    case ALU_OP_XOR: OP_ALU(^, OP_BIT_FLAGS, OP_BIT_RSFLAGS, true)
    case ALU_OP_CMP: OP_ALU(-, OP_ANY_FLAGS, 0, false)

    case ALU_OP_ROL: printf("NOT IMPLEMENTED"); while (1);
    case ALU_OP_ROR: printf("NOT IMPLEMENTED"); while (1);
    case ALU_OP_RCL: printf("NOT IMPLEMENTED"); while (1);
    case ALU_OP_RCR: printf("NOT IMPLEMENTED"); while (1);
    case ALU_OP_SHL_SAL: 
    case ALU_OP_SAL_SHL: OP_ALU(<< , OP_ANY_FLAGS, 0, true)
    case ALU_OP_SHR: OP_ALU(>> , OP_ANY_FLAGS, 0, true)
    case ALU_OP_SAR: printf("NOT IMPLEMENTED"); while (1);

    case ALU_OP_TEST:
    case ALU_OP_TEST2: OP_ALU(&, OP_BIT_FLAGS, OP_BIT_RSFLAGS, false)
    case ALU_OP_NOT: printf("NOT IMPLEMENTED"); while (1);
    case ALU_OP_NEG: printf("NOT IMPLEMENTED"); while (1);
    case ALU_OP_MUL: 
        switch (refWidth)
        {
        case ref_width_t::BYTE:
            {
                auto result = uint64_t(uint8_t(eax)) * uint64_t(lhsRef.getValueUnsigned(*this, ref_width_t::BYTE));
                eflags = calculateFlags(result, eflags, cpu_flags_t(OP_ANY_FLAGS), cpu_flags_t(0));
                eax = (eax & 0xffff0000) | uint16_t(result);
            }
            break;
        case ref_width_t::WORD:
            {
                auto result = uint64_t(uint16_t(eax)) * uint64_t(lhsRef.getValueUnsigned(*this, ref_width_t::WORD));
                eflags = calculateFlags(result, eflags, cpu_flags_t(OP_ANY_FLAGS), cpu_flags_t(0));
                eax = (eax & 0xffff0000) | uint16_t(result);
                edx = (edx & 0xffff0000) | uint16_t(result >> 16);
            }
            break;
        case ref_width_t::DWRD:
            printf("NOT IMPLEMENTED"); while (1);
        }
        break;
    case ALU_OP_IMUL:
        switch (refWidth)
        {
        case ref_width_t::BYTE:
            {
                auto result = int64_t(int8_t(eax)) * int64_t(lhsRef.getValue(*this, ref_width_t::BYTE));
                eflags = calculateFlags(result, eflags, cpu_flags_t(OP_ANY_FLAGS), cpu_flags_t(0));
                eax = (eax & 0xffff0000) | uint16_t(result);
            }
            break;
        case ref_width_t::WORD:
            {
                auto result = int64_t(int16_t(eax)) * int64_t(lhsRef.getValue(*this, ref_width_t::WORD));
                eflags = calculateFlags(result, eflags, cpu_flags_t(OP_ANY_FLAGS), cpu_flags_t(0));
                eax = (eax & 0xffff0000) | uint16_t(result);
                edx = (edx & 0xffff0000) | uint16_t(result >> 16);
            }
            break;
        case ref_width_t::DWRD:
            printf("NOT IMPLEMENTED"); while (1);
        }
        break;
    case ALU_OP_DIV:
        switch (refWidth)
        {
        case ref_width_t::BYTE:
            {
                auto quotient = uint64_t(uint16_t(eax)) / uint64_t(lhsRef.getValueUnsigned(*this, ref_width_t::BYTE));
                auto remainder = uint64_t(uint16_t(eax)) % uint64_t(lhsRef.getValueUnsigned(*this, ref_width_t::BYTE));
                eax = (eax & 0xffff0000) | uint8_t(quotient) | (uint8_t(remainder) << 8);
            }
            break;
        case ref_width_t::WORD:
            {
                auto quotient = uint64_t(uint16_t(eax) | (uint16_t(edx) << 16)) / uint64_t(lhsRef.getValueUnsigned(*this, ref_width_t::WORD));
                auto remainder = uint64_t(uint16_t(eax) | (uint16_t(edx) << 16)) % uint64_t(lhsRef.getValueUnsigned(*this, ref_width_t::WORD));
                eax = (eax & 0xffff0000) | uint16_t(quotient);
                edx = (edx & 0xffff0000) | uint16_t(remainder);
            }
            break;
        case ref_width_t::DWRD:
            printf("NOT IMPLEMENTED"); while (1);
        }
        break;
    case ALU_OP_IDIV:
        switch (refWidth)
        {
        case ref_width_t::BYTE:
        {
            auto quotient = int64_t(int16_t(eax)) / int64_t(lhsRef.getValue(*this, ref_width_t::BYTE));
            auto remainder = int64_t(int16_t(eax)) % int64_t(lhsRef.getValue(*this, ref_width_t::BYTE));
            eax = (eax & 0xffff0000) | uint8_t(quotient) | (uint8_t(remainder) << 8);
        }
        break;
        case ref_width_t::WORD:
        {
            auto quotient = int64_t(int16_t(eax) | int32_t(uint16_t(edx) << 16)) / int64_t(lhsRef.getValue(*this, ref_width_t::WORD));
            auto remainder = int64_t(int16_t(eax) | int32_t(uint16_t(edx) << 16)) % int64_t(lhsRef.getValue(*this, ref_width_t::WORD));
            eax = (eax & 0xffff0000) | uint16_t(quotient);
            edx = (edx & 0xffff0000) | uint16_t(remainder);
        }
        break;
        case ref_width_t::DWRD:
            printf("NOT IMPLEMENTED"); while (1);
        }
        break;

    default: printf("UNSUPPORTED"); while (1); // todo unsupported
    }
}