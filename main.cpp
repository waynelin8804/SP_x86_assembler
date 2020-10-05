// x86 Assembler
// 10627121 Lin, Jun-Wei

#include "sp1.h"
#include "sp2.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <string>

struct pseudoFormat_s {
  int (*pFunc)(int line, tokenLine_t tokenLine, struct pseudoFormat_s *p);
  Str100 tokenName;
};

struct compToken_s {
  int tokenType;
  Str100 tokenName; // "" means don't compare
};

struct instFormat_s {
  int (*pFunc)(int line, tokenLine_t tokenLine, struct instFormat_s *p);
  unsigned char opCode;
  unsigned char reg;
  int tokenCount;
  compToken_s compTokenArray[50];
};

Str100 fileName;
int LocP = 0;
vector<token_s> equMap[100];

int definedSymbolMap[100];
vector<int> usedSymbolMap[100];

symbol_s symAddrTable[100];
vector<int> forwardRefLineVector;

Table is16bitRegMap;
Table is8bitRegMap;
Table isSegmentMap;
Table isPointerRegMap;

#define Is16bitReg(tokenName) (is16bitRegMap.find(tokenName) != is16bitRegMap.end())
#define Is8bitReg(tokenName) (is8bitRegMap.find(tokenName) != is8bitRegMap.end())
#define IsSegment(tokenName) (isSegmentMap.find(tokenName) != isSegmentMap.end())
#define IsPointerReg(tokenName) (isPointerRegMap.find(tokenName) != isPointerRegMap.end())

int StrToInt(Str100 str) {
    int i;

    if (toupper(str[strlen(str) - 1]) == 'H') {
        str[strlen(str) - 1] = '\0';
        sscanf(str, "%x", &i);
    } else
        i = atoi(str);
    return i;
} // StrToInt()

void FillMachineCode(Str100 machineCode, unsigned char *pData, int size, bool isTail16Bit) {
    machineCode[0] = '\0';
    if (isTail16Bit)
        size -= 2;
    for (int j = 0 ; j < size ; j++)
        sprintf(machineCode+strlen(machineCode), "%02X ", pData[j]);

    if (isTail16Bit)
        sprintf(machineCode + strlen(machineCode), "%02X%02X",
            pData[size + 1], pData[size]);
} // FillMachineCode()

int FuncOrg(int line, tokenLine_t tokenLine, struct pseudoFormat_s *p) {
    if (tokenLine[1].type == TYPE_INTEGER) {
        LocP = StrToInt(tokenLine[1].token);
    } else
        sprintf(inOutLine_vector[line].errorMsg, "%s(%d) : Error: '%s' not integer", fileName, line, tokenLine[1].token);
    return 0;
} // FuncOrg()

int FuncByte(int line, tokenLine_t tokenLine, struct pseudoFormat_s *p) {
    int value, machineCodeByteLen = 0;

    for (int i = 1, size = tokenLine.size() ; i < size ; i++) {
        if (tokenLine[i].token[0] == ',' || tokenLine[i].token[0] == '\'')
            continue;
        if (tokenLine[i].type == TYPE_INTEGER) {
            value = StrToInt(tokenLine[i].token);
            sprintf(inOutLine_vector[line].machineCode+strlen(inOutLine_vector[line].machineCode),
                "%02X ", value);
            machineCodeByteLen++;
        } // if
        else if (tokenLine[i].type == TYPE_STRING) {
            for (int j = 0 ; j < strlen(tokenLine[i].token) ; j++) {
                value = tokenLine[i].token[j];
                sprintf(inOutLine_vector[line].machineCode+strlen(inOutLine_vector[line].machineCode),
                    "%02X ", value);
                machineCodeByteLen++;
            } // for
        } else
            sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' not integer or string", tokenLine[i].token);
    } // for
    if (machineCodeByteLen > 0) {
        inOutLine_vector[line].address = LocP;
        LocP += machineCodeByteLen;
    } // if
    return 0;
} // FuncByte()

int FuncWord(int line, tokenLine_t tokenLine, struct pseudoFormat_s *p) {
    int value, machineCodeByteLen = 0;

    for (int i = 1, size = tokenLine.size() ; i < size ; i++) {
        if (tokenLine[i].token[0] == ',' || tokenLine[i].token[0] == '\'')
            continue;
        if (tokenLine[i].type == TYPE_INTEGER) {
            value = StrToInt(tokenLine[i].token);
            if (tokenLine[i].token[0] == '-')
                value = -value;
            if (value >= -128 && value <= 127) {
                sprintf(inOutLine_vector[line].machineCode+strlen(inOutLine_vector[line].machineCode),
                "%02X ", value);
                machineCodeByteLen++;
            } // if
            else if (value >= -32768 && value <= 32767) {
                sprintf(inOutLine_vector[line].machineCode+strlen(inOutLine_vector[line].machineCode),
                "%02X%02X ", (value & 0xFF00) >> 8, value & 0x00FF);
                machineCodeByteLen += 2;
            } // else if
            else {
                sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[i].token);
                return 0;
            } // else
        } // if
        else if (tokenLine[i].type == TYPE_STRING) {
            if (strlen(tokenLine[i].token) == 1) {
                sprintf(inOutLine_vector[line].machineCode+strlen(inOutLine_vector[line].machineCode),
                    "00%02X ", tokenLine[i].token[0]);
                machineCodeByteLen++;
            } else if (strlen(tokenLine[i].token) == 2) {
                sprintf(inOutLine_vector[line].machineCode+strlen(inOutLine_vector[line].machineCode),
                    "%02X%02X ", tokenLine[i].token[0], tokenLine[i].token[1]);
                machineCodeByteLen++;
            } else
                sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[i].token);
        } else
            sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' not integer or string", tokenLine[i].token);
    } // for
    if (machineCodeByteLen > 0) {
        inOutLine_vector[line].address = LocP;
        LocP += machineCodeByteLen;
    } // if
    return 0;
} // FuncWord()

int FuncSaveAddr(int line, tokenLine_t tokenLine, struct pseudoFormat_s *p) {
    inOutLine_vector[line].address = LocP;
    return 0;
} // FuncSaveAddr

pseudoFormat_s pseudoTable[] = {
    { FuncOrg, "ORG" },
    { FuncByte, "BYTE" },
    { FuncByte, "DB" },
    { FuncWord, "WORD" },
    { FuncWord, "DW" },
    { FuncSaveAddr, "CODE" },
    { FuncSaveAddr, "PROC" },
    { FuncSaveAddr, "ENDP" },
};
#define PSEUDO_TABLE_LEN (sizeof(pseudoTable) / sizeof(pseudoTable[0]))

int FuncNoOperand(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    inOutLine_vector[line].address = LocP;
    sprintf(inOutLine_vector[line].machineCode, "%02X ", p->opCode);
    LocP++;
    return 0;
} // FuncNoOperand()

int FuncNoOperand2(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    inOutLine_vector[line].address = LocP;
    sprintf(inOutLine_vector[line].machineCode, "%02X0A ", p->opCode);
    LocP+=2;
    return 0;
} // FuncNoOperand2()

int FuncInt(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    IntMachineCode_s code;

    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    code.number = StrToInt(tokenLine[1].token);
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    LocP += sizeof(code);
    return 0;
} // FuncInt()

int FuncRegToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    d_w_mod_reg_rm_s code;

    // Fill machine code
    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    code.d = 1;
    if (Is16bitReg(tokenLine[1].token) || Is16bitReg(tokenLine[3].token))
        code.w = 1;
    else
        code.w = 0;
    code.mod = 3;
    if (Is16bitReg(tokenLine[1].token))
        code.reg = is16bitRegMap[tokenLine[1].token];
    else
        code.reg = is8bitRegMap[tokenLine[1].token];
    if (Is16bitReg(tokenLine[3].token))
        code.rm = is16bitRegMap[tokenLine[3].token];
    else
        code.rm = is8bitRegMap[tokenLine[3].token];
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    LocP += sizeof(code);
    return 0;
} // FuncRegToReg()

int FuncMovRegToSeg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    mod_reg_rm_s code;

    // Check error
    if (IsSegment(tokenLine[3].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' is a segment", tokenLine[3].token);
        return 0;
    } // if

    // Fill machine code
    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    code.mod = 3;
    code.reg = isSegmentMap[tokenLine[1].token];
    code.rm = is16bitRegMap[tokenLine[3].token];
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    LocP += sizeof(code);
    return 0;
} // FuncMovRegToSeg()

int FuncMovSegToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    mod_reg_rm_s code;

    // Check error
    if (IsSegment(tokenLine[1].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' is a segment", tokenLine[1].token);
        return 0;
    } // if

    // Fill machine code
    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    code.mod = 3;
    code.reg = isSegmentMap[tokenLine[3].token];
    code.rm = is16bitRegMap[tokenLine[1].token];
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    LocP += sizeof(code);
    return 0;
} // FuncMovSegToReg()

int FuncMovImmToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    w_reg_data_s code;
    int value;

    // Check error
    if (tokenLine[1].token[1] == 'S') {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' is a segment", tokenLine[3].token);
        return 0;
    } // if

    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    code.w = Is16bitReg(tokenLine[1].token);
    value = StrToInt(tokenLine[3].token);
    code.dataLow = value & 0x00FF;
    code.dataHigh = (value & 0xFF00) >> 8;
    value = code.w ? sizeof(code) : sizeof(code) - 1;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, value, code.w);
    LocP += value;
    return 0;
}

int FuncMovMemToAcc(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr;
    w_addr_s code;
    Str100 machineCode;

    inOutLine_vector[line].address = LocP;
    code.w = Is16bitReg(tokenLine[1].token);
    code.op = p->opCode;
    symAddr = symAddrTable[tokenLine[5].value].addr;
    if (symAddr != -1) {
        code.addrLow = symAddr & 0x00FF;
        code.addrHigh = (symAddr & 0xFF00) >> 8;
    } // if
    else
        forwardRefLineVector.push_back(line); // 캯짾�歷�

    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), true);
    LocP += 1 + sizeof(code);
    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
}

int FuncMovAccToMem(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr;
    w_addr_s code;
    Str100 machineCode;

    inOutLine_vector[line].address = LocP;
    code.w = Is16bitReg(tokenLine[5].token);
    code.op = p->opCode;
    symAddr = symAddrTable[tokenLine[3].value].addr;
    if (symAddr != -1) {
        code.addrLow = symAddr & 0x00FF;
        code.addrHigh = (symAddr & 0xFF00) >> 8;
    } // if
    else
        forwardRefLineVector.push_back(line); // 캯짾�歷�

    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), true);
    LocP += 1 + sizeof(code);
    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
}

int FuncMovMemToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr, value;
    Str100 machineCode;
    d_w_mod_reg_rm_s code;

    // Check error
    if (strcmp(tokenLine[3].token, "WORD") == 0 && !Is16bitReg(tokenLine[1].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' not WORD register", tokenLine[1].token);
        return 0;
    }
    if (strcmp(tokenLine[3].token, "BYTE") == 0 && !Is8bitReg(tokenLine[1].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' not BYTE register", tokenLine[1].token);
        return 0;
    }

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    code.rm = 6;
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    code.mod = 0;
    symAddr = symAddrTable[tokenLine[5].value].addr;
    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), false);
    if (symAddr == -1)
        forwardRefLineVector.push_back(line); // 캯짾�歷�
    else
        FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&symAddr, 2, true);
    inOutLine_vector[line].address = LocP;
    LocP += 1 + sizeof(code) + 2;

    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
} // FuncMovMemToReg()

int FuncMovMemToReg2(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr, value;
    Str100 machineCode;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    code.rm = 6;
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    code.mod = 0;
    symAddr = symAddrTable[tokenLine[4].value].addr;
    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), false);
    if (symAddr == -1)
        forwardRefLineVector.push_back(line); // 캯짾�歷�
    else
        FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&symAddr, 2, true);
    inOutLine_vector[line].address = LocP;
    LocP += 1 + sizeof(code) + 2;

    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
} // FuncMovMemToReg2()

int FuncADCOffsetMemToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int disp, value;
    Str100 machineCode, str;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    sprintf(str, "%s+%s", tokenLine[4].token, tokenLine[7].token);
    if (!IsPointerReg(str)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", str);
        return 0;
    } // if
    code.rm = StrToInt(str);
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    disp = StrToInt(tokenLine[10].token);
    if (disp >= -128 && disp <= 127)
        code.mod = 1;
    else if (disp >= -32768 && disp <= 32767)
        code.mod = 2;
    else {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[10].token);
        return 0;
    } // else
    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), false);
    FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&disp, code.mod, (code.mod == 2));
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code) + code.mod;
    strcpy(inOutLine_vector[line].machineCode, machineCode);
    return 0;
} // FuncADCOffsetMemToReg()

int FuncADCMemToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    Str100 str;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    sprintf(str, "%s+%s", tokenLine[4].token, tokenLine[7].token);
    if (!IsPointerReg(str)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", str);
        return 0;
    } // if
    code.rm = StrToInt(str);
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    code.mod = 0;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncADCMemToReg()

int FuncImmToAcc(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int data, value;
    w_data_s code;

    code.op = p->opCode;
    code.w = (tokenLine[1].token[1] == 'X');
    data = StrToInt(tokenLine[3].token);
    if (data >= -128 && data <= 127)
        value = 2;
    else if ( code.w == 1 && (data >= -32768 && data <= 32767))
        value = 3;
    else {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[3].token);
        return 0;
    } // else
    code.dataLow = data & 0x00FF;
    code.dataHigh = (data & 0xFF00) >> 8;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, value, (value == 3));
    inOutLine_vector[line].address = LocP;
    LocP += value;
    return 0;
} // FuncImmToAcc()

int FuncOrImmToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value, data;
    d_w_mod_reg_rm_data_s code;

    code.op = p->opCode;
    code.d = 1;
    code.w = (tokenLine[1].token[1] == 'X');
    code.mod = 3;
    code.reg = p->reg;
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.rm = value;
    data = StrToInt(tokenLine[3].token);
    if (data >= -128 && data <= 127)
        value = 3;
    else if ( code.w == 1 && (data >= -32768 && data <= 32767))
        value = 4;
    else {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[3].token);
        return 0;
    } // else
    code.dataLow = data & 0x00FF;
    code.dataHigh = (data & 0xFF00) >> 8;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, value, (value == 4));
    inOutLine_vector[line].address = LocP;
    LocP += value;
    return 0;
} // FuncOrImmToReg()

int FuncMovRegToMem(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr, value;
    Str100 machineCode;
    d_w_mod_reg_rm_s code;

    // Check error
    if (strcmp(tokenLine[1].token, "WORD") == 0 && !Is16bitReg(tokenLine[5].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' not WORD register", tokenLine[5].token);
        return 0;
    }
    if (strcmp(tokenLine[1].token, "BYTE") == 0 && !Is8bitReg(tokenLine[5].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' not BYTE register", tokenLine[5].token);
        return 0;
    }

    // Fill code
    code.w = Is16bitReg(tokenLine[5].token);
    code.d = 0;
    code.op = p->opCode;
    code.rm = 6;
    if (Is16bitReg(tokenLine[5].token))
        value = is16bitRegMap[tokenLine[5].token];
    else
        value = is8bitRegMap[tokenLine[5].token];
    code.reg = value;
    code.mod = 0;
    symAddr = symAddrTable[tokenLine[3].value].addr;
    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), false);
    if (symAddr == -1)
        forwardRefLineVector.push_back(line); // 캯짾�歷�
    else
        FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&symAddr, 2, true);
    inOutLine_vector[line].address = LocP;
    LocP += 1 + sizeof(code) + 2;

    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
}

int Func1A(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    Str100 str;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    sprintf(str, "%s+%s", tokenLine[4].token, tokenLine[6].token);
    if (!IsPointerReg(str)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", str);
        return 0;
    } // if
    code.rm = isPointerRegMap[str];
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    code.mod = 0;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
}

int Func1B(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    Str100 str;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[7].token);
    code.d = 0;
    code.op = p->opCode;
    sprintf(str, "%s+%s", tokenLine[2].token, tokenLine[4].token);
    if (!IsPointerReg(str)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", str);
        return 0;
    } // if
    code.rm = isPointerRegMap[str];
    if (Is16bitReg(tokenLine[7].token))
        value = is16bitRegMap[tokenLine[7].token];
    else
        value = is8bitRegMap[tokenLine[7].token];
    code.reg = value;
    code.mod = 0;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
}

int Func2A(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    if (!IsPointerReg(tokenLine[4].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", tokenLine[4].token);
        return 0;
    } // if
    code.rm = isPointerRegMap[tokenLine[4].token];
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    code.mod = 0;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // Func2A()

int Func2B(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[5].token);
    code.d = 0;
    code.op = p->opCode;
    if (!IsPointerReg(tokenLine[2].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", tokenLine[2].token);
        return 0;
    } // if
    code.rm = isPointerRegMap[tokenLine[2].token];
    if (Is16bitReg(tokenLine[5].token))
        value = is16bitRegMap[tokenLine[5].token];
    else
        value = is8bitRegMap[tokenLine[5].token];
    code.reg = value;
    code.mod = 0;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
}

int Func3A(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int disp, value;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    if (!IsPointerReg(tokenLine[4].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", tokenLine[4].token);
        return 0;
    } // if
    code.rm = isPointerRegMap[tokenLine[4].token];
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    disp = StrToInt(tokenLine[6].token);
    if (tokenLine[5].token[0] == '-')
        disp = -disp;
    if (disp >= -128 && disp <= 127)
        code.mod = 1;
    else if (disp >= -32768 && disp <= 32767)
        code.mod = 2;
    else {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[6].token);
        return 0;
    } // else
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    FillMachineCode(inOutLine_vector[line].machineCode + strlen(inOutLine_vector[line].machineCode), (unsigned char *)&disp, code.mod, (code.mod == 2));
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code) + code.mod;
    return 0;
}

int Func3B(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int disp, value;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[7].token);
    code.d = 0;
    code.op = p->opCode;
    if (!IsPointerReg(tokenLine[2].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", tokenLine[2].token);
        return 0;
    } // if
    code.rm = isPointerRegMap[tokenLine[2].token];
    if (Is16bitReg(tokenLine[7].token))
        value = is16bitRegMap[tokenLine[7].token];
    else
        value = is8bitRegMap[tokenLine[7].token];
    code.reg = value;
    disp = StrToInt(tokenLine[4].token);
    if (tokenLine[3].token[0] == '-')
        disp = -disp;
    if (disp >= -128 && disp <= 127)
        code.mod = 1;
    else if (disp >= -32768 && disp <= 32767)
        code.mod = 2;
    else {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[4].token);
        return 0;
    } // else
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    FillMachineCode(inOutLine_vector[line].machineCode + strlen(inOutLine_vector[line].machineCode), (unsigned char *)&disp, code.mod, (code.mod == 2));
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code) + code.mod;
    return 0;
}

int Func4A(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int disp, value;
    Str100 str;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    sprintf(str, "%s+%s", tokenLine[4].token, tokenLine[6].token);
    if (!IsPointerReg(str)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", str);
        return 0;
    } // if
    code.rm = isPointerRegMap[str];
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    disp = StrToInt(tokenLine[8].token);
    if (tokenLine[7].token[0] == '-')
        disp = -disp;
    if (disp >= -128 && disp <= 127)
        code.mod = 1;
    else if (disp >= -32768 && disp <= 32767)
        code.mod = 2;
    else {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[8].token);
        return 0;
    } // else
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    FillMachineCode(inOutLine_vector[line].machineCode + strlen(inOutLine_vector[line].machineCode), (unsigned char *)&disp, code.mod, (code.mod == 2));
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code) + code.mod;
    return 0;
} // Func4A()

int Func4B(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int disp, value;
    Str100 str;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[9].token);
    code.d = 0;
    code.op = p->opCode;
    sprintf(str, "%s+%s", tokenLine[2].token, tokenLine[4].token);
    if (!IsPointerReg(tokenLine[2].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", str);
        return 0;
    } // if
    code.rm = isPointerRegMap[str];
    if (Is16bitReg(tokenLine[9].token))
        value = is16bitRegMap[tokenLine[9].token];
    else
        value = is8bitRegMap[tokenLine[9].token];
    code.reg = value;
    disp = StrToInt(tokenLine[6].token);
    if (tokenLine[5].token[0] == '-')
        disp = -disp;
    if (disp >= -128 && disp <= 127)
        code.mod = 1;
    else if (disp >= -32768 && disp <= 32767)
        code.mod = 2;
    else {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[6].token);
        return 0;
    } // else
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    FillMachineCode(inOutLine_vector[line].machineCode + strlen(inOutLine_vector[line].machineCode), (unsigned char *)&disp, code.mod, (code.mod == 2));
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code) + code.mod;
    return 0;
} // Func4B()

int FuncAccIn(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    op_port_s code;

    code.op = p->opCode;
    value = StrToInt(tokenLine[3].token);
    if (value < -128 || value > 127) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[3].token);
        return 0;
    } // if
    code.port = value;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncAccIn()

int FuncAccOut(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    op_port_s code;

    code.op = p->opCode;
    value = StrToInt(tokenLine[1].token);
    if (value < -128 || value > 127) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' constant value too large", tokenLine[1].token);
        return 0;
    } // if
    code.port = value;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncAccOut()

int FuncRepScas(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    op1_op2_s code;
    Str100 machineCode;

    code.op1 = p->opCode;
    code.op2 = p->reg;
    FillMachineCode(machineCode, (unsigned char *)&code.op1, sizeof(code.op1), false);
    strcat(machineCode, "/");
    FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&code.op2, sizeof(code.op2), false);
    strcpy(inOutLine_vector[line].machineCode, machineCode);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncRepScas()

int FuncPushMem(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr;
    Str100 machineCode;
    mod_reg_rm_s code;

    // Fill code
    code.op = p->opCode;
    code.rm = 6;
    code.reg = p->reg;
    code.mod = 0;
    symAddr = symAddrTable[tokenLine[1].value].addr;
    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), false);
    if (symAddr == -1)
        forwardRefLineVector.push_back(line); // 캯짾�歷�
    else
        FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&symAddr, 2, true);
    inOutLine_vector[line].address = LocP;
    LocP += 1 + sizeof(code) + 2;

    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
} // FuncPushMem()

int FuncPopReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    op_reg_s code;

    // Fill code
    code.op = p->opCode;
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncPopReg()

int FuncNOT(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 1;
    code.op = p->opCode;
    code.reg = p->reg;
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.rm = value;
    code.mod = 3;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncNOT()

int FuncShiftOrRotate(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.w = Is16bitReg(tokenLine[1].token);
    code.d = 0;
    code.op = p->opCode;
    code.reg = p->reg;
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.rm = value;
    code.mod = 3;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncShiftOrRotate()

int FuncIncMem(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr;
    Str100 machineCode;
    d_w_mod_reg_rm_s code;

    // Fill code
    code.op = p->opCode;
    code.d = 1;
    code.rm = 6;
    code.reg = p->reg;
    code.mod = 0;
    symAddr = symAddrTable[tokenLine[1].value].addr;
    if (symAddr == -1)
        forwardRefLineVector.push_back(line); // 캯짾�歷�
    else {
        code.w = symAddrTable[tokenLine[1].value].isWord;
        FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), false);
        FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&symAddr, 2, true);
    } // else
    inOutLine_vector[line].address = LocP;
    LocP += 1 + sizeof(code) + 2;

    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
} // FuncIncMem()

int FuncLoadToReg(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int value;
    mod_reg_rm_s code;

    // Fill code
    code.op = p->opCode;
    if (!IsPointerReg(tokenLine[4].token)) {
        sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' syntax error", tokenLine[4].token);
        return 0;
    } // if
    code.rm = isPointerRegMap[tokenLine[4].token];
    if (Is16bitReg(tokenLine[1].token))
        value = is16bitRegMap[tokenLine[1].token];
    else
        value = is8bitRegMap[tokenLine[1].token];
    code.reg = value;
    code.mod = 0;
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    inOutLine_vector[line].address = LocP;
    LocP += sizeof(code);
    return 0;
} // FuncLoadToReg()

int FuncTest(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    d_w_mod_reg_rm_s code;

    // Fill machine code
    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    code.d = 0;
    if (Is16bitReg(tokenLine[1].token) || Is16bitReg(tokenLine[3].token))
        code.w = 0;
    code.mod = 3;
    if (Is16bitReg(tokenLine[1].token))
        code.reg = is16bitRegMap[tokenLine[1].token];
    else
        code.reg = is8bitRegMap[tokenLine[1].token];
    if (Is16bitReg(tokenLine[3].token))
        code.rm = is16bitRegMap[tokenLine[3].token];
    else
        code.rm = is8bitRegMap[tokenLine[3].token];
    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    LocP += sizeof(code);
    return 0;
} // FuncTest()

int FuncDirectCall(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    displacement_s code;
    int symAddr;

    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    symAddr = symAddrTable[tokenLine[1].value].addr;
    if (symAddr != -1) {
        symAddr = symAddr - LocP - 3;
        code.displacementLow = symAddr & 0x00FF;
        code.displacementHigh = (symAddr & 0xFF00) >> 8;
    } // if
    else
        forwardRefLineVector.push_back(line); // 캯짾�歷�

    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), true);
    LocP += sizeof(code);

    return 0;
} // FuncDirectCall()

int FuncInDirectCall(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    int symAddr;
    Str100 machineCode;
    mod_reg_rm_s code;

    // Fill code
    code.op = p->opCode;
    code.rm = 6;
    code.reg = p->reg;
    code.mod = 0;
    symAddr = symAddrTable[tokenLine[1].value].addr;
    FillMachineCode(machineCode, (unsigned char *)&code, sizeof(code), false);
    if (symAddr == -1)
        forwardRefLineVector.push_back(line); // 캯짾�歷�
    else
        FillMachineCode(machineCode + strlen(machineCode), (unsigned char *)&symAddr, 2, true);
    inOutLine_vector[line].address = LocP;
    LocP += 1 + sizeof(code) + 2;

    sprintf(inOutLine_vector[line].machineCode, "2E: %s R", machineCode);
    return 0;
} // FuncInDirectCall()

int FuncCondJmp(int line, tokenLine_t tokenLine, struct instFormat_s *p) {
    condition_jmp_s code;
    int symAddr;

    inOutLine_vector[line].address = LocP;
    code.op = p->opCode;
    symAddr = symAddrTable[tokenLine[1].value].addr;
    if (symAddr != -1) {
        symAddr = symAddr - LocP - 2;
        code.displacement = symAddr;
    } // if
    else
        forwardRefLineVector.push_back(line); // 캯짾�歷�

    FillMachineCode(inOutLine_vector[line].machineCode, (unsigned char *)&code, sizeof(code), false);
    LocP += sizeof(code);

    return 0;
} // FuncCondJmp()

instFormat_s instTable[] = {
    { FuncNoOperand2, 0xD5, 0x00, 1, { {TYPE_TABLE1,"AAD"} } },
    { FuncNoOperand2, 0xD4, 0x00, 1, { {TYPE_TABLE1,"AAM"} } },
    { FuncNoOperand, 0x37, 0x00, 1, { {TYPE_TABLE1,"AAA"} } },
    { FuncNoOperand, 0x3F, 0x00, 1, { {TYPE_TABLE1,"AAS"} } },
    { FuncNoOperand, 0x98, 0x00, 1, { {TYPE_TABLE1,"CBW"} } },
    { FuncNoOperand, 0xF8, 0x00, 1, { {TYPE_TABLE1,"CLC"} } },
    { FuncNoOperand, 0xFC, 0x00, 1, { {TYPE_TABLE1,"CLD"} } },
    { FuncNoOperand, 0xFA, 0x00, 1, { {TYPE_TABLE1,"CLI"} } },
    { FuncNoOperand, 0xF5, 0x00, 1, { {TYPE_TABLE1,"CMC"} } },
    { FuncNoOperand, 0xA6, 0x00, 1, { {TYPE_TABLE1,"CMPSB"} } },
    { FuncNoOperand, 0xA7, 0x00, 1, { {TYPE_TABLE1,"CMPSW"} } },
    { FuncNoOperand, 0x99, 0x00, 1, { {TYPE_TABLE1,"CWD"} } },
    { FuncNoOperand, 0x27, 0x00, 1, { {TYPE_TABLE1,"DAA"} } },
    { FuncNoOperand, 0x2F, 0x00, 1, { {TYPE_TABLE1,"DAS"} } },
    { FuncNoOperand, 0x9B, 0x00, 1, { {TYPE_TABLE1,"FWAIT"} } },
    { FuncNoOperand, 0xF4, 0x00, 1, { {TYPE_TABLE1,"HLT"} } },
    { FuncNoOperand, 0xCE, 0x00, 1, { {TYPE_TABLE1,"INTO"} } },
    { FuncNoOperand, 0xCF, 0x00, 1, { {TYPE_TABLE1,"IRET"} } },
    { FuncNoOperand, 0x9F, 0x00, 1, { {TYPE_TABLE1,"LAHF"} } },
    { FuncNoOperand, 0xAC, 0x00, 1, { {TYPE_TABLE1,"LODSB"} } },
    { FuncNoOperand, 0xAD, 0x00, 1, { {TYPE_TABLE1,"LODSW"} } },
    { FuncNoOperand, 0xA4, 0x00, 1, { {TYPE_TABLE1,"MOVSB"} } },
    { FuncNoOperand, 0xA5, 0x00, 1, { {TYPE_TABLE1,"MOVSW"} } },
    { FuncNoOperand, 0x90, 0x00, 1, { {TYPE_TABLE1,"NOP"} } },
    { FuncNoOperand, 0x9D, 0x00, 1, { {TYPE_TABLE1,"POPF"} } },
    { FuncNoOperand, 0x9C, 0x00, 1, { {TYPE_TABLE1,"PUSHF"} } },
    { FuncNoOperand, 0x9E, 0x00, 1, { {TYPE_TABLE1,"SAHF"} } },
    { FuncNoOperand, 0xAA, 0x00, 1, { {TYPE_TABLE1,"STOSB"} } },
    { FuncNoOperand, 0xAB, 0x00, 1, { {TYPE_TABLE1,"STOSW"} } },
    { FuncNoOperand, 0xF9, 0x00, 1, { {TYPE_TABLE1,"STC"} } },
    { FuncNoOperand, 0xFD, 0x00, 1, { {TYPE_TABLE1,"STD"} } },
    { FuncNoOperand, 0xFB, 0x00, 1, { {TYPE_TABLE1,"STI"} } },
    { FuncNoOperand, 0x9B, 0x00, 1, { {TYPE_TABLE1,"WAIT"} } },
    { FuncNoOperand, 0xD7, 0x00, 1, { {TYPE_TABLE1,"XLAT"} } },
    { FuncNoOperand, 0xC3, 0x00, 1, { {TYPE_TABLE1,"RET"} } },
    { FuncNoOperand, 0xCB, 0x00, 1, { {TYPE_TABLE1,"RETF"} } },

    { FuncAccIn, 0xE4, 0x00, 4, { {TYPE_TABLE1,"IN"}, {TYPE_TABLE3,"AL"}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncAccIn, 0xE5, 0x00, 4, { {TYPE_TABLE1,"IN"}, {TYPE_TABLE3,"AX"}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncAccOut, 0xE6, 0x00, 4, { {TYPE_TABLE1,"OUT"}, {TYPE_INTEGER,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"AL"} } },
    { FuncAccOut, 0xE7, 0x00, 4, { {TYPE_TABLE1,"OUT"}, {TYPE_INTEGER,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"AX"} } },
    { FuncRepScas, 0xF3, 0xAE, 2, { {TYPE_TABLE1,"REP"}, {TYPE_TABLE1,"SCASB"} } },
    { FuncRepScas, 0xF3, 0xAF, 2, { {TYPE_TABLE1,"REPE"}, {TYPE_TABLE1,"SCASW"} } },
    { FuncRepScas, 0xF2, 0xAE, 2, { {TYPE_TABLE1,"REPNE"}, {TYPE_TABLE1,"SCASB"} } },
    { FuncRepScas, 0xF2, 0xAF, 2, { {TYPE_TABLE1,"REPNZ"}, {TYPE_TABLE1,"SCASW"} } },
    { FuncRepScas, 0xF3, 0xAE, 2, { {TYPE_TABLE1,"REPZ"}, {TYPE_TABLE1,"SCASB"} } },
    { FuncInt, 0xCD, 0x00, 2, { {TYPE_TABLE1,"INT"}, {TYPE_INTEGER,""} } },
    { FuncPushMem, 0xFF, 0x06, 2, { {TYPE_TABLE1,"PUSH"}, {TYPE_SYMBOL,""} } },
    { FuncPopReg, 0x0B, 0x00, 2, { {TYPE_TABLE1,"POP"}, {TYPE_TABLE3,""} } },

    // Arithmetic
    { FuncIncMem, 0x3F, 0x00, 2, { {TYPE_TABLE1,"INC"}, {TYPE_SYMBOL,""} } },
    { FuncIncMem, 0x3F, 0x01, 2, { {TYPE_TABLE1,"DEC"}, {TYPE_SYMBOL,""} } },
    { FuncNOT, 0x3D, 0x03, 2, { {TYPE_TABLE1,"NEG"}, {TYPE_TABLE3,""} } },
    { FuncNOT, 0x3D, 0x04, 2, { {TYPE_TABLE1,"MUL"}, {TYPE_TABLE3,""} } },
    { FuncNOT, 0x3D, 0x05, 2, { {TYPE_TABLE1,"IMUL"}, {TYPE_TABLE3,""} } },
    { FuncNOT, 0x3D, 0x06, 2, { {TYPE_TABLE1,"DIV"}, {TYPE_TABLE3,""} } },
    { FuncNOT, 0x3D, 0x07, 2, { {TYPE_TABLE1,"IDIV"}, {TYPE_TABLE3,""} } },

    // Logic
    { FuncADCOffsetMemToReg, 0x04, 0x00, 11, { {TYPE_TABLE1,"ADC"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,"+"}, {TYPE_INTEGER,""} } },
    { Func2A, 0x00, 0x00, 6, { {TYPE_TABLE1,"ADD"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,"+"}, {TYPE_INTEGER,""} } },
    { FuncImmToAcc, 0x12, 0x00, 4, { {TYPE_TABLE1,"AND"}, {TYPE_TABLE3,"AX"}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncImmToAcc, 0x12, 0x00, 4, { {TYPE_TABLE1,"AND"}, {TYPE_TABLE3,"AL"}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncOrImmToReg, 0x20, 0x01, 4, { {TYPE_TABLE1,"OR"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncOrImmToReg, 0x20, 0x06, 4, { {TYPE_TABLE1,"XOR"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncImmToAcc, 0x0E, 0x00, 4, { {TYPE_TABLE1,"SBB"}, {TYPE_TABLE3,"AX"}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncImmToAcc, 0x0E, 0x00, 4, { {TYPE_TABLE1,"SBB"}, {TYPE_TABLE3,"AL"}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncADCMemToReg, 0x0A, 0x00, 9, { {TYPE_TABLE1,"SUB"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"} } },
    { FuncRegToReg, 0x0E, 0x00, 4, { {TYPE_TABLE1,"CMP"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncNOT, 0x3D, 0x02, 2, { {TYPE_TABLE1,"NOT"}, {TYPE_TABLE3,""} } },
    { FuncShiftOrRotate, 0x34, 0x00, 4, { {TYPE_TABLE1,"ROL"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncShiftOrRotate, 0x34, 0x01, 4, { {TYPE_TABLE1,"ROR"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncShiftOrRotate, 0x34, 0x02, 4, { {TYPE_TABLE1,"RCL"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncShiftOrRotate, 0x34, 0x03, 4, { {TYPE_TABLE1,"RCR"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncShiftOrRotate, 0x34, 0x04, 4, { {TYPE_TABLE1,"SHL"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncShiftOrRotate, 0x34, 0x05, 4, { {TYPE_TABLE1,"SHR"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncShiftOrRotate, 0x34, 0x07, 4, { {TYPE_TABLE1,"SAR"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncRegToReg, 0x21, 0x00, 4, { {TYPE_TABLE1,"XCHG"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncTest, 0x21, 0x00, 4, { {TYPE_TABLE1,"TEST"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },

    { FuncMovRegToSeg, 0x8E, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,"ES"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncMovRegToSeg, 0x8E, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,"CS"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncMovRegToSeg, 0x8E, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,"SS"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncMovRegToSeg, 0x8E, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,"DS"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncMovSegToReg, 0x8C, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"ES"} } },
    { FuncMovSegToReg, 0x8C, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"CS"} } },
    { FuncMovSegToReg, 0x8C, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"SS"} } },
    { FuncMovSegToReg, 0x8C, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"DS"} } },
    { FuncMovImmToReg, 0x0B, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_INTEGER,""} } },
    { FuncMovMemToAcc, 0x50, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,"AX"}, {TYPE_TABLE4,","}, {TYPE_TABLE2,"WORD"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""} } },
    { FuncMovMemToAcc, 0x50, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,"AL"}, {TYPE_TABLE4,","}, {TYPE_TABLE2,"BYTE"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""} } },
    { FuncMovAccToMem, 0x51, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE2,"WORD"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"AX"} } },
    { FuncMovAccToMem, 0x51, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE2,"BYTE"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,"AL"} } },
    { FuncMovMemToReg, 0x22, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE2,"WORD"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""} } },
    { FuncMovMemToReg, 0x22, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE2,"BYTE"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""} } },
    { FuncMovMemToReg2, 0x22, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_SYMBOL,""}, {TYPE_TABLE4,"]"} } },
    { FuncMovRegToMem, 0x22, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE2,"WORD"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncMovRegToMem, 0x22, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE2,"BYTE"}, {TYPE_TABLE2,"PTR"}, {TYPE_SYMBOL,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { Func1A, 0x22, 0x00, 8, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"+"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"} } },
    { Func1B, 0x22, 0x00, 8, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"+"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { Func2A, 0x22, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"} } },
    { Func2B, 0x22, 0x00, 6, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { Func3A, 0x22, 0x00, 8, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,""}, {TYPE_INTEGER,""}, {TYPE_TABLE4,"]"} } },
    { Func3B, 0x22, 0x00, 8, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,""}, {TYPE_INTEGER,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { Func4A, 0x22, 0x00, 10, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"+"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,""}, {TYPE_INTEGER,""}, {TYPE_TABLE4,"]"} } },
    { Func4B, 0x22, 0x00, 10, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"+"},{TYPE_TABLE3,""}, {TYPE_TABLE4,""}, {TYPE_INTEGER,""}, {TYPE_TABLE4,"]"}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },
    { FuncRegToReg, 0x22, 0x00, 4, { {TYPE_TABLE1,"MOV"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE3,""} } },

    { FuncLoadToReg, 0x8D, 0x00, 6, { {TYPE_TABLE1,"LEA"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"} } },
    { FuncLoadToReg, 0xC4, 0x00, 6, { {TYPE_TABLE1,"LES"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"} } },
    { FuncLoadToReg, 0xC5, 0x00, 6, { {TYPE_TABLE1,"LDS"}, {TYPE_TABLE3,""}, {TYPE_TABLE4,","}, {TYPE_TABLE4,"["}, {TYPE_TABLE3,""}, {TYPE_TABLE4,"]"} } },

    // Control Transfer
    { FuncDirectCall, 0xE8, 0x00, 2, { {TYPE_TABLE1,"CALL"}, {TYPE_SYMBOL,""} } },
    { FuncInDirectCall, 0xFF, 0x02, 4, { {TYPE_TABLE1,"CALL"}, {TYPE_TABLE4,"["}, {TYPE_SYMBOL,""}, {TYPE_TABLE4,"]"} } },
    { FuncCondJmp, 0xEB, 0x00, 2, { {TYPE_TABLE1,"JMP"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x77, 0x00, 2, { {TYPE_TABLE1,"JA"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x73, 0x00, 2, { {TYPE_TABLE1,"JAE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x72, 0x00, 2, { {TYPE_TABLE1,"JB"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x76, 0x00, 2, { {TYPE_TABLE1,"JBE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x72, 0x00, 2, { {TYPE_TABLE1,"JC"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0xE3, 0x00, 2, { {TYPE_TABLE1,"JCXZ"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x74, 0x00, 2, { {TYPE_TABLE1,"JE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7F, 0x00, 2, { {TYPE_TABLE1,"JG"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7D, 0x00, 2, { {TYPE_TABLE1,"JGE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7C, 0x00, 2, { {TYPE_TABLE1,"JL"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7E, 0x00, 2, { {TYPE_TABLE1,"JLE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x76, 0x00, 2, { {TYPE_TABLE1,"JNA"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x72, 0x00, 2, { {TYPE_TABLE1,"JNAE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x73, 0x00, 2, { {TYPE_TABLE1,"JNB"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x77, 0x00, 2, { {TYPE_TABLE1,"JNBE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x73, 0x00, 2, { {TYPE_TABLE1,"JNC"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x75, 0x00, 2, { {TYPE_TABLE1,"JNE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7E, 0x00, 2, { {TYPE_TABLE1,"JNG"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7C, 0x00, 2, { {TYPE_TABLE1,"JNGE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7D, 0x00, 2, { {TYPE_TABLE1,"JNL"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7F, 0x00, 2, { {TYPE_TABLE1,"JNLE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x71, 0x00, 2, { {TYPE_TABLE1,"JNO"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7B, 0x00, 2, { {TYPE_TABLE1,"JNP"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x79, 0x00, 2, { {TYPE_TABLE1,"JNS"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x75, 0x00, 2, { {TYPE_TABLE1,"JNZ"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x70, 0x00, 2, { {TYPE_TABLE1,"JO"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7A, 0x00, 2, { {TYPE_TABLE1,"JP"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7A, 0x00, 2, { {TYPE_TABLE1,"JPE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x7B, 0x00, 2, { {TYPE_TABLE1,"JPO"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x78, 0x00, 2, { {TYPE_TABLE1,"JS"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0x74, 0x00, 2, { {TYPE_TABLE1,"JZ"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0xE2, 0x00, 2, { {TYPE_TABLE1,"LOOP"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0xE1, 0x00, 2, { {TYPE_TABLE1,"LOOPE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0xE0, 0x00, 2, { {TYPE_TABLE1,"LOOPNE"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0xE0, 0x00, 2, { {TYPE_TABLE1,"LOOPNZ"}, {TYPE_SYMBOL,""} } },
    { FuncCondJmp, 0xE1, 0x00, 2, { {TYPE_TABLE1,"LOOPZ"}, {TYPE_SYMBOL,""} } },
};
#define INST_TABLE_LEN (sizeof(instTable) / sizeof(instTable[0]))

//============================================================================
// I/O functions
//============================================================================

// Get input file name
bool GetInputFileName(Str100 fileName) {
    struct stat st;

    while (true) {
        printf("Please enter file name(0:Quit): ");
        fgets(fileName, 100, stdin);
        if (fileName[strlen(fileName) - 1] == '\n')
            fileName[strlen(fileName) - 1] = '\0';

        if (strcmp(fileName, "0") == 0)
            return false;
        if (stat(fileName, &st) == 0)
            break;

        printf("Error: File '%s' does not exist!\n", fileName);
    } // while

    return true;
} // GetInputFileName()

// Replace substring
void ReplaceStr(Str100 source, Str100 find, Str100 replace) {
    Str100 retStr = "";
    char * p = strstr(source, find);

    // Does not found
    if (p == NULL) {
        sprintf(retStr, "Output_%s", source);
        strcpy(source, retStr);
        return;
    } // if

    strncpy(retStr, source, p - source); // String before find
    strcpy(retStr + (p - source), replace);

    // String after find
    p += strlen(find);
    strcat(retStr, p);

    strcpy(source, retStr);
} // ReplaceStr()

void WriteFile(Str100 fileName) {
    FILE *outputFP = fopen(fileName, "wt");
    if (outputFP == NULL) {
        printf("Error: Cannot open output file '%s'\n", fileName);
        return;
    } // if

    // Write vector data to file
    for (int i = 0, size1 = inOutLine_vector.size(); i < size1; i++) {
        if (inOutLine_vector[i].address == -1)
            fprintf(outputFP, "      ");
        else
            fprintf(outputFP, " %04X ", inOutLine_vector[i].address);
        fprintf(outputFP, "\t%-18s \n", inOutLine_vector[i].machineCode);
        // fprintf(outputFP, "%s\n", inOutLine_vector[i].srcLine);
        if (inOutLine_vector[i].errorMsg[0] != '\0')
            fprintf(outputFP, "%s\n", inOutLine_vector[i].errorMsg);
    } // for

    fclose(outputFP);
} // WriteFile()

//============================================================================
// Code generation functions
//============================================================================
void ReplaceEQU() {
    tokenLine_t tokenLine;
    int value;

    // Create EQU map
    for (int line = 0, LineSize = inOutLine_vector.size() ; line < LineSize ; line++) {
        tokenLine = inOutLine_vector[line].tokenLine;
        if (tokenLine.size() >= 2 && (tokenLine[0].type == TYPE_SYMBOL) && strcmp(tokenLine[1].token, "EQU") == 0)
            equMap[tokenLine[0].value].assign(tokenLine.begin()+2, tokenLine.end());
    } // for define

    // Replace EQU usage
    for (int line = 0, LineSize = inOutLine_vector.size() ; line < LineSize ; line++) {
        tokenLine = inOutLine_vector[line].tokenLine;
        if (tokenLine.size() >= 2 && (tokenLine[0].type == TYPE_SYMBOL) && strcmp(tokenLine[1].token, "EQU") == 0)
            continue;
        for (int index = 0, size = tokenLine.size() ; index < size ; index++) {
            if (tokenLine[index].type == TYPE_SYMBOL &&
                !equMap[tokenLine[index].value].empty()) {
                value = tokenLine[index].value;
                tokenLine.erase(tokenLine.begin()+index);
                tokenLine.insert(tokenLine.begin()+index,
                    equMap[value].begin(),
                    equMap[value].end());
                size = tokenLine.size();
            } // if
        } // for
        inOutLine_vector[line].tokenLine.assign(tokenLine.begin(), tokenLine.end());
    } // for define
} // ReplaceEQU()

void CheckSymbol() {
    int value = 0;
    tokenLine_t tokenLine;
    bool isUsedSymbol;

    for (int line = 0, LineSize = inOutLine_vector.size() ; line < LineSize ; line++) {
        tokenLine = inOutLine_vector[line].tokenLine;
        for (int index = 0, size = tokenLine.size() ; index < size ; index++) {
            // Is symbol?
            if (tokenLine[index].type == TYPE_SYMBOL) {
                value = tokenLine[index].value;
                isUsedSymbol = false;
                if (index > 0)
                    isUsedSymbol = true;
                else if (size > 1 && (tokenLine[1].type == TYPE_TABLE2) && (strncmp(tokenLine[1].token,"END",3) == 0))
                    isUsedSymbol = true;
                if (isUsedSymbol)
                    usedSymbolMap[value].push_back(line);
                else if (definedSymbolMap[value] == 0)
                    definedSymbolMap[value] = 1;
                else // Duplicate symbol?
                    sprintf(inOutLine_vector[line].errorMsg, "Error: '%s' duplicate symbol!\n", tokenLine[index].token);
            } // if
        } // for (index)
    } // for (line)

    // Check undefined symbol
    for (int i = 0; i < 100 ; i++)
        if (!usedSymbolMap[i].empty() && definedSymbolMap[i] == 0)
            for (int j = 0, size = usedSymbolMap[i].size(); j < size ; j++) {
                int line = usedSymbolMap[i][j];
                sprintf(inOutLine_vector[line].errorMsg, "Error: undefined symbol!\n");
            } // for
} // CheckSymbol()

pseudoFormat_s *SearchPseudo(tokenLine_t tokenLine) {
    for (int i = 0; i < PSEUDO_TABLE_LEN; i++)
        if (strcmp(pseudoTable[i].tokenName, tokenLine[0].token) == 0)
            return &pseudoTable[i];

    return NULL;
} // SearchPseudo()

instFormat_s *SearchInst(tokenLine_t tokenLine) {
    int j;

    for (int i = 0; i < INST_TABLE_LEN; i++) {
        if (instTable[i].tokenCount != tokenLine.size())
            continue;
        for (j = 0; j < instTable[i].tokenCount; j++) {
            if (instTable[i].compTokenArray[j].tokenType != tokenLine[j].type)
                break;
            if (instTable[i].compTokenArray[j].tokenName[0] == '\0')
                continue;
            if (strcmp(instTable[i].compTokenArray[j].tokenName, tokenLine[j].token) != 0)
                break;
        } // for j
        if (j >= instTable[i].tokenCount)
            return &instTable[i];
    } // for i

    return NULL;
} // SearchInst()
void GenerateMachineCode() {
    symbol_s symbol;
    tokenLine_t tokenLine;
    pseudoFormat_s *pPseudo;
    instFormat_s *pInst;

    for (int line = 0, LineSize = inOutLine_vector.size() ; line < LineSize ; line++) {
        tokenLine = inOutLine_vector[line].tokenLine;
        // Remove leading symbol
        if (tokenLine[0].type == TYPE_SYMBOL) {
            if (tokenLine.size() > 1 && (strncmp(tokenLine[1].token,"END",3) != 0)) {
                symbol.addr = LocP;
                symbol.isWord = (strcmp(tokenLine[1].token, "WORD") == 0 || strcmp(tokenLine[1].token, "DW") == 0);
                symAddrTable[tokenLine[0].value] = symbol;
            } // if
            tokenLine.erase(tokenLine.begin());
        } // if
        if (tokenLine.size() > 0 && tokenLine[0].token[0] == ':') // label':'
            tokenLine.erase(tokenLine.begin());
        if (tokenLine.size() == 0)
            continue;

        // Check first token
        if (tokenLine[0].type == TYPE_TABLE2) {
            pPseudo = SearchPseudo(tokenLine);
            if (pPseudo != NULL)
                pPseudo->pFunc(line, tokenLine, pPseudo);
        } else if (tokenLine[0].type == TYPE_TABLE1) {
            pInst = SearchInst(tokenLine);
            if (pInst != NULL)
                pInst->pFunc(line, tokenLine, pInst);
            else
                sprintf(inOutLine_vector[line].errorMsg, "Error: Invalid operand");
        } else
            sprintf(inOutLine_vector[line].errorMsg, "Error: Invalid format");
    } // for

    for (int i = 0, size = forwardRefLineVector.size() ; i < size ; i++) {
        int line = forwardRefLineVector[i];
        LocP = inOutLine_vector[line].address;
        inOutLine_vector[line].machineCode[0] = '\0';
        tokenLine = inOutLine_vector[line].tokenLine;
        pInst = SearchInst(tokenLine);
        pInst->pFunc(line, tokenLine, pInst);
    } // for
} // GenerateMachinCode()

//============================================================================
// Main program
//============================================================================

int main() {
    memset(symAddrTable, -1, sizeof(symAddrTable));

    isPointerRegMap["BX+SI"] = 0;
    isPointerRegMap["SI+BX"] = 0;
    isPointerRegMap["BX+DI"] = 1;
    isPointerRegMap["DI+BX"] = 1;
    isPointerRegMap["BP+SI"] = 2;
    isPointerRegMap["SI+BP"] = 2;
    isPointerRegMap["BP+DI"] = 3;
    isPointerRegMap["DI+BP"] = 3;
    isPointerRegMap["SI"] = 4;
    isPointerRegMap["DI"] = 5;
    isPointerRegMap["BP"] = 6;
    isPointerRegMap["BX"] = 7;

    is16bitRegMap["AX"] = 0;
    is16bitRegMap["CX"] = 1;
    is16bitRegMap["DX"] = 2;
    is16bitRegMap["BX"] = 3;
    is16bitRegMap["SP"] = 4;
    is16bitRegMap["BP"] = 5;
    is16bitRegMap["SI"] = 6;
    is16bitRegMap["DI"] = 7;

    is8bitRegMap["AL"] = 0;
    is8bitRegMap["CL"] = 1;
    is8bitRegMap["DL"] = 2;
    is8bitRegMap["BL"] = 3;
    is8bitRegMap["AH"] = 4;
    is8bitRegMap["CH"] = 5;
    is8bitRegMap["DH"] = 6;
    is8bitRegMap["BH"] = 7;

    isSegmentMap["ES"] = 0;
    isSegmentMap["CS"] = 1;
    isSegmentMap["SS"] = 2;
    isSegmentMap["DS"] = 3;

    // Confirm input file
    if (!GetInputFileName(fileName))
        return 0;

    if (!lexical_analysis(fileName))
        return 0;

    ReplaceEQU();
    CheckSymbol();
    GenerateMachineCode();

    // Confirm output file
    ReplaceStr(fileName, (char *)"input", (char *)"output");
    WriteFile(fileName);

    return 0;
} // main()
