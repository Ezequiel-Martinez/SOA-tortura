/*
  Comandos para comunicarse por medio del Serial Monitor:
	"K": Solamente se podrá usar la pantalla LCD junto con el keypad
	"B": Solamente se podrá usar el buzzer con el botón
	"N": Vuelve al estado normal, se utilizan tanto el keypad como el botón
*/

// Includes
#include <Servo.h>		   //para el servo motor
#include <Keypad.h>		   //para el keypad
#include <LiquidCrystal.h> //para el lcd

// Defines
#define DEBUG_MODE 0 // para hacer debug por consola

#define NUM_ROWS_LCD 3	  // cantidad de filas que usamos en el keypad
#define NUM_COLS_LCD 3	  // cantidad de columnas que usamos en el keypad
#define NUM_ROWS_KEYPAD 3 // cantidad de filas que usamos en el keypad
#define NUM_COLS_KEYPAD 3 // cantidad de columnas que usamos en el keypad

#define MAX_TIME_SERVO 10 // tiempo para que se evalue si el servo gira o no
#define MIN_TIME_SERVO 50 // tiempo para que se evalue si el servo gira o no

#define MAX_BUZZER_TIMER 1000		// cantidad de tiempo minimo que debe pasar entre un buzzer_game y otro ( es un cooldown ) medido en milisegundos
#define MIN_SENSOR_PRESSURE 50		// presion minima en el sensor para que el juego funcione
#define MAX_BUTTON_PRESS_DELAY 400	// cantidad de tiempo maximo que tiene el usuario para presionar el boton luego de escuchar el sonido en microsegundos
#define MAX_TIME_ELECTRIC_SHOCK 100 // tiempo que quiero electrocutar al usuario en microsegundos

#define RANDOM_NUM_LENGTH 11		   // cantidad de digitos que queremos que tenga el numero random ( contemplando el '\0')
#define BUTTON_POINTS_TO_WIN 3		   // cantidad de puntos que necesito para ganar
#define KEYPAD_POINTS_TO_WIN 3		   // cantidad de puntos que necesito para ganar
#define KEYPAD_GAME_TIME_REDUCTION 500 // cantidad de tiempo que se va a restar a max_keypad_game_time cada vez que el usuario gana

#define LED_PIN 3
#define SERVO_PIN 10
#define SENSOR_PIN A3
#define BUTTON_PIN 2
#define BUZZER_PIN A4

// Prototipos
void fsm(void);
void none(void);
void init_(void);
void error(void);
void sensor_wait(void);
void wait_mode(void);
void button_game(void);
void keypad_game(void);
void normal_game(void);
void buzzer_processing(void);

void servo(void);
void clear_serial(void);
bool is_sensor_pressed(void);
void incoming_byte_processing(void);
void modify_electric_shock(bool mode);
void generate_random_number(char *buffer);
void print_string_on_lcd(const char *buffer, int row, int column);

// Maquina de estados
enum State
{
	STATE_INIT,
	STATE_SENSOR_WAIT,
	STATE_WAIT_MODE,
	STATE_BUTTON_GAME,
	STATE_KEYPAD_GAME,
	STATE_NORMAL_GAME,
	STATE_UNKNOWN,
	NUMBER_OF_STATES
};
enum Event
{
	EVENT_CONT,
	EVENT_SENSOR_PRESSED,
	EVENT_SENSOR_RELEASED,
	EVENT_CHARACTER_B,
	EVENT_CHARACTER_K,
	EVENT_CHARACTER_N,
	EVENT_UNKNOWK,
	NUMBER_OF_EVENTS
};

typedef void (*T_transition)(void);
//

enum ShockIntensity
{
	INTENSITY_LOW = 2,
	INTENSITY_MEDIUM = 4,
	INTENSITY_HIGH
};
enum SerialMonitorCommands
{
	KEYPAD = 'K',
	BUTTON = 'B',
	NORMAL = 'N'
};

typedef struct
{
	long start_time;
	long finish_time;
} t_timer;

// Variables globales
Event event;
State current_state;

t_timer tone_timer;
t_timer shock_timer;
t_timer servo_timer;
t_timer buzzer_timer;
t_timer lcd_clear_timer; // timer que uso para saber cuanto lleva el numero random en el lcd

ShockIntensity intensity;
Servo servo_motor;
LiquidCrystal lcd(4, 5, 6, 7, 8, 9); // los parametros son los pines a los cuales esta conectado el lcd

int speed;
int button_points; // cantidad de puntos que llevo hasta el momento
int keypad_points; // cantidad de puntos que llevo hasta el momento
int servo_degrees;
int incoming_byte;		  // es el caracter en número ASCII que nos pasan por Serial Monitor
int n_key_pressed;		  // cantidad de keys que se presionaron en el keypad
int number_of_attempts;	  // cantidad de intentos que lleva el usuario con el keypad_game
int max_keypad_game_time; // cantidad maxima de tiempo que va a tener el numero random para estar en el lcd en milisegundos ( lo pongo como variable para poder modificarlo )

bool playing; // indica si el usuario esta jugando a alguno de los modos disponibles
bool positive;
bool clear_lcd;		// para indicar si es necesario limpiar el lcd o no
bool electrocuting; // para indicar si se esta electrocutando al usuario o no
bool sound_playing; // para indicar si el buzzer esta reproduciendo un sonido o no
bool print_sensor_message;
bool print_game_mode_message;

char random_number_buffer[RANDOM_NUM_LENGTH];														  // buffer para almacenar el numero random generado en forma de string
char key_map[NUM_ROWS_KEYPAD][NUM_COLS_KEYPAD] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}}; // mapa de teclas que vamos a usar del keypad

byte row_pins[NUM_ROWS_KEYPAD] = {A2, A1, A0}; // pines a los cuales conectamos las filas del keypad
byte col_pins[NUM_COLS_KEYPAD] = {13, 12, 11}; // pines a los cuales conectamos las columnas del keypad

// creamos la variable del keypad
Keypad keypad = Keypad(makeKeymap(key_map), row_pins, col_pins, NUM_ROWS_KEYPAD, NUM_COLS_KEYPAD);

T_transition state_table[NUMBER_OF_STATES][NUMBER_OF_EVENTS] =
	{
	 // EVENT_CONT, EVENT_SENSOR_PRESSED, 	EVENT_SENSOR_RELEASED, 	EVENT_CHARACTER_B, 	EVENT_CHARACTER_K, 	EVENT_CHARACTER_N, 	EVENT_UNKNOWK
		init_, 		error, 					error, 					error, 				error, 				error, 				error,			// STATE_INIT
		none, 		wait_mode, 				sensor_wait,			none, 				none, 				none, 				none,			// STATE_SENSOR_WAIT
		none, 		wait_mode, 				sensor_wait, 			button_game, 		keypad_game, 		normal_game, 		error, 			// STATE_WAIT_MODE
		none, 		button_game, 			sensor_wait, 			none, 				none, 				none, 				error,			// STATE_BUTTON_GAME
		none, 		keypad_game, 			sensor_wait, 			none, 				none, 				none, 				error,			// STATE_KEYPAD_GAME
		none, 		normal_game, 			sensor_wait, 			none, 				none, 				none, 				error,			// STATE_NORMAL_GAME
		error, 		error, 					error, 					error, 				error, 				error, 				error			// STATE_UNKNOWN
};

/*De esta forma queda mas facil de leer y cada juego tiene su estado, de forma que evitamos switches engorrosos

- init_ es el setup
- none la funcion de no hacer nada, un return
- wait_mode es la funcion que espera que se ingrese el modo de juego
- sensor_wait es la funcion que espera a que se presione el sensor
- button_game, keypad_game y norma_game son las funciones de cada juego. queda pendiente revisar la logica.
en teoria deberia salir cada que termine sus validaciones y vuelva a preguntar estados
- error es un estado en que el sistema se frena, por lo que no vamos a tocarlo por ahora
*/

// Carga el reconocimiento de pines
void setup()
{
	Serial.begin(9600);

	// Lcd
	lcd.begin(NUM_ROWS_LCD, NUM_COLS_LCD);

	// Boton
	pinMode(BUTTON_PIN, INPUT);

	// Servo
	servo_degrees = 0;
	servo_motor.attach(SERVO_PIN, 500, 2500);
	servo_motor.write(servo_degrees);

	// Led
	pinMode(LED_PIN, OUTPUT);

	// Sensor
	pinMode(SENSOR_PIN, INPUT);
	print_sensor_message = true; // Dejar esto aca

	// Keypad
	max_keypad_game_time = 1000;
	pinMode(A0, INPUT); // pin que usamos para declarar la variable keypad
	pinMode(A1, INPUT); // pin que usamos para declarar la variable keypad
	pinMode(A2, INPUT); // pin que usamos para declarar la variable keypad

	// Timer
	buzzer_timer.start_time = millis();

	// State machine
	event = EVENT_CONT;
	current_state = STATE_INIT;

	randomSeed(analogRead(A5)); // seteamos un seed para el generador random
}

// Proceso principal del dispositivo
void loop()
{
	fsm();
}

// Procesamiento de estados
void fsm(void)
{
	if (event >= 0 && event < NUMBER_OF_EVENTS && current_state >= 0 && current_state < NUMBER_OF_STATES)
	{
		state_table[current_state][event]();
		return;
	}

	state_table[STATE_UNKNOWN][EVENT_UNKNOWK](); // si llega a ocurrir algun error queda siempre aca
}

// Funcion DUMMY
void none(void)
{
	// No hace nada
}

// Reinicia las variables
void init_(void)
{
	n_key_pressed = 0;
	button_points = 0;
	keypad_points = 0;
	number_of_attempts = 0;
	positive = true;
	playing = false;
	clear_lcd = false;
	electrocuting = false;
	sound_playing = false;
	speed = MAX_TIME_SERVO;
	intensity = ShockIntensity::INTENSITY_LOW; // por defecto arrancamos con una intensidad baja
	event = is_sensor_pressed() ? EVENT_SENSOR_PRESSED : EVENT_SENSOR_RELEASED;
	current_state = STATE_SENSOR_WAIT;
	digitalWrite(LED_PIN, 0);
	noTone(BUZZER_PIN);
}

// Pone al dispositivo en punto muerto
void error(void)
{
	init_();
	current_state = STATE_UNKNOWN;
}

// Espera a que se presione el sensor
void sensor_wait(void)
{
	if (analogRead(SENSOR_PIN) < MIN_SENSOR_PRESSURE)
	{

		clear_serial();

		if (print_sensor_message)
		{
			event = EVENT_CONT;
			current_state = STATE_INIT;
			print_sensor_message = false;
			print_game_mode_message = true;
			print_string_on_lcd("Presione sensor.", 0, 0);

#if DEBUG_MODE
			Serial.println("El sensor no esta presionado correctamente");
#endif
		}

		return;
	}

	event = EVENT_SENSOR_PRESSED;
	current_state = STATE_WAIT_MODE;
}

// Espera el modo de juego
void wait_mode(void)
{
	if (!is_sensor_pressed())
	{
		event = EVENT_SENSOR_RELEASED;
		return;
	}

	if (print_game_mode_message)
	{
		print_sensor_message = true;
		print_game_mode_message = false;
		print_string_on_lcd("Seleccione modo.", 0, 0);

#if DEBUG_MODE
		Serial.println("Seleccionando modo de juego");
#endif
	}

	if (!playing && Serial.available() > 0)
	{ // Cuando hay algo para leer desde Serial Monitor
		incoming_byte_processing();
	}
}

// Funcion que se encarga de todo lo relacionado al juego del boton
void button_game(void)
{
	if (current_state != STATE_NORMAL_GAME)
	{
		current_state = STATE_BUTTON_GAME;
	}
	
	if (!is_sensor_pressed())
	{
		event = EVENT_SENSOR_RELEASED;
		return;
	}
	else // Provicional
	{
		event = EVENT_SENSOR_PRESSED;
	}

	shock_timer.finish_time = buzzer_timer.finish_time = millis();

	if (electrocuting)
	{
		if (shock_timer.finish_time - shock_timer.start_time >= MAX_TIME_ELECTRIC_SHOCK)
		{
			modify_electric_shock(false);
			electrocuting = false;
		}

		return;
	}

	// espera el cooldown para hacer sonar el buzzer
	if (!sound_playing && (buzzer_timer.finish_time - buzzer_timer.start_time < MAX_BUZZER_TIMER))
	{
		return;
	}

	buzzer_timer.start_time = buzzer_timer.finish_time;

	// Compara el ultimo caracter del numero generado con 1 (50% de chance)
	if (random(0, 100) & 1)
	{
		if (!sound_playing)
		{
#if DEBUG_MODE
			Serial.println("Apreta el boton");
#endif

			sound_playing = true;
			tone(BUZZER_PIN, 800);
			tone_timer.start_time = millis();
		}

		buzzer_processing();
	}
}

// Funcion que se encarga de todo lo relacionado al juego del keypad
void keypad_game(void)
{
	if (current_state != STATE_NORMAL_GAME)
	{
		current_state = STATE_KEYPAD_GAME;
	}
	
	if (!is_sensor_pressed())
	{
		event = EVENT_SENSOR_RELEASED;
		return;
	}
	else // Provicional
	{
		event = EVENT_SENSOR_PRESSED;
	}

	servo();

	if (clear_lcd && ((lcd_clear_timer.finish_time = millis()) - lcd_clear_timer.start_time) >= max_keypad_game_time)
	{
		clear_lcd = false;
		print_string_on_lcd("", 0, 0);
		lcd_clear_timer.start_time = lcd_clear_timer.finish_time;
	}

	char key = keypad.getKey();

	if (key == NO_KEY)
	{
		return;
	}

#if DEBUG_MODE
	Serial.print("Tecla presionada: ");
	Serial.println(key);
#endif

	if (key != *(random_number_buffer + n_key_pressed))
	{
#if DEBUG_MODE
		Serial.print("El numero ingresado es erroneo. Se esperaba: ");
		Serial.print(*(random_number_buffer + n_key_pressed));
		Serial.print(" , y se recibio: ");
		Serial.println(key);
#endif

		number_of_attempts++; // sumo uno a la cantidad de intentos que lleva el usuario en el keypad_game

		if (number_of_attempts > ShockIntensity::INTENSITY_MEDIUM)
		{
			intensity = ShockIntensity::INTENSITY_HIGH;

#if DEBUG_MODE
			Serial.println("Cambiando a intensidad alta");
#endif
		}
		else if (number_of_attempts > ShockIntensity::INTENSITY_LOW)
		{
			intensity = ShockIntensity::INTENSITY_MEDIUM;

#if DEBUG_MODE
			Serial.println("Cambiando a intensidad media");
#endif
		}

		modify_electric_shock(true); // Enciendo el led indicando que se electrocuto con la intensidad correspondiente
		modify_electric_shock(false);

		n_key_pressed = 0; // reinicio la cantidad de teclas presionadas
		generate_random_number(random_number_buffer);
		print_string_on_lcd(random_number_buffer, 0, 3);
		lcd_clear_timer.start_time = millis();
		clear_lcd = true;

		return;
	}

	n_key_pressed++;

	if (n_key_pressed == RANDOM_NUM_LENGTH - 1)
	{
#if DEBUG_MODE
		Serial.println("El numero ingresado es correcto");
#endif

		keypad_points++;

		if (keypad_points == KEYPAD_POINTS_TO_WIN)
		{
			print_game_mode_message = true;
			event = EVENT_CONT;
			current_state = STATE_INIT;

#if DEBUG_MODE
			Serial.println("Ganaste el juego del keypad");
#endif
			return;
		}

		n_key_pressed = 0;									// reinicio la cantidad de teclas presionadas
		max_keypad_game_time -= KEYPAD_GAME_TIME_REDUCTION; // el siguiente numero va a tener menos tiempo en el lcd
		generate_random_number(random_number_buffer);
		print_string_on_lcd(random_number_buffer, 0, 3);
		lcd_clear_timer.start_time = millis();
		clear_lcd = true;
	}
}

// Funcion que ejecuta ambos juegos
void normal_game(void)
{
	current_state = STATE_NORMAL_GAME;

	button_game();
	keypad_game();
}

// Funcion que se encarga de procesar el buzzer asociado con el juego del boton
void buzzer_processing(void)
{
	int button_state;

	if ((tone_timer.finish_time = millis()) - tone_timer.start_time >= MAX_BUTTON_PRESS_DELAY)
	{
		button_state = digitalRead(BUTTON_PIN);

		noTone(BUZZER_PIN);
		sound_playing = false;
		tone_timer.start_time = tone_timer.finish_time;

		if (button_state == HIGH)
		{
#if DEBUG_MODE
			Serial.println("Boton apretado a tiempo");
#endif
			button_points++;

			if (button_points == BUTTON_POINTS_TO_WIN)
			{
				print_game_mode_message = true;
				event = EVENT_CONT;
				current_state = STATE_INIT;

#if DEBUG_MODE
				Serial.println("Ganaste el juego del boton");
#endif
			}

			return;
		}

#if DEBUG_MODE
		Serial.println("Demasiado lento apretando el boton");
#endif

		button_points = 0;
		electrocuting = true;
		shock_timer.start_time = millis();
		modify_electric_shock(true);
	}
}

// Funcion que limpia el contenido de la consola
void clear_serial(void)
{
	if (Serial.available() > 0)
	{
		int value = Serial.read();
	}
}

// Funcion para girar el servo motor de forma aleatoria
void servo(void)
{
	if (((servo_timer.finish_time = millis()) - servo_timer.start_time) >= speed)
	{
		servo_timer.start_time = servo_timer.finish_time;
		servo_motor.write(servo_degrees);

		if (positive)
		{
			if (servo_degrees + 1 == 180)
			{
				speed = MIN_TIME_SERVO;
#if DEBUG_MODE
				Serial.println("Servo motor modo lento");
#endif

				positive = false;
			}
			servo_degrees++;
		}
		else
		{
			if ((servo_degrees - 1 == 0))
			{
				speed = MAX_TIME_SERVO;
#if DEBUG_MODE
				Serial.println("Servo motor modo rapido");
#endif
				positive = true;
			}
			servo_degrees--;
		}
	}
}

// Funcion que retorna true si el sensor esta presionado correctamente, de lo contrario retorna false
bool is_sensor_pressed(void)
{
	return analogRead(SENSOR_PIN) > MIN_SENSOR_PRESSURE;
}

// Procesa el byte cargado en consola
void incoming_byte_processing()
{
	playing = true;
	incoming_byte = Serial.read(); // Lo lee en ascii, si ingreso la letra "a", entonces se recibe el número 97

#if DEBUG_MODE
	Serial.print("Se recibio por Serial Monitor el caracter: ");
	Serial.println((char)incoming_byte);
#endif

	switch (incoming_byte)
	{
	case SerialMonitorCommands::BUTTON: // Paso por Serial Monitor para usar solamente el buzzer y el boton
		print_string_on_lcd("Usando buzzer", 0, 0);
		event = EVENT_CHARACTER_B;
#if DEBUG_MODE
		Serial.println("Modo de juego: buzzer - boton");
#endif
		break;
	case SerialMonitorCommands::KEYPAD: // Paso por Serial Monitor para usar solamente el Keypad
		generate_random_number(random_number_buffer);
		print_string_on_lcd(random_number_buffer, 0, 3);
		clear_lcd = true;
		event = EVENT_CHARACTER_K;
		lcd_clear_timer.start_time = millis();

#if DEBUG_MODE
		Serial.println("Modo de juego: keypad");
#endif
		break;
	case SerialMonitorCommands::NORMAL: // Paso por Serial Monitor el funcionamiento normal (buzzer, boton y keypad)
		generate_random_number(random_number_buffer);
		print_string_on_lcd(random_number_buffer, 0, 3);
		clear_lcd = true;
		event = EVENT_CHARACTER_N;
		lcd_clear_timer.start_time = millis();

#if DEBUG_MODE
		Serial.println("Modo de juego: buzzer - boton - keypad");
#endif
		break;
	default:
		playing = false;
		event = EVENT_UNKNOWK;

#if DEBUG_MODE
		Serial.println("El comando ingresado es invalido");
#endif
		break;
	}
}

// Funcion que modifica el valor de la intensidad del shock
void modify_electric_shock(bool mode)
{
	// True cambia la intensidad, False la apaga
	if (!mode)
	{
		digitalWrite(LED_PIN, 0);

#if DEBUG_MODE
		Serial.println("Terminando choque electrico");
#endif
		return;
	}

	switch (intensity)
	{
	case INTENSITY_LOW:
		digitalWrite(LED_PIN, 70);

#if DEBUG_MODE
		Serial.println("Choque electrico con intensidad baja");
#endif
		break;
	case INTENSITY_MEDIUM:
		digitalWrite(LED_PIN, 150);

#if DEBUG_MODE
		Serial.println("Choque electrico con intensidad media");
#endif
		break;
	case INTENSITY_HIGH:
		digitalWrite(LED_PIN, 255);

#if DEBUG_MODE
		Serial.println("Choque electrico con intensidad alta");
#endif
		break;
	default:
		break;
	}
}

// Funcion que genera un numero random de 10 digitos y lo almacena en el buffer recibido
void generate_random_number(char *buffer)
{
	for (int i = 0; i < RANDOM_NUM_LENGTH; i++)
	{
		*(buffer + i) = random(49, 57); // genero el ascii de un numero entre 1-9
	}

	*(buffer + RANDOM_NUM_LENGTH - 1) = '\0';

#if DEBUG_MODE
	Serial.print("Numero random generado: ");
	Serial.println(buffer);
#endif
}

// Funcion que se encarga de mostrar en el lcd el contenido del buffer que recibe por parametro, row y column son para posicionar el cursor del lcd
void print_string_on_lcd(const char *buffer, int row, int column)
{
	lcd.clear();				// limpio cualquier cosa que tenga el lcd
	lcd.setCursor(row, column); // para que el numero quede centrado
	lcd.print(buffer);

#if DEBUG_MODE
	Serial.print("Buffer a imprimir en el lcd: ");
	Serial.println(buffer);
#endif
}
