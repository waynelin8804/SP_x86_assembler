// x86 Lexical Analysis
// 10627121 Lin, Jun-Wei

#include "sp1.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

///#define TABLE_USE_2_COLUMN

// Table variables

Table table[4];
Hash hashTable[3];
tokenLine_t tokenLine;
vector<inOutLine_s> inOutLine_vector;

// I/O variables

FILE * inputFP;
int gLine = 1;              // 「下一個要讀進來的字元」所在的line number
int gColumn = 1;            // 「下一個要讀進來的字元」所在的column number
int gLastGottenChar;
int gPreviousLine;
int gPreviousColumn;
int inLineLen = 0;
Str1000 inLineBuf;
int outLineLen = 0;
Str1000 outLineBuf;

//============================================================================
// Utility functions
//============================================================================

// String to upper case
char * StrUpr(char * str) {
    char * p;

    for (p = str; *p != '\0'; p++)
        *p = toupper(*p);

    return str;
} // StrUpr()

//============================================================================
// Table functions
//============================================================================

// Pre-process
// clear table 1 to 4, and set size of table 5 to 7 as 100
void ResetTable() {
    for (int i = 0; i < 4; i++)
        table[i].clear();

    for (int i = 0; i < 3; i++) {
        hashTable[i].clear();
        for (int j = 0; j < 100; j++)
            hashTable[i][j] = "";
    } // for
} // ResetTable()

// Read table 1, 2, 3 and 4
bool ReadTable() {
    FILE * fp;
    Str100 fileName, token;
    bool ret = true;
    int tokenID;

    for (int i = 0; i < 4; i++) {
        sprintf(fileName, "Table%d.table", i + 1);

        // Open table file
        fp = fopen(fileName, "rt");
        if (fp == NULL) {
            printf("Error: %s does not exist!\n", fileName);
            ret = false;
            continue;
        } // if

        // Process file each line
#ifdef TABLE_USE_2_COLUMN
        fscanf(fp, "%d %s", &tokenID, token);
#else
        fscanf(fp, "%s", token);
#endif
        tokenID = 1;
        while (strcmp(token, "") != 0) {
            StrUpr(token);
            table[i][token] = tokenID++;
            strcpy(token, "");
#ifdef TABLE_USE_2_COLUMN
            fscanf(fp, "%d %s", &tokenID, token);
#else
            fscanf(fp, "%s", token);
#endif
        } // while

        // 關檔
        fclose(fp);
    } // for

    return ret;
} // ReadTable()

// Token classify. Return associated ID of token string.
int GetTableValueByToken(Table table, Str100 token) {
    Table::iterator result;

    result = table.find(token);
    return result == table.end() ? EOF : result->second;
} // GetTableVauleByToken()

// Token classify. Return associated ID of token char.
int GetTableValueByToken(Table table, char token) {
    char str[2];

    str[0] = token;
    str[1] = '\0';
    return GetTableValueByToken(table, str);
} // GetTableVauleByToken()

// Hash value of Symbol/Integer/String token
int HashFunction(Str100 token) {
    int result = 0, size = strlen(token);

    for (int i = 0; i < size; i++)
        result = (result + token[i]) % 100;

    return result;
} // HashFunction()

// Insert token to hash
void InsertTokenToHash(Hash hashTable, Str100 token, int & value) {
    int start = value;

    // Linear probe
    while (hashTable[value].compare("") != 0) {
        // Token is already put into the table?
        if (hashTable[value].compare(token) == 0)
            return;

        value = ++value % 100;

        // Hash full?
        if (value == start) {
            printf("Error: Hash is full!\n");
            return;
        } // if
    } // while

    hashTable[value] = token;
} // InsertTokenToHash()

//============================================================================
// I/O functions
//============================================================================

// Write assembly input line into buffer
void WriteToSrcLine(int ch) {
    if (inLineLen < sizeof(inLineBuf) - 1)
        inLineBuf[inLineLen++] = ch;
} // WriteToSrcLine()

// Delete one char from source line buffer
void DeleteSrcLineTail() {
    if (inLineLen > 0)
        inLineLen--;
} // DeleteSrcLineTail()

// Write token output line into buffer
void WriteToOutLine(int type, int value, Str100 tokenStr) {
    token_s token;

    token.type = type;
    token.value = value;
    strcpy(token.token, tokenStr);
    tokenLine.push_back(token);
} // WriteToOutLine()

// Write output file
void WriteInOutLineToFile() {
    inOutLine_s inOutLine;

    if (!tokenLine.empty()) {
        // Write assembly input line to file
        if (inLineBuf[inLineLen - 1] == '\n')
            inLineLen--;
        inLineBuf[inLineLen] = '\0';
        strcpy(inOutLine.srcLine, inLineBuf);
        inLineLen = 0;

        // Write token output line to file
        inOutLine.tokenLine.assign(tokenLine.begin(), tokenLine.end());
        inOutLine.errorMsg[0] = '\0';
        inOutLine.machineCode[0] = '\0';
        inOutLine.address = -1;
        inOutLine_vector.push_back(inOutLine);
        tokenLine.clear();
    } // if
} // WriteInOutLineToFile()

//============================================================================
// Token functions
//============================================================================

// check if token is Integer
bool IsInt(Str100 token) {
    int size = strlen(token);

    // Integer token need start with digit and end with 'H' or 'h'
    if (!isdigit(token[0]))
        return false;
/*
    if (token[size - 1] != 'H' && token[size - 1] != 'h')
        return false;

    // Check hexadecimal digit for each char
    for (int i = 0; i < size - 1; i++)
        if (!isxdigit(token[i]))
            return false;
*/

    return true;
} // IsInt()

// Get char from input file
void F1_GetChar(int & ch) {
    // Backup line column
    gPreviousLine = gLine;
    gPreviousColumn = gColumn;

    // New line is reached?
    if (gColumn == 1)
        WriteInOutLineToFile();

    // Get a char
    ch = fgetc(inputFP);
    gLastGottenChar = ch;
    if (ch == EOF)
        return;

    WriteToSrcLine(ch);

    // Update line column
    if (ch != '\n')
        gColumn++;
    else {
        gLine++;
        gColumn = 1;
    } // else
} // F1_GetChar()

// Put back char to input file buffer
void F1_UngetChar(int ch) {
    // Put back char
    if (ch == EOF)
        return;

    // Call standard C function to unget char
    ungetc(ch, inputFP);
    DeleteSrcLineTail();

    // Restore line column
    gLine = gPreviousLine;
    gColumn = gPreviousColumn;
} // F1_UngetChar()

// Get first non-white-space char. Need skip comment.
void F2_GetNonWhiteSpaceChar(int & firstChar, int & line, int & column) {
    int ch;
    bool ok = true;

    while (1) {
        // Get non-white-space char
        F1_GetChar(firstChar);
        if (firstChar == EOF)
            return;
        while (isspace(firstChar)) {
            F1_GetChar(firstChar);
            if (firstChar == EOF)
                return;
        } // while

        // Set start line column of token
        line = gPreviousLine;
        column = gPreviousColumn;

        if (firstChar != ';')
            return;

        // Skip comment
        // WriteToOutLine(4, GetTableValueByToken(TABLE4, (char *)";"));
        F1_GetChar(ch);
        while (ch != EOF && ch != '\n')
            F1_GetChar(ch);
    } // while
} // F2_GetNonWhiteSpaceChar()

// Get string token
void F4_GetConstantString(Str100 token) {
    int ch, value;
    int i = 0;

    // Put char into token
    F1_GetChar(ch);
    while (ch != EOF && ch != '\n') {
        token[i++] = ch;
        if (ch == '\'') {
            F1_GetChar(ch);
            if (ch != '\'') {
                F1_UngetChar(ch);
                ch = '\'';
                i--;
                break;
            } // if
        } // if
        F1_GetChar(ch);
    } // while

    token[i] = '\0';

    // Write beginning '\'' token
    WriteToOutLine(4, GetTableValueByToken(TABLE4, '\''), "\'");

    // Write hash value
    value = HashFunction(token);
    if (strcmp(token, "") != 0) {
        InsertTokenToHash(TABLE7, token, value);
        WriteToOutLine(7, value, token);
    } // if

    // Write ending '\'' token
    if (ch == '\'')
        WriteToOutLine(4, GetTableValueByToken(TABLE4, '\''), "\'");
} // F4_GetConstantString()

// Get cut string token
void GetCutStr(Str100 token) {
    int ch;
    int i = 1;

    // Put char into token
    F1_GetChar(ch);
    while (ch != EOF && !isspace(ch) && GetTableValueByToken(TABLE4, ch) == EOF) {
        token[i++] = ch;
        F1_GetChar(ch);
    } // while

    // Unget white space or delimiter
    if (ch != EOF)
        F1_UngetChar(ch);
    token[i] = '\0';
} // GetCutStr()

// Get every case token
bool GetToken(Str100 token, int & line, int & column, TokenType & type) {
    int ch;
    int i = 0;

    type = TYPE_ERROR;

    // Get start char of token
    token[0] = '\0';
    F2_GetNonWhiteSpaceChar(ch, line, column);
    if (ch == EOF)
        return false;
    token[i++] = ch;
    token[i] = '\0';

    // Classify
    if (ch == '\'') {
        type = TYPE_STRING;
        F4_GetConstantString(token);
    } // if
    else if (GetTableValueByToken(TABLE4, ch) != EOF)
        type = TYPE_TABLE4;
    else {
        GetCutStr(token);
        StrUpr(token);
        if (GetTableValueByToken(TABLE1, token) != EOF)
            type = TYPE_TABLE1;
        else if (GetTableValueByToken(TABLE2, token) != EOF)
            type = TYPE_TABLE2;
        else if (GetTableValueByToken(TABLE3, token) != EOF)
            type = TYPE_TABLE3;
        else if (isdigit(token[0])) {
            if (!IsInt(token))
                printf("Error: Invalid integer '%s'!\n", token);
            else
                type = TYPE_INTEGER;
        } // else if
        else
            type = TYPE_SYMBOL;
    } // else

    return true;
} // GetToken()

// Get token and put into linked list
void ParseToken() {
    Str100 token = "";
    int line, column, value;
    TokenType type;

    while (GetToken(token, line, column, type)) {
        switch (type) {
        case TYPE_TABLE1:
            value = GetTableValueByToken(TABLE1, token);
            break;
        case TYPE_TABLE2:
            value = GetTableValueByToken(TABLE2, token);
            break;
        case TYPE_TABLE3:
            value = GetTableValueByToken(TABLE3, token);
            break;
        case TYPE_TABLE4:
            value = GetTableValueByToken(TABLE4, token);
            break;
        case TYPE_SYMBOL:
            value = HashFunction(token);
            InsertTokenToHash(TABLE5, token, value);
            break;
        case TYPE_INTEGER:
            value = HashFunction(token);
            InsertTokenToHash(TABLE6, token, value);
            break;
        } // switch

        if (type != TYPE_ERROR && type != TYPE_STRING)
            WriteToOutLine(type, value, token);
    } // while()

    WriteInOutLineToFile();
} // ParseToken()

//============================================================================
// Main program
//============================================================================

// Main function
bool lexical_analysis(Str100 fileName) {
    inputFP = fopen(fileName, "rt");

    if (inputFP == NULL) {
        printf("Error: Cannot open input file '%s'\n", fileName);
        return false;
    } // if

    // Set up tables
    ResetTable();
    if (!ReadTable()) // Some tables have error
        return false;

    ParseToken();

    // Close i/o file
    fclose(inputFP);
    return true;
} // lexical_analysis()
