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
unsigned long long rob_head, rob_tail = 0;
int inst_count = 0;
unsigned long long ex_pos;
unsigned long long iq_pos;
unsigned long long wb_pos;

void execute()
{
    if(!ex_list.empty)
    {
        for(int i = 0; i<width*5; i++)
        {
            if(ex_list.reg[i].cycles>0)
            {
                ex_list.reg[i].cycles--;
            }

            if(ex_list.reg[i].inst_num != -1 && output_table[ex_list.reg[i].inst_num].EX.first==0)
            {
                if(ex_list.reg[i].optype == 0)
                {
                    output_table[ex_list.reg[i].inst_num].EX = {sim_cycle, 1};
                }
                else if(ex_list.reg[i].optype == 1)
                {
                    output_table[ex_list.reg[i].inst_num].EX = {sim_cycle, 2};
                }
                else
                {
                    output_table[ex_list.reg[i].inst_num].EX = {sim_cycle, 5};
                }
            }
        }

        for(int i=0; i<width*5 ; i++)
        {
            if(ex_list.reg[i].cycles == 0)
            {
                WB.reg[wb_pos].inst_num = ex_list.reg[i].inst_num;
                WB.reg[wb_pos].optype = ex_list.reg[i].optype;
                WB.reg[wb_pos].pc = ex_list.reg[i].pc;
                WB.reg[wb_pos].cycles = ex_list.reg[i].cycles;
                WB.reg[wb_pos].rs1 = ex_list.reg[i].rs1;
                WB.reg[wb_pos].rs2 = ex_list.reg[i].rs2;
                WB.reg[wb_pos].dest_reg = ex_list.reg[i].dest_reg;

                for(int j = 0; j<iq_size; j++)
                {
                    if(issue_queue_table[j].rs1_rdy == false && issue_queue_table[j].rs1_tag_or_val == WB.reg[wb_pos].dest_reg)
                    {
                        issue_queue_table[j].rs1_rdy = true;
                    }
                    if(issue_queue_table[j].rs2_rdy == false && issue_queue_table[j].rs2_tag_or_val == WB.reg[wb_pos].dest_reg)
                    {
                        issue_queue_table[j].rs2_rdy =true;
                    }
                }

                for(int j= 0; j<width; j++)
                {
                    if(RR.reg[j].rs1 == WB.reg[wb_pos].dest_reg && RR.reg[j].rs1_rdy == false)
                    {
                        RR.reg[j].rs1_rdy = true;
                    }
                    if(RR.reg[j].rs2 == WB.reg[wb_pos].dest_reg && RR.reg[j].rs2_rdy == false)
                    {
                        RR.reg[j].rs2_rdy = true;
                    }

                    if(DI.reg[j].rs1 == WB.reg[wb_pos].dest_reg && DI.reg[j].rs1_rdy == false)
                    {
                        DI.reg[j].rs1_rdy = true;
                    }
                    if(DI.reg[j].rs1 == WB.reg[wb_pos].dest_reg && DI.reg[j].rs1_rdy == false)
                    {
                        DI.reg[j].rs2_rdy = true;
                    }
                }

                WB.current_size++;
                WB.empty = false;
                wb_pos = (wb_pos + 1)%(width*5);

                ex_list.reg[ex_pos].inst_num = -1;
                ex_list.reg[ex_pos].optype = 0;
                ex_list.reg[ex_pos].pc = 0;
                ex_list.reg[ex_pos].cycles = -1;
                ex_list.reg[ex_pos].rs1 = 0;
                ex_list.reg[ex_pos].rs2 = 0;
                ex_list.reg[ex_pos].dest_reg = 0;

            }
        }
    }
    return;
}

bool iq_empty()
{
    int count = 0;
    for(int i=0; i<iq_size; i++)
    {
        if(issue_queue_table[i].valid == false)
        {
            count++;
        }
    }

    if(count!=iq_size)
    {
        return false;
    }

    return true;
}

void issue()
{
    if(!iq_empty())
    {
        for(int i=0; i<iq_size; i++)
        {
            if( issue_queue_table[i].age != -1 && issue_queue_table[i].valid == true && output_table[issue_queue_table[i].age].IS.first == 0)
            {
                output_table[issue_queue_table[i].age].IS = {sim_cycle, 1};
            }
            else if(issue_queue_table[i].age != -1 && issue_queue_table[i].valid == true)
            {
                output_table[issue_queue_table[i].age].IS.second++;
            }
        }

        for(int i =0; i<width; i++)
        {
            int min = INT_MAX;
            int remove_iq_inst = -1;
            int k = -2;
            for(int j = 0; j<iq_size; j++)
            {
                if(issue_queue_table[j].valid && (issue_queue_table[j].rs1_rdy && issue_queue_table[j].rs2_rdy) && (issue_queue_table[j].age < min))
                {
                    ex_list.reg[ex_pos].inst_num = issue_queue_table[j].inst_num;
                    ex_list.reg[ex_pos].optype = issue_queue_table[j].optype;
                    ex_list.reg[ex_pos].pc = issue_queue_table[j].pc;
                    ex_list.reg[ex_pos].cycles = issue_queue_table[j].cycles;
                    ex_list.reg[ex_pos].rs1 = issue_queue_table[j].rs1_tag_or_val;
                    ex_list.reg[ex_pos].rs2 = issue_queue_table[j].rs2_tag_or_val;
                    ex_list.reg[ex_pos].dest_reg = issue_queue_table[j].d_tag;

                    min = issue_queue_table[j].age;
                    remove_iq_inst = j;
                    k = issue_queue_table[j].inst_num;
                }
            }
            if(k!=-2)
            {
                issue_queue_table[remove_iq_inst].valid = false;
                issue_queue_table[remove_iq_inst].d_tag = 0;
                issue_queue_table[remove_iq_inst].pc = 0;
                issue_queue_table[remove_iq_inst].optype = 0;
                issue_queue_table[remove_iq_inst].cycles = 0;
                issue_queue_table[remove_iq_inst].inst_num = -1;
                ex_pos = (ex_pos+1)%(width*5);
            }
            
        }

        ex_list.empty = false;
    }
    
    return;
}

bool check_iq_status()
{
    int count = 0;
    for(int i=0; i<iq_size; i++)
    {
        if(issue_queue_table[i].valid == false)
        {
            count++;
        }
    }

    if(count>=width)
    {
        return false;
    }

    return true;
}

void dispatch()
{
    if(!DI.empty)
    {
        bool iq_full = check_iq_status();
        if(iq_full)
        {
            for(int i=0; i<width; i++)
            {
                if(DI.reg[i].inst_num != -1 && output_table[DI.reg[i].inst_num].DI.first == 0)
                {
                    output_table[DI.reg[i].inst_num].DI = {sim_cycle, 1};
                }
                else if(DI.reg[i].inst_num != -1)
                {
                    output_table[DI.reg[i].inst_num].DI.second++;
                }
            }

            return;
        }
        else
        {
            for(int i = 0; i<width; i++)
            {
                for(int j = 0; j<iq_size; j++)
                {
                    if(issue_queue_table[j].valid == false)
                    {
                        iq_pos = j;
                        break;
                    }
                }

                issue_queue_table[iq_pos].valid = true;
                issue_queue_table[iq_pos].d_tag = DI.reg[i].dest_reg;
                issue_queue_table[iq_pos].inst_num = DI.reg[i].inst_num;
                issue_queue_table[iq_pos].pc = DI.reg[i].pc;
                issue_queue_table[iq_pos].optype = DI.reg[i].optype;
                issue_queue_table[iq_pos].cycles = DI.reg[i].cycles;

                if(DI.reg[i].inst_num != -1 && output_table[DI.reg[i].inst_num].DI.first == 0)
                {
                    output_table[DI.reg[i].inst_num].DI = {sim_cycle, 1};
                }
                else if(DI.reg[i].inst_num != -1)
                {
                    output_table[DI.reg[i].inst_num].DI.second++;
                }

                issue_queue_table[iq_pos].age = inst_count;
                inst_count++;

                int src1 = DI.reg[i].rs1;
                int src2 = DI.reg[i].rs2;

                if(src1 != -1)
                {
                    if(reorder_buffer_table[src1].valid && DI.reg[i].rob_rs1)
                    {
                        issue_queue_table[iq_pos].rs1_rdy = reorder_buffer_table[src1].rdy;
                        issue_queue_table->rs1_tag_or_val = src1;
                    }
                    else
                    {
                        issue_queue_table[iq_pos].rs1_rdy = true;
                        issue_queue_table->rs1_tag_or_val = src1;
                    }

                    if(issue_queue_table[iq_pos].rs1_rdy == false && DI.reg[i].rs1_rdy == true)
                    {
                        issue_queue_table[iq_pos].rs1_rdy = true;
                    }

                }
                else
                {
                    issue_queue_table[iq_pos].rs1_rdy = true;
                    issue_queue_table->rs1_tag_or_val = 0;
                }

                if(src2 != -1)
                {
                    if(reorder_buffer_table[src2].valid && DI.reg[i].rob_rs2)
                    {
                        issue_queue_table[iq_pos].rs2_rdy = reorder_buffer_table[src2].rdy;
                        issue_queue_table->rs2_tag_or_val = src2;
                    }
                    else
                    {
                        issue_queue_table[iq_pos].rs2_rdy = true;
                        issue_queue_table->rs2_tag_or_val = src2;
                    }

                    if(issue_queue_table[iq_pos].rs2_rdy == false && DI.reg[i].rs2_rdy == true)
                    {
                        issue_queue_table[iq_pos].rs2_rdy = true;
                    }

                }
                else
                {
                    issue_queue_table[iq_pos].rs2_rdy = true;
                    issue_queue_table->rs2_tag_or_val = 0;
                }

                iq_pos = (iq_pos+1)%iq_size;
                
                DI.reg[i].inst_num = -1;
                DI.reg[i].cycles = -1;
                DI.reg[i].pc = 0;
                DI.reg[i].optype=0;
                DI.reg[i].dest_reg=0;
                DI.reg[i].rs1=0;
                DI.reg[i].rs2=0;
            }

            DI.empty = true;
            DI.full = false;
        }
    }

    return;
}

void regread()
{
    if(!RR.empty)
    {
        if(DI.full)
        {
            for(int i=0; i<width; i++)
            {
                if(RR.reg[i].inst_num != -1 && output_table[RR.reg[i].inst_num].RR.first == 0)
                {
                    output_table[RR.reg[i].inst_num].RR = {sim_cycle, 1};
                }
                else if(RR.reg[i].inst_num != -1)
                {
                    output_table[RR.reg[i].inst_num].RR.second++;
                }
            }

            return;
        }
        else
        {
            //copy RR to DI

            for(int i = 0; i<width; i++)
            {
                DI.reg[i].inst_num = RR.reg[i].inst_num;
                DI.reg[i].cycles = RR.reg[i].cycles;
                DI.reg[i].pc = RR.reg[i].pc;
                DI.reg[i].optype = RR.reg[i].optype;
                DI.reg[i].dest_reg = RR.reg[i].dest_reg;
                DI.reg[i].rs1 = RR.reg[i].rs1;
                DI.reg[i].rs2 = RR.reg[i].rs2;
                DI.reg[i].rob_rs1 = RR.reg[i].rob_rs1;
                DI.reg[i].rob_rs2 = RR.reg[i].rob_rs2;
                DI.reg[i].rs1_rdy = RR.reg[i].rs1_rdy;
                DI.reg[i].rs2_rdy = RR.reg[i].rs2_rdy;

                // RR.current_size--;

                if(RR.reg[i].inst_num != -1 && output_table[RR.reg[i].inst_num].RR.first == 0)
                {
                    output_table[RR.reg[i].inst_num].RR = {sim_cycle, 1};
                }
                else if(RR.reg[i].inst_num != -1)
                {
                    output_table[RR.reg[i].inst_num].RR.second++;
                }

                if(reorder_buffer_table[RR.reg[i].rs1].valid && reorder_buffer_table[RR.reg[i].rs1].rdy && DI.reg[i].rob_rs1 && DI.reg[i].rs1_rdy == false)
                {
                    DI.reg[i].rs1_rdy = true;
                }
                if(reorder_buffer_table[RR.reg[i].rs2].valid && reorder_buffer_table[RR.reg[i].rs2].rdy && DI.reg[i].rob_rs2 && DI.reg[i].rs2_rdy == false)
                {
                    DI.reg[i].rs2_rdy = true;
                }



                RR.reg[i].inst_num = -1;
                RR.reg[i].cycles = -1;
                RR.reg[i].pc = 0;
                RR.reg[i].optype=0;
                RR.reg[i].dest_reg=0;
                RR.reg[i].rs1=0;
                RR.reg[i].rs2=0;
                RR.reg[i].rob_rs1 = false;
                RR.reg[i].rob_rs2 = false;
                RR.reg[i].rs1_rdy = false;
                RR.reg[i].rs2_rdy = false;
            }

            RR.empty = true;
            RR.full = false;
            DI.empty = false;
            DI.full = true;
        }
    }
    return;
}

bool check_rob_status()
{
    int count = 0;
    for(int i=0; i<rob_size; i++)
    {
        if(reorder_buffer_table[i].valid == false)
        {
            count++;
        }
    }

    if(count>=width)
    {
        return false;
    }

    return true;
}

void rename()
{
    if(!RN.empty)
    {
        bool rob_full = check_rob_status();
        if(RR.full || rob_full )
        {
            for(int i=0; i<width; i++)
            {
                if(RN.reg[i].inst_num != -1 && output_table[RN.reg[i].inst_num].RN.first == 0)
                {
                    output_table[RN.reg[i].inst_num].RN = {sim_cycle, 1};
                }
                else if(RN.reg[i].inst_num != -1)
                {
                    output_table[RN.reg[i].inst_num].RN.second++;
                }
            }

            return;

        }
        else
        {
            for(int i=0; i<width; i++)
            {
                int src1, src2, dest = -1;

                src1 = RN.reg[i].rs1;
                src2 = RN.reg[i].rs2;
                dest = RN.reg[i].dest_reg;

                RR.reg[i].rob_rs1 = false;
                RR.reg[i].rob_rs2 = false;
                RR.reg[i].rs1_rdy = false;
                RR.reg[i].rs2_rdy = false;

                if(RN.reg[i].rs1!=-1)
                {
                    if(rename_map_table[RN.reg[i].rs1].valid)
                    {
                        src1 = rename_map_table[RN.reg[i].rs1].rob_tag;
                        RR.reg[i].rob_rs1 = true;
                    }
                }

                if(RN.reg[i].rs2!=-1)
                {
                    if(rename_map_table[RN.reg[i].rs2].valid)
                    {
                        src2 = rename_map_table[RN.reg[i].rs2].rob_tag;
                        RR.reg[i].rob_rs2 = true;
                    }
                }

                if(RN.reg[i].dest_reg!=-1)
                {
                    rename_map_table[RN.reg[i].dest_reg].valid = true;
                    rename_map_table[RN.reg[i].dest_reg].rob_tag = rob_tail;
                    dest = rob_tail;

                    reorder_buffer_table[rob_tail].valid = true;
                    reorder_buffer_table[rob_tail].dest = RN.reg[i].dest_reg;
                    reorder_buffer_table[rob_tail].value = -1;
                    reorder_buffer_table[rob_tail].rdy = false;
                    reorder_buffer_table[rob_tail].exc = false;
                    reorder_buffer_table[rob_tail].miss_pred = false;
                    reorder_buffer_table[rob_tail].pc = RN.reg[i].inst_num;

                    rob_tail = (rob_tail+1)%rob_size; //cyclic queue 

                }
                else
                {
                    reorder_buffer_table[rob_tail].valid = true;
                    reorder_buffer_table[rob_tail].dest = RN.reg[i].dest_reg;
                    reorder_buffer_table[rob_tail].value = -1;
                    reorder_buffer_table[rob_tail].rdy = false;
                    reorder_buffer_table[rob_tail].exc = false;
                    reorder_buffer_table[rob_tail].miss_pred = false;
                    reorder_buffer_table[rob_tail].pc = RN.reg[i].inst_num;

                    rob_tail = (rob_tail+1)%rob_size; //cyclic queue 
                }

                RR.reg[i].inst_num = RN.reg[i].inst_num;
                RR.reg[i].cycles = RN.reg[i].cycles;
                RR.reg[i].pc = RN.reg[i].pc;
                RR.reg[i].optype = RN.reg[i].optype;
                RR.reg[i].dest_reg = dest;
                RR.reg[i].rs1 = src1;
                RR.reg[i].rs2 = src2;

                if(RN.reg[i].inst_num != -1 && output_table[RN.reg[i].inst_num].RN.first == 0)
                {
                    output_table[RN.reg[i].inst_num].RN = {sim_cycle, 1};
                }
                else if(RN.reg[i].inst_num != -1)
                {
                    output_table[RN.reg[i].inst_num].RN.second++;
                }

                RN.reg[i].inst_num = -1;
                RN.reg[i].cycles = -1;
                RN.reg[i].pc = 0;
                RN.reg[i].optype=0;
                RN.reg[i].dest_reg=0;
                RN.reg[i].rs1=0;
                RN.reg[i].rs2=0;
                RN.reg[i].rob_rs1 = false;
                RN.reg[i].rob_rs2 = false;
                RN.reg[i].rs1_rdy = false;
                RN.reg[i].rs2_rdy = false;
            }

            RN.empty = true;
            RN.full = false;
            RR.empty = false;
            RR.full = true;
        }
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
            // adv_DE_to_RN(&DE, &RN);
            for(int i = 0; i<width; i++)
            {
                RN.reg[i].inst_num = DE.reg[i].inst_num;
                RN.reg[i].cycles = DE.reg[i].cycles;
                RN.reg[i].pc = DE.reg[i].pc;
                RN.reg[i].optype = DE.reg[i].optype;
                RN.reg[i].dest_reg = DE.reg[i].dest_reg;
                RN.reg[i].rs1 = DE.reg[i].rs1;
                RN.reg[i].rs2 = DE.reg[i].rs2;

                DE.current_size--;

                if(RN.reg[i].inst_num != -1 && output_table[RN.reg[i].inst_num].DE.first == 0)
                {
                    output_table[RN.reg[i].inst_num].DE = {sim_cycle, 1};
                }
                else if(RN.reg[i].inst_num != -1)
                {
                    output_table[RN.reg[i].inst_num].DE.second++;
                }

                DE.reg[i].inst_num = -1;
                DE.reg[i].cycles = -1;
                DE.reg[i].pc = 0;
                DE.reg[i].optype=0;
                DE.reg[i].dest_reg=0;
                DE.reg[i].rs1=0;
                DE.reg[i].rs2=0;
            }
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
        issue_queue_table[i] = issue_queue(false, 0, false, 0, false, 0, -1, 0, 0, 0, 0);
    }

    for(int i =0; i<params.rob_size; i++)
    {
        reorder_buffer_table[i] = reorder_buffer(0, 0, false, false, false, 0, false);
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
        execute();
        issue();
        dispatch();
        regread();
        rename();
        decode();
        fetch(&FP);
    } while (adv_cycle());
    
    return 0;
}
