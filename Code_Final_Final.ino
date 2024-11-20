#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <NewPing.h>
#include <Stepper.h>
#include <SPI.h>
#include <MFRC522.h>

#define TRIGGER_PIN 12
#define ECHO_PIN 11    
#define distance 15   

#define RELAY_1_PIN 2 // Broche de commande du relai 1
#define RELAY_2_PIN 3 // Broche de commande du relai 2

LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD
NewPing sonar(TRIGGER_PIN, ECHO_PIN, distance); //Ultra-son

// KEY-PAD
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {7, 8, 9, 10};
byte colPins[COLS] = {11, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int trigPin = 6;  // Broche de déclenchement
const int echoPin = 4; // Broche de retour
const int stepsPerRevolution = 2048; // Pour le moteur pas à pas 28BYJ-48
Stepper myStepper(stepsPerRevolution, 22, 24, 26, 28); // (pas, pin1, pin2, pin3, pin4)

long duration;
int Distance;

// RFID
#define SS_PIN 53 // SDA pin of RFID
#define RST_PIN 5 // RST pin of RFID
MFRC522 rfid(SS_PIN, RST_PIN);

// List of authorized UIDs
byte authorizedUIDs[][4] = {
  {0xF3, 0xFA, 0x20, 0xB7}, // Replace with actual UIDs
  {0xF3, 0xDD, 0x0F, 0xAA}
};

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(RELAY_1_PIN, OUTPUT); // Initialisation du relai 1
  pinMode(RELAY_2_PIN, OUTPUT);
  digitalWrite(RELAY_1_PIN, HIGH); // Initialisation du relai 2
  digitalWrite(RELAY_2_PIN, HIGH); // Initialisation du relai 2
  
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID card");
  
  myStepper.setSpeed(5); // Réglage de la vitesse en tr/min, à ajuster selon les besoins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize SPI bus and MFRC522
  SPI.begin();
  rfid.PCD_Init();
}

bool isAuthorizedUID(byte *uid) {
  for (int i = 0; i < sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]); i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (authorizedUIDs[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Check if the card is authorized
  if (!isAuthorizedUID(rfid.uid.uidByte)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unauthorized");
    lcd.setCursor(0, 1);
    lcd.print("Scan again");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan RFID card");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selectionnez");
  lcd.setCursor(0, 1);
  lcd.print("votre boisson:");
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  char key = 0;
  while (!key) {
    key = keypad.getKey();
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Boisson ");
  lcd.print(key);

  switch (key) {
    case '1':
      controlPumps(100, 0);
      break;
    case '2':
      controlPumps(50, 50);
      break;
    case '3':
      controlPumps(25, 75);
      break;
    case '4':
      controlPumps(25, 75);
      break;
    case '5':
      controlPumps(0, 100);
      break;
    default:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Selection err");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Selectionnez");
      lcd.setCursor(0, 1);
      lcd.print("votre boisson:");
      return;
  }

  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selectionnez");
  lcd.setCursor(0, 1);
  lcd.print("votre boisson:");

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  Distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(Distance);
  Serial.println(" cm");

  if (Distance < distance) {
    myStepper.step(stepsPerRevolution);
    delay(1000);
  }

  delay(6000);
}

void controlPumps(int pump1, int pump2) {
  // Active les relais en fonction de la configuration des pompes
  if (pump1 > 0) {
    digitalWrite(RELAY_1_PIN, LOW);
    delay(pump1 * 50); // Convertit le temps en secondes en millisecondes
    digitalWrite(RELAY_1_PIN, HIGH); // Arrête la pompe 1
  }
  
  if (pump2 > 0) {
    digitalWrite(RELAY_2_PIN, LOW);
    delay(pump2 * 50); // Convertit le temps en secondes en millisecondes
    digitalWrite(RELAY_2_PIN, HIGH); // Arrête la pompe 2
  }
}