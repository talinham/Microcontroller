
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>
#include <usart.h>
#include <string.h>


#include "Main.h"
#include "I2C_Support.h"
#include "I2C_Soft.h"
#include "TFT_ST7735.h"
#include "Interrupt.h"
#include "Main_Screen.h"

#pragma config OSC      =   INTIO67
#pragma config BOREN    =   OFF
#pragma config WDT      =   OFF
#pragma config LVP      =   OFF
#pragma config CCP2MX   =   PORTBE

#define OFF 0                                           // Defines OFF as decimal value 0
#define RED 1                                           // Defines RED as decimal value 1
#define GREEN 2                                         // Defines GREEN as decimal value 2
#define YELLOW 3                                        // Defines YELLOW as decimal value 3
#define BLUE 4                                          // Defines BLUE as decimal value 4
#define PURPLE 5                                        // Defines PURPLE as decimal value 5
#define CYAN 6                                          // Defines CYAN as decimal value 3
#define WHITE 7                                         // Defines YELLOW as decimal value 3

void Initialize_Screen(void);                           // function prototypes
void Update_Screen(void);
void Do_Init(void);
float read_volt();
int get_duty_cycle(int,int);
int get_RPM(); 
void Monitor_Fan();
void Turn_Off_Fan();
void Turn_On_Fan();
unsigned int get_full_ADC();
void Get_Temp(void);
void Update_Volt(void);
void Test_Alarm(void);
void Activate_Buzzer();
void Deactivate_Buzzer();

void Main_Screen(void);
void Do_Setup(void);
void do_update_pwm(char);
void Set_RGB_Color(char color);

char buffer[31]         = " ECE3301L Fall'20 L12\0";    // text storage for title text
char *nbr;
char *txt;
char tempC[]            = "+25";                        // text storage for tempC
char tempF[]            = "+77";                        // text storage for tempF
char time[]             = "00:00:00";                   // text storage for time
char date[]             = "00/00/00";                   // text storage for date
char alarm_time[]       = "00:00:00";                   // text storage for alarm time
char Alarm_SW_Txt[]     = "OFF";                        // text storage for alarm mode
char Fan_SW_Txt[]       = "OFF";                        // text storage for Heater Mode
char Fan_Set_Temp_Txt[] = "075F";                       // text for fan setup temp
char Volt_Txt[]         = "0.00V";                      // text storage for Volt     
char DC_Txt[]           = "000";                        // text storage for Duty Cycle value
char RTC_ALARM_Txt[]    = "0";                          // text storage for alarm and clock match 
char RPM_Txt[]          = "0000";                       // text storage for RPM

char setup_time[]       = "00:00:00";                   // text storage for setup time
char setup_date[]       = "01/01/00";                   // text storage for setup date
char setup_alarm_time[] = "00:00:00";                   // text storage for setup alarm time
char setup_fan_text[]   = "075F";                       // text storage for fan setup

signed int DS1621_tempC, DS1621_tempF;                  // defining variables

int INT0_flag, INT1_flag, INT2_flag, Tach_cnt;
int ALARMEN;
int FANEN,i;
int alarm_mode, MATCHED, color;
unsigned char second, minute, hour, dow, day, month, year, old_sec;
unsigned char alarm_second, alarm_minute, alarm_hour, alarm_date;
unsigned char setup_alarm_second, setup_alarm_minute, setup_alarm_hour;
unsigned char setup_second, setup_minute, setup_hour, setup_day, setup_month, setup_year;
unsigned char setup_fan_temp = 75;
float volt;
int duty_cycle;
int rpm;

int Tach_cnt = 0;		

void putch (char c)
{   
    while (!TRMT);       
    TXREG = c;
}

void init_UART()                                        // functions to display on TeraTerm
{
    OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 25);
    OSCCON = 0x70;
}

void Init_ADC()         
{
    ADCON0 = 0x01;                                     // select channel AN0, and turn on the ADC subsystem               
    ADCON1= 0x0E;                                      // select pins AN0 through AN3 as analog signal
    ADCON2= 0xA9;                                      // right justify the result. Set the bit conversion time (TAD) and  acquisition time
}

void Init_IO()
{
    TRISA = 0xFF;                                      // setting TRISA as input 
    TRISB = 0x07;                                      // setting TRISB as input and outputs 
    TRISC = 0x01;                                      // setting TRISC as input and outputs
    TRISD = 0x00;                                      // setting TRISD as output
    TRISE = 0x00;                                      // setting TRISE as output
}

void Do_Init()                                         // Initialize the ports 
{ 
    init_UART();                                       // Initialize the uart
    OSCCON = 0x70;                                     // Set oscillator to 8 MHz 
    Init_ADC();                                        // calls function Init_ADC
    Init_IO();                                         // calls function Init_IO
    RBPU = 0;                                          // PORTB enable bit, pull up
       
    TMR1L = 0x00;                                      // clear the count                           
    T1CON = 0x03;                                      // start Timer1 as counter of number of pulses
 
    T0CON = 0x03;                                      // Timer 0, 16-bit mode, prescaler 1:16
                                               
    TMR0L = 0xDB;                                      // set the lower byte of TMR                         
    TMR0H = 0x0B;                                      // set the higher bytes of TMR
    INTCONbits.TMR0IF = 0;                             // Clear the interrupt flag        
    T0CONbits.TMR0ON = 1;                              // Enable the timer 0         
    INTCONbits.TMR0IE = 1;                             // enable timer 0 interrupt        
    Init_Interrupt();                                  // initialize the other interrupts
    
    I2C_Init(100000);                                  // calls function I2C_Init      
    DS1621_Init();                                     // calls function DS1621_Init

} 

void main()                                            // main starts here
{
    Do_Init();                                         // Initialization    

    txt = buffer;     

    Initialize_Screen();

    old_sec = 0xff;                                    // initializing old sec
    Turn_Off_Fan();                                    // turn fan off
    DS3231_Write_Initial_Alarm_Time();                 // uncommented this line if alarm time was corrupted    
    DS3231_Read_Time();                                // Read time for the first time
    DS3231_Read_Alarm_Time();                          // Read alarm time for the first time
    DS3231_Turn_Off_Alarm();                           // turn alarm off

    while(TRUE)
    { 
        if (enter_setup == 0)                          // If setup switch is LOW...
        {
            Main_Screen();                             // stay on main screen.
        }
        else                                           // Else,
        {
            Do_Setup();                                // Go to setup screen.
        }
    }
}

void Main_Screen()
{
    if (INT0_flag == 1)                                // check INT0 flag
        {
            INT0_flag=0;                               // set the flag to zero
            printf("INT0 interrupt pin detected \r\n");  // print on TeraTerm that interrupt has occured
            Turn_Off_Fan();                            // calls Turn_Off_Fan function
           
        }
    if (INT1_flag == 1)                                // check INT1 flag
        {
            INT1_flag=0;                               // set the flag to zero
            printf("INT1 interrupt pin detected \r\n");  // print on TeraTerm that interrupt has occured
            Turn_On_Fan();                             // calls Turn_on_Fan function
            
        }
    if (INT2_flag ==1)                                // check INT2 flag 
    {
        INT2_flag=0;                                  // set INT2 flag to zero
        ALARMEN = (ALARMEN + 1) % 2;                  // calculating alarm on and off mode
        
    }
   
        
    DS3231_Read_Time();                              // Read time
    if (old_sec != second)                           // check the second
    {
        old_sec = second;                            // if not equal old sec is new second
        
        Get_Temp();                                  // calls get temp function
        volt=read_volt();                            // read the light sensor light's voltage and store it into the variable 'volt'
        
        if ( FANEN ==1)                              // check FANEN 
        {
           Monitor_Fan();                            // if one monitor the fan
        }
        Test_Alarm();                                // calls test alarm to check the alarm

        printf ("%02x:%02x:%02x %02x/%02x/%02x ",hour,minute,second,month,day,year);
        printf ("duty cycle = %d  RPM = %d ", duty_cycle, rpm); 

        Update_Screen();
    }    
}

void Do_Setup()
{
    
       if (setup_sel0 == 0 && setup_sel1==0 )       // if switch is 00 go to setup time
        {
            Setup_Time();
 
        }
        if(setup_sel1 == 0 && setup_sel0==1)        // if switch is 01 go to setup alarm time
        {
            Setup_Alarm_Time();
            
        }
        if((setup_sel1 == 1 && setup_sel0==0) || (setup_sel1 == 1 && setup_sel0==1))  // if switch is 01 or 11 go to setup fan
        {
            Setup_Temp_Fan();
        }
        
}

void Get_Temp(void)
{
    DS1621_tempC = DS1621_Read_Temp();              // Read temp


    if ((DS1621_tempC & 0x80) == 0x80)
    {
        DS1621_tempC = 0x80 - (DS1621_tempC & 0x7f);
        DS1621_tempF = 32 - DS1621_tempC * 9 /5;
        printf ("Temperature = -%dC or %dF\r\n", DS1621_tempC, DS1621_tempF);
        DS1621_tempC = 0x80 | DS1621_tempC;            
    }
    else
    {
        DS1621_tempF = DS1621_tempC * 9 /5 + 32;
        printf ("Temperature = %dC or %dF\r\n", DS1621_tempC, DS1621_tempF);            
    }
}

void Monitor_Fan()
{
    duty_cycle = get_duty_cycle(DS1621_tempF, setup_fan_temp);          // call get_duty_cycle function and store the value in duty_cycle variable
    do_update_pwm(duty_cycle);                                          // update pwm with the duty cycle 
    rpm = get_RPM();                                                    // call get_RPM() function and store the value in rpm variable
}

float read_volt()
{
	int nStep = get_full_ADC();                                         // calculates the # of steps for analog conversion
    volt = nStep * 5 /1024.0;                                           // gets the voltage in Volts, using 5V as reference s instead of 4, also divide by 1024 
	return (volt);                                                      // return the volt value
}

int get_duty_cycle(int temp, int set_temp)
{	
    int dc = (int) (set_temp-temp)*2;                                   // calculate the duty cycle value by subtracting two temperatures and multiply by 2
    if (dc>100)                                                 
    {
        dc=100;                                                         // if duty cycle > 100, force to be 100
    }
    if (dc<0)
    {
        dc=0;                                                           // if duty cycle < 0, force to be zero
    }
    return (dc);                                                        // return the duty cycle value

}

int get_RPM()
{
    int RPS = Tach_cnt;                                                 // read the count. Since there are 2 pulses per rev
    return (RPS * 60);                                                  // return RPM = 60 * RPS 
}


void Turn_Off_Fan()
{
    duty_cycle = 0;                                                     // sets duty_cycle variable to zero
    do_update_pwm(duty_cycle);                                          // calls the do_update_pwm function to set to zero
    rpm = 0;                                                            // sets rpm variable to zero
    FANEN = 0;                                                          // sets fan enable variable to zero
    FANEN_LED = 0;                                                      // sets fan LED to off
 
}

void Turn_On_Fan()
{
    FANEN = 1;                                                          // sets fan enable variable to 1
    FANEN_LED = 1;                                                      // sets fan LED to on 
}

void do_update_pwm(char duty_cycle) 
{ 
    float dc_f;
    int dc_I;
    PR2 = 0b00000100 ;                                                  // set the frequency for 25 Khz
    T2CON = 0b00000111 ;                             
    dc_f = ( 4.0 * duty_cycle / 20.0) ;                                 // calculate factor of duty cycle versus a 25 Khz signal
    dc_I = (int) dc_f;                                                  // get the integer part
    if (dc_I > duty_cycle) dc_I++;                                      // round up function
    CCP1CON = ((dc_I & 0x03) << 4) | 0b00001100;
    CCPR1L = (dc_I) >> 2;
}

unsigned int get_full_ADC()
{
    unsigned int result;
    ADCON0bits.GO=1;                                                    // Start Conversion
    while(ADCON0bits.DONE==1);                                          // wait for conversion to be completed
    result = (ADRESH * 0x100) + ADRESL;                                 // combine result of upper byte and lower byte into result                                              // lower byte into result
    return (result);                                                    // return the result.
}

void Activate_Buzzer()                                                 // function activates the buzzer   
{
    PR2 = 0b11111001 ;
    T2CON = 0b00000101 ;
    CCPR2L = 0b01001010 ;
    CCP2CON = 0b00111100 ;
}

void Deactivate_Buzzer()                                               // function deactivates the buzzer
{
    CCP2CON = 0x0;
    PORTBbits.RB3 = 0;
}

void Test_Alarm()
{
    
    if (ALARMEN==1 && alarm_mode ==0)                                 // check ALARMEN and alarm_mode
    {
        alarm_mode==1;                                                // if ALARMEN is one then alarm_mode should be one                                      
        DS3231_Turn_On_Alarm();                                       // set alarm to on
    }
    if (ALARMEN==0 && alarm_mode ==1 )                                // check ALARMEN and alarm_mode
    {
        alarm_mode==0;                                                // if ALARMEN is zero then alarm_mode should be zero                                                
        DS3231_Turn_Off_Alarm();                                      // set alarm to off
        MATCHED = 0;                                                  // clear matched variable
        Deactivate_Buzzer();                                          // call Deactivate_Buzzer to turn off buzzer
        Set_RGB_Color(0);                                             // set the LED to off
    } 
    if(ALARMEN==1 && alarm_mode ==1 )                                 // check ALARMEN and alarm_mode
    {
        
        if (RTC_ALARM_NOT ==0)                                        // if RTC_ALARM_NOT is low  
        {
            MATCHED = 1;                                              // then the alarm is matched with time
            i=1;                                                      // variable i to count the RGB colros
            Set_RGB_Color(0);                                         // set the LED to off
            Activate_Buzzer();                                        // call Activate_Buzzer to turn on buzzer
        }

        if (read_volt()>3.5)                                          // check the voltage on light sensor 
        {
            MATCHED = 0;                                              // if >3.5 deactivate the alarm set match to zero
            Deactivate_Buzzer();                                      // call Deactivate_Buzzer to turn off buzzer
            Set_RGB_Color(0);                                         // set the led to off
            DS3231_Turn_Off_Alarm();                                  // set alarm to off
        }
        else if (MATCHED ==1)                                         // check MATCHED if 1 
        {
            Set_RGB_Color(i);                                         // change the LED color every second
            i++;
            if (i>7)
            {
               i=0; 
            }
      
        }
    }
}

void Set_RGB_Color(char color)
{
    switch(color)
    {
        case OFF: D1_RED =0;D1_GREEN=0;D1_BLUE = 0;break;           // sets LED OFF
        case RED: D1_RED =1;D1_GREEN=0;D1_BLUE = 0;break;           // Sets LED RED
        case GREEN: D1_RED =0;D1_GREEN=1;D1_BLUE = 0;break;         // sets LED GREEN
        case YELLOW: D1_RED =1;D1_GREEN=1;D1_BLUE = 0;break;        // sets LED YELLOW
        case BLUE: D1_RED =0;D1_GREEN=0;D1_BLUE = 1;break;          // sets LED BLUE
        case PURPLE: D1_RED =1;D1_GREEN=0;D1_BLUE = 1;break;        // sets LED PURPLE
        case CYAN: D1_RED =0;D1_GREEN=1;D1_BLUE = 1;break;          // sets LED CYAN
        case WHITE: D1_RED =1;D1_GREEN=1;D1_BLUE = 1;break;         // sets LED WHITE
    }
}

