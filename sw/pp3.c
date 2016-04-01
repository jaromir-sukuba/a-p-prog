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


int parse_hex (char * filename, unsigned char * progmem, unsigned char * config);

char* COM = "";
//char* COM = "/dev/ttyS0";


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
	
		
/*	
int getIntArg(char* arg) {
	if (strlen(arg)>=2 && memcmp(arg,"0x",2)==0) {
		unsigned int u;
		sscanf(arg+2,"%X",&u);
		return u;
		}
	else {
		int d;
		sscanf(arg,"%d",&d);
		return d;
		}
	}
	*/

void printHelp() {
		flsprintf(stdout,"pp programmer\n");
	exit(0);
	}
	

void setCPUtype(char* cpu) {
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

	else {
		flsprintf(stderr,"Unsupported CPU type '%s'\n",cpu);
		abort();
		}
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


int p16a_program_page (unsigned int ptr, unsigned char num)
	{
	unsigned char i;
	if (verbose>2)
		flsprintf(stdout,"Programming page of %d bytes at %d\n", num,ptr);

	putByte(0x08);
	putByte(num+1);
	putByte(num);
	for (i=0;i<num;i++)
		putByte(file_image[ptr+i]);
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
if (verbose>2) flsprintf(stdout,"Getting config +%d - lo:%2.2x,hi:%2.2x\n",n,devid_lo,devid_hi);
retval = (((unsigned int)(devid_lo))<<0) + (((unsigned int)(devid_hi))<<8);
return retval;
}


int p16a_program_config(void)
{
p16a_rst_pointer();
p16a_load_config();
p16a_inc_pointer(7);
p16a_program_page(2*0x8007,2);
p16a_program_page(2*0x8008,2);
return 0;
}


int p18a_read_page (unsigned char * data, int address, unsigned char num)
{
unsigned char i;
	if (verbose>2)
		flsprintf(stdout,"Reading page of %d bytes at 0x%4.4x\n", num, address);
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
	flsprintf(stdout,"Writing page of %d bytes at 0x%4.4x\n", num, address);
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
unsigned char i;
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
	else 	if (chip_family==CF_P18F_A) putByte(0x10);
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

if (chip_family==CF_P16F_A) return p16a_get_devid();
else 	if (chip_family==CF_P18F_A)
	{
	p18a_read_page((char *)&mem_str, 0x3FFFFE, 2);
	devid = (((unsigned int)(mem_str[1]))<<8) + (((unsigned int)(mem_str[0]))<<0);
	devid = devid & devid_mask;
	return devid;
	}
return 0;
}




/*
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
	int line_pointer, line_len, line_type, line_address, line_address_offset;
	if (verbose>2) printf ("Opening filename %s \n", filename);
	FILE* sf = fopen(filename, "r");
	if (sf==0)
		return -1;
	line_address_offset = 0;
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
		if (verbose>2) printf("Line len %d B, type %d, address 0x%4.4x offset 0x%4.4x\n",line_len,line_type,line_address,line_address_offset);
		if (line_type==0)
			{
			for (i=0;i<line_len;i++)
				{
				sscanf(line+9+i*2,"%2X",&temp);
				line_content[i] = temp;
				}
			if (line_address_offset==0)
				{
				if (verbose>2) printf("PM ");
				for (i=0;i<line_len;i++) progmem[line_address+i] = line_content[i];
				}
			if (line_address_offset==0x30)
				{
				if (verbose>2) printf("CB ");
				for (i=0;i<line_len;i++) config[line_address+i] = line_content[i];
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
	int i,j,empty,pages_performed;
	unsigned char * pm_point, * cm_point;
	unsigned char tdat[200];
	parseArgs(argc,argv);
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
	if (program==1)
		{
		pages_performed = 0;
		p18a_mass_erase();
		printf ("Programming FLASH (%d B in %d pages per %d bytes): \n",flash_size,flash_size/page_size,page_size);
		fflush(stdout); 
		for (i=0;i<flash_size;i=i+page_size)
			{
			if (verbose>1) 
				{
				printf (".");
				fflush(stdout); 
				}
			if (is_empty(progmem+i,page_size)==0) 
				{
				p18a_write_page(progmem+i,i,page_size);
				pages_performed++;
				}
			}
		printf ("%d pages programmed\n",pages_performed);
		printf ("Programming config\n");
		for (i=0;i<16;i=i+2)
			p18a_write_cfg(config_bytes[i],config_bytes[i+1],0x300000+i);
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
					printf (".");
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
	prog_exit_progmode();
	return 0;
	}
