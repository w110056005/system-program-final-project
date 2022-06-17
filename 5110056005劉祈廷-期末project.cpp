#include <iostream>
#include <fstream>
#include <iterator>
#include <queue>
#include <map>
#include <sstream>
#include <iomanip>
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
    string obj_code = "";
    int is_directive = 0;
};

string ToHexString(int source, int string_length)
{
    ostringstream ss;
    ss << uppercase << setfill('0') << setw(string_length) << hex << source;
    string result = ss.str();
    return result;
}

int main()
{
    map<int, instruction> instructs; // 紀錄instructions
    map<string, string> optab;       // 紀錄OPTAB
    int location_counter = 0x0;      // 紀錄location counter
    map<string, int> symtab;         // 紀錄 symbol table

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

    ////////
    /*
    START Pass 1
    */
    ////////

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

        // 遇到END 就跳過，其他則紀錄location_counter
        if (instructs[location_counter].opcode == "END")
        {
            instructs[location_counter].is_directive = 1;
            break;
        }
        else
            instructs[location_counter].loc_count = location_counter;

        // 如果有label且Symbol table 尚未有此record就寫到 Symbol table
        if (!instructs[location_counter].label.empty() && !symtab.count(instructs[location_counter].label))
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
                // Start with X (X'05')
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

    auto iter = instructs.begin();
    while (iter != instructs.end())
    {
        if (iter->second.loc_count != 0)
            ofs << setw(6) << uppercase << std::left << hex << iter->second.loc_count;
        else
            ofs << setw(6) << std::left << "";
        ofs << setw(8) << iter->second.label;
        ofs << setw(8) << iter->second.opcode;
        ofs << setw(10) << iter->second.operand << endl;
        ++iter;
    }
    ofs.close();

    // Pass1 寫入檔案-Symbol Table
    ofs.open(pass1_symtap_path);
    auto iter_symtab = symtab.begin();
    while (iter_symtab != symtab.end())
    {
        ofs << setw(8) << std::left << iter_symtab->first << uppercase << iter_symtab->second << endl;
        ++iter_symtab;
    }
    ofs.close();

    ////////
    /*
    START Pass 2
    */
    ////////

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
    }

    map<int, string> object_program; // 紀錄object program
    int inst_count = 0;
    int next_line_flag = 0;
    int line = 0;
    int line_start = 0x0;
    int line_end = 0x0;
    int add_line_length = 1;
    string text = "";

    iter = instructs.begin();
    while (iter != instructs.end())
    {
        if (iter->second.is_directive != 1)
        {
            if (add_line_length)
            {
                line_end = iter->second.loc_count;
            }

            if (inst_count == 10 || next_line_flag)
            {
                // object program 一行滿了，寫入並換行
                ostringstream ss;
                ss << "T " << setfill('0') << setw(6) << uppercase << hex << line_start << " " << ToHexString(line_end - line_start, 2) << text;
                object_program[line++] = ss.str();

                // reset 所有flag
                inst_count = 0;
                next_line_flag = 0;
                line_start = iter->second.loc_count;
                text = "";
                add_line_length = 1;
            }

            if (iter->second.opcode == "BYTE")
            {
                // BYTE系列另外處理
                if (iter->second.operand[0] == 'C')
                {
                    // C: 將字串轉成ASCII 16進位
                    string real_operand = iter->second.operand.substr(2, iter->second.operand.size() - 3);
                    for (unsigned i = 0; i < real_operand.size(); i++)
                    {
                        iter->second.obj_code += ToHexString(int(real_operand[i]), 2);
                    }
                }
                else if (iter->second.operand[0] == 'X')
                {
                    // X: 直接輸出文字
                    iter->second.obj_code = iter->second.operand.substr(2, iter->second.operand.size() - 3);
                }
            }
            else if (iter->second.opcode == "RESW" || iter->second.opcode == "RESB")
            {
                // RES 系列不產生 object code，所以不計算line length
                add_line_length = 0;

                if (iter->second.opcode == "RESB")
                    // 遇到 RESB 要換行
                    next_line_flag = 1;
            }
            else if (iter->second.opcode == "WORD")
            {
                // WORD 把後面數值直接轉16進位並補滿6位
                int number = stoi(iter->second.operand);
                iter->second.obj_code = ToHexString(number, 6);
            }
            else if (iter->second.opcode == "RSUB")
            {
                // RSUB 輸出OP code 後面補滿0
                iter->second.obj_code = optab[iter->second.opcode] + "0000";
            }
            else if (iter->second.operand.find(",X") != std::string::npos)
            {
                // Index addressing, add 0x8000讓 X bit == 1
                string real_operand = iter->second.operand.substr(0, iter->second.operand.size() - 2);
                iter->second.obj_code = optab[iter->second.opcode] + ToHexString(symtab[real_operand] + 0x8000, 4);
            }
            else
            {
                iter->second.obj_code = optab[iter->second.opcode] + ToHexString(symtab[iter->second.operand], 4);
            }

            //是有產生 object code的指令，將其記錄至暫存
            if (iter->second.obj_code.size() > 0)
            {
                text += " " + iter->second.obj_code;
                inst_count++;
            }
        }
        else
        {
            if (iter->second.opcode == "START")
            {
                // 寫入 H line
                ostringstream ss;
                ss << "H " << setw(6) << std::left << iter->second.label << " " << ToHexString(iter->second.loc_count, 6) << " " << uppercase << hex << location_counter - instructs[0x0].loc_count;
                object_program[line++] += ss.str();
                line_start = iter->second.loc_count;
            }
            else if (iter->second.opcode == "END")
            {
                // 紀錄最後一組text
                ostringstream ss1;
                ss1 << "T " << setfill('0') << setw(6) << uppercase << hex << line_start << " " << ToHexString(location_counter - line_start, 2) << text;
                object_program[line++] = ss1.str();

                // 寫入 E line
                ostringstream ss2;
                ss2 << "E " << ToHexString(instructs[0x0].loc_count, 6);
                object_program[line] += ss2.str();
            }
        }

        ++iter;
    }

    // Pass2 寫入檔案-加入object code 的instructs
    ofs.open(pass2_instructions_path);
    iter = instructs.begin();
    while (iter != instructs.end())
    {
        if (iter->second.loc_count != 0)
            ofs << setw(6) << uppercase << std::left << hex << iter->second.loc_count;
        else
            ofs << setw(6) << std::left << "";
        ofs << setw(8) << std::right << iter->second.label;
        ofs << setw(8) << iter->second.opcode;
        ofs << setw(10) << iter->second.operand;
        ofs << setw(10) << iter->second.obj_code << endl;
        ++iter;
    }
    ofs.close();

    // Pass2 寫入檔案-object program
    ofs.open(pass2_object_program_path);
    auto iter_obj = object_program.begin();
    while (iter_obj != object_program.end())
    {
        ofs << iter_obj->second << endl;
        ++iter_obj;
    }
    ofs.close();
}