// C++ code
// Includes
#include <Keypad.h>			//para el keypad
#include <LiquidCrystal.h>	//para el lcd

//Defines
//#define DEBUG_MODE				//para hacer debug por consola
#define NUM_ROWS_LCD 3			//cantidad de filas que usamos en el keypad
#define NUM_COLS_LCD 3			//cantidad de columnas que usamos en el keypad
#define NUM_ROWS_KEYPAD 3		//cantidad de filas que usamos en el keypad
#define NUM_COLS_KEYPAD 3		//cantidad de columnas que usamos en el keypad
#define RANDOM_NUM_LENGTH 11	//cantidad de digitos que queremos que tenga el numero random ( contemplando el '\0')
#define BUZZER_COOLDOWN 1000	//cantidad de tiempo minimo que debe pasar entre un button_game y otro ( es un cooldown ) medido en milisegundos
#define MAX_BUTTON_PRESS_DELAY 10 //cantidad de tiempo que maximo que tiene el usuario para presionar el boton luego de escuchar el sonido

typedef struct{

	long start_time;
	long finish_time;

} Timer;

//Variables globales
int button_pin;
Timer buzzer_cooldown;
char random_number_buffer[RANDOM_NUM_LENGTH];
LiquidCrystal lcd(1,2,3,4,5,6);	//los parametros son los pines a los cuales esta conectado el lcd

int print_on_lcd;	//variable que indica si es necesario hacer un print en el lcd o no
int n_key_pressed;	//cantidad de keys que se presionaron en el keypad

//mapa de teclas que vamos a usar del keypad
char key_map[NUM_ROWS_KEYPAD][NUM_COLS_KEYPAD] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
};

byte row_pins[NUM_ROWS_KEYPAD] = {A4,A3,A2}; 	//pines a los cuales conectamos las filas del keypad
byte col_pins[NUM_COLS_KEYPAD] = {13,12,11};	//pines a los cuales conectamos las columnas del keypad

Keypad keypad = Keypad(makeKeymap(key_map),row_pins,col_pins,NUM_ROWS_KEYPAD,NUM_COLS_KEYPAD);

//Prototipos
void button_game(void);
void keypad_game(void);
void generate_random_number(char *buffer);
void print_string_on_lcd(const char *buffer);

void setup()
{

#ifdef DEBUG_MODE
  	Serial.begin(9600);
#endif
  
  randomSeed(analogRead(A0));
  
  //Lcd
  lcd.begin(NUM_ROWS_LCD,NUM_COLS_LCD);
 
  //Boton
  button_pin = A5;
  pinMode(button_pin,INPUT);
  
  //Keypad
  print_on_lcd = 1;
  n_key_pressed = 0;
  pinMode(A2,INPUT);
  pinMode(A3,INPUT);
  pinMode(A4,INPUT);
  
  //Timer
  buzzer_cooldown.start_time = millis();
  
}

void loop()
{
  
  if(print_on_lcd){
	generate_random_number(random_number_buffer);    
  	print_string_on_lcd(random_number_buffer);
  }
  
  button_game(); 
  keypad_game(random_number_buffer);
  
}

void button_game(void){

  int button_state;
  int random_number;

  buzzer_cooldown.finish_time = millis();

  if(buzzer_cooldown.finish_time - buzzer_cooldown.start_time < BUZZER_COOLDOWN){
    return;
  }

  buzzer_cooldown.start_time = buzzer_cooldown.finish_time;
  random_number = random(0,100);	//genero un numero entre 0-99

  //Para tener un 50-50 de chances, pregunto si el numero es impar, si lo es reproduzco el sonido y reviso el boton
  if(random_number & 1){
    
#ifdef DEBUG_MODE
    Serial.println("Apreta el boton");
#endif
    
    //aca hay que poner un delay, porque sino no hay manera de darle tiempo al usuario a presionar el boton
    button_state = analogRead(button_pin);

    //El 1023 es por el valor maximo que retorna analogRead()
    if(button_state == 1023){
      
#ifdef DEBUG_MODE
      Serial.println("Boton apretado a tiempo");
#endif
      
    }
    else{
      
#ifdef DEBUG_MODE
      Serial.println("Demasiado lento apretando el boton");
#endif
      
    }

  }  
  
}

void keypad_game(const char *random_number_buffer){
  
  char key;
  
  key = keypad.getKey();
  
  if(key == NO_KEY){
  	return;
  }
    
  if(key != *(random_number_buffer + n_key_pressed)){
    
#ifdef DEBUG_MODE
    Serial.print("El numero ingresado es erroneo");
#endif    
    
    //si pierdo necesito indicar que se tiene que mostrar otro numero en el lcd
    print_on_lcd = 1;
    n_key_pressed = 0;
    //encender el led indicando que se electrocuto    
    return;
    
  }
  
  n_key_pressed++;
  
  if(n_key_pressed == RANDOM_NUM_LENGTH - 1){
    
#ifdef DEBUG_MODE
    Serial.println("El numero ingresado es correcto");
#endif        
    
    //si gano necesito indicar que se tiene que mostrar otro numero en el lcd
    print_on_lcd = 1;
    //ver que hacer cuando ingresa el numero correctamente
    
  }

}

//Funcion que genera un numero random de 10 digitos y lo almacena en el buffer recibido
void generate_random_number(char *buffer){

  for(int i = 0; i < RANDOM_NUM_LENGTH; i++){
  	*(buffer + i) = random(49,58);
  }
  
  *(buffer + RANDOM_NUM_LENGTH - 1) = '\0';

#ifdef DEBUG_MODE
  Serial.println(buffer);
#endif
  
}

void print_string_on_lcd(const char *buffer){
  
  lcd.clear();
  lcd.setCursor(0,3);
  lcd.print(buffer);
  print_on_lcd = 0;
  
}