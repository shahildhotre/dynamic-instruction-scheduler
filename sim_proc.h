#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <vector>
#include <utility>

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct issue_queue{
    bool valid;
    long int d_tag;
    bool rs1_rdy;
    bool rs2_rdy;
    long int rs1_tag_or_val;
    long int rs2_tag_or_val;

    issue_queue(bool valid, long int d_tag, bool rs1_rdy, bool rs2_rdy, long int rs1_tag_or_val, long int rs2_tag_or_val)
    {
        this->valid = valid;
        this->d_tag = d_tag;
        this->rs1_rdy = rs1_rdy;
        this->rs1_tag_or_val = rs1_tag_or_val;
        this->rs2_rdy = rs2_rdy;
        this->rs2_tag_or_val = rs2_tag_or_val;
    }
}issue_queue;

typedef struct reorder_buffer{
    long int value;
    long int dest;
    bool rdy;
    bool exc;
    bool miss_pred;
    unsigned long long pc;

    reorder_buffer(long int value, long int dest, bool rdy, bool exc, bool miss_pred, unsigned long long pc)
    {
        this->value = value;
        this->dest = dest;
        this->rdy = rdy;
        this->exc = exc;
        this->miss_pred = miss_pred;
        this->pc = pc;
    }
}reorder_buffer;

typedef struct rmt{
    bool valid;
    long int rob_tag;

    rmt(bool valid, long int rob_tag)
    {
        this->valid = valid;
        this->rob_tag = rob_tag;
    }
}rmt;

typedef struct pipeline{
    int seq;
    int optype;
    int rs1;
    int rs2;
    int dest_reg;
    int fe, de, rn, rr, di, is, ex, wb, retire_begin, retire_end;
}pipleine;

typedef struct DE_reg{
    unsigned long int pc;
    unsigned long long inst_num;
    int optype;
    int cycles;
    // bool valid;
    int dest_reg;
    int rs1;
    int rs2;

    DE_reg(long long int inst_num,unsigned long long pc, int optype, int cycles, int dest_reg, int rs1, int rs2)
    {
        this->pc = pc;
        this->inst_num = inst_num;
        this->dest_reg = dest_reg;
        this->optype = optype;
        this->cycles = cycles;
        this->rs1 = rs1;
        this->rs2 = rs2;
    }
}DE_reg;

typedef struct whole_DE_reg{
    bool full;
    bool empty;
    unsigned int current_size;
    DE_reg* reg;
}whole_DE_reg;

typedef struct OO_reg{
    unsigned long int pc;
    unsigned long long inst_num;
    int optype;
    int cycles;
    // bool valid;
    int dest_reg;
    int rs1;
    int rs2;

    bool rob_rs1, rob_rs2, rs1_rdy, rs2_rdy;

    OO_reg(long long int inst_num,unsigned long long pc, int optype, int cycles, int dest_reg, int rs1, int rs2, bool rob_rs1, bool rob_rs2, bool rs1_rdy, bool rs2_rdy)
    {
        this->pc = pc;
        this->inst_num = inst_num;
        this->dest_reg = dest_reg;
        this->optype = optype;
        this->cycles = cycles;
        this->rs1 = rs1;
        this->rs2 = rs2;
        this->rob_rs1 = rob_rs1;
        this->rob_rs2 = rob_rs2;
        this->rs1_rdy = rs1_rdy;
        this->rs2_rdy = rs2_rdy;
    }

}OO_reg;

typedef struct whole_OO_reg{
    bool full;
    bool empty;
    unsigned int current_size;
    OO_reg* reg;
}whole_OO_reg;

typedef struct output{
    unsigned int fu;
    unsigned int dest;
    std::pair<unsigned long long, unsigned long long> src, FE, DE, RN, RR, DI, IS, EX, WB, RT;

    output(unsigned int fu,  std::pair<unsigned long long, unsigned long long> src, unsigned int dest, std::pair<unsigned long long, unsigned long long> FE, std::pair<unsigned long long, unsigned long long> DE, std::pair<unsigned long long, unsigned long long> RN, std::pair<unsigned long long, unsigned long long> RR, std::pair<unsigned long long, unsigned long long> DI, std::pair<unsigned long long, unsigned long long> IS, std::pair<unsigned long long, unsigned long long> EX, std::pair<unsigned long long, unsigned long long> WB, std::pair<unsigned long long, unsigned long long> RT){
        this->fu = fu;
        this->src = src;
        this->dest = dest;
        this->FE = FE;
        this->DE = DE;
        this->RN = RN;
        this->RR = RR;
        this->DI = DI;
        this->IS = IS;
        this->EX = EX;
        this->WB = WB;
        this->RT = RT;
    }


}output;





// Put additional data structures here as per your requirement

#define total_arch_reg 67

#endif
