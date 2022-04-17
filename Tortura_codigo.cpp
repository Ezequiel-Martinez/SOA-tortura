// C++ code
// Includes
#include <stdio.h>
#include <Keypad.h>			//para el keypad
#include <LiquidCrystal.h>	//para el lcd

//Defines
#define NUM_ROWS_LCD 3			//cantidad de filas que usamos en el keypad
#define NUM_COLS_LCD 3			//cantidad de columnas que usamos en el keypad
#define NUM_ROWS_KEYPAD 3		//cantidad de filas que usamos en el keypad
#define NUM_COLS_KEYPAD 3		//cantidad de columnas que usamos en el keypad
#define RANDOM_NUM_LENGTH 11	//cantidad de digitos que queremos que tenga el numero random ( contemplando el '\0')


//Variables globales
LiquidCrystal lcd(1,2,3,4,5,6);	//los parametros son los pines a los cuales esta conectado el lcd

//mapa de teclas que vamos a usar del keypad
char key_map[NUM_ROWS_KEYPAD][NUM_COLS_KEYPAD] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
};

byte row_pins[NUM_ROWS_KEYPAD] = {12,11,10}; 	//pines a los cuales conectamos las filas del keypad
byte col_pins[NUM_COLS_KEYPAD] = {9,8,7}; 		//pines a los cuales conectamos las columnas del keypad

Keypad keypad = Keypad(makeKeymap(key_map),row_pins,col_pins,NUM_ROWS_KEYPAD,NUM_COLS_KEYPAD);

//Prototipos
void read_keypad_input(void);
void write_text_on_lcd(char *text);
void generate_random_number(char *buffer);

void setup()
{
  randomSeed(analogRead(A0));
  lcd.begin(NUM_ROWS_LCD,NUM_COLS_LCD);
}

void loop()
{
 
  char random_number_buffer[RANDOM_NUM_LENGTH];
  
  generate_random_number(random_number_buffer);
  write_text_on_lcd(random_number_buffer);
  
}

void generate_random_number(char *buffer){

  long random_number;
  
  random_number = random(0,10000000000);
  
  for(int i = 0; i < RANDOM_NUM_LENGTH; i++){
  	*(buffer + i) = random_number % 10 + '0';
    random_number /= 10;
  }
  
  *(buffer + RANDOM_NUM_LENGTH - 1) = '\0';

}

void write_text_on_lcd(char *text){

  lcd.setCursor(3,0);
  lcd.print(text);
  //delay(500);
  lcd.clear();
  
}

void read_keypad_input(void){

  
  
}