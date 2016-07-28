
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>


const int PIN_FILLING_LED = 1;
const int PIN_EMPTYING_LED = 2;
const int TRIGGER_PIN = 6; //blue wire. vcc= light yellow
const int ECO_PIN = 7; //green wire . gnd= gray 
const int BOILER_PIN = 4;
const int TEMPERATURE_PIN = 8;
const int BUTTON_PIN = 12;
const int GAS_VALVE_PIN = 3;
const int LED_BUTTON_PIN = 11;

const int PERIOD_ULTRASONIC_SENSOR = 2000;
const int NUM_AVERAGE_TEMPERATURE = 1;

const float DS18B20_MIN_TEMPERATURE = -20; // -55
const float DS18B20_MAX_TEMPERATURE = 60; // 125
const float DS18B20_SCALE = 255.0 / (DS18B20_MAX_TEMPERATURE - DS18B20_MIN_TEMPERATURE);

const uint8_t I2C_MY_ADDRESS = 0x8;

// commands from master to slave
const uint8_t I2C_CMD_GET_TEMP = 0x2;
const uint8_t I2C_CMD_GET_ULTRASONIC = 0x3;
const uint8_t I2C_CMD_NONE = 0x0;

uint8_t i2c_command = I2C_CMD_NONE;

unsigned long last_time_ultrasonic_sensor;
float average_temperature = 0;
float average_distance = 0;


boolean boiler_on = false;
boolean button_down = true;
boolean previous_button_down = true;

OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Wire.begin(I2C_MY_ADDRESS);
  Wire.onRequest(i2cSend);
  Wire.onReceive(i2cReceive);

  Serial.begin(9600);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECO_PIN, INPUT);
  pinMode(PIN_FILLING_LED, OUTPUT);
  pinMode(PIN_EMPTYING_LED, OUTPUT);
  pinMode(BOILER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(GAS_VALVE_PIN, OUTPUT);
  pinMode(LED_BUTTON_PIN, OUTPUT);

  digitalWrite(TRIGGER_PIN, LOW);
  digitalWrite(PIN_FILLING_LED, LOW);
  digitalWrite(PIN_EMPTYING_LED, LOW);
  digitalWrite(BOILER_PIN, HIGH);
  digitalWrite(GAS_VALVE_PIN, HIGH);
  digitalWrite(LED_BUTTON_PIN, LOW);

  sensors.begin();

  turn_off_boiler();
  read_button();
}

void loop() {

  unsigned long cur_time;
  cur_time = millis();
  if (cur_time < last_time_ultrasonic_sensor) {
    last_time_ultrasonic_sensor = 0;
  }

  if (previous_button_down) {
    Serial.println("Button was down");
  } else {
    Serial.println("Button was up");
  }
  read_button();
  if (button_down) {
    turn_off_boiler();
    turn_on_gas_valve();
    
    temperature_reading();
    ultrasonic_sensor_reading();
    Serial.println("The cooking mode");
    previous_button_down = true;
  }

  if (!button_down && cur_time > last_time_ultrasonic_sensor + PERIOD_ULTRASONIC_SENSOR) {

    turn_off_gas_valve();
    Serial.println("The Ecogen burner mode");
    temperature_reading();
    ultrasonic_sensor_reading();
    Serial.print("The distance from ultrasonic sensor is:");
    Serial.print(average_distance);
    Serial.println("cm");
    Serial.print("Temperature: ");
    Serial.print(average_temperature);
    Serial.println("C");

    if (boiler_on || previous_button_down) {
      if (average_distance > 75) {
        turn_off_boiler();
        Serial.println("The boiler is off");
      }
      else {
        turn_on_boiler();
        Serial.println("The boiler is on");
      }
    } else {
      if (average_distance< 15) {
        turn_on_boiler();
        Serial.println("The boiler is on");
      }
      else {
        turn_off_boiler();
        Serial.println("The boiler is off");
      }
    }
    previous_button_down = false;
  }
}

int ultrasonic_sensor_reading() {
  int travel_time;
  float distance_sum = 0;
  //sends out the ultrasonic wave
  for (int i = 0; i < 20; i++) {
    digitalWrite(TRIGGER_PIN, HIGH);
    delay(100);
    digitalWrite(TRIGGER_PIN, LOW);

    travel_time = pulseIn(ECO_PIN, HIGH);

    //The approximate speed of sound in dry air is given by the formula:
    //c = 331.5 + 0.6 * [air temperature in degrees Celsius]
    //At 20°C, c = 331.5 + 0.6 * 20 = 343.5 m/s
    //If we convert the speed in centimetres per microseconds we get:
    //c = 343.5 * 100 / 1000000 = 0.03435 cm/ss
    //The distance is therefore, D = (Δt/2) * c
    //or
    //The Pace of Sound = 1 / Speed of Sound = 1 / 0.03435 = 29.1 ss/cm
    float sound_speed = (331.5 + 0.6 * average_temperature) / 10000;
    distance_sum += (travel_time / 2) * sound_speed ;
  }
  average_distance = distance_sum / 20;
  return average_distance;
}

float temperature_reading() {
  float temperature_sum = 0;
  for (int i = 0; i < NUM_AVERAGE_TEMPERATURE; i++) {
    sensors.requestTemperatures(); // Send the command to get temperatures
    temperature_sum += sensors.getTempCByIndex(0);
  }
  average_temperature = temperature_sum / NUM_AVERAGE_TEMPERATURE;
  return average_temperature;
}

void turn_on_boiler() {
  boiler_on = true;
  digitalWrite(BOILER_PIN, LOW);
  digitalWrite(PIN_FILLING_LED, HIGH);
  digitalWrite(PIN_EMPTYING_LED, LOW);
}


void turn_off_boiler() {
  boiler_on = false;
  digitalWrite(BOILER_PIN, HIGH);
  digitalWrite(PIN_EMPTYING_LED, HIGH);
  digitalWrite(PIN_FILLING_LED, LOW);

}

void read_button() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    button_down = true;
  } else {
    button_down = false;

  }
}

void turn_on_gas_valve() {
  digitalWrite(GAS_VALVE_PIN, LOW);
  digitalWrite(LED_BUTTON_PIN, HIGH);
}

void turn_off_gas_valve() {
  digitalWrite(GAS_VALVE_PIN, HIGH);
  digitalWrite(LED_BUTTON_PIN, LOW);
}

void i2cReceive(int howMany) {
  while (Wire.available() > 0) {
    // read the current command
    i2c_command = Wire.read();
  }
}

void i2cSend() {
  switch (i2c_command) {
    case I2C_CMD_GET_TEMP:
      {
        // encode temperature
        float normalised_temperature = average_temperature;
        normalised_temperature -= DS18B20_MIN_TEMPERATURE;
        normalised_temperature = min(normalised_temperature, DS18B20_MAX_TEMPERATURE - DS18B20_MIN_TEMPERATURE);
        normalised_temperature *= DS18B20_SCALE;
        uint16_t encoded_temperature = normalised_temperature;
        Wire.write(encoded_temperature);

      }
      break;
    case I2C_CMD_GET_ULTRASONIC:
      Wire.write((int)average_distance);
      break;
    default:
      Serial.print("ERROR: Receieved unknown command byte ");
      Serial.print(i2c_command, HEX);
      Serial.println("!");
      Wire.write(0xff);
  }
}



