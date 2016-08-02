
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <e-lib.h> // Epiphany cores library

//#include <openssl/rc4.h>

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
/**
check_bit:1 0
check_bit:2 1
check_bit:4 2
check_bit:8 3
check_bit:16 4
check_bit:32 5
check_bit:64 6
check_bit:128 7
check_bit:256 8
check_bit:512 9
check_bit:1024 10
check_bit:2048 11
check_bit:4096 12
check_bit:0 13
check_bit:0 14
check_bit:0 15
check_bit:0 16
check_bit:0 17
check_bit:0 18
check_bit:0 19
check_bit:0 20
check_bit:0 21
check_bit:0 22
check_bit:0 23
check_bit:0 24
check_bit:0 25
check_bit:0 26
check_bit:0 27
check_bit:0 28
check_bit:0 29
check_bit:0 30
check_bit:0 31
/**/
unsigned short flags[] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
//rc4 stuff
volatile unsigned char S[256];
volatile unsigned int g, h;

//functions
void swap(volatile unsigned char *s, unsigned int i, unsigned int j);
void rc4_init(unsigned char *key, unsigned int key_length);
unsigned char rc4_output();
unsigned char RC4(unsigned int data_size,unsigned char * data,unsigned char * un_data_section);
unsigned char returnedbyte=0;
int data_size = 3;
unsigned char un_data_section[3];

volatile uint16_t *resultflags;
volatile unsigned char *key; //5;
volatile unsigned char *iv; //3;
volatile unsigned char *data; //3;

char core_start[] = {0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,255};

int main(void) {

  unsigned core_row, core_col,
           group_rows, group_cols,
           core_num;

  core_row = e_group_config.core_row;
  core_col = e_group_config.core_col;
  group_rows = e_group_config.group_rows;
  group_cols = e_group_config.group_cols;

  core_num = core_row * group_cols + core_col;
/**
  // starts at the beginning of sdram
  resultflags  = (volatile uint16_t *) (0x8f000000 + 0x2);
  iv = (volatile unsigned char*) (0x8f0000002 + 0x3);
  data = (volatile unsigned char*) (0x8f000005 + 0x2);
  key = (volatile unsigned char *) (0x8f000007 + 0x5*core_num);
/**/
  // starts at the beginning of sdram
  resultflags  = (volatile uint16_t *) (0x8f000000);
  iv = (volatile unsigned char*) (0x8f000002);
  data = (volatile unsigned char*) (0x8f000005);
  key = (volatile unsigned char *) (0x8f000008 + 0x5*core_num);

        e_coreid_t id = e_get_coreid();

        //run through a keyspace
        //we got something that we should try to crack
        unsigned char * icvp;
        unsigned int crc;
        unsigned char testkey[8];
	unsigned char localdata[3];
	int a,b,c,d,e;
	//unsigned int compcrc;

	localdata[0] = data[0];
	localdata[1] = data[1];
	localdata[2] = data[2];

        //data[0]=0x22;
        //data[1]=0xAA;

        testkey[0] = iv[0];//iv1
        testkey[1] = iv[1];//iv2
        testkey[2] = iv[2];//iv3
        //sections to try for our loop

	//*result = 0;
  while(1) {

        //for(int a = 0x00; a <= 0xFF;a++)
	for(a = core_start[core_num]; a <= core_start[core_num+1];a++)
        {
                for(b = 0x00;b <= 0xFF;b++)
                {
                        for(c = 0x00;c <= 0xFF;c++)
                        {
                                for(d = 0x00;d <= 0xFF;d++)
                                {
                                        for(e = 0x00;e <= 0xFF;e++)
                                        {

                                                testkey[3] = a;//a
                                                testkey[4] = b;//b
                                                testkey[5] = c;//c
                                                testkey[6] = d;//d
                                                testkey[7] = e;//e
/**
RC4_KEY rc4_key;
RC4_set_key(&rc4_key, sizeof(key), key);
RC4(&rc4_key, data_size, data, un_data_section);
/**/
/**/
                                                rc4_init(testkey, 8);
                                                returnedbyte = RC4(data_size, localdata, un_data_section);
/**/
key[0] = testkey[3];
key[1] = testkey[4];
key[2] = testkey[5];
key[3] = testkey[6];
key[4] = testkey[7];
/**/

if(un_data_section[0] == 0xAA && un_data_section[1] == 0xAA && un_data_section[2] == 0x03)// && un_data_section[2] == 0x03)
{
	key[0] = testkey[3];
	key[1] = testkey[4];
	key[2] = testkey[5];
	key[3] = testkey[6];
	key[4] = testkey[7];

        //possible snap header
	if(CHECK_BIT(*resultflags,core_num) == 0)
        	*resultflags += flags[core_num];
}

                                        }//e for loop
                                }//d for loop
                        }//c for loop
                }//b for loop
        }//a for loop
  }
}

void swap(volatile unsigned char *s, unsigned int g, unsigned int h) {
    unsigned char temp = s[g];
    s[g] = s[h];
    s[h] = temp;
}
// KSA 
void rc4_init(unsigned char *key, unsigned int key_length) {
    for (g = 0; g < 256; g++)
        S[g] = g;

    for (g = h = 0; g < 256; g++) {
        h = (h + key[g % key_length] + S[g]) & 255;
        swap(S, g, h);
    }

    g = h = 0;
}
// PRGA
unsigned char rc4_output() {
    g = (g + 1) & 255;
    h = (h + S[g]) & 255;

    swap(S, g, h);

    return S[(S[g] + S[h]) & 255];
}
//per suggestion from the openciphers site
//look for the start of the snap header, if not there abort that key
unsigned char RC4(unsigned int data_size,unsigned char * data,unsigned char * un_data_section)
{
        return rc4_output();
}


