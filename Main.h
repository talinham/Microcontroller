
#define _XTAL_FREQ      8000000

#define ACK             1
#define NAK             0

#define ACCESS_CFG      0xAC
#define START_CONV      0xEE
#define READ_TEMP       0xAA
#define CONT_CONV       0x02

#define TFT_DC          PORTDbits.RD0
#define TFT_CS          PORTDbits.RD1
#define TFT_RST         PORTDbits.RD2

#define enter_setup     PORTAbits.RA1                 // defining bit 1 of PORTA as enter_setup 
#define setup_sel0      PORTAbits.RA4                 // defining bit 4 of PORTA as setup_sel0 
#define setup_sel1      PORTAbits.RA5                 // defining bit 5 of PORTA as setup_sel1 

#define SEC_LED         PORTDbits.RD6                 // defining bit 6 of PORTD as SEC_LED

#define FANEN_LED       PORTDbits.RD7                 // defining bit 7 of PORTD as FANEN_LED
#define RTC_ALARM_NOT   PORTEbits.RE0                 // defining bit 0 of PORTE as RTC_ALARM_NOT


#define D1_RED      PORTDbits.RD3                     // defining bit 3 of PORT D 
#define D1_GREEN    PORTDbits.RD4                     // defining bit 4 of PORT D
#define D1_BLUE     PORTDbits.RD5                     // defining bit 5 of PORT D
