#ifndef _SP2_H_
#define _SP2_H_

struct symbol_s{
    int addr;
    bool isWord;
};

struct IntMachineCode_s {
    unsigned char op;
    unsigned char number;
};

struct op1_op2_s {
    unsigned char op1;
    unsigned char op2;
};

struct op_port_s {
    unsigned char op;
    unsigned char port;
};

struct op_reg_s {
    unsigned char reg : 3;
    unsigned char op : 5;
};

struct d_w_mod_reg_rm_s {
    unsigned char w : 1;
    unsigned char d : 1;
    unsigned char op : 6;

    unsigned char rm : 3;
    unsigned char reg : 3;
    unsigned char mod : 2;
};

struct d_w_mod_reg_rm_data_s {
    unsigned char w : 1;
    unsigned char d : 1;
    unsigned char op : 6;

    unsigned char rm : 3;
    unsigned char reg : 3;
    unsigned char mod : 2;

    unsigned char dataLow;
    unsigned char dataHigh;
};

struct mod_reg_rm_s {
    unsigned char op : 8;

    unsigned char rm : 3;
    unsigned char reg : 3;
    unsigned char mod : 2;
};

struct w_reg_data_s {
    unsigned char reg : 3;
    unsigned char w : 1;
    unsigned char op : 4;

    unsigned char dataLow;
    unsigned char dataHigh;
};

struct w_addr_s {
    unsigned char w : 1;
    unsigned char op : 7;

    unsigned char addrLow;
    unsigned char addrHigh;
};

struct w_data_s {
    unsigned char w : 1;
    unsigned char op : 7;

    unsigned char dataLow;
    unsigned char dataHigh;
};

struct condition_jmp_s {
    unsigned char op : 8;

    unsigned char displacement;
};

struct displacement_s {
    unsigned char op : 8;

    unsigned char displacementLow;
    unsigned char displacementHigh;
};

#endif
