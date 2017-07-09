#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <array>
#include <cmath>
#include <bitset>
#include <iostream>

#include "interpret_table.h"
#include "instruction.h"
#include "rtype.h"
#include "itype.h"
#include "jtype.h"
#include "table.h" // stores the labels of the indices.
#include "parse.h"

using std::vector;
using std::string;
using std::unique_ptr;
using std::shared_ptr;
using std::stoi;
using std::find;
using std::array;
using std::make_shared;
using std::abs;

// registers are saved in interpret_table.h

// this function checks if something is pointing to a data segment.
// used to increment address and not add to the value its pointing to.
//
void print_reg(string reg)
{
    std::cout << reg << ": " << *registers[reg] << '\n';
}
bool is_data_address(const int* reg)
{
    for(auto& data : words)
    {
        int* address = data.second.data();
        for(; *address < data.second.size(); ++address) {
            if(reg == address) return true;
        }
    }
    return false;
}

void la(const vector<string>& instr)
{
    string reg = instr[1];
    string address_label = instr[2];
    auto spot = words.find(address_label);
    int* address = spot == words.end() ? &labels[address_label]
                                        : words[address_label].data();
    registers[reg] = address;
}

void lw(const vector<string>& instr)
{
    string rt = instr[1];
    string offset = instr[2];
    string base = instr[3];
    *registers[rt] = *(registers[base] + (stoi(offset) / 4));
}

void sw(const vector<string>& instr)
{
    string rt = instr[1];
    string offset = instr[2];
    string base = instr[3];
    *(registers[base] + (stoi(offset) / 4)) = *registers[rt];
}

void ori(const vector<string>& instr)
{
    string rs = instr[2];
    string rt = instr[1];
    string i = instr[3];
    int conv = is_num(i) ? stoi(i) : label_indices[i];
    *registers[rt] = *registers[rs] | conv;
}

void or_(const vector<string>& instr)
{
    string rd = instr[1];
    string rs = instr[2];
    string rt = instr[3];
    *registers[rd] = *registers[rs] | *registers[rt];
}

void xor_(const vector<string>& instr)
{
    string rd = instr[1];
    string rs = instr[2];
    string rt = instr[3];
    *registers[rd] = *registers[rs] ^ *registers[rt];
}

void nor(const vector<string>& instr)
{
    string rd = instr[1];
    string rs = instr[2];
    string rt = instr[3];
    *registers[rd] = ~(*registers[rs] | *registers[rt]);
}

void andi(const vector<string>& instr)
{
    string rs = instr[2];
    string rt = instr[1];
    string i = instr[3];
    *registers[rt] = *registers[rs] & stoi(i);
}

void and_(const vector<string>& instr)
{
    string rd = instr[1];
    string rs = instr[2];
    string rt = instr[3];
    *registers[rd] = *registers[rs] & *registers[rt];
}

void lui(const vector<string>& instr)
{
    string rt = instr[1];
    string i = instr[2];
    int conv = is_num(i) ? stoi(i) : label_indices[i];
    conv = stoi(i) & ~0xFFFF; // sets the bottom 16 bits to 0
    *registers[rt] = conv; 
}

void addi(const vector<string>& instr)
{
    string rs = instr[2];
    string rt = instr[1];
    string i = instr[3];
    *registers[rt] = *registers[rs] + stoi(i);
}

void add(const vector<string>& instr)
{
    string rd = instr[1];
    string rs = instr[2];
    string rt = instr[3];
    *registers[rd] = *registers[rs] + *registers[rt];
}

void addu(const vector<string>& instr)
{
    string rd = instr[1];
    string rs = instr[2];
    string rt = instr[3];
    *registers[rd] = *registers[rs] + *registers[rt];
}

void addiu(const vector<string>& instr)
{
    string rt = instr[1];
    string rs = instr[2];
    string i = instr[3];
    if(is_data_address(registers[rs]))
    {
        registers[rt] = &*(registers[rs] + (stoi(i) / 4));
    }
    else {
        *registers[rt] = *registers[rs] + stoi(i);
    }
}

void sub(const vector<string>& instr)
{
    string rd = instr[1];
    string rs = instr[2];
    string rt = instr[3];
    if(is_data_address(registers[rs]))
    {
        registers[rt] = &*(registers[rs] - (*registers[rt] / 4));
    }
    else
    {
        *registers[rd] = *registers[rs] - *registers[rt];
    }
}

void subi(const vector<string>& instr)
{
    string rt = instr[1];
    string rs = instr[2];
    string i = instr[3];
    *registers[rt] = *registers[rs] - stoi(i);
}

int j(const vector<string>& instr)
{
    string offset = instr[1];
    if(is_num(offset))
    {
        return stoi(offset);
    }
    return label_indices[offset];
}

// saves the return address in $ra.
int jal(const vector<string>& instr, int pc)
{
    *registers["ra"] = pc + 1;
    return j(instr);
}

int jr(const vector<string>& instr)
{
    string rs = instr[1];
    return *registers[rs];
}

int beq(const vector<string>& instr, int pc)
{
    string rs = instr[1];
    string rt = instr[2];
    string offset = instr[3];
    if(registers[rs] == registers[rt])
    {
        if(is_num(offset))
        {
            pc += (stoi(offset) / 4);
        }
        else
        {
            pc = label_indices[offset];
        }
    }
    return pc;
}

int bne(const vector<string>& instr, int pc)
{
    string rs = instr[1];
    string rt = instr[2];
    string offset = instr[3];

    if(registers[rs] != registers[rt]) {
        if(is_num(offset))
        {
            pc += (stoi(offset) / 4);
        }
        else
        {
            pc = label_indices[offset];
        }
    }
    return pc;
}

//should be 0 extended by default?
void lbu(const vector<string>& instr)
{
    string rt = instr[1];
    string offset = instr[2];
    string base = instr[3];
    uint8_t byte = *(registers[base] + (stoi(offset) / 4));
    *registers[rt] = static_cast<int>(byte); 
}

void lhu(const vector<string>& instr)
{
    string rt = instr[1];
    string offset = instr[2];
    string base = instr[3];
    uint16_t half = *(registers[base] + (stoi(offset) / 4));
    *registers[rt] = static_cast<int>(half);
}

void sll(const vector<string>& instr)
{
    string rd = instr[1];
    string rt = instr[2];
    string sa = instr[3];
    *registers[rd] = *registers[rt] << stoi(sa);
}

void sr_(const vector<string>& instr)
{
    string rd = instr[1];
    string rt = instr[2];
    string sa = instr[3];
    *registers[rd] = *registers[rt] >> stoi(sa);
}

void mult(const vector<string>& instr)
{
    string rs = instr[1];
    string rt = instr[2];
    const int val1 = *registers[rs];
    const int val2 = *registers[rt];
    std::bitset<64> prod(val1 * val2);
    string binary_s_prod = prod.to_string();
    string upper(32, '0');
    string lower(32, '0');
    for(int i = 0; i < 32; ++i)
    {
        upper[i] = binary_s_prod[i];
        lower[i] = binary_s_prod[i+32];
    }
    special_registers::hi = stoi(upper, nullptr, 2);
    special_registers::lo = stoi(lower, nullptr, 2);
}

void div(const vector<string>& instr)
{
    string rs = instr[1];
    string rt = instr[2];
    const int val1 = *registers[rs];
    const int val2 = *registers[rt];
    std::bitset<64> qout(val1 / val2);
    string binary_s_quot = qout.to_string();
    string upper(32, '0');
    string lower(32, '0');
    for(int i = 0; i < 32; ++i)
    {
        upper[i] = binary_s_quot[i];
        lower[i] = binary_s_quot[i+32];
    }
    special_registers::hi = stoi(upper, nullptr, 2);
    special_registers::lo = stoi(lower, nullptr, 2);
}

bool is_la(array<vector<string>, 2>& coms)
{
    vector<string> one = coms[0];
    vector<string> two = coms[1];
    return one[0] == "lui" && two[0] == "ori" && one[2] == two[3];
}

bool interpret(vector<unique_ptr<Instruction>>& instructions)
{
    int program_counter = 0;
    const int final_pc = instructions.size();

    int return_index = 0;

    for( ; program_counter < final_pc; ++program_counter )
    {
        const vector<string> com = instructions[program_counter]
                                                    ->
                                                    get_original();
        if(com[0] == "addi")
        {
            addi(com);
        }
        else if(com[0] == "add")
        {
            add(com);
        }
        else if(com[0] == "sub")
        {
            sub(com);
        }
        else if(com[0] == "subi")
        {
            subi(com);
        }
        else if(com[0] == "lui")
        {
            vector<string> next = instructions[program_counter + 1]
                                                    ->
                                                    get_original();
            array<vector<string>, 2> coms {
                com,
                next
            };
            if(is_la(coms))
            {
                vector<string> real {
                    "la",
                    next[1],
                    com[2],
                };
                la(real);
                ++program_counter;
            }
            else {
                lui(com);
            }
        }
        else if(com[0] == "lw")
        {
            lw(com);
        }
        else if(com[0] == "sw")
        {
            sw(com);
        }
        else if(com[0] == "andi")
        {
            andi(com);
        }
        else if(com[0] == "j")
        {
            program_counter = j(com);
        }
        else if(com[0] == "beq")
        {
            program_counter = beq(com, program_counter);
        }
        else if(com[0] == "bne")
        {
            program_counter = bne(com, program_counter);
        }
        else if(com[0] == "addiu")
        {
            addiu(com);
        }
        else if(com[0] == "addu")
        {
            addu(com);
        }
        else if(com[0] == "and")
        {
            and_(com);
        }
        else if(com[0] == "jal")
        {
            program_counter = jal(com, program_counter);
        }
        else if(com[0] == "jr")
        {
            program_counter = jr(com);
        }
        else if(com[0] == "ori")
        {
            ori(com);
        }
        else if(com[0] == "or")
        {
            or_(com);
        }
        else if(com[0] == "lbu")
        {
            lbu(com);
        }
        else if(com[0] == "lhu")
        {
            lhu(com);
        }
        else if(com[0] == "sll")
        {
            sll(com);
        }
        else if(com[0] == "srl")
        {
            // assuming c++ will choose srl or sra.
            sr_(com);
        }
        else if(com[0] == "sra")
        {
            sr_(com);
        }
        else if(com[0] == "xor")
        {
            xor_(com);
        }
        else if(com[0] == "nor")
        {
            nor(com);
        }
        else if(com[0] == "mult")
        {
            mult(com);
        }
    }
    print_reg("$t5");
    print_reg("$s1");
    std::cout << special_registers::hi << special_registers::lo << '\n';
    return true; // successful.
}
