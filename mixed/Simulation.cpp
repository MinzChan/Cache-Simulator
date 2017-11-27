#include "Simulation.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "stdio.h"
#include <string>
#include <fstream>
#include <iomanip>
using namespace std;

extern void read_elf(const char*);
extern unsigned int dadr;
extern unsigned int dsize;
extern unsigned int cadr;
extern unsigned int csize;
extern unsigned int vadr;
extern unsigned int vdadr;
extern unsigned long long gp;  // read out from symbol table.
extern unsigned int madr;
extern unsigned int endPC;
extern unsigned int entry;
extern bool single_step;
extern bool run_til;
extern unsigned int pause_addr;
extern FILE *file;
extern FILE *ftmp;

//#define DEBUG
//#define LOG

/* Configure memory and cache */
void Configure(){
    level1.SetLower(&level2);
    level2.SetLower(&level3);
    level3.SetLower(&memory);
    //    level1.SetLower(&memory);
    
    StorageStats s;
    s.access_time = 0;
    memory.SetStats(s);
    level1.SetStats(s);
    level2.SetStats(s);
    
    StorageLatency ml;
    ml.bus_latency = 0;
    ml.hit_latency = 100;
    memory.SetLatency(ml);
    
    StorageLatency l1, l2, l3;
    CacheConfig cfg1, cfg2, cfg3;
    l1.bus_latency = 0;
    l1.hit_latency = 1;
    cfg1.size = (1 << 15);
    cfg1.associativity = 8;
    cfg1.line_size = 64;
    cfg1.write_policy = WRITE_BACK;
    cfg1.write_allocate_policy = WRITE_ALLOCATE;
    
    l2.bus_latency = 0;
    l2.hit_latency = 8;
    cfg2.size = (1 << 18);
    cfg2.associativity = 8;
    cfg2.line_size = 64;
    cfg2.write_policy = WRITE_BACK;
    cfg2.write_allocate_policy = WRITE_ALLOCATE;
    
    l3.bus_latency = 0;
    l3.hit_latency = 20;
    cfg3.size = (1 << 23);
    cfg3.associativity = 8;
    cfg3.line_size =64;
    cfg3.write_policy = WRITE_BACK;
    cfg3.write_allocate_policy = WRITE_ALLOCATE;
    
    level1.SetLatency(l1);
    level2.SetLatency(l2);
    level3.SetLatency(l3);
    
    level1.SetConfig(cfg1);
    level2.SetConfig(cfg2);
    level3.SetConfig(cfg3);
    
    /* build cache */
    level1.BuildCache();
    level2.BuildCache();
    level3.BuildCache();
}

/* load data and instructions */
void load_memory()
{
    /* load .text */
    fseek(file,cadr,SEEK_SET);
    fread(&(memory._memory[vadr]), 1, csize, file);
    cout << hex << *((unsigned int*)(&memory._memory[0x101ac])) << endl;
    
    /* load .data */
    fseek(file, dadr, SEEK_SET);
    fread(&(memory._memory[vdadr]), 1, dsize, file);
}

void fetch_instruction(int index){
#ifdef LOG
    cout << "---in Instruction_Fetch()-----"<< endl;
    cout << "PC: "<< hex << info[index].tmp_PC<< endl;
#endif
    /* put instruction in info[index] */
    info[index].inst = read_memory_word(info[index].tmp_PC);
    info[index].is_bubble = NO;
    
    /* in order to predict whether there are any data hazards */
    decode(index);
    
    /* update next_PC */
    switch (PCSrc) {
        case R_RS1_IMM: // jalr
            info[index].PC = reg[info[index].rs1] + info[index].imm;
            break;
        
        case PC_IMM: // jal, branch
            info[index].PC = info[index].tmp_PC + info[index].imm;
            break;
            
        case NORMAL:
            info[index].PC = info[index].tmp_PC + 4;
            break;
    }
    
    next_PC = info[index].PC;
}

/* decode and execute */
void decode(int index){
#ifdef LOG
    cout << "---in Decode()-----"<< endl;
    if(info[index].is_bubble == YES){
        cout << "@@@@@@@@@@IS BUBBLE!!!@@@@@@@@"<<endl;
        return;
    }
    cout << "PC: "<< info[index].tmp_PC<< endl;
#endif
    if(info[index].is_bubble == YES){
        return;
    }
    unsigned int inst = info[index].inst;
    unsigned int tmp;
    
    opcode = getbit(inst, 0, 6);
    
    switch (opcode){
            /* R */
        case 0x33:
            rd = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            
            fill_control_R(index);
            break;
            
            /* I */
        case 0x03:
        case 0x13:
        case 0x1B:
        case 0x67:
        case 0x73:
            rd = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            func7 = getbit(inst, 25, 31);
            imm12 = getbit(inst, 20, 31);
            imm = ext_signed((unsigned int)imm12, 11);
            
            fill_control_I(index);
            break;
            
            /* S (store) */
        case 0x23:
            imm5 = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            imm7 = getbit(inst, 25, 31);
            tmp = (imm7 << 5) | imm5;
            imm = ext_signed(tmp, 11);
            
            fill_control_S(index);
            break;
            
            /* SB (switch branch) */
        case 0x63:
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            tmp = (getbit(inst, 8, 11) << 1) | (getbit(inst, 25, 30) << 5) | (getbit(inst, 7, 7) << 11) | (getbit(inst, 31, 31) << 12);
            imm = ext_signed(tmp, 12);
            
            fill_control_SB(index);
            break;
            
            /* U */
        case 0x17:
        case 0x37:
            rd = getbit(inst, 7, 11);
            imm20 = getbit(inst, 12, 31);
            func7 = getbit(inst, 25, 31);
            tmp = imm20;
            imm = ext_signed(tmp, 31);
            
            fill_control_U(index);
            break;
            
            /* UJ */
        case 0x6f:
            rd = getbit(inst, 7, 11);
            func7 = getbit(inst, 25, 31);
            tmp = (getbit(inst, 12, 19) << 12) | (getbit(inst, 20, 20) << 11) | (getbit(inst, 21, 30) << 1) | (getbit(inst, 31, 31) << 20);
            imm = ext_signed(tmp, 20);
            
            fill_control_UJ(index);
            break;
            
            /* extended */
        case 0x3b:
            rd = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            
            fill_control_R(index);
            break;
    }
    
    /* store related Control Info in info[index] */
    write_inst_info(index);
}

void fill_control_R(int index){
    ALUSrcA = R_RS1;
    ALUSrcB = R_RS2;
    MemRead = NO;
    MemWrite = NO;
    MemtoReg = NO;
    RegWrite = YES;
    PCSrc = NORMAL;
    ExtOp = NO;
    
    switch(opcode){
        /* regular R-type */
        case 0x33:
            data_type = DOUBLEWORD;
            
            switch (func3){
                case 0x0:
                    switch (func7){
                        case 0x00: // add rd, rs1, rs2
                            ALUOp = ADD;
                            break;
                        case 0x01: // mul rd, rs1, rs2
                            ALUOp = MUL;
                            break;
                        case 0x20: // sub rd, rs1, rs2
                            ALUOp = SUB;
                            break;
                    }
                    break;
                    
                case 0x1:
                    switch (func7){
                        case 0x00: // sll rd, rs1, rs2
                            ALUOp = SLL;
                            break;
                        case 0x01: // mulh rd, rs1, rs2
                            ALUOp = MULH;
                            break;
                    }
                    break;
                    
                case 0x2:
                    if(func7 == 0x00){ // slt rd, rs1, rs2
                        ALUOp = SLT;
                    }
                    break;
                    
                case 0x4:
                    switch (func7){
                        case 0x00: // xor rd, rs1, rs2
                            ALUOp = XOR;
                            break;
                        case 0x01: // div rd, rs1, rs2
                            ALUOp = DIV;
                            break;
                    }
                    break;
                    
                case 0x5:
                    switch (func7){
                        case 0x00: // srl rd, rs1, rs2
                            ALUOp = SRL;
                            break;
                        case 0x20: // sra rd, rs1, rs2
                            ALUOp = SRA;
                            ExtOp = YES;
                            break;
                    }
                    break;
                    
                case 0x6:
                    switch (func7){
                        case 0x00: // or rd, rs1, rs2
                            ALUOp = OR;
                            break;
                        case 0x01: // rem rd, rs1, rs2
                            ALUOp = REM;
                            break;
                    }
                    break;
                    
                case 0x7:
                    if(func7 == 0x00){ // and rd, rs1, rs2
                        ALUOp = AND;
                    }
                    break;
            }
            break;
        
        /* extended */
        case 0x3b:
            data_type = WORD;
            
            switch (func3){
                case 0x0:
                    if(func7 == 0x00){ // addw rd, rs1, rs2
                        ALUOp = ADD;
                    }
                    else if(func7 == 0x20){ // subw rd, rs1, rs2
                        ALUOp = SUB;
                    }
                    else if(func7 == 0x01){ // mulw rd, rs1, rs2
                        ALUOp = MUL;
                    }
                    break;
                    
                case 0x1: // sllw
                    ALUOp = SLL;
                    break;
                    
                case 0x4: // divw
                    ALUOp = DIV;
                    break;
                    
                case 0x5:
                    if(func7 == 0x00){ // srlw rd, rs1, rs2
                        ALUOp = SRL;
                    }
                    else{ // sraw rd, rs1, rs2
                        ALUOp = SRA;
                        ExtOp = YES;
                    }
                    break;
            }
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_I(int index){
    unsigned int tmp;
    switch (opcode) {
        case 0x03:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            ALUOp = ADD;
            MemRead = YES;
            MemWrite = NO;
            MemtoReg = YES;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = YES;
            
            switch (func3){
                case 0x0: // lb rd, offset(rs1)
                    data_type = BYTE;
                    break;
                    
                case 0x1: // lh rd, offset(rs1)
                    data_type = HALF;
                    break;
                    
                case 0x2: // lw rd, offset(rs1)
                    data_type = WORD;
                    break;
                    
                case 0x3: // ld rd, offset(rs1)
                    data_type = DOUBLEWORD;
                    break;
            }
            break;
            
        case 0x13:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            
            switch (func3){
                case 0x0: // addi rd, rs1, imm
                    ALUOp = ADD;
                    break;
                    
                case 0x1: // slli rd, rs1, imm
                    if (func7 == 0x00){
                        ALUOp = SLL;
                        ALUSrcB = IMM_05;
                    }
                    break;
                    
                case 0x2: // slti rd, rs1, imm
                    ALUOp = SLT;
                    break;
                    
                case 0x4: // xori rd, rs1, imm
                    ALUOp = XOR;
                    break;
                    
                case 0x5: // srli rd, rs1, imm
                    if (func7 == 0x00){
                        ALUOp = SRL;
                        ALUSrcB = IMM_05;
                    }
                    else if (func7 == 0x10){ // srai rd, rs1, imm
                        ALUSrcB = IMM_05;
                        ALUOp = SRA;
                        ExtOp = YES;
                    }
                    break;
                    
                case 0x6: // ori rd, rs1, imm
                    ALUOp = OR;
                    break;
                    
                case 0x7: // andi rd, rs1, imm
                    ALUOp = AND;
                    break;
            }
            break;
            
        case 0x1B:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = YES;
            data_type = WORD;
            
            switch (func3){
                case 0x0: // addiw rd, rs1, imm
                    ALUOp = ADD;
                    break;
                case 0x1: // slliw rd, rs1, imm
                    ALUOp = SLL;
                    break;
                case 0x5:
                    // srliw rd, rs1, imm
                    if (getbit(info[index].inst, 30, 30) == 0){
                        ALUOp = SRL;
                        ALUSrcB = IMM_05;
                    }
                    // sraiw rd, rs1, imm
                    else{
                        ALUOp = SRA;
                        ALUSrcB = IMM_05;
                    }
                    break;
            }
            break;
            
        case 0x67:
            switch (func3){
                case 0x0: // Jalr rd, rs1, imm
                    ALUSrcA = PROGRAM_COUNTER;
                    ALUSrcB = FOUR;
                    ALUOp = ADD;
                    MemRead = NO;
                    MemWrite = NO;
                    MemtoReg = NO;
                    RegWrite = YES;
                    PCSrc = R_RS1_IMM;  // R[rs1] + imm
                    ExtOp = NO;
                    data_type = DOUBLEWORD;
                    break;
            }
            break;
            
        case 0x73:
            switch (func3){
                case 0x0: // ecall
                    if(func7 == 0x000){ // (Transfers control to operating system)
                        exit_flag = 1;
                    }
                    break;
            }
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_S(int index){
    switch (opcode){
        case 0x23:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            ALUOp = ADD;
            MemRead = NO;
            MemWrite = YES;
            MemtoReg = NO;
            RegWrite = NO;
            PCSrc = NORMAL;
            ExtOp = NO;
            
            if(func3 == 0x0){ // sb rs2, offset(rs1)
                data_type = BYTE;
            }
            else if(func3 == 0x1){ // sh rs2, offset(rs1)
                data_type = HALF;
            }
            else if(func3 == 0x2){ // sw rs2, offset(rs1)
                data_type = WORD;
            }
            else if(func3 == 0x3){ // sd rs2, offset(rs1)
                data_type = DOUBLEWORD;
            }
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_SB(int index){
    switch (opcode){
        case 0x63:
            ALUSrcA = R_RS1;
            ALUSrcB = R_RS2;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = NO;
            PCSrc = PC_IMM;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            
            if(func3 == 0x0){ // beq rs1, rs2, offset
                ALUOp = EQ;
            }
            else if(func3 == 0x1){ // bne rs1, rs2, offset
                ALUOp = NEQ;
            }
            else if(func3 == 0x4){ // blt rs1, rs2, offset
                ALUOp = LT;
            }
            else if(func3 == 0x5){ // bge rs1, rs2, offset
                ALUOp = GE;
            }
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_U(int index){
    switch (opcode){
        case 0x17: // auipc rd, offset
            ALUSrcA = PROGRAM_COUNTER;
            ALUSrcB = IMM_SLL_12;
            ALUOp = ADD;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            break;
            
        case 0x37: // lui rd, offset
            ALUSrcA = ZERO;
            ALUSrcB = IMM_SLL_12;
            ALUOp = ADD;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_UJ(int index){
#ifdef DEBUG
    cout << "entered fill_control_UJ()" << endl;
#endif
    
    switch (opcode){
        case 0x6f: // jal rd, imm
            ALUSrcA = PROGRAM_COUNTER;
            ALUSrcB = FOUR;
            data_type = ADD;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = PC_IMM;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void execute(int index){
#ifdef LOG
    cout << "---in Execute()-----"<< endl;
    if(info[index].is_bubble == YES){
        cout << "@@@@@@@@@@IS BUBBLE!!!@@@@@@@@"<<endl;
        return;
    }
    cout << "PC: "<< info[index].tmp_PC<< endl;
#endif
    if(info[index].is_bubble == YES){
        return;
    }
    
    unsigned long long op1;
    unsigned long long op2;
    int ext_pos;  // the index of sign bit

    /* find the sign bit */
    switch(info[index].data_type){
        case BYTE:
            ext_pos = 7;
            break;
            
        case HALF:
            ext_pos = 15;
            break;
            
        case WORD:
            ext_pos = 31;
            break;
        
        default:
            ext_pos = 63;
    }
    
    /* the first operand */
    switch(info[index].ALUSrcA){
        case R_RS1:
            op1 = reg[info[index].rs1];
            break;
            
        case PROGRAM_COUNTER:
            op1 = info[index].tmp_PC;
            break;
            
        case ZERO:
            op1 = 0;
            break;
    }

    /* the second operand */
    switch (info[index].ALUSrcB) {
        case R_RS2:
            op2 = reg[info[index].rs2];
            break;
            
        case IMM:
            op2 = info[index].imm;
            break;
            
        case IMM_05:
            op2 = info[index].imm & 63;
            break;
            
        case IMM_SLL_12:
            op2 = info[index].imm << 12;
            break;
            
        case FOUR:
            op2 = 4;
            break;
    }
    
    /* which operation */
    switch(info[index].ALUOp){
        case ADD:
            alu_rst = op1 + op2;
            break;
            
        case SUB:
            alu_rst = op1 - op2;
            break;
            
        case MUL:
            alu_rst = op1 * op2;
            break;
            
        case DIV:
            alu_rst = op1 / op2;
            break;
            
        case SLL:
            alu_rst = op1 << op2;
            break;
            
        case XOR:
            alu_rst = op1 ^ op2;
            break;
            
        case SRA:  // and EXT_OP, special operations
            ext_pos = 63 - (int)reg[info[index].rs2]; //63 - (int)reg[rs2]
        case SRL:
            alu_rst = op1 >> op2;
            break;
            
        case OR:
            alu_rst = op1 | op2;
            break;
            
        case AND:
            alu_rst = op1 & op2;
            break;
            
        case REM:
            alu_rst = op1 % op2;
            break;
            
        case EQ:
            alu_rst = (op1 == op2);
            break;
            
        case NEQ:
            alu_rst = (op1 != op2);
            break;
            
        case GT:
            alu_rst = (op1 > op2);
            break;
            
        case LT:
            alu_rst = (op1 < op2);
            break;
            
        case GE:
            alu_rst = (op1 >= op2);
            break;
        
        case SLT:  // change to signed number
            alu_rst = ((long long)op1 < (long long)op2);
            break;
            
        case MULH:
            long long ah = ((op1 & 0xffff0000) >> 32) & 0xffff;
            long long al = op1 & 0x0000ffff;
            long long bh = ((op2 & 0xffff0000) >> 32) & 0xffff;
            long long bl = op2 & 0x0000ffff;
            long long ah_mul_bh = ah * bh;
            long long ah_mul_bl = ((ah * bl) >> 32) & 0xffff;
            long long al_mul_bh = ((al * bh) >> 32) & 0xffff;
            alu_rst = ah_mul_bh + ah_mul_bl + al_mul_bh;
            break;
    }
    
    /* sign extending */
    if(info[index].ExtOp && info[index].MemRead == NO){
        alu_rst = ext_signed(alu_rst, ext_pos);
    }
    
    /* for 32-bit */
    if(info[index].MemRead == NO && info[index].MemWrite == NO && info[index].data_type == WORD){
        alu_rst = 0xffffffff & alu_rst;
    }
    
    /* store alu_rst back to info[index] */
    info[index].alu_rst = alu_rst;
    
    /* update PC for branch */
    switch(info[index].PCSrc){
        case PC_IMM:
            if(info[index].alu_rst == 0){  // wrong prediction!
                info[index].PC = info[index].tmp_PC + 4;
#ifdef LOG
                cout << "~~~[[[[[[Wrong Prediction!!]]]]]]~~~"<< endl;
#endif
                next_PC = info[index].PC;
                
                create_bubble(0);
                create_bubble(1);
                
                inst_num -= 2;
                control_hazard += 2;
            }
            break;
    }
}

void memory_read_write(int index){
#ifdef LOG
    cout << "---in Memory_Read_Write()-----"<< endl;
    if(info[index].is_bubble == YES){
        cout << "@@@@@@@@@@IS BUBBLE!!!@@@@@@@@"<<endl;
        return;
    }
    cout << "PC: "<< info[index].tmp_PC<< endl;
#endif
    if(info[index].is_bubble == YES){
        return;
    }
    
    if(info[index].MemRead == YES){
        switch(info[index].data_type){
            case BYTE:
                data_from_memory = read_memory_byte(info[index].alu_rst);
                data_from_memory = ext_signed(data_from_memory, 7);
                break;
                
            case HALF:
                data_from_memory = read_memory_half(info[index].alu_rst);
                data_from_memory = ext_signed(data_from_memory, 15);
                break;
                
            case WORD:
                data_from_memory = read_memory_word(info[index].alu_rst);
                data_from_memory = ext_signed(data_from_memory, 31);
                break;
                
            case DOUBLEWORD:
                data_from_memory = read_memory_doubleword(info[index].alu_rst);
                break;
        }
        
        /* write data_from_memory to info[index] */
        info[index].data_from_memory = data_from_memory;
    }
    
    if(info[index].MemWrite == YES){
        switch(info[index].data_type){
            case BYTE:  // data_to_memory would always be in reg[rs2]
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 1);
                break;
                
            case HALF:
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 2);
                break;
                
            case WORD:
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 4);
                break;
                
            case DOUBLEWORD:
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 8);
                break;
        }
    }
}

void write_back(int index){
#ifdef LOG
    cout << "---in Write_Back()-----"<< endl;
    if(info[index].is_bubble == YES){
        cout << "@@@@@@@@@@IS BUBBLE!!!@@@@@@@@"<<endl;
        return;
    }
    cout << "PC: "<< info[index].tmp_PC<< endl;
#endif
    if(info[index].is_bubble == YES){
        return;
    }
    
    if(info[index].RegWrite == YES){
        if(info[index].MemtoReg == YES){
            reg[info[index].rd] = info[index].data_from_memory;
        }
        else{
            reg[info[index].rd] = info[index].alu_rst;
        }
    }
}

void simulate()
{
    /* set when to end, the PC of atexit */
    int end_PC =(int)endPC - 8; // the addr of 'ret' in main()
    if (end_PC % 4 == 2){  // wierd actually.
        end_PC -= 2;
    }
#ifdef DEBUG
    cout << "endPC: " << hex << end_PC << endl;
#endif
    
    if(single_step){
        single_step_mode_description();
    }
    
    /* initialize pipeline */
    initialize_pipeline();
    
    while(info[4].PC != end_PC)
    {
        bool add_inst = true;  // whether a new instruction gets into pipeline
        
        reg[0] = 0; // 一直为零
        
        if (run_til && info[0].PC == pause_addr){
            single_step = true;
            run_til = false;
        }
        
        /* five steps */
        fetch_instruction(0);
        
        /* whether there are data hazards */
        if(have_hazard()){
#ifdef LOG
            cout << "####HAVE HAZARD!!!!!!######"<< endl;
#endif
            next_PC = info[0].tmp_PC;  // the instruction that will be bubbled out have to execute again
            create_bubble(0);
            add_inst = false;
        }
        if(reach_end() || info[0].tmp_PC == 0){
#ifdef LOG
            cout << "Reach end!"<< endl;
#endif
            create_bubble(0);
            add_inst = false;
        }

#ifndef OPTIMIZE
        write_back(4);  // try to bubble out one less instruction when data_hazard occurs
#endif
        decode(1);
        execute(2);
        memory_read_write(3);
#ifdef OPTIMIZE
        write_back(4);
#endif

#ifdef DEBUG
        print_pipeline();
#endif
        
        if(exit_flag==1){
            cerr << "Can not simulate system calls." << endl;
            break;
        }
    
        if(single_step){
            cout << "The " << dec << inst_num << " instruction: " << hex << setw(8) << setfill('0') << info[0].inst << endl;
            cout << "tmp_PC: " << info[0].tmp_PC << endl;
            cout << "next_PC: " << next_PC << endl;
            debug_choices();
        }
        
        /* move pipeline to next stage */
        move_pipeline();
        
        /* update CPI accordingly */
        update_cpi(add_inst);
    }
}

int main(int argc, char * argv[])
{
    /* get file name from command */
    if(argc < 2){
        cerr << "Missing operand. Please specify the executable file." << endl;
        return 0;
    }
    else if(argc > 2){
        cerr << "Too many operands. Please specify the executable file only." << endl;
        return 0;
    }
    const char* file_name = argv[1];
    
    /* decide whether to debug step by step */
    while (true){
        char response;
        cout << endl << separator << endl;
        cout << blank << "Enter Debug Mode? (y/n)"<< endl;
        cin >> response;
        fflush(stdin);
        if(response == 'y' || response == 'Y'){
            single_step = true;
            break;
        }
        else if(response == 'n' || response == 'N'){
            break;
        }
        else{
            cerr << "Can not understand. Please input 'y' or 'n'."<< endl;
        }
    }
    
    /* preparations */
	read_elf(file_name);  // explain the ELF file
    load_memory();
    PC_ = entry; // set the entrance PC
    next_PC = PC_;
    reg[3]=gp; // set the global pointer
    reg[2]=MAX / 2; // set the stack pointer

    /* Configure Cache */
    Configure();
    
    /* start simulation */
    simulate();

    end_description();
    end_check();
    print_cpi();
    
    /* close file */
    fclose(file);
    fclose(ftmp);
    
    printf("Total Memory access time: %d\n", memory._visit_cnt);
    cout << "Miss Rate: " << 1 - (float)hit_cnt / request_cnt << endl;
	return 0;
}
