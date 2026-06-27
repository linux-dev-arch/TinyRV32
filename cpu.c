#include <stdint.h>
#include <stdbool.h>
#ifndef RAMSIZE
#define RAMSIZE 1024
#endif
uint8_t led=0;
uint8_t uart_tx=0x0;
uint8_t uart_rx=0x0;
uint8_t ram[RAMSIZE] __attribute__((section(".noinit")));//arduino AVR specific to enable persistence of ram contents after resets.remove attribute for other mcu targets
uint8_t uart_rx_buf[20];
uint8_t uart_rx_head=0;
uint8_t uart_rx_tail=0;
uint32_t pc=0x80000000;
uint32_t ram_base=0x80000000;
uint32_t regs[32];
uint8_t get_byte(uint32_t addr){
    if (addr>=ram_base){
        if (addr < sizeof(ram)+0x80000000){
            return ram[addr-ram_base];
        }
        else{
            return 0;
            }
        }
    else{
        if (addr == 0x1000004){
            if (uart_rx_head == uart_rx_tail) {
                // Buffer empty
                return 0;
            }
            uint8_t c = uart_rx_buf[uart_rx_tail];
            uart_rx_tail = (uart_rx_tail + 1) % 20;
            return c;
        }
        return 0;
    }
}
void write_byte(uint32_t addr,uint8_t value){
    if (addr>=ram_base){
        ram[addr-ram_base] = value;
    }
    if(addr == 0x1000000){
        uart_tx=value;
    }
    if(addr == 0x1000100){
        led=value;
    }
}
void write_2_bytes(uint32_t addr,uint16_t value){
    write_byte(addr,value&0xFF);
    write_byte(addr+1,(value >> 8)&0xff);
}

void write_4_bytes(uint32_t addr,uint32_t value){
    write_byte(addr,value&0xFF);
    write_byte(addr+1,(value >> 8)&0xff);
    write_byte(addr+2,(value >> 16)&0xff);
    write_byte(addr+3,(value >> 24)&0xff);
}
/*uint32_t sign_extend(uint32_t val, uint8_t bits){
    if( val & (1 << (bits - 1))){
        val -= (1 << bits);
    }
    return val;
}*/
uint32_t sign_extend(uint32_t val, uint8_t bits)
{
    if(bits == 32)
        return val;

    uint32_t m = 1UL << (bits - 1);
    return (val ^ m) - m;
}
/*uint32_t get_instr(uint32_t addr){
    return get_byte(addr)+(get_byte(addr+1)<<8)+(get_byte(addr+2)<<16)+(get_byte(addr+3)<<24);
}*/
uint32_t get_instr(uint32_t addr){
    return ((uint32_t)get_byte(addr))
    | ((uint32_t)get_byte(addr+1) << 8)
    | ((uint32_t)get_byte(addr+2) << 16)
    | ((uint32_t)get_byte(addr+3) << 24);
}
uint32_t get_4_bytes(uint32_t addr){
    return ((uint32_t)get_byte(addr))
    | ((uint32_t)get_byte(addr+1) << 8)
    | ((uint32_t)get_byte(addr+2) << 16)
    | ((uint32_t)get_byte(addr+3) << 24);
}
uint8_t get_opcode(uint32_t instr){
    return instr&0x7f;
}
uint8_t get_rd(uint32_t instr){
    return (instr& 0xf80)>>7;
}
uint32_t get_imm_j(uint32_t instr){
    uint32_t imm20    = (instr >> 31) & 0x1;
    // instruction[30:21] -> imm[10:1]
    uint32_t imm10_1  = (instr>> 21) & 0x3FF;
    // instruction[20] -> imm[11]
    uint32_t imm11    = (instr>> 20) & 0x1;
    //instruction[19:12] -> imm[19:12]
    uint32_t imm19_12 = (instr>> 12) & 0xFF;

    //Reassemble bits into the 21-bit final immediate (with the implied 0)
    //Result structure: [imm20][imm19_12][imm11][imm10_1][0]
    uint32_t imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
    return sign_extend(imm,21);
}
uint16_t get_funct3(uint32_t instr){
    return (instr & 0x7000)>> 12;
}
uint8_t get_funct5(uint32_t instr){
    return (instr >> 27) & 0x1F;
}
uint8_t get_rs1(uint32_t instr){
    return (instr  >> 15) & 0x1f;
}
uint8_t get_rs2(uint32_t instr){
    return (instr >> 20 ) &0x1f;
}
uint32_t get_imm_i(uint32_t instr){
    uint32_t imm = (instr >> 20)&0xfff;
    return sign_extend(imm,12);
}
uint32_t get_imm_u(uint32_t instr){
    return ((instr  >> 12 ) & 0xFFFFF);
}

uint32_t get_funct7(uint32_t instr){
    return (instr >> 25) &0x7f;
}
uint32_t get_imm_b(uint32_t instr){
    uint32_t imm_12   = (instr >> 31) & 0x01;
    uint32_t imm_11   = (instr >> 7)  & 0x01;
    uint32_t imm_10_5 = (instr >> 25) & 0x3F;
    uint32_t imm_4_1  = (instr >> 8)  & 0x0F;
    return sign_extend((imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1),13);
}
uint32_t get_imm_s(uint32_t instr){
    uint32_t imm_11_5 = (instr >> 25) & 0x7F;
    uint32_t imm_4_0 = (instr >> 7) & 0x1F;
    // Combine immediate bits and sign-extend to 32-bit
    uint32_t imm = (imm_11_5 << 5) | imm_4_0;
    return sign_extend(imm,12);
}
void cpu_step(uint32_t instr){
    regs[0]=0;
    uint8_t opcode=get_opcode(instr);
    if (opcode==0x6f){
        //jal
        uint8_t rd = get_rd(instr);
        uint32_t imm=get_imm_j(instr);
        regs[rd]=(pc+4)&0xFFFFFFFF;
        pc=pc+imm;
    }
    else if(opcode==0xf){
        uint16_t funct3=get_funct3(instr);
        if (funct3 == 1){
            //fence.i
            pc=pc+4;
        }
        else if(funct3 == 0){
            pc=pc+4;
        }
    }
    else if (opcode == 0x13){
        uint16_t funct3 = get_funct3(instr);
        uint32_t funct7 = get_funct7(instr);
        if (funct3 ==0){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            regs[rd]=(regs[rs1]+imm )& 0xffffffff;
            pc=pc+4;
        }
        else if (funct3 == 7){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            regs[rd]=(regs[rs1]&imm)&0xffffffff;
            pc=pc+4;
        }
        else if( funct3 == 4){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            regs[rd]=(regs[rs1]^imm)&0xffffffff;
            pc=pc+4;
        }
        else if(funct3 == 6){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            regs[rd]=(regs[rs1]|imm)&0xffffffff;
            pc=pc+4;
        }
        else if(funct3==1){
            if (funct7 == 0){
                uint8_t rs1=get_rs1(instr);
                uint8_t rd = get_rd(instr);
                uint8_t shamt = (instr >> 20) & 0x1F;
                regs[rd]=(regs[rs1] << shamt) & 0xffffffff;
                pc=pc+4;
            }
        }
        else if (funct3==5 && funct7==0x20){
            //srai
            uint8_t rs1=get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint8_t shamt = (instr >> 20) & 0x1F;
            uint32_t v=sign_extend(regs[rs1]&0xffffffff,32);
            regs[rd] = (v >> shamt) & 0xffffffff;
            pc=pc+4;
        }
        else if (funct3==5 && funct7==0x0){
            //srli
            uint8_t rs1=get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint8_t shamt = (instr >> 20) & 0x1F;
            uint32_t v=regs[rs1]&0xffffffff;
            regs[rd] = (v >> shamt) & 0xffffffff;
            pc=pc+4;
        }
        else if (funct3==3){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            uint32_t imm = get_imm_i(instr);
            uint32_t v1 = regs[rs1] & 0xffffffff;
            uint32_t v2 = imm&0xffffffff;
            if (v1 < v2){
                regs[rd] = 1;
            }
            else{
                regs[rd] = 0;
            }
            pc=pc+4;
        }
        else if (funct3==2){
            //slti
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            uint32_t imm = get_imm_i(instr);
            uint32_t v1 = sign_extend(regs[rs1] & 0xffffffff,32);
            uint32_t v2 = imm;
            if (v1 < v2){
                regs[rd] = 1;
            }
            else{
                regs[rd] = 0;

            }
            pc=pc+4;
        }
    }
    else if (opcode==0x67){
        uint8_t funct3=get_funct3(instr);
        if (funct3 == 0){
            //jalr
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint32_t t=(pc+4)&0xffffffff;
            pc = ((imm + regs[rs1])&0xffffffff);
            pc = pc & ~1;
            regs[rd] = t;
        }
    }
    else if(opcode == 0x17){
        uint8_t rd = get_rd(instr);
        uint32_t imm = get_imm_u(instr);
        regs[rd] =(pc+(imm << 12)) & 0xffffffff;
        pc=pc+4;
    }
    else if (opcode == 0x37){
        uint8_t rd = get_rd(instr);
        uint32_t imm = get_imm_u(instr);
        regs[rd] = (imm << 12)& 0xffffffff;
        pc=pc+4;
    }
    else if (opcode == 0x63){
        uint8_t funct3 = get_funct3(instr);
        if (funct3 == 4){
            //blt
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint32_t imm = get_imm_b(instr);
            uint32_t v1 = sign_extend(regs[rs1] & 0xFFFFFFFF, 32);
            uint32_t v2 = sign_extend(regs[rs2] & 0xFFFFFFFF, 32);
            if (v1 < v2){
                pc = pc + imm;
            }
            else{
                pc =pc+ 4;
            }
        }
        else if(funct3 == 5){
            //bge
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t imm = get_imm_b(instr);
            uint32_t v1 = sign_extend(regs[rs1] & 0xFFFFFFFF, 32);
            uint32_t v2 = sign_extend(regs[rs2] & 0xFFFFFFFF, 32);
            if (v1 >= v2){
                pc = pc + imm;
            }
            else{
                pc =pc+ 4;
            }
        }
        else if (funct3 == 7){
            //bgeu
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint32_t imm = get_imm_b(instr);
            uint32_t v1 = regs[rs1] & 0xFFFFFFFF;
            uint32_t v2 = regs[rs2] & 0xFFFFFFFF;
            if (v1 >= v2){
                pc = pc + imm;
            }
            else{
                pc =pc + 4;
            }
        }
        else if (funct3 == 1){
            //bne
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint32_t imm = get_imm_b(instr);
            uint32_t v1 = (regs[rs1] & 0xFFFFFFFF);
            uint32_t v2 = (regs[rs2] & 0xFFFFFFFF);
            if (v1 != v2){
                pc = pc + imm;
            }
            else{
                pc =pc+ 4;
            }
        }
        else if (funct3 == 0){
            //beq
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint32_t imm = get_imm_b(instr);
            uint32_t v1 = (regs[rs1] & 0xFFFFFFFF);
            uint32_t v2 = (regs[rs2] & 0xFFFFFFFF);
            if (v1 == v2){
                pc = pc + imm;
            }
            else{
                pc =pc+ 4;
            }
        }
        else if (funct3 == 6){
            //bltu
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint32_t imm = get_imm_b(instr);
            uint32_t v1 = regs[rs1] & 0xFFFFFFFF;
            uint32_t v2 = regs[rs2] & 0xFFFFFFFF;
            if (v1 < v2){
                pc = pc + imm;
            }
            else{
                pc = pc+ 4;
            }
        }
    }
    else if (opcode == 0x23){
        uint8_t funct3=get_funct3(instr);
        if (funct3 == 0){
            uint8_t rs1=get_rs1(instr);
            uint8_t rs2=get_rs2(instr);
            uint32_t imm = get_imm_s(instr);
            uint32_t addr = regs[rs1]+imm;
            write_byte(addr,regs[rs2] &0xff);
            pc=pc+4;
        }
        else if (funct3 == 2){
            //sw
            uint8_t rs1=get_rs1(instr);
            uint8_t rs2=get_rs2(instr);
            uint32_t imm = get_imm_s(instr);
            uint32_t addr = regs[rs1]+imm;
            write_4_bytes(addr,regs[rs2]);
            pc=pc+4;
        }
        else if(funct3==1){
            //sh
            uint8_t rs1=get_rs1(instr);
            uint8_t rs2=get_rs2(instr);
            uint8_t imm = get_imm_s(instr);
            uint32_t addr = regs[rs1]+imm;
            uint32_t val = regs[rs2] & 0xffff;
            write_2_bytes(addr,val);
            pc=pc+4;
        }
    }
    else if (opcode == 0x03){
        uint8_t funct3 = get_funct3(instr);
        if (funct3 == 2){
            //lw
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint32_t addr = (regs[rs1]+imm)&0xffffffff;
            uint32_t val = get_4_bytes(addr);
            regs[rd] = val&0xffffffff;
            pc=pc+4;
        }
        else if (funct3==4){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint32_t addr = (regs[rs1]+imm)&0xffffffff;
            uint8_t val = get_byte(addr);
            regs[rd] = val&0xff;
            pc=pc+4;
        }
        else if(funct3==0){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint32_t addr = (regs[rs1] + imm) & 0xffffffff;
            uint8_t val = get_byte(addr);
            regs[rd] = sign_extend(val,8) & 0xffffffff;
            pc=pc+4;
        }
        else if (funct3==5){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint32_t addr = (regs[rs1] + imm) & 0xffffffff;
            uint8_t b0 = get_byte(addr);
            uint8_t b1 = get_byte(addr+1);
            uint32_t val = b0 | (b1 << 8);
            regs[rd] = val & 0xffff;
            pc=pc+4;
        }
        else if (funct3==1){
            uint32_t imm = get_imm_i(instr);
            uint8_t rs1 = get_rs1(instr);
            uint8_t rd = get_rd(instr);
            uint32_t addr = (regs[rs1] + imm) & 0xffffffff;
            uint8_t b0 = get_byte(addr);
            uint8_t b1 = get_byte(addr+1);
            uint8_t val = b0 | (b1 << 8);
            regs[rd] = sign_extend(val ,16) & 0xffffffff;
            pc=pc+4;
        }
    }
    else if (opcode == 0x33){
        uint8_t funct3=get_funct3(instr);
        uint32_t funct7=get_funct7(instr);
        if (funct3==1 && funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd]=(regs[rs1] << (regs[rs2] & 0x1F)) &0xffffffff;
            pc=pc+4;
        }
        else if (funct3==5 && funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd]=(regs[rs1] >> (regs[rs2] & 0x1F)) &0xffffffff;
            pc=pc+4;
        }
        else if (funct3==0 && funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd] = (regs[rs1] + regs[rs2])&0xffffffff;
            pc=pc+4;
        }
        else if (funct3==0 && funct7==0x20){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd] = (regs[rs1] - regs[rs2])&0xffffffff;
            pc=pc+4;
        }
        else if (funct3==7 &&  funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd] = (regs[rs1] & regs[rs2])&0xffffffff;
            pc=pc+4;
        }
        else if (funct3==6 && funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd] = (regs[rs1] | regs[rs2])&0xffffffff;
            pc=pc+4;
        }
        else if (funct3 == 3 && funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            uint8_t v1 =regs[rs1] & 0xffffffff;
            uint8_t v2 =regs[rs2] & 0xffffffff;
            if (v1 < v2){
                regs[rd] = 1;
            }
            else{
                regs[rd] = 0;
            }
            pc=pc+4;
        }
        else if (funct3==4 && funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd] = (regs[rs1] ^ regs[rs2])&0xffffffff;
            pc+=4;
        }
        else if (funct3==5 && funct7==0x20){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            regs[rd]=(sign_extend(regs[rs1]&0xffffffff,32) >> (regs[rs2] & 0x1F)) &0xffffffff;
            pc+=4;
        }
        else if (funct3 == 2 && funct7==0){
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd = get_rd(instr);
            uint8_t v1 =sign_extend(regs[rs1] & 0xffffffff,32);
            uint8_t v2 =sign_extend(regs[rs2] & 0xffffffff,32);
            if (v1 < v2){
                regs[rd] = 1;
            }
            else{
                regs[rd] = 0;
            }
            pc+=4;
        }
        else if (funct3 == 0 && funct7==1){
            //MUL
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd  = get_rd(instr);

            regs[rd] = (regs[rs1] * regs[rs2]) & 0xffffffff;
            pc += 4;
        }
        else if (funct3 == 4 && funct7==1){
            //DIV
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd  = get_rd(instr);

            int32_t a = (int32_t)regs[rs1];
            int32_t b = (int32_t)regs[rs2];

            if (b == 0)
                regs[rd] = 0xffffffff;
            else
                regs[rd] = a / b;

            pc += 4;
        }
        else if (funct3 == 5 && funct7 == 0x01) {
            // DIVU
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd  = get_rd(instr);

            uint32_t a = regs[rs1];
            uint32_t b = regs[rs2];

            if (b == 0)
                regs[rd] = 0xffffffff;
            else
                regs[rd] = a / b;
            pc += 4;
        }
        else if (funct3 == 1 and funct7 == 1){
        //MULH
        uint8_t rs1=get_rs1(instr);
        uint8_t rs2=get_rs2(instr);
        uint8_t rd=get_rd(instr);
        int64_t a = (int64_t)(int32_t)regs[rs1];
        int64_t b = (int64_t)(int32_t)regs[rs2];

        int64_t result = a * b;

        regs[rd] = (uint32_t)(result >> 32);

        pc += 4;
        }
        else if (funct3 == 6 && funct7 == 0x01) {
            // REM
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd  = get_rd(instr);

            int32_t a = (int32_t)regs[rs1];
            int32_t b = (int32_t)regs[rs2];

            if (b == 0)
                regs[rd] = (uint32_t)a;
            else
                regs[rd] = (uint32_t)(a % b);

            pc += 4;
        }
        else if (funct3 == 7 && funct7 == 0x01) {
            //  REMU
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd  = get_rd(instr);

            uint32_t a = regs[rs1];
            uint32_t b = regs[rs2];

            if (b == 0)
                regs[rd] = a;
            else
                regs[rd] = a % b;

            pc += 4;
        }
        else if (funct3 == 2 && funct7 == 0x01) {
            // MULHSU
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd  = get_rd(instr);

            int64_t a = (int64_t)(int32_t)regs[rs1];
            uint64_t b = (uint64_t)regs[rs2];

            uint64_t result = (uint64_t)(a * (int64_t)b);

            regs[rd] = (uint32_t)(result >> 32);

            pc += 4;
        }
        else if (funct3 == 3 && funct7 == 0x01) {
            // MULHU
            uint8_t rs1 = get_rs1(instr);
            uint8_t rs2 = get_rs2(instr);
            uint8_t rd  = get_rd(instr);

            uint64_t result =
            (uint64_t)regs[rs1] *
            (uint64_t)regs[rs2];

            regs[rd] = (uint32_t)(result >> 32);

            pc += 4;
        }
    }
}

