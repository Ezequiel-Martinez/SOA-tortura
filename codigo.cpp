// C++ code
// Includes
#include <Servo.h> //para el servo motor
#include <Keypad.h> //para el keypad
#include <LiquidCrystal.h> //para el lcd

//Defines
#define DEBUG_MODE 0 //para hacer debug por consola
#define NUM_ROWS_LCD 3 //cantidad de filas que usamos en el keypad
#define NUM_COLS_LCD 3 //cantidad de columnas que usamos en el keypad
#define NUM_ROWS_KEYPAD 3 //cantidad de filas que usamos en el keypad
#define NUM_COLS_KEYPAD 3 //cantidad de columnas que usamos en el keypad
#define RANDOM_NUM_LENGTH 11 //cantidad de digitos que queremos que tenga el numero random ( contemplando el '\0')
#define BUZZER_COOLDOWN 1000 //cantidad de tiempo minimo que debe pasar entre un button_game y otro ( es un cooldown ) medido en milisegundos
#define MAX_BUTTON_PRESS_DELAY 1000 //cantidad de tiempo maximo que tiene el usuario para presionar el boton luego de escuchar el sonido en microsegundos
#define MAX_TIME_ELECTRIC_SHOCK 1000 //tiempo que quiero electrocutar al usuario en microsegundos
#define KEYPAD_GAME_TIME_REDUCTION 500 //cantidad de tiempo que se va a restar a max_keypad_game_time cada vez que el usuario gana

enum ShockIntensity { INTENSITY_LOW = 5, INTENSITY_MEDIUM = 10, INTENSITY_HIGH = 15 };

typedef struct{

	long start_time;
	long finish_time;

} t_timer;

//Variables globales
t_timer buzzer_cooldown;
t_timer lcd_clear_timer; //timer que uso para saber cuanto lleva el numero random en el lcd
ShockIntensity intensity;

Servo servo_motor;
LiquidCrystal lcd(1,2,3,4,5,7); //los parametros son los pines a los cuales esta conectado el lcd

int buzzer_pin;
int button_pin;
int servo_motor_pin;
int rgb_led_pin_r;
int rgb_led_pin_g;
int rgb_led_pin_b;

int print_on_lcd; //variable que indica si es necesario hacer un print en el lcd o no
int n_key_pressed; //cantidad de keys que se presionaron en el keypad
int number_of_attempts; //cantidad de intentos que lleva el usuario con el keypad_game
int max_keypad_game_time; //cantidad maxima de tiempo que va a tener el numero random para estar en el lcd en milisegundos ( lo pongo como variable para poder modificarlo )
char random_number_buffer[RANDOM_NUM_LENGTH];

//mapa de teclas que vamos a usar del keypad
char key_map[NUM_ROWS_KEYPAD][NUM_COLS_KEYPAD] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
};

byte row_pins[NUM_ROWS_KEYPAD] = {A4,A3,A2}; //pines a los cuales conectamos las filas del keypad
byte col_pins[NUM_COLS_KEYPAD] = {13,12,8}; //pines a los cuales conectamos las columnas del keypad

Keypad keypad = Keypad(makeKeymap(key_map),row_pins,col_pins,NUM_ROWS_KEYPAD,NUM_COLS_KEYPAD);

//Prototipos
void button_game(void);
void keypad_game(void);
void generate_random_number(char *buffer);
void stop_electric_shock(void);
void give_electric_shock(void);
void print_string_on_lcd(const char *buffer, int row, int column);

void setup()
{

#if DEBUG_MODE
  	Serial.begin(9600);
#endif
  
  number_of_attempts = 0;
  randomSeed(analogRead(A0));
  
  //Lcd
  lcd.begin(NUM_ROWS_LCD,NUM_COLS_LCD);
  
  //Buzzer
  buzzer_pin = A1;
 
  //Boton
  button_pin = A5;
  
  //Servo
  servo_motor_pin = 6;
  servo_motor.attach(servo_motor_pin);
  servo_motor.write(90); //TODO: eliminar, es solo un test para ver si gira correctamente

  pinMode(button_pin,INPUT);
  
  //Led rgb
  rgb_led_pin_r = 11;
  rgb_led_pin_g = 9;
  rgb_led_pin_b = 10;
  
  pinMode(rgb_led_pin_r,OUTPUT);
  pinMode(rgb_led_pin_g,OUTPUT);
  pinMode(rgb_led_pin_b,OUTPUT);  
  
  //Keypad
  print_on_lcd = 1;
  n_key_pressed = 0;
  max_keypad_game_time = 1000;
  pinMode(A2,INPUT);
  pinMode(A3,INPUT);
  pinMode(A4,INPUT);
  
  //Timer
  buzzer_cooldown.start_time = millis();
  
  //Shock intensity
  intensity = INTENSITY_LOW; //por defecto arrancamos con una intensidad baja
  
}

void loop()
{
  
  if(print_on_lcd){
	generate_random_number(random_number_buffer);    
  	print_string_on_lcd(random_number_buffer,0,3);
  	lcd_clear_timer.start_time = millis();
  }
  
  button_game(); 
  keypad_game(random_number_buffer);
  
}

//Funcion que se encarga con todo lo relacionado con el boton
void button_game(void){

  int button_state;
  int random_number;

  buzzer_cooldown.finish_time = millis();

  if(buzzer_cooldown.finish_time - buzzer_cooldown.start_time < BUZZER_COOLDOWN){
    return;
  }

  buzzer_cooldown.start_time = buzzer_cooldown.finish_time;
  random_number = random(0,100); //genero un numero entre 0-99

  //Para tener un 50-50 de chances, pregunto si el numero es impar, si lo es reproduzco el sonido y reviso el boton
  if(random_number & 1){
    
#if DEBUG_MODE
    Serial.println("Apreta el boton");
#endif

	tone(buzzer_pin,800);	
	    
    //aca hay que poner un delay, porque sino no hay manera de darle tiempo al usuario a presionar el boton ?
    //test
    delayMicroseconds(MAX_BUTTON_PRESS_DELAY);
    button_state = analogRead(button_pin);

	noTone(buzzer_pin);

    //El 1023 es por el valor maximo que retorna analogRead()
    if(button_state == 1023){
      
#if DEBUG_MODE
      Serial.println("Boton apretado a tiempo");
#endif
      
    }
    else{
      
#if DEBUG_MODE
      Serial.println("Demasiado lento apretando el boton");
#endif

      give_electric_shock();
      delayMicroseconds(MAX_TIME_ELECTRIC_SHOCK);
      stop_electric_shock();
      
    }

  }  
  
}

//Funcion que se encarga de todo lo relacionado con el keypad
void keypad_game(const char *random_number_buffer){
  
  char key;
  
  key = keypad.getKey();
  
  if(key == NO_KEY){
  	return;
  }
  
#if DEBUG_MODE
    Serial.print("Tecla presionada: ");
    Serial.println(key);
#endif    
  
  lcd_clear_timer.finish_time = millis();
  
  if(lcd_clear_timer.finish_time - lcd_clear_timer.start_time > max_keypad_game_time){

#if DEBUG_MODE
    Serial.println("Se alcanzo el tiempo maximo para mostrar el numero en el lcd");
#endif    

  	lcd.clear();
  	lcd_clear_timer.start_time = lcd_clear_timer.finish_time;

  }  
    
  if(key != *(random_number_buffer + n_key_pressed)){
    
#if DEBUG_MODE
    Serial.print("El numero ingresado es erroneo. Se esperaba: ");
    Serial.print(*(random_number_buffer + n_key_pressed));
    Serial.print(" , y se recibio: ");
    Serial.println(key);
#endif    
    
    print_on_lcd = 1; //si pierdo necesito indicar que se tiene que mostrar otro numero en el lcd
    n_key_pressed = 0; //reinicio la cantidad de teclas presionadas
    number_of_attempts++; //sumo uno a la cantidad de intentos que lleva el usuario en el keypad_game
    
    if(number_of_attempts > INTENSITY_MEDIUM){

#if DEBUG_MODE
		Serial.println("Cambiando a intensidad alta");
#endif    

    	intensity = INTENSITY_HIGH;
    	
    }
    else if( number_of_attempts > INTENSITY_LOW){
    
#if DEBUG_MODE
		Serial.println("Cambiando a intensidad media");
#endif    

    	intensity = INTENSITY_MEDIUM;
    	
    }
    
    give_electric_shock(); //Enciendo el led indicando que se electrocuto con la intensidad correspondiente
    delayMicroseconds(MAX_TIME_ELECTRIC_SHOCK);
    stop_electric_shock();
    return;
    
  }
  
  n_key_pressed++;
  
  if(n_key_pressed == RANDOM_NUM_LENGTH - 1){
    
#if DEBUG_MODE
    Serial.println("El numero ingresado es correcto");
#endif        
    
    print_on_lcd = 1; //si gano necesito indicar que se tiene que mostrar otro numero en el lcd
    max_keypad_game_time -= KEYPAD_GAME_TIME_REDUCTION; //el siguiente numero va a tener menos tiempo en el lcd
    //ver que hacer cuando ingresa el numero correctamente
    
  }

}

//Funcion que genera un numero random de 10 digitos y lo almacena en el buffer recibido
void generate_random_number(char *buffer){

  for(int i = 0; i < RANDOM_NUM_LENGTH; i++){
  	*(buffer + i) = random(49,58); //genero un numero entre 1-9
  }
  
  *(buffer + RANDOM_NUM_LENGTH - 1) = '\0';

#if DEBUG_MODE
  Serial.print("Numero random generado: ");
  Serial.println(buffer);
#endif
  
}

void stop_electric_shock(void){

#if DEBUG_MODE
  Serial.println("Terminando choque electrico");
#endif  		

  analogWrite(rgb_led_pin_r,0);
  analogWrite(rgb_led_pin_g,0);
  analogWrite(rgb_led_pin_b,0);			
	
}

void give_electric_shock(void){
	
  switch(intensity){
  
  	case INTENSITY_LOW:

#if DEBUG_MODE
  Serial.println("Choque electrico con intensidad baja");
#endif  	

		analogWrite(rgb_led_pin_r,0);
		analogWrite(rgb_led_pin_g,255);
		analogWrite(rgb_led_pin_b,0);
  	
  	break;
  	
  	case INTENSITY_MEDIUM:

#if DEBUG_MODE
  Serial.println("Choque electrico con intensidad media");
#endif  	
  	
		analogWrite(rgb_led_pin_r,255);
		analogWrite(rgb_led_pin_g,255);
		analogWrite(rgb_led_pin_b,0);
      	
  	break;
  	
  	case INTENSITY_HIGH:

#if DEBUG_MODE
  Serial.println("Choque electrico con intensidad alta");
#endif  	

		analogWrite(rgb_led_pin_r,255);
		analogWrite(rgb_led_pin_g,0);
		analogWrite(rgb_led_pin_b,0);	
  	
  	break;
  	
  	default:
  	break;
  	
  }	
	
}

//Funcion que se encarga de mostrar en el lcd el contenido del buffer que recibe por parametro, row y column son para posicionar el cursor del lcd
void print_string_on_lcd(const char *buffer, int row, int column){
  
  lcd.clear(); //limpio cualquier cosa que tenga el lcd
  lcd.setCursor(row,column); //para que el numero quede centrado
  lcd.print(buffer);
  print_on_lcd = 0;
  
#if DEBUG_MODE
  Serial.print("Buffer a imprimir en el lcd: ");
  Serial.println(buffer);
#endif  
  
}
