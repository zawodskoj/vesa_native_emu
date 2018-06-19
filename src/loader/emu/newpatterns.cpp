#include <emu/newpat.h>

using namespace emu::patterns;
using reg_t = emu::patterns::register_t;

#define ACT(...) NEWPAT_ACT_LAM_WRAP(__VA_ARGS__)
#define ARG (args[0])
#define LHS (args[0])
#define RHS (args[1])
#define DEREF_REG(arg) (reg_t(arg.deref()))
#define GET_REG(arg, w) (hw->reg(reg_t(arg.deref()), w))
#define SET_REG(arg, v, w) (hw->setReg(reg_t(arg.deref()), v, w))

const pattern_t patterns[] = {
    pattern_t("add", { aluOpcode(0) })
        .withArgs({ aluLeft, aluRight })
        .ACT(LHS.set(alu->aAdd(LHS, RHS))),
    pattern_t("push", { 6 })
        .withArgs({ sreg(ES) })
        .ACT(hw->push(ARG, WORD)),
    pattern_t("pop", { 7 })
        .ACT(hw->setSreg(ES, hw->pop(WORD))),
    pattern_t("or", { aluOpcode(8) })
        .withArgs({ aluLeft, aluRight })
        .ACT(LHS.set(alu->aOr(LHS, RHS))),
    pattern_t("push", { 0x0e })
        .withArgs({ sreg(CS) })
        .ACT(hw->push(ARG, WORD)),
    /* skipping two-byte instructions */
    pattern_t("push", { 0x1e })
        .withArgs({ sreg(DS) })
        .ACT(hw->push(ARG, WORD)),
    pattern_t("pop", { 0x1f })
        .withArgs({ sreg(DS) })
        .ACT(hw->push(ARG, WORD)),
    pattern_t("and", { aluOpcode(0x20) })
        .withArgs({ aluLeft, aluRight })
        .ACT(LHS.set(alu->aAnd(LHS, RHS))),
    pattern_t("sub", { aluOpcode(0x28) })
        .withArgs({ aluLeft, aluRight })
        .ACT(LHS.set(alu->aSub(LHS, RHS))),
    pattern_t("xor", { aluOpcode(0x30) })
        .withArgs({ aluLeft, aluRight })
        .ACT(LHS.set(alu->aXor(LHS, RHS))),
    pattern_t("cmp", { aluOpcode(0x38) })
        .withArgs({ aluLeft, aluRight })
        .ACT(alu->aCmp(LHS, RHS)),
    pattern_t("inc", { ranged(0x40, 0x47) })
        .withArgs({ rangeReg })
        .ACT(ARG.set(ARG.deref() + 1)),
    pattern_t("dec", { ranged(0x48, 0x4f) })
        .withArgs({ rangeReg })
        .ACT(ARG.set(ARG.deref() - 1)),
    pattern_t("push", { ranged(0x50, 0x57) })
        .withArgs({ rangeReg })
        .ACT(hw->push(ARG, w)),
    pattern_t("pop", { ranged(0x58, 0x5f) })
        .withArgs({ rangeReg })
        .ACT(ARG.set(hw->pop(w))),
    pattern_t("pusha", { 0x60 })
        .ACT(
            auto sp = hw->reg(SP, w);
            hw->push(hw->reg(AX, w), w);
            hw->push(hw->reg(CX, w), w);
            hw->push(hw->reg(DX, w), w);
            hw->push(hw->reg(BX, w), w);
            hw->push(sp, w);
            hw->push(hw->reg(BP, w), w);
            hw->push(hw->reg(SI, w), w);
            hw->push(hw->reg(DI, w), w);
        ),
    pattern_t("popa", { 0x61 })
        .ACT(
            hw->setReg(DI, hw->pop(w), w);
            hw->setReg(SI, hw->pop(w), w);
            hw->setReg(BP, hw->pop(w), w);
            hw->pop(w);
            hw->setReg(BX, hw->pop(w), w);
            hw->setReg(DX, hw->pop(w), w);
            hw->setReg(CX, hw->pop(w), w);
            hw->setReg(AX, hw->pop(w), w);
        ),
    pattern_t("jo", { 0x70 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_OVERFLOW), ARG)),
    pattern_t("jno", { 0x71 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(!hw->hasFlag(ALUFLAG_OVERFLOW), ARG)),
    pattern_t("jb", { 0x72 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_CARRY), ARG)),
    pattern_t("jnb", { 0x73 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(!hw->hasFlag(ALUFLAG_CARRY), ARG)),
    pattern_t("jz", { 0x74 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_ZERO), ARG)),
    pattern_t("jnz", { 0x75 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(!hw->hasFlag(ALUFLAG_ZERO), ARG)),
    pattern_t("jbe", { 0x76 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_ZERO) || hw->hasFlag(ALUFLAG_CARRY), ARG)),
    pattern_t("jnbe", { 0x77 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(!hw->hasFlag(ALUFLAG_ZERO) && !hw->hasFlag(ALUFLAG_CARRY), ARG)),
    pattern_t("js", { 0x78 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_SIGN), ARG)),
    pattern_t("jns", { 0x79 })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(!hw->hasFlag(ALUFLAG_SIGN), ARG)),
    // no parity jumps (parity flag is not supported)
    pattern_t("jl", { 0x7c })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_SIGN) != hw->hasFlag(ALUFLAG_OVERFLOW), ARG)),
    pattern_t("jge", { 0x7d })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_SIGN) == hw->hasFlag(ALUFLAG_OVERFLOW), ARG)),
    pattern_t("jle", { 0x7e })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(hw->hasFlag(ALUFLAG_ZERO) || (hw->hasFlag(ALUFLAG_SIGN) != hw->hasFlag(ALUFLAG_OVERFLOW)), ARG)),
    pattern_t("jg", { 0x7f })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEipIf(!hw->hasFlag(ALUFLAG_ZERO) && (hw->hasFlag(ALUFLAG_SIGN) == hw->hasFlag(ALUFLAG_OVERFLOW)), ARG)),
    pattern_t(selectByRangeIx({ "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" }), { encodedAluOpcode(0x80) })
        .withArgs({ aluLeft, aluRight, rangeIx })
        .ACT(LHS.set(alu->dispatch(ALU_OP80, args[2].deref(), LHS, RHS))),
    pattern_t("test", { sizeMarked(0x84) })
        .withArgs({ modrmReference, modrmRegRegister })
        .ACT(alu->aAnd(LHS, RHS)),
    pattern_t("xchg", { sizeMarked(0x86) })
        .withArgs({ modrmRegRegister, modrmReference })
        .ACT(
            auto temp = LHS.deref();
            LHS.set(RHS);
            RHS.set(temp);
        ),
    pattern_t("mov", { sizeAndOrderMarked(0x88) })
        .withArgs({ modrmRegRegister, modrmReference })
        .ACT(LHS.set(RHS)),
    pattern_t("mov", { 0x8c })
        .withArgs({ modrmReference, modrmSegRegister }) // todo check: only register or memory reference, no r/m (wtf?)
        .ACT(LHS.set(RHS)),
    // lea is not supported now
    pattern_t("mov", { 0x8e })
        .withArgs({ modrmSegRegister, modrmReference })
        .ACT(LHS.set(RHS)),
    pattern_t("pop", { 0x8f })
        .withArgs({ modrmReference })
        .ACT(ARG.set(hw->pop(w))),
    pattern_t("nop", { 0x90 }).ACT(),
    pattern_t("xchg", { ranged(0x90, 0x97) })
        .withArgs({ rangeReg, reg(AX) })
        .ACT(
            auto temp = LHS.deref();
            LHS.set(RHS);
            RHS.set(temp);
        ),
    pattern_t("pushf", { 0x9c })
        .ACT(hw->push(hw->eflags(), w)),
    pattern_t("popf", { 0x9c })
        .ACT(hw->setEflags(aluflags_t(hw->pop(w)))),
    pattern_t("mov", { 0xa3 })
        .withArgs({ memoffset, reg(AX) })
        .ACT(LHS.set(RHS)),
    pattern_t(withSizePostfix({ "movsb", "movsw", "movsd" }), { strOp(0xa4) })
        .ACT(
            hw->poke(
                linearFromSegmented(hw->sreg(ES), hw->reg(DI, WORD)),
                hw->peek(linearFromSegmented(hw->sreg(DS), hw->reg(SI, WORD)), w),
                w);
            bool df = hw->hasFlag(ALUFLAG_DIRECTION);
            hw->setReg(DI, hw->reg(DI, WORD) + (df ? -1 : 1) * int(w), WORD); // TODO: ADDR-SIZE PREFIX
            hw->setReg(SI, hw->reg(SI, WORD) + (df ? -1 : 1) * int(w), WORD);
        ),
    pattern_t("test", { 0xab })
        .withArgs({ reg(AX), immediate })
            .allByteSized()
        .ACT(alu->aAnd(LHS, RHS)),
    pattern_t(withSizePostfix({ "stosb", "stosw", "stosd" }), { strOp(0xaa) })
        .ACT(
            hw->poke(
                linearFromSegmented(hw->sreg(ES), hw->reg(DI, WORD)),
                hw->reg(AX, w),
                w);
            bool df = hw->hasFlag(ALUFLAG_DIRECTION);
            hw->setReg(DI, hw->reg(DI, WORD) + (df ? -1 : 1) * int(w), WORD); // TODO: ADDR-SIZE PREFIX
        ),
    pattern_t("mov", { ranged(0xb0, 0xb7) })
        .withArgs({ rangeReg, immediate })
            .allByteSized()
        .ACT(LHS.set(RHS)),
    pattern_t("mov", { ranged(0xb8, 0xbf) })
        .withArgs({ rangeReg, immediate })
        .ACT(LHS.set(RHS)),
    pattern_t(selectByRangeIx({ "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar" }), { sizeMarked(0xc0) })
        .withArgs({ modrmReference, immediate.withSize(BYTE) })
        .ACT(LHS.set(alu->dispatch(ALU_OPC0, args[2].deref(), LHS, RHS))),
    pattern_t("ret", { 0xc3 })
        .ACT(hw->setEip(hw->pop(WORD))),
    pattern_t("int", { 0xcd })
        .withArgs({ immediate.withSize(BYTE) })
        .ACT(
            hw->enterInterrupt();
            hw->push(hw->eflags(), WORD);
            hw->push(hw->sreg(CS), WORD);
            hw->push(hw->eip(), WORD);
            auto farptr = hw->peek(ARG * sizeof(uint32_t), DWORD);
            hw->setSreg(CS, farptr >> 16);
            hw->setEip(farptr);
        ),
    pattern_t("iret", { 0xcf })
        .ACT(
            hw->setEip(hw->pop(WORD));
            hw->setSreg(CS, hw->pop(WORD));
            hw->setEflags(aluflags_t(hw->pop(WORD)));
            hw->leaveInterrupt();
        ),
    pattern_t(selectByRangeIx({ "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar" }), { sizeMarked(0xd0) })
        .withArgs({ modrmReference, exact(1) })
        .ACT(LHS.set(alu->dispatch(ALU_OPC0, args[2].deref(), LHS, RHS))),
    pattern_t(selectByRangeIx({ "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar" }), { sizeMarked(0xd2) })
        .withArgs({ modrmReference, reg(CX).withSize(BYTE) })
        .ACT(LHS.set(alu->dispatch(ALU_OPC0, args[2].deref(), LHS, RHS))),
    pattern_t("call", { 0xe8 })
        .withArgs({ relative })
        .ACT(
            hw->push(hw->eip(), WORD);
            hw->setEip(ARG);
        ),
    pattern_t("jmp", { 0xe9 })
        .withArgs({ relative })
        .ACT(hw->setEip(ARG)),
    pattern_t("jmp", { 0xeb })
        .withArgs({ relative.withSize(BYTE) })
        .ACT(hw->setEip(ARG)),
    pattern_t("in", { 0xec })
        .withArgs({ reg(AX).withSize(BYTE), reg(DX) })
        .ACT(LHS.set(hw->in(RHS, BYTE))),
    pattern_t("in", { 0xed })
        .withArgs({ reg(AX), reg(DX) })
        .ACT(LHS.set(hw->in(RHS, w))),
    pattern_t("out", { 0xec })
        .withArgs({ reg(DX), reg(AX).withSize(BYTE) })
        .ACT(hw->out(RHS, LHS, BYTE)),
    pattern_t("out", { 0xed })
        .withArgs({ reg(DX), reg(AX) })
        .ACT(hw->out(RHS, LHS, w)),
    pattern_t("test", { sizeMarked(0xf6) })
        .onlyOnRegInRange(0, 1)
        .withArgs({ modrmReference, immediate })
        .ACT(alu->aAnd(LHS, RHS)),
    pattern_t(selectByRegIx({ nullptr, nullptr, nullptr, nullptr, "mul", "imul", "div", "idiv" }), { sizeMarked(0xf6) })
        .onlyOnRegInRange(4, 7)
        .withArgs({ modrmRegValue, immediate })
        .ACT(alu->multiplicative(alu_multiplicative_op_t(args[0].deref()), args[1], w)),
    pattern_t("cld", { 0xfc })
        .ACT(hw->setEflags(aluflags_t(hw->eflags() & ~ALUFLAG_DIRECTION))),
    pattern_t("std", { 0xfd })
        .ACT(hw->setEflags(aluflags_t(hw->eflags() | ALUFLAG_DIRECTION))),
    pattern_t("call", { sizeMarked(0xff) })
        .onlyOnReg(2)
        .withArgs({ modrmReference })
        .ACT(
            hw->push(hw->eip(), WORD);
            hw->setEip(ARG);
        ),
    pattern_t("jmp", { sizeMarked(0xff) })
        .onlyOnReg(4)
        .withArgs({ modrmReference })
        .ACT(hw->setEip(ARG)),
    pattern_t("push", { sizeMarked(0xff) })
        .onlyOnReg(6)
        .withArgs({ modrmReference })
        .ACT(hw->push(ARG, w)),
    last_pattern
};