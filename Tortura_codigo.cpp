// C++ code
// Includes
#include <stdio.h>
#include <Keypad.h>			//para el keypad
#include <LiquidCrystal.h>	//para el lcd

//Defines
#define NUM_ROWS_LCD 3			//cantidad de filas que usamos en la pantalla lcd
#define NUM_COLS_LCD 3			//cantidad de columnas que usamos en la pantalla lcd
#define NUM_ROWS_KEYPAD 3		//cantidad de filas que usamos en el keypad
#define NUM_COLS_KEYPAD 3		//cantidad de columnas que usamos en el keypad
#define RANDOM_NUM_LENGTH 11	//cantidad de digitos que queremos que tenga el numero random (contemplando el '/0')


//Variables globales
LiquidCrystal lcd(1,2,A1,A2,A3,A4);	//los parametros son los pines a los cuales esta conectado el lcd
char random_number_buffer[RANDOM_NUM_LENGTH]; // almacena el número random
int LED = 3;
int buzzer = 4;
int button = 5;
int buttonState = 0;
int key_count = 0;
int pain_level = 255; // 0 a 255

//mapa de teclas que vamos a usar del keypad
char key_map[NUM_ROWS_KEYPAD][NUM_COLS_KEYPAD] = {
	{'1','2','3'},
	{'4','5','6'},
	{'7','8','9'}
};

byte row_pins[NUM_ROWS_KEYPAD] = {12,11,10}; 	//pines a los cuales conectamos las filas del keypad
byte col_pins[NUM_COLS_KEYPAD] = {9,8,7}; 		//pines a los cuales conectamos las columnas del keypad

// Inicializamos el keypad
Keypad keypad = Keypad(makeKeymap(key_map),row_pins,col_pins,NUM_ROWS_KEYPAD,NUM_COLS_KEYPAD);

//Prototipos
void keypad_game();
void button_game();
void generate_random_number(char *buffer);

// Escribimos el buffer con los dígitos aleatorios
void generate_random_number(char *buffer) {

	int random_digit;

	for (int i = 0; i < RANDOM_NUM_LENGTH - 1; i++) {
	randomSeed(analogRead(A0)); // para mayor aleatoriedad en los números leemos un pin no conectado
	random_digit = random(49, 58); // números del 1 al 9 en ASCII
	*(buffer + i) = (char) random_digit;
	}

}

void keypad_game() {
	char key = keypad.getKey();

	if (key != NO_KEY){

	Serial.print(key);

	// Acierto
	if (key == *(random_number_buffer + key_count)) {
		key_count++;
	}
	else { // Pierde
		key_count = 0;
		Serial.println("PICANAZO");
		analogWrite(LED, pain_level);
	}

	// GANA
	if (key_count == RANDOM_NUM_LENGTH - 1) {
		Serial.println("GANASTE");
	}

	}
}

void button_game() {
	lcd.print("Presione cuando sonido");
	int random_number;

	buttonState = digitalRead(button);

	

	if (buttonState == HIGH) {
		Serial.println(buttonState);
	}

	// la idea para esperar un tiempo random y que suene el buzzer es
	// generar numeros random y cuando sea igual a uno que elegimos nosotros suene

	// while (true) {
	// 	delay(random(3000, 7000));

	// 	tone(buzzer, 800);
	// 	delay(300);
		
	// 	noTone(buzzer);

	// }


}

void setup()
{
	Serial.begin(9600);

	pinMode(LED, OUTPUT);
	pinMode(buzzer, OUTPUT);
	pinMode(button, INPUT);
	generate_random_number(random_number_buffer);
	lcd.print(random_number_buffer);
	

}

void loop()
{ 
	
	// keypad_game();
	button_game();
	
}