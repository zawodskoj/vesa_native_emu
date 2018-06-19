#include <emu/newpat.h>

using namespace emu::patterns;

static inline void rewrite(uint32_t *val, uint32_t newVal, word_size_t newValSz) {
    if (newValSz == DWORD) {
        *val = newVal;
    } else {
        auto mask = (1 << (int(newValSz) * 8)) - 1; 
        *val = (*val & ~mask) | (newVal & mask);
    }
}

static inline uint32_t take(uint32_t val, word_size_t newValSz, bool isSigned) {
    if (newValSz == DWORD) {
        return val;
    } else {
        auto mask = (1 << (int(newValSz) * 8)) - 1; 
        auto unsig = val & mask;
        return isSigned ? unsig | ((0xffffffff * !!(unsig & 1 << (int(newValSz) * 8 - 1))) & ~mask) : unsig;
    }
}

void hardware_t::poke(uintptr_t ptr, uint32_t value, word_size_t size) {
    rewrite(reinterpret_cast<uint32_t*>(ptr), value, size);
}
uint32_t hardware_t::peek(uintptr_t ptr, word_size_t size) { return take(*reinterpret_cast<uint32_t*>(ptr), size, false); }
uint32_t hardware_t::peekSigned(uintptr_t ptr, word_size_t size) { return take(*reinterpret_cast<uint32_t*>(ptr), size, true); }

void hardware_t::out(uint16_t port, uint32_t value, word_size_t size) {
    printf("io is not supported now!\n"); while (1);
}
uint32_t hardware_t::in(uint16_t port, word_size_t size) {
    printf("io is not supported now!\n"); while (1); return 0;
}
uint32_t hardware_t::inSigned(uint16_t port, word_size_t size) {
    printf("io is not supported now!\n"); while (1); return 0;
}

uint32_t hardware_t::reg(register_t reg, word_size_t size) { 
    if (size == BYTE && reg >= AH)
        return take(m_registers[reg - AH], WORD, false) >> 8;
    else
        return take(m_registers[reg], size, false);
}
uint32_t hardware_t::regSigned(register_t reg, word_size_t size) {
    if (size == BYTE && reg >= AH)
        return uint32_t(int32_t(take(m_registers[reg - AH], WORD, true)) >> 8);
    else
        return take(m_registers[reg], size, true);
}
void hardware_t::setReg(register_t reg, uint32_t value, word_size_t size) {
    if (size == BYTE && reg >= AH) {
        value = take(m_registers[reg - AH], BYTE, false) | ((value & 0xff) << 8);
        rewrite(&m_registers[reg - AH], value, WORD);
    } else rewrite(&m_registers[reg], value, size);
}

uint16_t hardware_t::sreg(seg_register_t reg) { return m_segments[reg]; }
void hardware_t::setSreg(seg_register_t reg, uint16_t value) { m_segments[reg] = value; }

uint16_t hardware_t::eip() { return m_eip; }
void hardware_t::setEip(uint16_t value) { m_eip = value; }
void hardware_t::setEipIf(bool condition, uint16_t value) { if (condition) m_eip = value; }

aluflags_t hardware_t::eflags() { return m_eflags; }
void hardware_t::setEflags(aluflags_t value) { m_eflags = value; }
bool hardware_t::hasFlag(aluflags_t flag) { return m_eflags & flag; }

void hardware_t::push(uint32_t value, word_size_t size) {
    if (size == BYTE) { printf("BYTE size is not allowed for push/pop!\n"); while (1); }

    auto sp = reg(SP, WORD);
    if (sp > 0xffff - unsigned(size)) { printf("#SS: stack overflow!\n"); while (1); }
    setReg(SP, sp - int(size), WORD);
    poke(linearFromSegmented(sreg(SS), sp), value, size);
}

uint32_t hardware_t::pop(word_size_t size) {
    if (size == BYTE) { printf("BYTE size is not allowed for push/pop!\n"); while (1); }

    auto sp = reg(SP, WORD);
    if (sp < unsigned(size)) { printf("#SS: stack overflow!\n"); while (1); }
    setReg(SP, sp + unsigned(size), WORD);
    return peek(linearFromSegmented(sreg(SS), sp - int(size)), size);
}

int hardware_t::interruptDepth() { return m_depth; }
void hardware_t::enterInterrupt() { m_depth++; }
void hardware_t::leaveInterrupt() { m_depth--; }