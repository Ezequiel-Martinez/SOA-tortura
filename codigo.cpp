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
#define MAX_TIME_SERVO 2000// tiempo para que se evalue si el servo gira o no
#define MAX_BUZZER_TIMER 1000 //cantidad de tiempo minimo que debe pasar entre un button_game y otro ( es un cooldown ) medido en milisegundos
#define MIN_SENSOR_PRESSURE 50 //presion minima en el sensor para que el juego funcione
#define MAX_BUTTON_PRESS_DELAY 400 //cantidad de tiempo maximo que tiene el usuario para presionar el boton luego de escuchar el sonido en microsegundos
#define MAX_TIME_ELECTRIC_SHOCK 100 //tiempo que quiero electrocutar al usuario en microsegundos
#define KEYPAD_GAME_TIME_REDUCTION 500 //cantidad de tiempo que se va a restar a max_keypad_game_time cada vez que el usuario gana

// Comandos para comunicarse por medio del Serial Monitor
// "K": Solamente se podrá usar la pantalla LCD junto con el keypad
// "B": Solamente se podrá usar el buzzer con el botón
// "N": Vuelve al estado normal, se utilizan tanto el keypad como el botón

enum ShockIntensity { INTENSITY_LOW = 2, INTENSITY_MEDIUM = 4, INTENSITY_HIGH };
enum SerialMonitorCommands {KEYPAD = 'K', BUTTON = 'B', NORMAL = 'N', NONE = 'Z'};

typedef struct{

  long start_time;
  long finish_time;

} t_timer;

//Variables globales
t_timer tone_timer;
t_timer servo_timer;
t_timer buzzer_timer;
t_timer lcd_clear_timer; //timer que uso para saber cuanto lleva el numero random en el lcd
t_timer electric_shock_timer;

ShockIntensity intensity;
Servo servo_motor;
LiquidCrystal lcd(4,5,6,7,8,9); //los parametros son los pines a los cuales esta conectado el lcd

int led_pin;
int sensor_pin;
int buzzer_pin;
int button_pin;
int servo_motor_pin;

int button_points; //cantidad de aciertos
int button_points_to_win;
int keypad_points;
int keypad_points_to_win;
int servo_degrees;
int incoming_byte; // Es el caracter en número ASCII que nos pasan por Serial Monitor
int n_key_pressed; //cantidad de keys que se presionaron en el keypad
int number_of_attempts; //cantidad de intentos que lleva el usuario con el keypad_game
int max_keypad_game_time; //cantidad maxima de tiempo que va a tener el numero random para estar en el lcd en milisegundos ( lo pongo como variable para poder modificarlo )

bool playing; //indica si el usuario esta jugando a alguno de los modos disponibles
bool electrocuting; //para indicar si se esta electrocutando al usuario o no
bool sound_playing; //para indicar si el buzzer esta reproduciendo un sonido o no 
bool print_sensor_message;
bool print_game_mode_message;

char random_number_buffer[RANDOM_NUM_LENGTH];

//mapa de teclas que vamos a usar del keypad
char key_map[NUM_ROWS_KEYPAD][NUM_COLS_KEYPAD] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
};

byte row_pins[NUM_ROWS_KEYPAD] = {A2,A1,A0}; //pines a los cuales conectamos las filas del keypad
byte col_pins[NUM_COLS_KEYPAD] = {13,12,11}; //pines a los cuales conectamos las columnas del keypad

Keypad keypad = Keypad(makeKeymap(key_map),row_pins,col_pins,NUM_ROWS_KEYPAD,NUM_COLS_KEYPAD);

//Prototipos
void button_game(void);
void keypad_game(void);
void clear_serial(void);
void init_variables(void);
void servo_motor_game(void);
void stop_electric_shock(void);
void give_electric_shock(void);
void generate_random_number(char *buffer);
void print_string_on_lcd(const char *buffer, int row, int column);

void setup()
{

  Serial.begin(9600);
  
  init_variables();
  
  //Lcd
  lcd.begin(NUM_ROWS_LCD,NUM_COLS_LCD);
  
  //Buzzer
  buzzer_pin = A4;
 
  //Boton
  button_pin = 2;
  pinMode(button_pin,INPUT);
  
  //Servo
  servo_degrees = 180;
  servo_motor_pin = 10;
  servo_motor.attach(servo_motor_pin, 500, 2500);
  
  //Led
  led_pin = 3;
  pinMode(led_pin,OUTPUT);
  
  //Sensor
  sensor_pin = A3;
  pinMode(sensor_pin,INPUT);
  print_sensor_message = true; //Dejar esto aca
  
  //Keypad
  max_keypad_game_time = 1000;
  pinMode(A0,INPUT); //pin que usamos para declarar la variable keypad
  pinMode(A1,INPUT); //pin que usamos para declarar la variable keypad
  pinMode(A2,INPUT); //pin que usamos para declarar la variable keypad
  
  //Timer
  buzzer_timer.start_time = millis();

}

void loop()
{
  
  //pregunto si el usuario no esta presionando el sensor
  if(analogRead(sensor_pin) < MIN_SENSOR_PRESSURE){

	clear_serial();

    if(print_sensor_message){
    
      init_variables();
      print_sensor_message = false;
      print_game_mode_message = true;
      print_string_on_lcd("Presione sensor.",0,0);

#if DEBUG_MODE
      Serial.println("El sensor no esta presionado correctamente");
#endif  

    }

  }
  else{
 
    // Cuando hay algo para leer desde Serial Monitor
    if(!playing && Serial.available() > 0){

      incoming_byte = Serial.read(); // Lo lee en ascii, si ingreso la letra "a", entonces se recibe el número 97

#if DEBUG_MODE
      Serial.print("Se recibio por Serial Monitor el caracter: ");
      Serial.println((char)incoming_byte);
#endif

      switch(incoming_byte){

        // Paso por Serial Monitor para usar solamente el buzzer y el boton
        case SerialMonitorCommands::BUTTON:
        
        playing = true;
        print_string_on_lcd("Usando buzzer",0,0);
        
#if DEBUG_MODE
        Serial.println("Modo de juego: buzzer - boton");
#endif

        break;

        // Paso por Serial Monitor para usar solamente el Keypad
        case SerialMonitorCommands::KEYPAD:
      
        playing = true;      
		generate_random_number(random_number_buffer);
		print_string_on_lcd(random_number_buffer,0,3);
		lcd_clear_timer.start_time = millis();

#if DEBUG_MODE
      	Serial.println("Modo de juego: keypad");
#endif
        
        break;

      	// Paso por Serial Monitor el funcionamiento normal (buzzer, boton y keypad)
        case SerialMonitorCommands::NORMAL:
        
        playing = true;        
		generate_random_number(random_number_buffer);
		print_string_on_lcd(random_number_buffer,0,3);
		lcd_clear_timer.start_time = millis();
        
#if DEBUG_MODE
        Serial.println("Modo de juego: buzzer - boton - keypad");
#endif
        
        break;

      default:
      
#if DEBUG_MODE
        Serial.println("El comando ingresado es invalido");
#endif      
      
        break;
        
      }
    
    }
    else{
    
      if(print_game_mode_message){
        
        print_sensor_message = true;
        print_game_mode_message = false;
        print_string_on_lcd("Seleccione modo.",0,0);
        
#if DEBUG_MODE
      Serial.println("Seleccionando modo de juego");
#endif        
        
      }
    
    }
    
    if(playing){
    
      switch(incoming_byte){
    
        case SerialMonitorCommands::NORMAL:
      
        button_game();
        keypad_game();
        servo_motor_game();
      
        break;
    
        case SerialMonitorCommands::KEYPAD:
      
        keypad_game();
        servo_motor_game();
      
        break;
    
        case SerialMonitorCommands::BUTTON:
      
        button_game();
      
        break;
    
        case SerialMonitorCommands::NONE:
        break;
    
        default:
        break;
    
    }
    
    }
    
   
  }

}

void clear_serial(void){
  
  if(Serial.available() > 0){
    
    int value = Serial.read();
    
  }
	
}

void init_variables(void){

  //TODO: deberia inicializar los timer aca tambien ??
  n_key_pressed = 0;
  button_points = 0;
  keypad_points = 0;
  number_of_attempts = 0;
  button_points_to_win = 3;
  keypad_points_to_win = 5;
  playing = false;
  electrocuting = false;
  sound_playing = false;
  print_game_mode_message = true;  
  intensity = ShockIntensity::INTENSITY_LOW; //por defecto arrancamos con una intensidad baja
  incoming_byte = SerialMonitorCommands::NONE; //por defecto arranco el juego si un game mode
  randomSeed(analogRead(A5)); //seteamos un seed para el generador random

}

//Funcion que se encarga con todo lo relacionado con el boton
void button_game(void){

  electric_shock_timer.finish_time = millis();

  if(electrocuting && (electric_shock_timer.finish_time - electric_shock_timer.start_time >= MAX_TIME_ELECTRIC_SHOCK)){
  
    stop_electric_shock();
    electrocuting = false;
  
  }
  else{
    
    int button_state;
	int random_number;

	buzzer_timer.finish_time = millis();
  
	//espera el cooldown para hacer sonar el botón
	if(!sound_playing && (buzzer_timer.finish_time - buzzer_timer.start_time < MAX_BUZZER_TIMER)){
      return;
	}

	buzzer_timer.start_time = buzzer_timer.finish_time;
	random_number = random(0,100); //genero un numero entre 0-99

    //Para tener un 50-50 de chances, pregunto si el numero es impar, si lo es reproduzco el sonido y reviso el boton
    if(random_number & 1){
      
      if(!sound_playing){
      
#if DEBUG_MODE
        Serial.println("Apreta el boton");
#endif
		
        sound_playing = true;
		tone(buzzer_pin,800);
		tone_timer.start_time = millis();
		
      }
      
      tone_timer.finish_time = millis();
      
      if(tone_timer.finish_time - tone_timer.start_time >= MAX_BUTTON_PRESS_DELAY){
      	
        button_state = digitalRead(button_pin);
      	
      	noTone(buzzer_pin);
      	tone_timer.start_time = tone_timer.finish_time;
      	sound_playing = false;
      	
  	    //El 1023 es por el valor maximo que retorna analogRead()
        if(button_state == 1023 ){
	    
#if DEBUG_MODE
          Serial.println("Boton apretado a tiempo");
#endif
		  button_points++;
		  
          if(button_points == button_points_to_win){
		    
#if DEBUG_MODE
            Serial.println("Ganaste el juego del boton");
#endif		    

		    init_variables();
		    return;
		    
		  }

	    }
        else{
	    
#if DEBUG_MODE
          Serial.println("Demasiado lento apretando el boton");
#endif
		  
		  button_points = 0;
		  electrocuting = true;
		  electric_shock_timer.start_time = millis();
		  give_electric_shock();

	    }
	    
      }
      
	}

  }  

}

//Funcion que se encarga de todo lo relacionado con el keypad
void keypad_game(void){
  
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
  
  if(lcd_clear_timer.finish_time - lcd_clear_timer.start_time >= max_keypad_game_time){

#if DEBUG_MODE
    Serial.println("Se alcanzo el tiempo maximo para mostrar el numero en el lcd");
#endif    

  	print_string_on_lcd("",0,0);
  	lcd_clear_timer.start_time = lcd_clear_timer.finish_time;

  }  
    
  if(key != *(random_number_buffer + n_key_pressed)){
    
#if DEBUG_MODE
    Serial.print("El numero ingresado es erroneo. Se esperaba: ");
    Serial.print(*(random_number_buffer + n_key_pressed));
    Serial.print(" , y se recibio: ");
    Serial.println(key);
#endif    
        
    number_of_attempts++; //sumo uno a la cantidad de intentos que lleva el usuario en el keypad_game
    
    if(number_of_attempts > ShockIntensity::INTENSITY_MEDIUM){
      
#if DEBUG_MODE
      Serial.println("Cambiando a intensidad alta");
#endif    

      intensity = ShockIntensity::INTENSITY_HIGH;
    	
    }
    else if(number_of_attempts > ShockIntensity::INTENSITY_LOW){
    
#if DEBUG_MODE
      Serial.println("Cambiando a intensidad media");
#endif    

      intensity = ShockIntensity::INTENSITY_MEDIUM;
    	
    }
    
    give_electric_shock(); //Enciendo el led indicando que se electrocuto con la intensidad correspondiente
    stop_electric_shock();
    
    n_key_pressed = 0; //reinicio la cantidad de teclas presionadas
	generate_random_number(random_number_buffer);
	print_string_on_lcd(random_number_buffer,0,3);
	lcd_clear_timer.start_time = millis();    
    
    return;
    
  }
  
  n_key_pressed++;
  
  if(n_key_pressed == RANDOM_NUM_LENGTH - 1){
    
#if DEBUG_MODE
    Serial.println("El numero ingresado es correcto");
#endif        

    keypad_points++;
    
	if(keypad_points == keypad_points_to_win){
	  
#if DEBUG_MODE
	  Serial.println("Ganaste el juego del keypad");
#endif

	  init_variables();
	  return;

	}

    n_key_pressed = 0; //reinicio la cantidad de teclas presionadas
    max_keypad_game_time -= KEYPAD_GAME_TIME_REDUCTION; //el siguiente numero va a tener menos tiempo en el lcd
	generate_random_number(random_number_buffer);
	print_string_on_lcd(random_number_buffer,0,3);
	lcd_clear_timer.start_time = millis();
    
  }

}

//Funcion para girar el servo motor de forma aleatoria
void servo_motor_game(void){

  servo_timer.finish_time = millis();
  
  if((servo_timer.finish_time - servo_timer.start_time) >= MAX_TIME_SERVO){
  	
    servo_timer.start_time = servo_timer.finish_time;

#if DEBUG_MODE    
    Serial.println("Girando servo motor");
#endif

    servo_motor.write(servo_degrees);

    if(servo_degrees == 180){
      servo_degrees = 0;
    }
    else{
      servo_degrees += 90;
    }
    
  }
  
}

//Funcion que genera un numero random de 10 digitos y lo almacena en el buffer recibido
void generate_random_number(char *buffer){

  for(int i = 0; i < RANDOM_NUM_LENGTH; i++){
  	*(buffer + i) = random(49,57); //genero el ascii de un numero entre 1-9
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

  digitalWrite(led_pin,0);		
	
}

void give_electric_shock(void){
	
  switch(intensity){
  
    case INTENSITY_LOW:

#if DEBUG_MODE
  	Serial.println("Choque electrico con intensidad baja");
#endif  	

    digitalWrite(led_pin,70);
  	
  	break;
  	
  	case INTENSITY_MEDIUM:

#if DEBUG_MODE
    Serial.println("Choque electrico con intensidad media");
#endif  	
  	
	digitalWrite(led_pin,150);
      	
  	break;
  	
  	case INTENSITY_HIGH:

#if DEBUG_MODE
    Serial.println("Choque electrico con intensidad alta");
#endif  	

	digitalWrite(led_pin,255);
  	
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
  
#if DEBUG_MODE
  Serial.print("Buffer a imprimir en el lcd: ");
  Serial.println(buffer);
#endif  
  
}
