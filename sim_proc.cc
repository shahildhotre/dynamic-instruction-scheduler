#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<iostream>
#include <inttypes.h>
#include "sim_proc.h"
#include <vector>

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/

//global_variables
int num_of_instruction_in_pipeline = 0;
int sim_cycle = 0;
bool trace_depleted = false;
unsigned long int width;
unsigned long int rob_size;
unsigned long int iq_size;

issue_queue* issue_queue_table;
reorder_buffer* reorder_buffer_table;
rmt* rename_map_table;
std::vector<output> output_table;
whole_DE_reg DE;
whole_OO_reg RN;
whole_OO_reg RR;
whole_OO_reg DI;
whole_OO_reg ex_list;
whole_OO_reg WB;


unsigned long long program_counter = 0;
unsigned long long dynamic_instrunct_count = 0;

void adv_DE_to_RN(whole_DE_reg* src, whole_OO_reg* tar)
{
    for(int i = 0; i<width; i++)
    {
        tar->reg[i].inst_num = src->reg[i].inst_num;
        tar->reg[i].cycles = src->reg[i].cycles;
        tar->reg[i].pc = src->reg[i].pc;
        tar->reg[i].optype = src->reg[i].optype;
        tar->reg[i].dest_reg = src->reg[i].dest_reg;
        tar->reg[i].rs1 = src->reg[i].rs1;
        tar->reg[i].rs2 = src->reg[i].rs2;

        src->current_size--;

        if(tar->reg[i].inst_num != -1 && output_table[tar->reg[i].inst_num].DE.first == 0)
        {
            output_table[tar->reg[i].inst_num].DE = {sim_cycle, 1};
        }
        else if(tar->reg[i].inst_num != -1)
        {
            output_table[tar->reg[i].inst_num].DE.second++;
        }

        src->reg[i].inst_num = -1;
        src->reg[i].cycles = -1;
        src->reg[i].pc = 0;
        src->reg[i].optype=0;
        src->reg[i].dest_reg=0;
        src->reg[i].rs1=0;
        src->reg[i].rs2=0;
    }
    return;
}

void decode()
{
    if(!DE.empty)
    {
        if(RN.full)
        {
            for(int i=0; i<width; i++)
            {
                if(DE.reg[i].inst_num != -1 && output_table[DE.reg[i].inst_num].DE.first == 0)
                {
                    output_table[DE.reg[i].inst_num].DE = {sim_cycle, 1};
                }
                else if(DE.reg[i].inst_num != -1)
                {
                    output_table[DE.reg[i].inst_num].DE.second++;
                }
            }

            return;
        }
        else{
            //copy DE to RN;
            adv_DE_to_RN(&DE, &RN);
            DE.empty = true;
            DE.full = false;
            RN.empty = false;
            RN.full = true;
        }
    }
    return;
}

void fetch(FILE **fp)
{
    if(DE.full || trace_depleted)
    {
        return;
    }

    for(int i=0; i<width; i++)
    {
        std::cout<<i<<std::endl;
        unsigned long int pc;
        int op_type, dest, src1, src2;

        if(fscanf(*fp, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2)!= EOF)
        {
            dynamic_instrunct_count++;
            DE.reg[i].inst_num = program_counter++;
            DE.reg[i].pc = pc;
            DE.reg[i].optype = op_type;
            DE.reg[i].dest_reg = dest;
            DE.reg[i].rs1 = src1;
            DE.reg[i].rs2 = src2;
            DE.current_size++;
            DE.empty = false;

            if(DE.current_size == width)
            {
                DE.full = true;
            }
            if(op_type == 0)
            {
                DE.reg[i].cycles = 1;
            }
            else if(op_type == 1)
            {
                DE.reg[i].cycles = 2;
            }
            else if(op_type == 2)
            {
                DE.reg[i].cycles = 5;
            }
            
            output_table.push_back(output(DE.reg[i].optype, {src1, src2}, dest, {sim_cycle, 1}, {0, 0}, {0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0}));
            num_of_instruction_in_pipeline++;
        }
        else
        {
            trace_depleted = true;
            std::cout<<"iamhere"<<std::endl;
        }

    }

    return;
}

bool adv_cycle()
{
    sim_cycle++;

    if(num_of_instruction_in_pipeline <= 0 && trace_depleted == true)
    {
        return false;
    }

    return true;
}

int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    uint64_t pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    printf("rob_size:%lu "
            "iq_size:%lu "
            "width:%lu "
            "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //     printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly

    //intializing to avoid seg and page fault issue

    issue_queue_table = (issue_queue*)malloc(params.iq_size*sizeof(issue_queue));
    reorder_buffer_table = (reorder_buffer*)malloc(params.rob_size*sizeof(reorder_buffer));
    rename_map_table = (rmt*)malloc(total_arch_reg*sizeof(rmt));
    DE.reg = (DE_reg*)malloc(params.width*sizeof(DE_reg));
    RN.reg = (OO_reg*)malloc(params.width*sizeof(OO_reg));
    RR.reg = (OO_reg*)malloc(params.width*sizeof(OO_reg));
    DI.reg = (OO_reg*)malloc(params.width*sizeof(OO_reg));
    ex_list.reg = (OO_reg*)malloc(params.width*5*sizeof(OO_reg));
    WB.reg = (OO_reg*)malloc(params.width*5*sizeof(OO_reg));

    for(int i=0; i<params.iq_size; i++)
    {
        issue_queue_table[i] = issue_queue(false, 0, false, 0, false, 0);
    }

    for(int i =0; i<params.rob_size; i++)
    {
        reorder_buffer_table[i] = reorder_buffer(0, 0, false, false, false, 0);
    }

    for(int i = 0; i<total_arch_reg; i++)
    {
        rename_map_table[i] = rmt(false, 0);
    }

    for(int i=0; i<params.width; i++)
    {
        DE.reg[i] = DE_reg(-1, 0, 0, 0, 0, 0, 0);
        RN.reg[i] = OO_reg(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
        RR.reg[i] = OO_reg(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
        DI.reg[i] = OO_reg(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0);

    }

    for(int i=0; i<params.width*5; i++)
    {
        ex_list.reg[i] = OO_reg(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
        WB.reg[i] = OO_reg(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0);
    }

    DE.full, RN.full, RR.full, DI.full, ex_list.full, WB.full = false;
    DE.empty,RN.empty, RR.empty, DI.empty, ex_list.empty, WB.empty = true;
    DE.current_size, RN.current_size, RR.current_size, DI.current_size, ex_list.current_size, WB.current_size = 0;

    width = params.width;
    rob_size = params.rob_size;
    iq_size = params.iq_size;


    do
    {
        /* code */
        decode();
        fetch(&FP);
    } while (adv_cycle());
    
    return 0;
}
