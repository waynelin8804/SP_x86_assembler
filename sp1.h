#ifndef _SP1_H_
#define _SP1_H_

#include <map>
#include <vector>
#include <string>
using namespace std;

enum TokenType {
    TYPE_ERROR = 0,
    TYPE_TABLE1 = 1,
    TYPE_TABLE2 = 2,
    TYPE_TABLE3 = 3,
    TYPE_TABLE4 = 4,
    TYPE_SYMBOL = 5,
    TYPE_INTEGER = 6,
    TYPE_STRING = 7
};

typedef char Str100[100];
typedef char Str1000[1000];
typedef map<string, int> Table;
typedef map<int, string> Hash;

#define TABLE1 table[0]
#define TABLE2 table[1]
#define TABLE3 table[2]
#define TABLE4 table[3]
#define TABLE5 hashTable[0]
#define TABLE6 hashTable[1]
#define TABLE7 hashTable[2]

struct token_s {
    int type;
    int value;
    Str100 token;
};

typedef vector<token_s> tokenLine_t;

struct inOutLine_s {
    Str1000 srcLine;
    tokenLine_t tokenLine;
    Str100 errorMsg;
    int address;
    Str100 machineCode;
};

extern Table table[4];
extern Hash hashTable[3];
extern vector<inOutLine_s> inOutLine_vector;

bool lexical_analysis(Str100 fileName);

#endif
