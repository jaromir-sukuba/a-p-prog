/*
 * pp programmer, for SW 0.99 and higher
 * 
 * 
 */


#include <avr/io.h>
#include <util/delay.h>

#define ISP_PORT  PORTC
#define ISP_DDR   DDRC
#define ISP_PIN   PINC
#define ISP_MCLR  3
#define ISP_DAT   1
#define ISP_CLK   0

#define  ISP_MCLR_1 ISP_PORT |= (1<<ISP_MCLR);
#define  ISP_MCLR_0 ISP_PORT &= ~(1<<ISP_MCLR);
#define  ISP_MCLR_D_I ISP_DDR &= ~(1<<ISP_MCLR);
#define  ISP_MCLR_D_0 ISP_DDR |= (1<<ISP_MCLR);

#define  ISP_DAT_1 ISP_PORT |= (1<<ISP_DAT);
#define  ISP_DAT_0 ISP_PORT &= ~(1<<ISP_DAT);
#define  ISP_DAT_V (ISP_PIN&(1<<ISP_DAT))
#define  ISP_DAT_D_I ISP_DDR &= ~(1<<ISP_DAT);
#define  ISP_DAT_D_0 ISP_DDR |= (1<<ISP_DAT);

#define  ISP_CLK_1 ISP_PORT |= (1<<ISP_CLK);
#define  ISP_CLK_0 ISP_PORT &= ~(1<<ISP_CLK);
#define  ISP_CLK_D_I ISP_DDR &= ~(1<<ISP_CLK);
#define  ISP_CLK_D_0 ISP_DDR |= (1<<ISP_CLK);

#define  ISP_CLK_DELAY  1
void isp_send (unsigned int data, unsigned char n);
unsigned int isp_read_16 (void);
unsigned char enter_progmode (void);
unsigned char exit_progmode (void);
void isp_read_pgm (unsigned int * data, unsigned char n);
void isp_write_pgm (unsigned int * data, unsigned char n);
void isp_mass_erase (void);
void isp_reset_pointer (void);
void isp_send_8_msb (unsigned char data);
unsigned int isp_read_8_msb (void);
unsigned int isp_read_16_msb (void);
unsigned char p16c_enter_progmode (void);
void p16c_set_pc (unsigned long pc);
void p16c_bulk_erase (void);
void p16c_load_nvm (unsigned char inc, unsigned int data);
unsigned int p16c_read_data_nvm (unsigned char inc);
void p16c_begin_prog (unsigned char cfg_bit);
void p16c_isp_write_cfg (unsigned int data, unsigned int addr);
void p18q_isp_write_pgm (unsigned int * data, unsigned long addr, unsigned char n);
void p18q_isp_write_cfg (unsigned int data, unsigned long addr);

unsigned char p18_enter_progmode (void);
unsigned int p18_get_ID (void);
void p18_send_cmd_payload (unsigned char cmd, unsigned int payload);
unsigned int p18_get_cmd_payload (unsigned char cmd);
unsigned int isp_read_8 (void);
void p18_set_tblptr (unsigned long val);
unsigned char p18_read_pgm_byte (void);
void p_18_modfied_nop (void);
void p18_isp_mass_erase (void);
void p18fk_isp_mass_erase (unsigned char data1, unsigned char data2, unsigned char data3);


void usart_tx_b(uint8_t data);
uint8_t usart_rx_rdy(void);
uint8_t usart_rx_b(void);
void usart_tx_s(uint8_t * data);

#define _BAUD	57600	// Baud rate (9600 is default)
#define _UBRR	(F_CPU/16)/_BAUD - 1	// Used for UBRRL and UBRRH 


unsigned int fmimg[32] = {
                            0x1234,0x1234,0x1234,0x1234,
                            0x1234,0x1234,0x1234,0x1234,
                            0x1234,0x1234,0x1234,0x1234,
                            0x1234,0x1234,0x1234,0x1234,
                            0x1234,0x1234,0x1234,0x1234,
                            0x1234,0x1234,0x1234,0x1234,
                            0x1234,0x1234,0x1234,0x1234,
                            0x1234,0x1234,0x1234,0x1234,
                            };



unsigned int dat;
unsigned char rx,i,main_state,bytes_to_receive,rx_state;
unsigned char rx_message[280],rx_message_ptr;
unsigned int flash_buffer[260];
unsigned int test,cfg_val;
unsigned long addr;
int main (void)
{
  UBRR0H = ((_UBRR) & 0xF00);
  UBRR0L = (uint8_t) ((_UBRR) & 0xFF); 
  UCSR0B |= _BV(TXEN0);
  UCSR0B |= _BV(RXEN0);
  
  ISP_CLK_D_0
  ISP_DAT_D_0
  ISP_DAT_0
  ISP_CLK_0
  ISP_MCLR_D_0
  ISP_MCLR_1
  rx_state = 0;
  /*
  while (1)
    {
    if (usart_rx_rdy())
      {
      rx = usart_rx_b();
      if (rx=='i')
        {
        usart_tx_b ('I');
        usart_tx_b (0x0A);
        enter_progmode();
//        test = get_ID();
        exit_progmode();
        usart_tx_hexa_16(test);
        usart_tx_b (0x0A);
        }
      if (rx=='r')
        {
        usart_tx_b ('R');
        usart_tx_b (0x0A);
        enter_progmode();
        isp_reset_pointer();
        isp_read_pgm(flash_buffer,64);
        exit_progmode();
        for (i=0;i<64;i++)
          {
          if (i%4==0) usart_tx_b (0x0A);
          usart_tx_hexa_16(flash_buffer[i]);
          }
        usart_tx_b (0x0A);
        }
      if (rx=='w')
        {
        usart_tx_b ('W');
        enter_progmode();
        isp_reset_pointer();
        isp_write_pgm(fmimg,32,0);
//        p16c_isp_write_pgm(fmimg,32,32);
        exit_progmode();
        usart_tx_b ('*');
        usart_tx_b (0x0A);
        }
      if (rx=='e')
        {
        usart_tx_b ('E');
        enter_progmode();
        isp_reset_pointer();
        isp_mass_erase();
        exit_progmode();
        usart_tx_b ('*');
        usart_tx_b (0x0A);
        }
          
      }
    
    }
    */
  while (1)
    {
    if (usart_rx_rdy())
      {
      rx = usart_rx_b();
      rx_state = rx_state_machine (rx_state,rx);
      if (rx_state==3)
        {
        if (rx_message[0]==0x01)
          {
          enter_progmode();
          usart_tx_b (0x81);
          rx_state = 0;
          }
        if (rx_message[0]==0x02)
          {
          exit_progmode();
          usart_tx_b (0x82);
          rx_state = 0;
          }
        if (rx_message[0]==0x03)
          {
          isp_reset_pointer();
          usart_tx_b (0x83);
          rx_state = 0;
          }
        if (rx_message[0]==0x04)
          {
          isp_send_config(0);
          usart_tx_b (0x84);
          rx_state = 0;
          }
        if (rx_message[0]==0x05)
          {
            for (i=0;i<rx_message[2];i++)
            {
            isp_inc_pointer();
            }
          usart_tx_b (0x85);
          rx_state = 0;
          }
        if (rx_message[0]==0x06)
          {
          usart_tx_b (0x86);
          isp_read_pgm(flash_buffer,rx_message[2]);
          for (i=0;i<rx_message[2];i++)
            {
            usart_tx_b (flash_buffer[i]&0xFF);
            usart_tx_b (flash_buffer[i]>>8);
            }
          rx_state = 0;
          }
        if (rx_message[0]==0x07)
          {
          isp_mass_erase();
          usart_tx_b (0x87);
          rx_state = 0;
          }
          
        if (rx_message[0]==0x08)
          {
          for (i=0;i<rx_message[2]/2;i++)
            {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+4]))<<8) + (((unsigned int)(rx_message[(2*i)+0+4]))<<0);
            }
          isp_write_pgm(flash_buffer,rx_message[2]/2,rx_message[3]);
          usart_tx_b (0x88);
          rx_state = 0;
          }
        if (rx_message[0]==0x09)
          {
          isp_reset_pointer_16d();
          usart_tx_b (0x89);
          rx_state = 0;
          }        
        if (rx_message[0]==0x10)
          {
          p18_enter_progmode();
          usart_tx_b (0x90);
          rx_state = 0;
          }
        if (rx_message[0]==0x11)
          {
          usart_tx_b (0x91);
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          p_18_isp_read_pgm (flash_buffer, addr, rx_message[2]);
          for (i=0;i<rx_message[2];i++)
            {
            usart_tx_b (flash_buffer[i]&0xFF);
            usart_tx_b (flash_buffer[i]>>8);
            }
          rx_state = 0;
          }
        if (rx_message[0]==0x12)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          for (i=0;i<rx_message[2]/2;i++)
            {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
            }
          p18_isp_write_pgm (flash_buffer, addr, rx_message[2]/2);
          usart_tx_b (0x92);
          rx_state = 0;
          }
        if (rx_message[0]==0x13)
          {
          p18_isp_mass_erase();
          usart_tx_b (0x93);
          rx_state = 0;
          }
        if (rx_message[0]==0x14)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          p18_isp_write_cfg (rx_message[6],rx_message[7], addr);
          usart_tx_b (0x94);
          rx_state = 0;
          }  
        if (rx_message[0]==0x23)
          {
          p18fj_isp_mass_erase();
          usart_tx_b (0xA3);
          rx_state = 0;
          }
        if (rx_message[0]==0x30)
          {
          p18fk_isp_mass_erase (rx_message[2], rx_message[3], rx_message[4]);
          usart_tx_b (0xB0);
          rx_state = 0;
          }
        if (rx_message[0]==0x31)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          for (i=0;i<rx_message[2]/2;i++)
            {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
            }
          p18fk_isp_write_pgm (flash_buffer, addr, rx_message[2]/2);
          usart_tx_b (0xB1);
          rx_state = 0;
          }
        if (rx_message[0]==0x32)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          p18fk_isp_write_cfg (rx_message[6],rx_message[7], addr);
          usart_tx_b (0xB2);
          rx_state = 0;
          }  
        if (rx_message[0]==0x40)
          {
          p16c_enter_progmode();
          usart_tx_b (0xC0);
          rx_state = 0;   
          }      
        if (rx_message[0]==0x41)
          {
          usart_tx_b (0xC1);
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          p16c_isp_read_pgm (flash_buffer, addr, rx_message[2]);
          for (i=0;i<rx_message[2];i++)
            {
            usart_tx_b (flash_buffer[i]&0xFF);
            usart_tx_b (flash_buffer[i]>>8);
            }
          rx_state = 0;
          }
        if (rx_message[0]==0x42)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          for (i=0;i<rx_message[2]/2;i++)
            {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
            }
          p16c_isp_write_pgm (flash_buffer, addr, rx_message[2]/2);
          usart_tx_b (0xC2);
          rx_state = 0;
          }
        if (rx_message[0]==0x43)
          {
          p16c_set_pc (0x8000);
          p16c_bulk_erase ();
          usart_tx_b (0xC3);
          rx_state = 0;
          }         
        if (rx_message[0]==0x44)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          cfg_val = rx_message[6];
          cfg_val = (cfg_val<<8) + rx_message[7];
          p16c_isp_write_cfg (cfg_val, addr);
          usart_tx_b (0xC4);
          rx_state = 0;
          }                      
        if (rx_message[0]==0x45)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          cfg_val = rx_message[6];
          cfg_val = (cfg_val<<8) + rx_message[7];
          p18q_isp_write_cfg (cfg_val, addr);
          usart_tx_b (0xC5);
          rx_state = 0;
          } 
        if (rx_message[0]==0x46)
          {
          addr = (((unsigned long)(rx_message[3]))<<16) + (((unsigned long)(rx_message[4]))<<8) + (((unsigned long)(rx_message[5]))<<0);
          for (i=0;i<rx_message[2]/2;i++)
            {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+6]))<<8) + (((unsigned int)(rx_message[(2*i)+0+6]))<<0);
            }
          p18q_isp_write_pgm (flash_buffer, addr, rx_message[2]/2);
          usart_tx_b (0xC6);
          rx_state = 0;
          }        
        }
      }      
    }
}



unsigned char rx_state_machine (unsigned char state, unsigned char rx_char)
{
if (state==0)
  {
    rx_message_ptr = 0;
    rx_message[rx_message_ptr++] = rx_char;
    return 1;
  }
if (state==1)
  {
    bytes_to_receive = rx_char;
    rx_message[rx_message_ptr++] = rx_char;
    if (bytes_to_receive==0) return 3;
    return 2;
  }
if (state==2)
  {
    rx_message[rx_message_ptr++] = rx_char;
    bytes_to_receive--;
    if (bytes_to_receive==0) return 3;
  }
return state;  
}


void isp_read_pgm (unsigned int * data, unsigned char n)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<n;i++)  
  {
  isp_send(0x04,6);
  data[i] = isp_read_14s();
  isp_send(0x06,6);
  }
}




void isp_write_pgm (unsigned int * data, unsigned char n, unsigned char slow)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<n;i++)  
  {
  isp_send(0x02,6);
  isp_send(data[i]<<1,16);  
  if (i!=(n-1))
    isp_send(0x06,6);
  }
isp_send(0x08,6);
if (slow==1)
  _delay_ms(5);
else
  _delay_ms(3);
isp_send(0x06,6);
}

void isp_send_config (unsigned int data)
{
isp_send(0x00,6);
isp_send(data,16);
}

void isp_mass_erase (void)
{
//_delay_ms(10);
//_delay_us(3*ISP_CLK_DELAY);
//isp_send(0x11,6);
isp_send_config(0);
isp_send(0x09,6);
_delay_ms(10);
//isp_send(0x0B,6);
//_delay_ms(10);
}



void isp_reset_pointer (void)
{
//_delay_us(3*ISP_CLK_DELAY);
isp_send(0x16,6);
}

void isp_reset_pointer_16d (void)
{
//_delay_us(3*ISP_CLK_DELAY);
isp_send(0x1D,6);
isp_send(0x0,8);
isp_send(0x0,8);
isp_send(0x0,8);
}

void isp_inc_pointer (void)
{
//_delay_us(3*ISP_CLK_DELAY);
isp_send(0x06,6);
}


unsigned int isp_read_16 (void)
{
unsigned char i;
unsigned int out;
out = 0;
ISP_DAT_D_I
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<16;i++)
  {
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_0
  _delay_us(ISP_CLK_DELAY);
  out = out >> 1;
  if (ISP_DAT_V)
    out = out | 0x8000;
  }
 return out;
}

unsigned int isp_read_8 (void)
{
unsigned char i;
unsigned int out;
out = 0;
ISP_DAT_D_I
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<8;i++)
  {
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_0
  _delay_us(ISP_CLK_DELAY);
  out = out >> 1;
  if (ISP_DAT_V)
    out = out | 0x80;
  }
 return out;
}

unsigned int isp_read_14s (void)
{
unsigned char i;
unsigned int out;
out = isp_read_16();
out = out &0x7FFE;
out = out >> 1;
return out;
}



void isp_send (unsigned int data, unsigned char n)
{
unsigned char i;
ISP_DAT_D_0
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<n;i++)
  {
  if (data&0x01)
    {
    ISP_DAT_1
    }
  else
    {
    ISP_DAT_0
    }
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_1
//  _delay_us(ISP_CLK_DELAY);
  data = data >> 1;
  ISP_CLK_0
  ISP_DAT_0
//  _delay_us(ISP_CLK_DELAY);
  }
}


void isp_send_24_msb (unsigned long data)
{
unsigned char i;
ISP_DAT_D_0
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<23;i++)
  {
  if (data&0x400000)
    {
    ISP_DAT_1
    }
  else
    {
    ISP_DAT_0
    }
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_1
 _delay_us(ISP_CLK_DELAY);
  data = data << 1;
  ISP_CLK_0
//  _delay_us(ISP_CLK_DELAY);
  }
  ISP_DAT_0
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_0
}

void isp_send_8_msb (unsigned char data)
{
unsigned char i;
ISP_DAT_D_0
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<8;i++)
  {
  if (data&0x80)
    {
    ISP_DAT_1
    }
  else
    {
    ISP_DAT_0
    }
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
  data = data << 1;
  ISP_CLK_0
  ISP_DAT_0
//  _delay_us(ISP_CLK_DELAY);
  }
}


unsigned int isp_read_8_msb (void)
{
unsigned char i;
unsigned int out;
out = 0;
ISP_DAT_D_I
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<8;i++)
  {
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_0
  _delay_us(ISP_CLK_DELAY);
  out = out << 1;
  if (ISP_DAT_V)
    out = out | 0x1;
  }
 return out;
}

unsigned int isp_read_16_msb (void)
{
unsigned char i;
unsigned int out;
out = 0;
ISP_DAT_D_I
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<16;i++)
  {
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_0
  _delay_us(ISP_CLK_DELAY);
  out = out << 1;
  if (ISP_DAT_V)
    out = out | 0x1;
  }
 return out;
}



unsigned char enter_progmode (void)
{
ISP_MCLR_0
_delay_us(300);
isp_send(0b01010000,8);
isp_send(0b01001000,8);
isp_send(0b01000011,8);
isp_send(0b01001101,8);

isp_send(0,1);

}

/**************************************************************************************************************************/

unsigned char p18_enter_progmode (void)
{
ISP_MCLR_0
_delay_us(300);
isp_send(0b10110010,8);
isp_send(0b11000010,8);
isp_send(0b00010010,8);
isp_send(0b00001010,8);
_delay_us(300);
ISP_MCLR_1
}


void p18_isp_mass_erase (void)
{
p18_set_tblptr(0x3C0005);
p18_send_cmd_payload(0x0C,0x0F0F);
p18_set_tblptr(0x3C0004);
p18_send_cmd_payload(0x0C,0x8F8F);
p18_send_cmd_payload(0,0x0000);
isp_send(0x00,4);
_delay_ms(20);
isp_send(0x00,16);
}

void p18fj_isp_mass_erase (void)
{
p18_set_tblptr(0x3C0005);
p18_send_cmd_payload(0x0C,0x0101);
p18_set_tblptr(0x3C0004);
p18_send_cmd_payload(0x0C,0x8080);
p18_send_cmd_payload(0,0x0000);
isp_send(0x00,4);
_delay_ms(600);
isp_send(0x00,16);
}


void p18fk_isp_mass_erase (unsigned char data1, unsigned char data2, unsigned char data3)
{
unsigned int tmp1, tmp2, tmp3;
tmp1 = data1;
tmp1 = (tmp1<<8)|data1;
tmp2 = data2;
tmp2 = (tmp2<<8)|data2;
tmp3 = data3;
tmp3 = (tmp3<<8)|data3;
p18_set_tblptr(0x3C0004);
p18_send_cmd_payload(0x0C,tmp3);
p18_set_tblptr(0x3C0005);
p18_send_cmd_payload(0x0C,tmp2);
p18_set_tblptr(0x3C0006);
p18_send_cmd_payload(0x0C,tmp1);
p18_send_cmd_payload(0x00,0);
isp_send(0x00,4);
_delay_ms(5);
isp_send(0x00,16);
}

void p18fk_isp_write_pgm (unsigned int * data, unsigned long addr, unsigned char n)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
p18_send_cmd_payload(0,0x8E7F);
p18_send_cmd_payload(0,0x9C7F);
p18_send_cmd_payload(0,0x847F);
p18_set_tblptr(addr);
for (i=0;i<n-1;i++)  
  p18_send_cmd_payload(0x0D,data[i]);  
p18_send_cmd_payload(0x0F,data[n-1]);  
p_18_modfied_nop(0);
}

void p18_isp_write_pgm (unsigned int * data, unsigned long addr, unsigned char n)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
p18_send_cmd_payload(0,0x8EA6);
p18_send_cmd_payload(0,0x9CA6);
p18_send_cmd_payload(0,0x84A6);
p18_set_tblptr(addr);
for (i=0;i<n-1;i++)  
  p18_send_cmd_payload(0x0D,data[i]);  
p18_send_cmd_payload(0x0F,data[n-1]);  
p_18_modfied_nop(1);
}

void p18_isp_write_cfg (unsigned char data1, unsigned char data2, unsigned long addr)
{
unsigned int i;
//_delay_us(3*ISP_CLK_DELAY);
p18_send_cmd_payload(0,0x8EA6);
p18_send_cmd_payload(0,0x8CA6);
p18_send_cmd_payload(0,0x84A6);
p18_set_tblptr(addr);
p18_send_cmd_payload(0x0F,data1);  
p_18_modfied_nop(1);
_delay_ms(5);
p18_set_tblptr(addr+1);
i = data2;
i = i << 8;
p18_send_cmd_payload(0x0F,i);  
p_18_modfied_nop(1);
_delay_ms(5);
}

void p18fk_isp_write_cfg (unsigned char data1, unsigned char data2, unsigned long addr)
{
unsigned int i;
//_delay_us(3*ISP_CLK_DELAY);
p18_send_cmd_payload(0,0x8E7F);
p18_send_cmd_payload(0,0x8C7F);
p18_set_tblptr(addr);
p18_send_cmd_payload(0x0F,data1);  
p_18_modfied_nop(1);
_delay_ms(5);
p18_set_tblptr(addr+1);
i = data2;
i = i << 8;
p18_send_cmd_payload(0x0F,i);  
p_18_modfied_nop(1);
_delay_ms(5);
}

void p_18_modfied_nop (unsigned char nop_long)
{
unsigned char i;
ISP_DAT_D_0
ISP_DAT_0
for (i=0;i<3;i++)
  {
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
   ISP_CLK_0
  }
_delay_us(ISP_CLK_DELAY);
ISP_CLK_1
if (nop_long==1) _delay_ms(4);
_delay_ms(1);
ISP_CLK_0
_delay_us(ISP_CLK_DELAY);
isp_send(0x00,16);
}

void p_18_isp_read_pgm (unsigned int * data, unsigned long addr, unsigned char n)
{
unsigned char i;
unsigned int tmp1,tmp2;
//_delay_us(3*ISP_CLK_DELAY);
p18_set_tblptr(addr);
for (i=0;i<n;i++)  
  {
  tmp1 =  p18_read_pgm_byte();
  tmp2 =  p18_read_pgm_byte();
  tmp2 = tmp2<<8;
  data[i] = tmp1|tmp2;
  }
}


void p18_set_tblptr (unsigned long val)
{
  p18_send_cmd_payload(0,0x0E00|((val>>16)&0xFF));
  p18_send_cmd_payload(0,0x6EF8);
  p18_send_cmd_payload(0,0x0E00|((val>>8)&0xFF));
  p18_send_cmd_payload(0,0x6EF7);
  p18_send_cmd_payload(0,0x0E00|((val>>0)&0xFF));
  p18_send_cmd_payload(0,0x6EF6);
  }


unsigned char p18_read_pgm_byte (void)
{
  isp_send(0x09,4);
  isp_send(0x00,8);
  return isp_read_8();
  }

unsigned int p18_get_ID (void)
{
  unsigned int temp;

  p18_set_tblptr(0x3FFFFE);
  temp = p18_read_pgm_byte();
  temp = temp << 8;
  temp = temp | p18_read_pgm_byte();
  return temp;
  }

void p18_send_cmd_payload (unsigned char cmd, unsigned int payload)
{
  isp_send(cmd,4);
  isp_send(payload,16);
  _delay_us(30);
  }

unsigned int p18_get_cmd_payload (unsigned char cmd)
{
  isp_send(cmd,4);
  return isp_read_16();
  }


unsigned char exit_progmode (void)
{
ISP_MCLR_1
_delay_ms(30);
ISP_MCLR_0
_delay_ms(30);
ISP_MCLR_1
}

//***********************************************************************************//

unsigned char p16c_enter_progmode (void)
{
ISP_MCLR_0
_delay_us(300);
isp_send_8_msb(0x4d);
isp_send_8_msb(0x43);
isp_send_8_msb(0x48);
isp_send_8_msb(0x50);
_delay_us(300);
}

void p16c_set_pc (unsigned long pc)
{
  isp_send_8_msb(0x80);
  _delay_us(2);
  isp_send_24_msb(pc);

}

void p16c_bulk_erase (void)
{
  isp_send_8_msb(0x18);
  _delay_ms(100);
}

void p16c_load_nvm (unsigned int data, unsigned char inc)
{
  if (inc==0) isp_send_8_msb(0x00);
  else isp_send_8_msb(0x02);
  _delay_us(2);
  isp_send_24_msb(data);
  _delay_us(2);
  }

unsigned int p16c_read_data_nvm (unsigned char inc)
{
  unsigned int retval;
  unsigned char tmp;
  if (inc==0) isp_send_8_msb(0xFC);
  else isp_send_8_msb(0xFE);
  _delay_us(2);
  tmp = isp_read_8_msb();
  retval = isp_read_16_msb();
  retval = retval >> 1;
  if (tmp&0x01) retval = retval | 0x8000;
  return retval;
  }

void p16c_begin_prog (unsigned char cfg_bit)
{
  isp_send_8_msb(0xE0);
  _delay_ms(3);  
  if (cfg_bit!=0) _delay_ms(3); 
  }

unsigned int p16c_get_ID (void)
{
  p16c_set_pc(0x8006);
  return p16c_read_data_nvm(1);
  }
  
void p16c_isp_write_pgm (unsigned int * data, unsigned long addr, unsigned char n)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
p16c_set_pc(addr);
for (i=0;i<n;i++)  
  p16c_load_nvm(data[i],1);  
p16c_set_pc(addr);
p16c_begin_prog(0);
}

void p16c_isp_read_pgm (unsigned int * data, unsigned long addr, unsigned char n)
{
unsigned char i;
unsigned int tmp1,tmp2;
//_delay_us(3*ISP_CLK_DELAY);
p16c_set_pc(addr);
for (i=0;i<n;i++)  
  data[i] = p16c_read_data_nvm(1);
}

void p16c_isp_write_cfg (unsigned int data, unsigned long addr)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
p16c_set_pc(addr);
p16c_load_nvm(data,0);  
p16c_begin_prog(1);
}

void p18q_isp_write_pgm (unsigned int * data, unsigned long addr, unsigned char n)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
p16c_set_pc(addr);
for (i=0;i<n;i++)
  {  
  isp_send_8_msb(0xE0);  
  isp_send_24_msb(data[i]);  
  _delay_us(65);
  }
}

void p18q_isp_write_cfg (unsigned int data, unsigned long addr)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
p16c_set_pc(addr);
isp_send_8_msb(0xE0);  
isp_send_24_msb(data);  
_delay_us(65); 
}


void usart_tx_b(uint8_t data)
{
while (!(UCSR0A & _BV(UDRE0)));
UDR0 = data;
} 

void usart_tx_s(uint8_t * data)
{
while (*data!=0) 
  usart_tx_b(*data++);
} 


uint8_t usart_rx_rdy(void)
{
if (UCSR0A & _BV(RXC0))
  return 1;
else
  return 0;
}

uint8_t usart_rx_b(void)
{
return (uint8_t) UDR0;
} 


void usart_tx_hexa_8 (uint8_t value)
{
uint8_t temp;
temp = value;
usart_tx_b('0');
usart_tx_b('x');
usart_tx_hexa_8b(value);
usart_tx_b(' ');
}

void usart_tx_hexa_8b (uint8_t value)
{
uint8_t temp;
temp = value;
temp = ((temp>>4)&0x0F);
if (temp<10) temp = temp + '0';
else temp = temp + 'A'- 10;
usart_tx_b(temp);
temp = value;
temp = ((temp>>0)&0x0F);
if (temp<10) temp = temp + '0';
else temp = temp + 'A' - 10;
usart_tx_b(temp);
}


void usart_tx_hexa_16 (uint16_t value)
{
  usart_tx_b('0');
usart_tx_b('x');
  usart_tx_hexa_8b((value>>8)&0xFF);
  usart_tx_hexa_8b(value&0xFF);
  usart_tx_b(' ');
}




