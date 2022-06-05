#include <iostream>
#include <fstream>
#include <iterator>
#include <queue>
#include <map>
#include <sstream>
using namespace std;

string source_path = "source.txt";
string opcode_path = "opcode.txt";

typedef struct optab optab;
typedef struct instruction instruction;

// 將輸入資料分割整理
void Split(const string &s, char delim, queue<string> &elems)
{
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim))
    {
        elems.push(item);
    }
}

struct optab
{
    string opcode;
    int val = 0x0;
};

struct instruction
{
    int loc_count = 0x0;
    string label;
    string opcode;
    string operand;
    int is_directive = 0;
};

int main()
{
    instruction instructions[500];
    int location_counter = 0x0;
    map<string, int> symtab;

    string row;
    ifstream file_source(source_path, ifstream::in);

    // 紀錄Start
    getline(file_source, row);
    queue<string> temp;
    Split(row, '\t', temp);
    instructions[0].label = temp.front();
    temp.pop();
    instructions[0].opcode = temp.front();
    temp.pop();
    instructions[0].operand = temp.front();
    temp.pop();
    instructions[0].is_directive = 1;

    // 取得起始address in hex
    string s = "0x" + instructions[0].operand;
    location_counter = std::stoul(s, nullptr, 16);
    instructions[0].loc_count = location_counter;

    int index = 1;
    // pass1 > 計算location counter & Symbol Table
    while (getline(file_source, row))
    {
        queue<string> row_values;
        Split(row, '\t', row_values);

        instructions[index].label = row_values.front();
        row_values.pop();
        instructions[index].opcode = row_values.front();
        row_values.pop();

        // 避免沒有 operand 的指令造成錯誤, ex RSUB
        if (!row_values.empty())
        {
            instructions[index].operand = row_values.front();
            row_values.pop();
        }

        // 紀錄 END 標誌
        instructions[index].opcode == "END"
            ? instructions[index].is_directive = 1
            : instructions[index].loc_count = location_counter;

        // 如果有label 就寫到 Symbol table
        if (!instructions[index].label.empty())
        {
            symtab[instructions[index].label] = location_counter;
        }

        // 每行指令3bytes
        location_counter += 3;
        index++;
    }

    // Pass1 寫入檔案-加入location counter 的instructions
    ofstream ofs;
    ofs.open("pass1-instructions_with_loc.txt");
    for (int i = 0;; i++)
    {
        ofs << hex << instructions[i].loc_count << "\t";
        ofs << instructions[i].label << "\t";
        ofs << instructions[i].opcode << "\t";
        ofs << instructions[i].operand << endl;

        if (instructions[i].opcode == "END")
        {
            break;
        }
    }
    ofs.close();

    // Pass1 寫入檔案-Symbol Table
    ofs.open("pass1-symbol_table.txt");
    auto iter = symtab.begin();
    while (iter != symtab.end())
    {
        ofs << iter->first << "\t" << iter->second << endl;
        ++iter;
    }
    ofs.close();
}