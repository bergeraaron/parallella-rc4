#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <e-hal.h>  // Epiphany Hardware Abstraction Layer
                    // functionality for communicating with epiphany chip when
                    // the application runs on a host, typically the ARM µp

#define BUFOFFSET (0x01000000)  // SDRAM is at 0x8f00'0000,
                                // offset in e_read starts at 0x8e00'0000
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
unsigned rows, cols, i, j, ncores, row, col, core_num;

/*
 * Init the epiphany platform
 */
void init_epiphany(e_platform_t * platform) {
  e_init(NULL);
  e_reset_system();
  e_get_platform_info(platform);
}

/*
 * Create the workgroup and load programs into it
 */
init_workgroup(e_epiphany_t * dev) {
  e_return_stat_t result;
  e_open(dev, 0, 0, rows, cols); // Create an epiphany cores workgroup
  e_reset_group(dev);
  // load programs into cores workgroup, do not execute it immediately
//  result = e_load_group("emain.srec", dev, 0, 0, rows, cols, E_FALSE);
  result = e_load_group("emain.elf", dev, 0, 0, rows, cols, E_FALSE);
  if(result != E_OK) {
    printf("Error Loading the Epiphany Application %i\n", result);
  }
  e_start_group(dev);
}

/*
 * Main entry
 */
int main(int argc, char * argv[]) {

  e_platform_t platform;  // platform infos
  e_epiphany_t dev;       // provides access to cores workgroup
  e_mem_t emem;           // shared memory buffer

  init_epiphany(&platform);

  rows = platform.rows;
  cols = platform.cols;
  ncores = rows * cols;

//  char result[ncores];     // to store the results, size of cores
  uint16_t counter=0;
  uint16_t resultflags;
  unsigned int memsize=0;
  unsigned char key[5];
  unsigned char iv[3];
  unsigned char data[3]; 
  int i=0;
  int offset=0;
  unsigned char keys[sizeof(key)*ncores];
  for(i=0;i<(sizeof(key)*ncores);i++)
    keys[i]=0;

  // allocate a space to share data between e_cores and here
  // offset starts from 0x8e00'0000
  // sdram (shared space) is at 0x8f00'0000
  // so 0x8e00'0000 + 0x0100'0000 = 0x8f00'0000
/**
  e_alloc(&emem, BUFOFFSET, ncores*sizeof(char) +
                            ncores*sizeof(uint32_t) +
                            ncores*sizeof(uint32_t)); // *2 'cause we store result and number of iterations
/**/
  e_alloc(&emem, BUFOFFSET, sizeof(uint16_t) +
			    sizeof(iv) +
			    sizeof(data) +
                            ncores*sizeof(key)
                            );
//uint16_t - flags for the cores
printf("flag memory allocation %08X \n",(sizeof(uint16_t)));
memsize+=(sizeof(uint16_t));
printf("key memory allocation %08X \n",(ncores*sizeof(key)));
memsize+=(ncores*sizeof(key));
printf("iv memory allocation %08X \n",sizeof(iv));
memsize+=(sizeof(iv));
printf("data memory allocation %08X \n",sizeof(data));
memsize+=(sizeof(data));
printf("total size: %08X \n",memsize);
unsigned char fullmem[88];
for(i=0;i<88;i++)
	fullmem[i]=0;

/**
iv: D0:01:00
snap: 7B:FC:A0
/**/

iv[0] = 0xD0;
iv[1] = 0x01;
iv[2] = 0x00;

data[0] = 0x7B;
data[1] = 0xFC;
data[2] = 0xA0;

  int breakout = 0;
  init_workgroup(&dev);

//write the data and iv to mem
uint16_t res=0;
printf("write res\n");
e_write(&emem, 0, 0, 0x0000, &res, sizeof(res));
printf("write iv\n");
e_write(&emem, 0, 0, 0x0002, &iv, sizeof(iv));
printf("write data\n");
e_write(&emem, 0, 0, 0x0005, &data, sizeof(data));
printf("run\n");
//usleep(5000000);

  // we read from the allocated space and store it to the result array
//  for(i = 0; i < game_iteration; i++) {
    while(1){
//printf("\n"); 
   usleep(1000);
    e_read(&emem, 0, 0, 0x0, &resultflags, sizeof(uint16_t)); // reads the resultflags
    e_read(&emem, 0, 0, 0x08, &keys, ncores*sizeof(key)); // reads the keys

    e_read(&emem, 0, 0, 0x0, &fullmem,88);
if(counter == 10000)
{
	printf("fullmem\n");
	for(i=0;i<88;i++)
		printf("%02X ",fullmem[i]);
	printf("\n");
	counter = 0;
}
counter++;
//    printf("resultflags: %04X \n",resultflags);
//printf("keys\n");
//for(i=0;i<(ncores*sizeof(key));i++)
//{
//	printf("%02X ",keys[i]);
//}
//	printf("\n");

    for(row = 0; row < rows; row++) {
      for(col = 0; col < cols; col++) {
//        fprintf(stdout, "%02X\t", result[row*cols+col]);

	offset = core_num*sizeof(key);
//	fprintf(stdout, "offset %02X\n", offset);
	key[0] = keys[offset];
        key[1] = keys[offset+1];
        key[2] = keys[offset+2];
        key[3] = keys[offset+3];
        key[4] = keys[offset+4];

	core_num = row*cols+col;
	//fprintf(stdout, "core_num:%d\n",core_num);
	if(CHECK_BIT(resultflags,core_num) > 0)
 	{
		fprintf(stdout, "core_num:%d we have something to check key: %02X:%02X:%02X:%02X:%02X\n",core_num,key[0],key[1],key[2],key[3],key[4]);
	}
      }
//      fprintf(stdout, "\n");
    }
//    fflush(stdout);
//    if(breakout == 5)
//        break;

breakout++;
  }

fprintf(stdout, "everyone done\n");

  e_close(&dev);
  return 0;
}
