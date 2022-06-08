#include <iostream>
#include <fstream>
#include <iterator>
#include <queue>
#include <map>
#include <sstream>
using namespace std;

string source_path = "source.txt";
string opcode_path = "opcode.txt";
string pass1_instructions_path = "pass1-instructions_list.txt";
string pass1_symtap_path = "pass1-symbol_table.txt";
string pass2_instructions_path = "pass2-instructions_list.txt";
string pass2_object_program_path = "pass2-object_program.txt";

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

struct instruction
{
    int loc_count = 0x0;
    string label;
    string opcode;
    string operand;
    string obj_code;
    int is_directive = 0;
};

int main()
{
    map<int, instruction> instructs;
    map<string, string> optab;
    int location_counter = 0x0;
    map<string, int> symtab;

    string row;
    ifstream file_source(source_path, ifstream::in);

    // 紀錄Start
    getline(file_source, row);
    queue<string> row_values;
    Split(row, '\t', row_values);

    instructs[0x0].label = row_values.front();
    row_values.pop();
    instructs[0x0].opcode = row_values.front();
    row_values.pop();
    instructs[0x0].operand = row_values.front();
    row_values.pop();
    instructs[0x0].is_directive = 1;

    // 取得起始address in hex
    string s = "0x" + instructs[0x0].operand;
    location_counter = std::stoul(s, nullptr, 16);
    instructs[0x0].loc_count = location_counter;

    // pass1 > 計算location counter & Symbol Table
    while (getline(file_source, row))
    {
        row_values = queue<string>(); // reset queue
        Split(row, '\t', row_values);

        instructs[location_counter].label = row_values.front();
        row_values.pop();
        instructs[location_counter].opcode = row_values.front();
        row_values.pop();

        // 避免沒有 operand 的指令造成錯誤, ex RSUB
        if (!row_values.empty())
        {
            instructs[location_counter].operand = row_values.front();
            row_values.pop();
        }

        // 紀錄 END 標誌
        instructs[location_counter].opcode == "END"
            ? instructs[location_counter].is_directive = 1
            : instructs[location_counter].loc_count = location_counter;

        // 如果有label 就寫到 Symbol table
        if (!instructs[location_counter].label.empty())
        {
            symtab[instructs[location_counter].label] = location_counter;
        }

        // 計算下一指令的location counter
        if (instructs[location_counter].opcode == "RESB")
        {
            // RESB 將operand 轉成數字後加入location
            int reserve_bytes = stoi(instructs[location_counter].operand);
            location_counter += reserve_bytes;
        }
        else if (instructs[location_counter].opcode == "BYTE")
        {
            // BYTE 有三種狀況
            if (instructs[location_counter].operand[0] == 'C')
            {
                // Start with C (C'EOF')
                location_counter += instructs[location_counter].operand.length() - 3;
            }
            else if (instructs[location_counter].operand[0] == 'X')
            {
                // Start with X (X'EOF')
                location_counter += (instructs[location_counter].operand.length() - 3) / 2;
            }
            else
                location_counter += 1;
        }
        else
            // 其他指令都+3
            location_counter += 3;
    }

    // Pass1 寫入檔案-加入location counter 的instructs
    ofstream ofs;
    ofs.open(pass1_instructions_path);

    auto iter_inst = instructs.begin();
    while (iter_inst != instructs.end())
    {
        ofs << hex << iter_inst->second.loc_count << "\t";
        ofs << iter_inst->second.label << "\t";
        ofs << iter_inst->second.opcode << "\t";
        ofs << iter_inst->second.operand << endl;
        ++iter_inst;
    }
    ofs.close();

    // Pass1 寫入檔案-Symbol Table
    ofs.open(pass1_symtap_path);
    auto iter_symtab = symtab.begin();
    while (iter_symtab != symtab.end())
    {
        ofs << iter_symtab->first << "\t" << iter_symtab->second << endl;
        ++iter_symtab;
    }
    ofs.close();

    /*
    START Pass 2
    */

    // 讀取 opcode table，先將opcode table建立map
    ifstream file_opcode(opcode_path, ifstream::in);
    while (getline(file_opcode, row))
    {
        row_values = queue<string>(); // reset queue
        Split(row, ' ', row_values);

        string opcode = row_values.front();
        row_values.pop();
        optab[opcode] = row_values.front();
        row_values.pop();
        cout << opcode << " " << optab[opcode] << endl;
    }
}