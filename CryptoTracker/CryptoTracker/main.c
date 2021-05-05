/*
 * CryptoTracker.c
 *
 * Created: 4/27/2021 5:32:56 PM
 * Author : Cole Brooks, Thomas Butler
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

// USART stuff
#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) -1)

// LCD interface definitions
#define lcd_D7_port     PORTD                   // lcd D7 connection
#define lcd_D7_bit      PORTD7
#define lcd_D7_ddr      DDRD

#define lcd_D6_port     PORTD                   // lcd D6 connection
#define lcd_D6_bit      PORTD6
#define lcd_D6_ddr      DDRD

#define lcd_D5_port     PORTD                   // lcd D5 connection
#define lcd_D5_bit      PORTD5
#define lcd_D5_ddr      DDRD

#define lcd_D4_port     PORTD                   // lcd D4 connection
#define lcd_D4_bit      PORTD4
#define lcd_D4_ddr      DDRD

#define lcd_E_port      PORTB                   // lcd Enable pin
#define lcd_E_bit       PORTB1
#define lcd_E_ddr       DDRB

#define lcd_RS_port     PORTB                   // lcd Register Select pin
#define lcd_RS_bit      PORTB0
#define lcd_RS_ddr      DDRB

// LCD specific definitions
#define lcd_LineOne     0x00                    // start of line 1
#define lcd_LineTwo     0x40                    // start of line 2

// LCD instructions
#define lcd_Clear           0b00000001          // clears the screen
#define lcd_Home            0b00000010          // return cursor to first position on first line
#define lcd_EntryMode       0b00000110          // move cursor from left to right on read/write
#define lcd_DisplayOff      0b00001000          // turns display off
#define lcd_DisplayOn       0b00001100          // display on, cursor off, blink off
#define lcd_reset           0b00110000          // reset 
#define lcd_setTo4Bit       0b00101000          // sets to 4 bit data entry mode
#define lcd_SetCursor       0b10000000          // sets cursor position

// Prototypes
const char* get_string(char input_str[]);
uint16_t usart_rx(void);
void usart_init(void);
void lcd_write(uint8_t);
void lcd_write_instruction(uint8_t);
void lcd_write_char(uint8_t);
void lcd_write_str(uint8_t *);
void lcd_init(void);
void move_to_line_2(void);

/////////////////////////////////////////////////
// Function: move_to_line_2
// Purpose: moves cursor to line 2
/////////////////////////////////////////////////
int main(void)
{	
	// State variable
	int Display_Cryptos = 0;
	int Configure_Alarm = 1;
	int Read_From_Bt = 2;
	
	int state = Display_Cryptos;
	
	// storage variables
	int alarmPercent = 10; // default alarm to 10 percent change
	int currentCrypto = 1; // 0 denotes bitcoin, 1 denotes eth. Add more if more supported cryptos are added
	char cryptos[2][10] = { "Bitcoin",		// supported crypto names. Index = current crypto int
							"Ethereum"};
	char prices[2][10] = {"54802.80", "3286.23"}; 
	
	// configure the data lines for output to LCD
    lcd_D7_ddr |= (1<<lcd_D7_bit);
    lcd_D6_ddr |= (1<<lcd_D6_bit);
    lcd_D5_ddr |= (1<<lcd_D5_bit);
    lcd_D4_ddr |= (1<<lcd_D4_bit);

	// configure the data lines for controlling the LCD
    lcd_E_ddr |= (1<<lcd_E_bit);        // Enable
    lcd_RS_ddr |= (1<<lcd_RS_bit);    // Register Select

	// init lcd and usart
    lcd_init();
	usart_init();

	// Type welcome message
    lcd_write_str("Welcome to");
    move_to_line_2();
    lcd_write_str("CryptoTicker");
	
	// display welcome message for 5 seconds
	// and then clear the screen
	_delay_ms(5000);
	lcd_write_instruction(lcd_Clear);
	_delay_ms(80);
	
	int debug = 0;
    // program loop
    while(1){
		
		/*// debug loop
		while(1){
			char buffer[10];
			char input_str;
			uint16_t input = usart_rx();
			itoa(input, buffer, 10);
			input_str = atoi(buffer);
			
			lcd_write_instruction(lcd_Clear);
			_delay_ms(80);
			lcd_write_str(input_str);
			_delay_ms(2000);
		}*/
		while(1){
			char input_str[10] = {};
			get_string(input_str);
			
			lcd_write_str(input_str);
			_delay_ms(5000);
			lcd_write_instruction(lcd_Clear);
			_delay_ms(80);
		}
		
		if(state == Display_Cryptos){
			
			lcd_write_str(cryptos[currentCrypto]);
		    move_to_line_2();
		    lcd_write_str(prices[currentCrypto]);
		            
		    if(currentCrypto == 0)
		    {
			    currentCrypto = 1;
				debug = 3;
		    }else
		    {
			    currentCrypto = 0;
		    }
			if(debug == 3){
				state = Configure_Alarm;
			}
		}else if(state == Configure_Alarm){
			lcd_write_str("Set Alarm:");
			move_to_line_2();
			char snum[10];
			itoa(alarmPercent, snum, 10);
			lcd_write_str(snum);
			//_delay_ms(10000);
		}else if(state == Read_From_Bt){
			// read the new input
			char input_str[10] = {};
			char output_str[20];
			get_string(input_str);
			lcd_write_str(input_str);
			
		}
		      
		//_delay_ms(5000);
		lcd_write_instruction(lcd_Clear);
		_delay_ms(80);
    }
    return 0;
} ///////////////////// END OF MAIN //////////////////////////

/////////////////////////////////////////////////
// function: usart_init
// purpose: initialize usart.
// - set baud rate
// - enable rx and tx
// - set frame format 8 data, 2 stop bit
/////////////////////////////////////////////////
void usart_init(void)
{
	// Set baud rate
	UBRR0H = (uint8_t)(UBRR_VALUE>>8);
	UBRR0L = (uint8_t)UBRR_VALUE;
	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
	//enable transmission and reception
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}

/////////////////////////////////////////////////
// function: usart_rx
// purpose: receives data from bluetooth module
/////////////////////////////////////////////////
uint16_t usart_rx(void)
{
	// Wait for byte to be received
	while(!(UCSR0A&(1<<RXC0))){};
	// Return received data
	return UDR0;
}

/////////////////////////////////////////////////
// function: get_string
// purpose: gets string from bluetooth module
/////////////////////////////////////////////////
const char* get_string(char input_str[]){
	char buffer[10];
	uint16_t input = usart_rx();
	
	int i = 0;
	while (input != 10){
		itoa(input, buffer, 10);
		input_str[i] = atoi(buffer);
		i = i + 1;
		input = usart_rx();
	}
	
	return input_str;
}

/////////////////////////////////////////////////
// function: lcd_init
// purpose: initializes the LCD in 4-bit data mode
/////////////////////////////////////////////////
void lcd_init(void)
{
  // delay for a bit so hardware can do it's thing
    _delay_ms(100);    
                               
    // note we start in 8 bit mode, so we gotta change that
    // Set up the RS and E lines for the 'lcd_write' subroutine.
    lcd_RS_port &= ~(1<<lcd_RS_bit);
    lcd_E_port &= ~(1<<lcd_E_bit);                  

    // Setup the LCD
    lcd_write(lcd_reset); // first part of reset sequence
    _delay_ms(10);

    lcd_write(lcd_reset); // second part of reset sequence
    _delay_us(200);

    lcd_write(lcd_reset); // third part of reset sequence
    _delay_us(200); 
 
    lcd_write(lcd_setTo4Bit); // set 4-bit mode
    _delay_us(80);

    // Function Set instruction
    lcd_write_instruction(lcd_setTo4Bit); 
    _delay_us(80);  

    // Display On
    lcd_write_instruction(lcd_DisplayOff);        
    _delay_us(80); 

    // Clear Display
    lcd_write_instruction(lcd_Clear);            
    _delay_ms(4);                                  

    // Entry Mode Set instruction
    lcd_write_instruction(lcd_EntryMode);  
    _delay_us(80);                                  
 
    // Display On/Off Control instruction
    lcd_write_instruction(lcd_DisplayOn);         
    _delay_us(80);                                 
}

/////////////////////////////////////////////////
// Function: move_to_line_2
// Purpose: moves cursor to line 2
/////////////////////////////////////////////////
void move_to_line_2(void){
  lcd_write_instruction(lcd_SetCursor | lcd_LineTwo);
  _delay_us(80);                                  // 40 uS delay (min)
}

/////////////////////////////////////////////////
// function: lcd_write_str
// purpose: sends a string to the LCD to be 
// displayed
/////////////////////////////////////////////////
void lcd_write_str(uint8_t theString[])
{
    volatile int i = 0;                             // character counter*/
    while (theString[i] != 0)
    {
        lcd_write_char(theString[i]);
        i++;
        _delay_us(80);                              // 40 uS delay (min)
    }
}

/////////////////////////////////////////////////
// function: lcd_write_char
// purpose: send a byte nibble by nibble as
// a character
/////////////////////////////////////////////////
void lcd_write_char(uint8_t theData)
{
    lcd_RS_port |= (1<<lcd_RS_bit);                 // select the Data Register (RS high)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // make sure E is initially low
    lcd_write(theData);                           // write the upper 4-bits of the data
    lcd_write(theData << 4);                      // write the lower 4-bits of the data
}

/////////////////////////////////////////////////
// function: lcd_write_instruction
// purpose: send a byte nibble by nibble to
// the LCD as an instruction
/////////////////////////////////////////////////
void lcd_write_instruction(uint8_t theInstruction)
{
    lcd_RS_port &= ~(1<<lcd_RS_bit);                // select the Instruction Register (RS low)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // make sure E is initially low
    lcd_write(theInstruction);                    // write the upper 4-bits of the data
    lcd_write(theInstruction << 4);               // write the lower 4-bits of the data
}

/////////////////////////////////////////////////
// function: lcd_write
// purpose: send a byte nibble by nibble to the LCD
/////////////////////////////////////////////////
void lcd_write(uint8_t theByte)
{
    lcd_D7_port &= ~(1<<lcd_D7_bit);                        // assume that data is '0'
    if (theByte & 1<<7) lcd_D7_port |= (1<<lcd_D7_bit);     // make data = '1' if necessary

    lcd_D6_port &= ~(1<<lcd_D6_bit);                        // repeat for each data bit
    if (theByte & 1<<6) lcd_D6_port |= (1<<lcd_D6_bit);

    lcd_D5_port &= ~(1<<lcd_D5_bit);
    if (theByte & 1<<5) lcd_D5_port |= (1<<lcd_D5_bit);

    lcd_D4_port &= ~(1<<lcd_D4_bit);
    if (theByte & 1<<4) lcd_D4_port |= (1<<lcd_D4_bit);

    lcd_E_port |= (1<<lcd_E_bit);                   // Enable pin high
    _delay_us(1);                                   // implement 'Data set-up time' (80 nS) and 'Enable pulse width' (230 nS)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // Enable pin low
    _delay_us(1);                                   // implement 'Data hold time' (10 nS) and 'Enable cycle time' (500 nS)
}