#include <stdint.h>
this is a test
this is a test
#include <stdlib.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;
ithis is another test

typedef uint32_t u32;
typedef uint64_t u64;

#define MEMORY_SIZE 1<<16

#define WORD_SIZE 2

// byte access is valid from 0x0000..0xFFFE
#define UPPER_16BIT_BOUND 0xFFFE

// word access is valid from 0x0000..0xFFFE
#define UPPER_8BIT_BOUND 0xFFFF

// 0000
#define HALT 0x0

// 0001
#define LDI 0x1

// 0010
#define MOV 0x2

// 0011
#define ADD 0x3

// 0100
#define LOAD 0x4

// 0101
#define STORE 0x5

//  - HALT
//  - LDI or LI
//  - MOV
//  - LOAD
//  - STORE
//  - ADD
//  - ADDI
//  - SUB
//  - CMP
//  - AND
//  - OR
//  - XOR
//  - JMP
//  - BR or conditional jump
//  - PUSH
//  - POP


// flags
#define ZF 0
#define NF 0
#define CF 0
#define VF 0

// 3 bits max
#define NOREG 0x0
#define R0 0x0
#define R1 0x1
#define R2 0x2
#define R3 0x3
#define R4 0x4
#define R5 0x5
#define R6 0x6
#define R7 0x7

typedef struct CPU {
    u16 registers[8];
    u16 pc;

    // Stack pointer model
    // ---
    // - memory is byte-addressed
    // - stack grows downward
    // - SP points to the next free stack slot
    // - one PUSH16 moves SP by 2
    // - one POP16 moves SP by 2
    // first word is always unused because SP points to the next "free word"
    u16 sp;
    u8 z;
    u8 c;
    u8 n;
    u8 v;
    u8 *memory;
} CPU;

CPU cpu = {
    .sp = UPPER_16BIT_BOUND,
    .pc = 0x0000,
};

u8 read8(u16 addr)
{
    return cpu.memory[addr];
}

void write8(u16 addr, u8 value)
{
    cpu.memory[addr] = value;
}

u16 read16(u16 addr) {
    if (addr + 1 > UPPER_16BIT_BOUND)
    {
        fprintf(
                stderr,
                "[ERROR]: memory access violation: %x | limit %x \n",
                addr,
                UPPER_16BIT_BOUND
               );
        exit(1);
    }

    u8 low = cpu.memory[addr];
    u8 high = cpu.memory[addr+1];

    return (high << 8) | low;
}

void write16(u16 addr, u16 value) {
    if (addr + 1 > UPPER_16BIT_BOUND)
    {
        fprintf(stderr, "[ERROR]: write16 error %x | limit %x", addr, UPPER_16BIT_BOUND);
        exit(1);
    }
    u8 low = value & 0x00FF;
    u8 high = (value >> 8) & 0x00FF;

    cpu.memory[addr+1] = high;
    cpu.memory[addr] = low;
}

void push16(u16 value)
{
    cpu.sp -= WORD_SIZE;
    write16(cpu.sp, value);
}

u16 pop16()
{
    if (cpu.sp == UPPER_16BIT_BOUND)
    {
        // underflow: popping from an empty stack
        fprintf(stderr, "[ERROR]: stack underflow %x ", cpu.sp);
        exit(1);
    }
    u16 r = read16(cpu.sp);
    cpu.sp += 2;
    return r;
}

u16 fetch16()
{
    u16 r = read16(cpu.pc);
    cpu.pc += WORD_SIZE;
    return r;
}

// [opcode] [ rd ] [ rs ] [ extra ]
//     4       3      3      6
u8 getop(u16 inst)
{
    //
    // total width of inst is 16
    // opcode width is 4
    // 16 - 4 = 12
    //
    // isnt >> 12
    //
    // if:
    //
    // inst = 1010 001 010 000111
    //
    // shifting right by 12 gives
    //
    // inst = 0000 0000 0000 1010
    //
    // shift hte leftmost 4 bits down to the right
    // 0x000F = 0000 0000 0000 1111
    // inst =   0000 0000 0000 1010
    // ----------------------------
    // value =  0000 0000 0000 1010
    //
    //
    return (inst >> 12) & 0x000F;
}

u8 getdest(u16 inst)
{
    // shift right until the field reaches the far right
    // so that's 9 bits
    // 0001(OP) 001(RD) 000(RS) 000000(EXTRA)
    //
    // 0x7 is 111 so it will zero out everything left after the shift that
    // is not the lowest 3 bits.
    return (inst >> 9) & 0x7;
}

u8 getsrc(u16 inst)
{
    // shift right until the field reaches the far right
    // so that's 6 bits
    // 0001(OP) 001(RD) 000(RS) 000000(EXTRA)
    //
    // 0x7 is 111 so it will zero out everything left after the shift that
    // is not the lowest 3 bits.
    return (inst >> 6) & 0x7;
}

u8 getextra(u16 inst)
{
    // shift right until the field reaches the far right
    // so that's 6 bits
    // 0001(OP) 001(RD) 000(RS) 000000(EXTRA)
    //
    // 0x3F is 0011 1111 so it will zero out everything left after the shift that
    // is not the lowest 6 bits.
    return inst & 0x3F;
}

u16 inst(u8 op, u8 dest, u8 src, u8 extra)
{
    return ((op << 12) | (dest << 9) | (src << 6) | extra);
}

void mov(u8 dest, u8 src)
{
    cpu.registers[dest] = cpu.registers[src];
}

void add(u8 dest, u8 src)
{
    u16 a = cpu.registers[dest];
    u16 b = cpu.registers[dest];

    // compute result
    u64 n = a + b;

    if (n > UPPER_16BIT_BOUND)
    {
        // cary flag: the result was greater than a 16 bit number
        cpu.c = 1;
    }
    else
    {
        cpu.c = 0;
    }

    u16 r = a + b;

    // zero flag: says if the result of an operation was 0
    cpu.z = (r == 0);

    // negative flag: says if the result should be intepreted as negative (for signed)
    // 0x8000
    // 1000 0000 0000 0000
    cpu.n = ((r & 0x8000) != 0);

    u16 sign_a = a & 0x8000;
    u16 sign_b = b & 0x8000;
    u16 sign_r = r & 0x8000;

    cpu.v = ((sign_a == sign_b) && (sign_a != sign_r));

    fprintf(stderr, "cpu.v = %d\n", cpu.v);

    // store result in dest
    cpu.registers[dest] = r;
}

int main()
{
    u8 *memory = (u8 *)(malloc(sizeof(u8) * MEMORY_SIZE));
    if (memory == NULL)
    {
        perror("cpu memorary init");
        exit(1);
    }
    cpu.memory = memory;

    // load program into memory

    write16(0x0000, inst(LDI, R0, NOREG, 0x0));
    write16(0x0002, 5);
    write16(0x0004, inst(MOV, R1, R0, 0x0));
    write16(0x0006, inst(LDI, R0, NOREG, 0x0));
    write16(0x0008, -3);
    write16(0x000A, inst(ADD, R1, R0, 0x0));
    write16(0x000C, inst(STORE, NOREG, NOREG, 0x0));
    write16(0x000E, inst(STORE, R2, NOREG, 0x0));
    write16(0x000E, 0x1000);
    write16(0x001F, HALT);

    u8 running = 1;
    while (running)
    {
        u16 inst = fetch16();
        u8 op = getop(inst);
        u8 dest = getdest(inst);
        u8 src = getsrc(inst);
        u8 extra = getextra(inst);

        if (op == STORE)
        {
            // TODO: implement this
            u16 value_inst = fetch16();


            continue;
        }

        if (op == LOAD)
        {
            exit(1);
            continue;
        }

        if (op == ADD)
        {
            add(dest, src);
            fprintf(stderr, "ADD: R1 = %d\n", cpu.registers[R1]);
            continue;
        }

        if (op == MOV)
        {
            mov(dest, src);
            fprintf(stderr, "R1 = %d\n", cpu.registers[R1]);
            continue;
        }

        if (op == LDI)
        {
            u16 value = fetch16();
            fprintf(stderr, "LDI: op %d, dest: %d, src: %d extra: %d value: %d\n", op, dest, src, extra, value);

            cpu.registers[dest] = value;
            continue;
        }

        if (op == HALT) {
            fprintf(stderr, "HALT\n");
            running = 0;
        }
    }

    return 0;
}
