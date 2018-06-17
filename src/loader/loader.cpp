#include <cstdint>
#include <cstring>
#include <cstdio>
#include <emu/cpu.h>

struct __attribute__ ((packed)) vesamode_info_t {
    uint16_t rModeAttrs; // mode characteristics
    uint8_t rWinAAttrs; // windowing system characteristics
    uint8_t rWinBAttrs; // windowing system characteristics
    uint16_t wWinGran; // window granularity, in Kbytes
    uint16_t wWinSize; // window size, in KB
    uint16_t wWinASeg; // window A segment address
    uint16_t wWinBSeg; // window B segment address
    uint32_t pfWinFnPtr; // far address of window-handling function
    uint16_t wScanLineSiz; // bytes per scan line
    uint16_t wHorizRes; // horizontal resolution, in pixels or char cells
    uint16_t wVertRes; // vertical resolution, in pixels or char cells
    uint8_t bCharWide; // character cell width
    uint8_t bCharHigh; // character cell height
    uint8_t bPlaneCnt; // number of video memory planes
    uint8_t bBitsPerPel; // number of bits per pixel
    uint8_t bBankCnt; // number of video memory banks
    uint8_t bMemModel; // memory model type code
    uint8_t bBankSize; // video memory bank size, in Kbytes
};

struct __attribute__ ((packed)) vesainfo_t {
    uint32_t abSignature;
    uint16_t wVersion;
    uint32_t pfszOemStr;
    uint32_t rAbilities;
    uint32_t pfawModes;
};

extern "C" void loader_main(void) {
    printf("trying to emulate bullshit\n");
    cpu_t cpu;
    
    printf("determining int handler: ");
    
    auto eip = *reinterpret_cast<uint16_t*>(0x40);
    auto cs = *reinterpret_cast<uint16_t*>(0x42);

    printf("%04x:%04x, querying vesa information...\n", unsigned(cpu.cs), unsigned(cpu.eip));

    cpu.eax = 0x4f00;
    cpu.es = 0x6000;
    cpu.edi = 0;
    cpu.esp = 0xffe8;
    cpu.eip = eip;
    cpu.cs = cs;
    
    cpu.runToIret();
    if ((cpu.eax & 0xffff) != 0x4f00) {
        printf("vesa is not supported: return code is %04x\n", unsigned(cpu.eax));
        while (1);
    }
    else printf("vesa ok\n");

    vesainfo_t *vesainfo = reinterpret_cast<vesainfo_t*>(0x60000);
    uint16_t *allowedModes = reinterpret_cast<uint16_t*>(((vesainfo->pfawModes & 0xffff0000) >> 12) + (vesainfo->pfawModes & 0xffff));

    printf("modes list: %p\n", allowedModes);

    while (*allowedModes != 0xffff) {
        printf("mode: %04x\n", *allowedModes++);
    }

    /*cpu.eax = 0x4f02;
    cpu.ecx = 0x101;
    cpu.ss = cpu.ds = 0x7000;
    cpu.esp = 0xffe8;
    cpu.es = 0x6000;
    cpu.edi = 0;
    cpu.eip = eip;
    cpu.cs = cs;

    cpu.runToIret();

    vesamode_info_t *modeinfo = reinterpret_cast<vesamode_info_t*>(0x60000);

    printf("W: %d, H: %d, bpp: %d\n", modeinfo->wHorizRes, modeinfo->wVertRes, modeinfo->bBitsPerPel);
    printf("modesetting...\n");
    
    cpu.eax = 0x13;
    cpu.ebx = 0x101;
    cpu.edx = 0x80;
    cpu.ecx = 0x90000;
    cpu.ss = cpu.ds = 0x0000;
    cpu.esp = 0xffd0;
    cpu.esi = 0xe0000;
    cpu.edi = 0xffac;
    cpu.eip = eip;
    cpu.cs = cs;

    cpu.runToIret();
    
    printf("emulated, testing output\n");
    for (int i = 0; i < 320; i++)
        for (int j = 0; j < 200; j++)
        {
            *reinterpret_cast<uint8_t*>(0xa0000 + j * 640 + i) = i * j;
        }*/
    
    while (1);
}
