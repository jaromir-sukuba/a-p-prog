#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>


#if defined(__linux__)
#include <termios.h>
#else
#include <windows.h>
#endif

void setCPUtype(char* cpu);
int parse_hex (char * filename, unsigned char * progmem, unsigned char * config);

char* COM = "";
//char* COM = "/dev/ttyS0";

char * PP_VERSION = "0.91";


#define	PROGMEM_LEN	260000
#define	CONFIG_LEN	32
unsigned char progmem[PROGMEM_LEN], config_bytes[CONFIG_LEN];

int com;

int baudRate;
int verbose = 1;
int verify = 1;
int program = 1;
int devid_expected;
int devid_mask;
unsigned char file_image[70000];
int flash_size = 4096;
int page_size = 32;
int sleep_time = 0;
int chip_family = 0;

#define	CF_P16F_A	0
#define	CF_P18F_A	1
#define	CF_P16F_B	2
#define	CF_P18F_B	3


void comErr(char *fmt, ...) {
	char buf[ 500 ];
	va_list va;
	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	fprintf(stderr,"%s", buf);
	perror(COM);
	va_end(va);
	abort(); 
	}

void flsprintf(FILE* f, char *fmt, ...) {
	char buf[ 500 ];
	va_list va;
	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	fprintf(f,"%s", buf);
	fflush(f);
	va_end(va);
	}
	
	
#if defined(__linux__)
	
void initSerialPort() {
	baudRate=B57600;
	if (verbose>2)
		printf("Opening: %s at %d\n",COM,baudRate);
	com =  open(COM, O_RDWR | O_NOCTTY | O_NDELAY);
	if (com <0) 
		comErr("Failed to open serial port");

	struct termios opts;
	memset (&opts,0,sizeof (opts));	
	
	fcntl(com, F_SETFL, 0);	

	if (tcgetattr(com, &opts)!=0)
		{
		printf("Err tcgetattr\n");
		}

	cfsetispeed(&opts, baudRate);   
	cfsetospeed(&opts, baudRate);  

	opts.c_lflag  &=  ~(ICANON | ECHO | ECHOE | ISIG);

	opts.c_cflag |=  (CLOCAL | CREAD);
	opts.c_cflag &=  ~PARENB;
	opts.c_cflag &= ~CSTOPB;
	opts.c_cflag &=  ~CSIZE;
	opts.c_cflag |=  CS8;
	
	opts.c_oflag &=  ~OPOST;
	
	opts.c_iflag &=  ~INPCK;
	opts.c_iflag &=  ~ICRNL;		//do NOT translate CR to NL
	opts.c_iflag &=  ~(IXON | IXOFF | IXANY);	
	opts.c_cc[ VMIN ] = 0;
	opts.c_cc[ VTIME ] = 10;//0.1 sec
	


	if (tcsetattr(com, TCSANOW, &opts) != 0) {
		perror(COM); 
		printf("set attr error");
		abort(); 
		}
		
	tcflush(com,TCIOFLUSH); // just in case some crap is the buffers
	/*	
	char buf = -2;
	while (read(com, &buf, 1)>0) {
		if (verbose)
			printf("Unexpected data from serial port: %02X\n",buf & 0xFF);
		}
*/
	}

	
void putByte(int byte) {
	char buf = byte;
	if (verbose>3)
		flsprintf(stdout,"TX: 0x%02X\n", byte);
	int n = write(com, &buf, 1);
	if (n != 1)
		comErr("Serial port failed to send a byte, write returned %d\n", n);
	}
	
	
void putBytes (unsigned char * data, int len)
{

int i;
for (i=0;i<len;i++)	
	putByte(data[i]);
/*
	if (verbose>3)
		flsprintf(stdout,"TXP: %d B\n", len);
int n = write(com, data, len);
	if (n != len)
		comErr("Serial port failed to send %d bytes, write returned %d\n", len,n);
*/
}
	
int getByte() {
	char buf;
	int n = read(com, &buf, 1);
	if (verbose>3)
		flsprintf(stdout,n<1?"RX: fail\n":"RX:  0x%02X\n", buf & 0xFF);
	if (n == 1)
		return buf & 0xFF;
	
	comErr("Serial port failed to receive a byte, read returned %d\n", n);
	return -1; // never reached
	}
#else

HANDLE port_handle;
	
void initSerialPort() 
{

char mode[40],portname[20];
COMMTIMEOUTS timeout_sets;
DCB port_sets;
strcpy(portname,"\\\\.\\");
strcat(portname,COM);
  port_handle = CreateFileA(portname,
                      GENERIC_READ|GENERIC_WRITE,
                      0,                          /* no share  */
                      NULL,                       /* no security */
                      OPEN_EXISTING,
                      0,                          /* no threads */
                      NULL);                      /* no templates */
  if(port_handle==INVALID_HANDLE_VALUE)
  {
    printf("unable to open port %s -> %s\n",COM, portname);
    exit(0);
  }
  strcpy (mode,"baud=57600 data=8 parity=n stop=1");
  memset(&port_sets, 0, sizeof(port_sets));  /* clear the new struct  */
  port_sets.DCBlength = sizeof(port_sets);
  
  if(!BuildCommDCBA(mode, &port_sets))
  {
	printf("dcb settings failed\n");
	CloseHandle(port_handle);
	exit(0);
  }

  if(!SetCommState(port_handle, &port_sets))
  {
    printf("cfg settings failed\n");
    CloseHandle(port_handle);
    exit(0);
  }


  timeout_sets.ReadIntervalTimeout         = 1;
  timeout_sets.ReadTotalTimeoutMultiplier  = 100;
  timeout_sets.ReadTotalTimeoutConstant    = 1;
  timeout_sets.WriteTotalTimeoutMultiplier = 100;
  timeout_sets.WriteTotalTimeoutConstant   = 1;

  if(!SetCommTimeouts(port_handle, &timeout_sets))
  {
    printf("timeout settings failed\n");
    CloseHandle(port_handle);
    exit(0);
  }
  
  
}
void putByte(int byte) 
{
  int n;
  	if (verbose>3)
		flsprintf(stdout,"TX: 0x%02X\n", byte);
  WriteFile(port_handle, &byte, 1, (LPDWORD)((void *)&n), NULL);
  	if (n != 1)
		comErr("Serial port failed to send a byte, write returned %d\n", n);
}

void putBytes (unsigned char * data, int len)
{
/*
int i;
for (i=0;i<len;i++)	
	putByte(data[i]);
*/
  int n;
  WriteFile(port_handle, data, len, (LPDWORD)((void *)&n), NULL);
  if (n != len)
	comErr("Serial port failed to send a byte, write returned %d\n", n);
}


	
int getByte() 
{
unsigned char buf[2];
int n;
ReadFile(port_handle, buf, 1, (LPDWORD)((void *)&n), NULL);
	if (verbose>3)
		flsprintf(stdout,n<1?"RX: fail\n":"RX:  0x%02X\n", buf[0] & 0xFF);	
	if (n == 1)
		return buf[0] & 0xFF;
	comErr("Serial port failed to receive a byte, read returned %d\n", n);
	return -1; // never reached
	}
#endif

	

void sleep_ms (int num)
{
	struct timespec tspec;
	tspec.tv_sec=num/1000;
	tspec.tv_nsec=(num%1000)*1000000; 
	nanosleep(&tspec,0);
}
	
void printHelp() {
		flsprintf(stdout,"pp programmer\n");
	exit(0);
	}
	

void parseArgs(int argc, char *argv[]) {	
	int c;
	while ((c = getopt (argc, argv, "c:nps:t:v:")) != -1) {
		switch (c) {
			case 'c' : 
				COM=optarg;
				break;
			case 'n':
		    	verify = 0;
			    break;
			case 'p':
		    	program = 0;
			    break;
			case 's' : 
				sscanf(optarg,"%d",&sleep_time);
				break;
			case 't' :
				setCPUtype(optarg);
				break;
			case 'v' :
				sscanf(optarg,"%d",&verbose);
				break;
			case '?' :
				if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
			  default:
				fprintf (stderr,"Bug, unhandled option '%c'\n",c);
				abort ();
			}
		}
	if (argc<=1) 
		printHelp();
	}



int p16a_rst_pointer (void)
	{
	if (verbose>2)
		flsprintf(stdout,"Resetting PC\n");

	putByte(0x03);
	putByte(0x00);
	getByte();
	return 0;
	}

int p16a_mass_erase (void)
	{
	if (verbose>2)
		flsprintf(stdout,"Mass erase\n");

	putByte(0x07);
	putByte(0x00);
	getByte();
	return 0;
	}

int p16a_load_config (void)
	{
	if (verbose>2)
		flsprintf(stdout,"Load config\n");
	putByte(0x04);
	putByte(0x00);
	getByte();
	return 0;
	}

int p16a_inc_pointer (unsigned char num)
	{
	if (verbose>2)
		flsprintf(stdout,"Inc pointer %d\n",num);
	putByte(0x05);
	putByte(0x01);
	putByte(num);
	getByte();
	return 0;
	}


int p16a_program_page (unsigned int ptr, unsigned char num, unsigned char slow)
	{
//	unsigned char i;
	if (verbose>2)
		flsprintf(stdout,"Programming page of %d bytes at 0x%4.4x\n", num,ptr);
	
	putByte(0x08);
	putByte(num+2);
	putByte(num);
	putByte(slow);
	/*
	for (i=0;i<num;i++)
		putByte(file_image[ptr+i]);
		*/
	putBytes(&file_image[ptr],num);
	getByte();
	return 0;
	}

int p16a_read_page (unsigned char * data, unsigned char num)
{
unsigned char i;
	if (verbose>2)
		flsprintf(stdout,"Reading page of %d bytes\n", num);
putByte(0x06);
putByte(0x01);
putByte(num/2);
getByte();
for (i=0;i<num;i++)
	{
	*data++ = getByte();
	}
return 0;
}


int p16a_get_devid (void)
{
unsigned char tdat[20],devid_lo,devid_hi;
unsigned int retval;
p16a_rst_pointer();
p16a_load_config();
p16a_inc_pointer(6);
p16a_read_page(tdat, 4);
devid_hi = tdat[(2*0)+1];
devid_lo = tdat[(2*0)+0];
if (verbose>2) flsprintf(stdout,"Getting devid - lo:%2.2x,hi:%2.2x\n",devid_lo,devid_hi);
retval = (((unsigned int)(devid_lo))<<0) + (((unsigned int)(devid_hi))<<8);
retval = retval & devid_mask;
return retval;
}

int p16a_get_config (unsigned char n)
{
unsigned char tdat[20],devid_lo,devid_hi;
unsigned int retval;
p16a_rst_pointer();
p16a_load_config();
p16a_inc_pointer(n);
p16a_read_page(tdat, 4);
devid_hi = tdat[(2*0)+1];
devid_lo = tdat[(2*0)+0];
retval = (((unsigned int)(devid_lo))<<0) + (((unsigned int)(devid_hi))<<8);
if (verbose>2) flsprintf(stdout,"Getting config +%d - lo:%2.2x,hi:%2.2x = %4.4x\n",n,devid_lo,devid_hi,retval);
return retval;
}


int p16a_program_config(void)
{
p16a_rst_pointer();
p16a_load_config();
p16a_inc_pointer(7);
p16a_program_page(2*0x8007,2,1);
p16a_program_page(2*0x8008,2,1);
if (chip_family==CF_P16F_B) p16a_program_page(2*0x8009,2,1);
return 0;
}


int p18a_read_page (unsigned char * data, int address, unsigned char num)
{
unsigned char i;
	if (verbose>2)
		flsprintf(stdout,"Reading page of %d bytes at 0x%6.6x\n", num, address);
putByte(0x11);
putByte(0x04);
putByte(num/2);
putByte((address>>16)&0xFF);
putByte((address>>8)&0xFF);
putByte((address>>0)&0xFF);
getByte();
for (i=0;i<num;i++)
	{
	*data++ = getByte();
	}
return 0;
}

int p18a_mass_erase (void)
	{
	if (verbose>2)
		flsprintf(stdout,"Mass erase\n");

	putByte(0x13);
	putByte(0x00);
	getByte();
	return 0;
	}

int p18b_mass_erase (void)
	{
	if (verbose>2)
		flsprintf(stdout,"Mass erase\n");
	putByte(0x23);
	putByte(0x00);
	getByte();
	return 0;
	}


int p18a_write_page (unsigned char * data, int address, unsigned char num)
{
unsigned char i,empty;
empty = 0;
for (i=0;i<num;i++)
{
	if (data[i]!=0xFF) empty = 0;
}
if (empty==1)	
{
if (verbose>3)
		flsprintf(stdout,"~");
	return 0;
}
if (verbose>2)
	flsprintf(stdout,"Writing page of %d bytes at 0x%6.6x\n", num, address);
putByte(0x12);
putByte(4+num);
putByte(num);
putByte((address>>16)&0xFF);
putByte((address>>8)&0xFF);
putByte((address>>0)&0xFF);
for (i=0;i<num;i++)
		putByte(data[i]);
getByte();
return 0;
}

int p18a_write_cfg (unsigned char data1, unsigned char data2, int address)
{
if (verbose>2)
	flsprintf(stdout,"Writing cfg 0x%2.2x 0x%2.2x at 0x%6.6x\n", data1, data2, address);
putByte(0x14);
putByte(6);
putByte(0);
putByte((address>>16)&0xFF);
putByte((address>>8)&0xFF);
putByte((address>>0)&0xFF);
putByte(data1);
putByte(data2);
getByte();
return 0;
}


int prog_enter_progmode (void)
	{
	if (verbose>2)
		flsprintf(stdout,"Entering programming mode\n");

		if (chip_family==CF_P16F_A) putByte(0x01);
	else 	if (chip_family==CF_P16F_B) putByte(0x01);
	else 	if (chip_family==CF_P18F_A) putByte(0x10);
	else 	if (chip_family==CF_P18F_B) putByte(0x10);
	putByte(0x00);
	getByte();
	return 0;
	}

int prog_exit_progmode (void)
	{
	if (verbose>2)
		flsprintf(stdout,"Exiting programming mode\n");

	putByte(0x02);
	putByte(0x00);
	getByte();
	return 0;
	}

int prog_get_device_id (void)
{
	unsigned char mem_str[10];
	unsigned int devid;

if ((chip_family==CF_P16F_A)|(chip_family==CF_P16F_B) )
	return p16a_get_devid();
else 	if ((chip_family==CF_P18F_A)|(chip_family==CF_P18F_B))
	{
	p18a_read_page((unsigned char *)&mem_str, 0x3FFFFE, 2);
	devid = (((unsigned int)(mem_str[1]))<<8) + (((unsigned int)(mem_str[0]))<<0);
	devid = devid & devid_mask;
	return devid;
	}
return 0;
}





size_t getlinex(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
    	return -1;
    }
    if (stream == NULL) {
    	return -1;
    }
    if (n == NULL) {
    	return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
    	return -1;
    }
    if (bufptr == NULL) {
    	bufptr = malloc(128);
    	if (bufptr == NULL) {
    		return -1;
    	}
    	size = 128;
    }
    p = bufptr;
    while(c != EOF) {
    	if ((p - bufptr) > (size - 1)) {
    		size = size + 128;
    		bufptr = realloc(bufptr, size);
    		if (bufptr == NULL) {
    			return -1;
    		}
    	}
    	*p++ = c;
    	if (c == '\n') {
    		break;
    	}
    	c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}


int parse_hex (char * filename, unsigned char * progmem, unsigned char * config)
{
	char * line = NULL;
	unsigned char line_content[128];
    size_t len = 0;
	int i,temp;
    int read;
    int p16_cfg = 0;
	int line_len, line_type, line_address, line_address_offset;
	int effective_address;
	if (verbose>2) printf ("Opening filename %s \n", filename);
	FILE* sf = fopen(filename, "r");
	if (sf==0)
		return -1;
	line_address_offset = 0;
	if (chip_family==CF_P16F_A)
		p16_cfg = 1;
	if (chip_family==CF_P16F_B)
		p16_cfg = 1;
	
	if (verbose>2) printf ("File open\n");
	while ((read =  getlinex(&line, &len, sf)) != -1) 
		{
		if (verbose>2) printf("\nRead %d chars: %s",read,line);
		if (line[0]!=':') 
			{
			if (verbose>1) printf("--- : invalid\n");
			return -1;
			}
		sscanf(line+1,"%2X",&line_len);
		sscanf(line+3,"%4X",&line_address);
		sscanf(line+7,"%2X",&line_type);
		effective_address = line_address+(65536*line_address_offset);
		if (verbose>2) printf("Line len %d B, type %d, address 0x%4.4x offset 0x%4.4x, EFF 0x%6.6x\n",line_len,line_type,line_address,line_address_offset,effective_address);
		if (line_type==0)
			{
			for (i=0;i<line_len;i++)
				{
				sscanf(line+9+i*2,"%2X",&temp);
				line_content[i] = temp;
				}
			if (effective_address<flash_size)
				{
				if (verbose>2) printf("PM ");
				for (i=0;i<line_len;i++) progmem[effective_address+i] = line_content[i];
				}
			if ((line_address_offset==0x30)&(chip_family==CF_P18F_A))
				{
				if (verbose>2) printf("CB ");
				for (i=0;i<line_len;i++) config[i] = line_content[i];
				}
			if ((chip_family==CF_P18F_B)&(effective_address==(flash_size-8)))
				{
				if (verbose>2) printf("CB ");
				for (i=0;i<line_len;i++) config[i] = line_content[i];
				}
			if ((line_address_offset==0x01)&(p16_cfg==1))
				{
				if (verbose>2) printf("CB ");
				for (i=0;i<line_len;i++) config[line_address+i-0x0E] = line_content[i];
				}
			}
		if (line_type==4)
			{
			sscanf(line+9,"%4X",&line_address_offset);
			}
		if (verbose>2) for (i=0;i<line_len;i++) printf("%2.2X",line_content[i]);
		if (verbose>2) printf("\n");
		}
	fclose(sf);
	return 0;
}

int is_empty (unsigned char * buff, int len)
{
int i,empty;
empty = 1;
for (i=0;i<len;i++)
	if (buff[i]!=0xFF) empty = 0;
return empty;
}

int main(int argc, char *argv[]) 
	{
	int i,j,pages_performed,config,econfig;
	unsigned char * pm_point, * cm_point;
	unsigned char tdat[200];
	parseArgs(argc,argv);
	printf ("PP programmer, version %s\n",PP_VERSION);
	printf ("Opening serial port\n");
	initSerialPort();
	if (sleep_time>0)
		{
		printf ("Sleeping for %d ms while arduino bootloader expires\n", sleep_time);
		fflush(stdout);
		sleep_ms (sleep_time);
		}

	for (i=0;i<PROGMEM_LEN;i++) progmem[i] = 0xFF;
	for (i=0;i<CONFIG_LEN;i++) config_bytes[i] = 0xFF;
	
	char* filename=argv[argc-1];
	pm_point = (unsigned char *)(&progmem);
	cm_point = (unsigned char *)(&config_bytes);
	printf ("pp\n");

	parse_hex(filename,pm_point,cm_point);
	
	//now this is ugly kludge
	//the original programmer expected only file_image holding the image of memory to be programmed
	//for PIC18, it is divided into two regions, program memory and config. to glue those two 
	//different approaches, I made this. not particulary proud of having this mess
	for (i=0;i<70000;i++) file_image [i] = progmem[i];
	for (i=0;i<4;i++) file_image [2*0x8007 + i] = config_bytes[i];
	for (i=0;i<70000;i++)
		{
		if ((i%2)!=0) 
			file_image[i] = 0x3F&file_image[i];
		}
/*
	for (i=0;i<32;i++)	
	{
		if ((i%8)==0)
			printf ("\nA: %4.4x : ", i);
		printf (" %2.2x", config_bytes[i]);
		fflush(stdout);
	}
	printf ("\n\n");

	for (i=2*0x8007;i<(2*0x8007+32);i++)	
	{
		if ((i%8)==0)
			printf ("\nA: %4.4x : ", i);
		printf (" %2.2x", file_image[i]);
		fflush(stdout);
	}
	printf ("\n\n");
	*/
		
	prog_enter_progmode();
	i = prog_get_device_id();
	if (i==devid_expected)
		printf ("Device ID: %4.4x \n", i);
	else
		{
		printf ("Wrong device ID: %4.4x, expected: %4.4x\n", i,devid_expected);
		printf ("Check for connection to target MCU, exiting now\n");
		prog_exit_progmode();
		return 1;
		}
	//ah, I need to unify programming interfaces for PIC16 and PIC18
	if ((chip_family==CF_P18F_A)|(chip_family==CF_P18F_B))
		{
		if (program==1)
			{
			pages_performed = 0;
			if (chip_family==CF_P18F_A)
				p18a_mass_erase();
			if (chip_family==CF_P18F_B)
				p18b_mass_erase();
			printf ("Programming FLASH (%d B in %d pages per %d bytes): \n",flash_size,flash_size/page_size,page_size);
			fflush(stdout); 
			for (i=0;i<flash_size;i=i+page_size)
				{
				if (is_empty(progmem+i,page_size)==0) 
					{
					p18a_write_page(progmem+i,i,page_size);
					pages_performed++;
					printf ("#");
					fflush(stdout); 
					}
				else if (verbose>2) 
					{
					printf (".");
					fflush(stdout); 
					}

				}
			printf ("%d pages programmed\n",pages_performed);
			printf ("Programming config\n");
			for (i=0;i<16;i=i+2)
				if (chip_family==CF_P18F_A) p18a_write_cfg(config_bytes[i],config_bytes[i+1],0x300000+i);
			for (i=0;i<8;i=i+2)
				if (chip_family==CF_P18F_B) p18a_write_cfg(config_bytes[i],config_bytes[i+1],0x300000+i);

			}
		if (verify==1)
			{
			pages_performed = 0;
			printf ("Verifying FLASH (%d B in %d pages per %d bytes): \n",flash_size,flash_size/page_size,page_size);
			for (i=0;i<flash_size;i=i+page_size)
				{
				if (is_empty(progmem+i,page_size))
					{
					if (verbose>2) 
						{	
						printf ("#");
						fflush(stdout); 
						}		
					}	
				else
					{
					p18a_read_page(tdat,i,page_size);
					pages_performed++;
					if (verbose>3) printf ("Verifying page at 0x%4.4X\n",i);
					if (verbose>1) 
						{
						printf ("#");
						fflush(stdout); 
						}
					for (j=0;j<page_size;j++)
						{
						if (progmem[i+j] != tdat[j])
							{
							printf ("Error at 0x%4.4X E:0x%2.2X R:0x%2.2X\n",i+j,progmem[i+j],tdat[j]);
							printf ("Exiting now\n");
							prog_exit_progmode();
							exit(0);
							}
						}
					}
				}
			printf ("%d pages verified\n",pages_performed);
			}
		}
	else
	{
	if (program==1)
		{
		p16a_mass_erase();
		p16a_rst_pointer();

		printf ("Programming FLASH (%d B in %d pages)",flash_size,flash_size/page_size);
		fflush(stdout); 
		for (i=0;i<flash_size;i=i+page_size)
			{
			if (verbose>1) 
				{
				printf (".");
				fflush(stdout); 
				}
			p16a_program_page(i,page_size,0);
			}
		printf ("\n");
		printf ("Programming config\n");
		p16a_program_config();
		}
	if (verify==1)
		{
		printf ("Verifying FLASH (%d B in %d pages)",flash_size,flash_size/page_size);
		fflush(stdout); 
		p16a_rst_pointer();
		for (i=0;i<flash_size;i=i+page_size)
			{
			if (verbose>1) 
				{
				printf (".");
				fflush(stdout); 
				}
			p16a_read_page(tdat,page_size);
			for (j=0;j<page_size;j++)
				{
				if (file_image[i+j] != tdat[j])
					{
					printf ("Error at 0x%4.4X E:0x%2.2X R:0x%2.2X\n",i+j,file_image[i+j],tdat[j]);
					prog_exit_progmode();
					exit(0);
					}
				}
			}
		printf ("\n");
		printf ("Verifying config\n");
		config = p16a_get_config(7);
		econfig = (((unsigned int)(file_image[2*0x8007]))<<0) + (((unsigned int)(file_image[2*0x8007+1]))<<8);

		if (config==econfig) 
			{
			if (verbose>1) printf ("config 1 OK: %4.4X\n",config);
			}
		else	printf ("config 1 error: E:0x%4.4X R:0x%4.4X\n",config,econfig);
		config = p16a_get_config(8);
		econfig = (((unsigned int)(file_image[2*0x8008]))<<0) + (((unsigned int)(file_image[2*0x8008+1]))<<8);
		if (config==econfig) 
			{
			if (verbose>1) printf ("config 2 OK: %4.4X\n",config);
			}
		else	printf ("config 2 error: E:0x%4.4X R:0x%4.4X\n",config,econfig);
		}
	}
	prog_exit_progmode();
	return 0;
	}

	
	/*
//this is how main looked for PIC16 only
int main(int argc, char *argv[]) 
	{
	unsigned char tdat[128];
	int i,j,devid,config,econfig;
	i = 5;

	printf ("\n");
	
	parseArgs(argc,argv);
	printf ("Opening serial port\n");
	initSerialPort();
	
	if (sleep_time>0)
		{
		printf ("Sleeping for %d ms\n", sleep_time);
		fflush(stdout);
		sleep_ms (sleep_time);
		}

	char* filename=argv[argc-1];
	if (verbose>2) printf ("Opening filename %s \n", filename);
	FILE* sf = fopen(filename, "rb");
	if (sf!=0)
		{
		if (verbose>2) printf ("File open\n");
		fread(file_image, 70000, 1, sf);
		fclose(sf);
		}
	else
		printf ("Error opening file \n");
	for (i=0;i<70000;i++)
		{
		if ((i%2)!=0) 
			file_image[i] = 0x3F&file_image[i];
		}

	
	enter_progmode();
	
	devid = get_devid();
	if (devid_expected == (devid&devid_mask))
		printf ("Device ID 0x%4.4X\n",devid);
	else
		printf ("Unexpected device ID 0x%4.4X\n",devid);
	

	if (program==1)
		{
		mass_erase();
		rst_pointer();

		printf ("Programming FLASH (%d B in %d pages)",flash_size,flash_size/page_size);
		fflush(stdout); 
		for (i=0;i<flash_size;i=i+page_size)
			{
			if (verbose>1) 
				{
				printf (".");
				fflush(stdout); 
				}
			program_page(i,page_size);
			}
		printf ("\n");
		printf ("Programming config\n");
		program_config();
		}
	if (verify==1)
		{
		printf ("Verifying FLASH (%d B in %d pages)",flash_size,flash_size/page_size);
		fflush(stdout); 
		rst_pointer();
		for (i=0;i<flash_size;i=i+page_size)
			{
			if (verbose>1) 
				{
				printf (".");
				fflush(stdout); 
				}
			read_page(tdat,page_size);
			for (j=0;j<page_size;j++)
				{
				if (file_image[i+j] != tdat[j])
					{
					printf ("Error at 0x%4.4X E:0x%2.2X R:0x%2.2X\n",i+j,file_image[i+j],tdat[j]);
					exit_progmode();
					exit(0);
					}
				}
			}
		printf ("\n");
		printf ("Verifying config\n");
		config = get_config(7);
		econfig = (((unsigned int)(file_image[2*0x8007]))<<0) + (((unsigned int)(file_image[2*0x8007+1]))<<8);

		if (config==econfig) 
			{
			if (verbose>1) printf ("config 1 OK: %4.4X\n",config);
			}
		else	printf ("config 1 error: E:0x%4.4X R:0x%4.4X\n",config,econfig);
		config = get_config(8);
		econfig = (((unsigned int)(file_image[2*0x8008]))<<0) + (((unsigned int)(file_image[2*0x8008+1]))<<8);
		if (config==econfig) 
			{
			if (verbose>1) printf ("config 2 OK: %4.4X\n",config);
			}
		else	printf ("config 2 error: E:0x%4.4X R:0x%4.4X\n",config,econfig);
		}
		
	printf ("Releasing MCLR\n");
	exit_progmode();

	return 0;
	}
*/



/*
* I'm not exactly happy about this piece of code. Simple, but way too long
* 1500 lines of code in one function, that is way too much. I should 
* TODO: rewrite it to table driven search
*/
void setCPUtype(char* cpu) 
{
	int i,len;
	len = strlen (cpu);
	for(i = 0; i<len; i++) cpu[i] = tolower(cpu[i]);
	if (strcmp("16f1507",cpu)==0) 
		{
		flash_size = 4096;		//bytes, where 1word = 2bytes, though actually being 14 bits
		page_size = 32;			//bytes
		devid_expected = 0x2D00;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1508",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x2D20;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1509",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x2D40;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1454",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3020;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1455",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3021;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1459",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3023;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1454",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3024;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1455",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3025;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1459",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3027;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}	
	else if (strcmp("16f1829",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x27E0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1829",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x28E0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1828",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x27C0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1828",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x28C0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}

	else if (strcmp("16f1825",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x2760;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1825",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x2860;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
		
	else if (strcmp("16f1826",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x2780;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1826",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x2880;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1827",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x27A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1827",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x28A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1824",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x2740;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1824",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x2840;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1847",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 32;
		devid_expected = 0x1480;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1847",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 32;
		devid_expected = 0x14A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}

	else if (strcmp("12f1840",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x1B80;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12lf1840",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 32;
		devid_expected = 0x1BC0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
		
	else if (strcmp("12f1822",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x2700;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12lf1822",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x2800;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12f1572",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x3050;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12f1571",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 16;
		devid_expected = 0x3051;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12lf1572",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x3052;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12lf1571",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 16;
		devid_expected = 0x3053;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1574",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x3000;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1574",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x3004;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1575",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3001;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1575",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3005;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1578",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x3002;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1578",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x3006;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1579",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3003;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1579",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x3007;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1503",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x2CE0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1503",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x2DA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12f1501",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 32;
		devid_expected = 0x2CC0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("12lf1501",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 32;
		devid_expected = 0x2D80;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}		
	else if (strcmp("12f1612",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x3058;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("12lf1612",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x3059;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16f1613",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x305C;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16lf1613",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 32;
		devid_expected = 0x305D;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16f1614",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x3078;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16lf1614",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x307A;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16f1615",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x307C;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16lf1615",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x307E;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16f1618",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x3079;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16lf1618",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x307B;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16f1619",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x307D;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16lf1619",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x307F;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_B;
		}
	else if (strcmp("16f1512",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 64;
		devid_expected = 0x1700;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1513",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x1640;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1516",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x1680;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1517",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x16A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1518",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x16C0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1519",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x16E0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1526",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x1580;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1527",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x15A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1512",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 64;
		devid_expected = 0x1720;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1513",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x1740;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1516",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x1780;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1517",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x17A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1518",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x17C0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1519",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x17E0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1526",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x15C0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1527",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x15E0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1782",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 64;
		devid_expected = 0x2A00;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1783",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x2A20;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1784",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x2A40;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1786",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x2A60;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1787",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x2A80;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1788",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x302B;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1789",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x302A;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1782",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 64;
		devid_expected = 0x2AA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1783",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x2AC0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1784",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x2AE0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1786",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x2B00;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1787",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x2B20;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1788",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x302D;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1789",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x302C;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1902",cpu)==0) 
		{
		flash_size = 2048;
		page_size = 16;
		devid_expected = 0x2C20;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1903",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x2C00;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1904",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x2C80;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1906",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2C60;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1907",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2C40;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1933",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x2300;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1934",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x2340;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1936",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2360;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1937",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2380;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1938",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x23A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1939",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x23C0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1946",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2500;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1947",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x2520;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
		
	else if (strcmp("16lf1933",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x2400;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1934",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x2440;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1936",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2460;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1937",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2480;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1938",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x24A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1939",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x24C0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1946",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x2580;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1947",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x25A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P16F_A;
		}

	else if (strcmp("16f1713",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x3049;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1716",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3048;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1717",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x305C;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1718",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x305B;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1719",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x305A;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1713",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 64;
		devid_expected = 0x304B;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1716",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x304A;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1717",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 64;
		devid_expected = 0x305F;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1718",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x305E;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1719",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 64;
		devid_expected = 0x305D;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}

	else if (strcmp("16f1703",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x3061;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1704",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3043;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1705",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3055;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1707",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x3060;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1708",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3042;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1709",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3054;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1703",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x3063;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1704",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3045;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1705",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3057;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1707",cpu)==0) 
		{
		flash_size = 4096;
		page_size = 16;
		devid_expected = 0x3062;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1708",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3044;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1709",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3056;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1764",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3080;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1765",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3081;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1768",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3084;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16f1769",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3085;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1764",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3082;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1765",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3083;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1768",cpu)==0) 
		{
		flash_size = 8192;
		page_size = 16;
		devid_expected = 0x3086;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("16lf1769",cpu)==0) 
		{
		flash_size = 16384;
		page_size = 16;
		devid_expected = 0x3087;
		devid_mask = 0xFFFF;
		chip_family = CF_P16F_A;
		}
	else if (strcmp("18f25k50",cpu)==0) 
		{
		flash_size = 32768;				//bytes, where 1word = 2bytes = 16 bits
		page_size = 64;					//bytes
		devid_expected = 0x5C20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf25k50",cpu)==0) 
		{
		flash_size = 32768;
		page_size = 64;
		devid_expected = 0x5CA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}

	else if (strcmp("18f23k22",cpu)==0) 
		{
		flash_size = 8192; 
		page_size = 64; 
		devid_expected = 0x5740;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18f24k22",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x5640;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf23k22",cpu)==0) 
		{
		flash_size = 8192; 
		page_size = 64; 
		devid_expected = 0x5760;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf24k22",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x5660;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18f43k22",cpu)==0) 
		{
		flash_size = 8192; 
		page_size = 64; 
		devid_expected = 0x5700;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18f44k22",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x5600;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf43k22",cpu)==0) 
		{
		flash_size = 8192; 
		page_size = 64; 
		devid_expected = 0x5720;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf44k22",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x5620;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}		
	else if (strcmp("18f46k22",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5400;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18f26k22",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5440;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18f45k22",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x5500;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18f25k22",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x5540;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf46k22",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5420;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf26k22",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5460;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf45k22",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x5520;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}
	else if (strcmp("18lf25k22",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x5560;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_A;
		}

	else if (strcmp("18f24j50",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x4C00;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf24j50",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x4CC0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f44j50",cpu)==0) 
		{
		flash_size = 16384;  
		page_size = 64; 
		devid_expected = 0x4C60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf44j50",cpu)==0) 
		{
		flash_size = 16384;  
		page_size = 64; 
		devid_expected = 0x4D20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f25j50",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x4C20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf25j50",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x4CE0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f45j50",cpu)==0) 
		{
		flash_size = 32768;  
		page_size = 64; 
		devid_expected = 0x4C80;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf45j50",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x4D40;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f26j50",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4C40;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf26j50",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4D00;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f46j50",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4CA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf46j50",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4D60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		






	else if (strcmp("18f24j11",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x4D80;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f25j11",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x4DA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f26j11",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4DC0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f44j11",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x4DE0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f45j11",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x4E00;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f46j11",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4E20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf24j11",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x4E40;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf25j11",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x4E60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf26j11",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4E80;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf44j11",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x4EA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf45j11",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x4EC0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18lf46j11",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x4EE0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18f24j10",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x1D00;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18f25j10",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x1C00;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18f44j10",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x1D20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18f45j10",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x1C20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18lf24j10",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x1D40;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18lf25j10",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x1C40;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18lf44j10",cpu)==0) 
		{
		flash_size = 16384; 
		page_size = 64; 
		devid_expected = 0x1D60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}				
	else if (strcmp("18lf45j10",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x1C60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18lf45j10",cpu)==0) 
		{
		flash_size = 32768; 
		page_size = 64; 
		devid_expected = 0x1C60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	
	else if (strcmp("18f26j13",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5920;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18f27j13",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x5960;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18f46j13",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x59A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18f47j13",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x59E0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}	
	else if (strcmp("18lf26j13",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5B20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18lf27j13",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x5B60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18lf46j13",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5BA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18lf47j13",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x5BE0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}		
	else if (strcmp("18f26j53",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5820;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18f27j53",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x5860;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18f46j53",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x58A0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18f47j53",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x58E0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}	
	else if (strcmp("18lf26j53",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5A20;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18lf27j53",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x5A60;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18lf46j53",cpu)==0) 
		{
		flash_size = 65536; 
		page_size = 64; 
		devid_expected = 0x5AA0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
	else if (strcmp("18lf47j53",cpu)==0) 
		{
		flash_size = 131072; 
		page_size = 64; 
		devid_expected = 0x5AE0;
		devid_mask = 0xFFE0;
		chip_family = CF_P18F_B;
		}			
		
				
	else {
		flsprintf(stderr,"Unsupported CPU type '%s'\n",cpu);
		abort();
		}
	}
