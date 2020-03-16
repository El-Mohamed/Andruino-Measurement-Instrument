#include <LiquidCrystal.h>
#include <Servo.h>
#include <EEPROM.h>

byte LEDs[6] = {A0, A1, A2, A3, A4, A5};  // INDICATOR EN MEET LEDS
int PIN_A = 2, PIN_B = 4, SELECT_BUTTON = 7;      // De rotary Encoder pinnen + button

const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 13;
const int Trigger_Pin = 3, Echo_Pin = 5;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
Servo Servo1;

byte LedSpeed_1 = 100, LedSpeed_2 = 200;
bool Blink = false;
unsigned long Last_Blink = 0;

String Menu_Items[][6] = {
  {"MES DISTANCE", "MES ANGLE"},
  {"SET MIN", "SET MAX", "SHOW LED VALUE", "START/STOP ", "BACK"},
  {"SET ANGLE", "ACTIVATE ANGLE", "BACK"},
  {"MIN VALUE (CM): ", "MAX VALUE (CM): ", "LED VAL (CM): "},
  {"ANGLE (0-180): "}
};

int Lengths[] = {1, 4, 2}; // lengte van de scroll menus

bool ToMeasure = false, ToRotate = false, ToEEPROM = true;
int Current_Distance = 0, Minimum_Distance = 0, Temp_Minimum_Distance = 0, Maximum_Distance = 0, Temp_Maximum_Distance = 0, Led_Value = 0;
byte Servo_Angle = 0, Temp_Servo_Angle = 0;
byte Cursor_Position = 0, Current_Menu = 0;
float Pulse_Input, Calculated_Distance;

volatile bool ToLeft = false, ToRight = false;
unsigned long Last_Time_Read = 0,  Last_Time_Update = 0; // for buuton and update method

void setup() {
  pinMode(SELECT_BUTTON, INPUT);

  pinMode(PIN_A, INPUT);
  pinMode(PIN_B, INPUT);

  pinMode(Trigger_Pin, OUTPUT);
  pinMode(Echo_Pin, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_A), Read_Pin, FALLING);

  for (int i = 0; i < 6; i++) {
    pinMode(LEDs[i], OUTPUT);
  }

  digitalWrite(Trigger_Pin, LOW);

  lcd.begin(16, 2);         // Aantal kolommen en rijen
  Servo1.attach(6);
  Serial.begin(9600);

  Minimum_Distance = EEPROM.read(0);
  Maximum_Distance = EEPROM.read(1);
  Servo_Angle = EEPROM.read(2);
}

void Read_Pin() {
  if (Last_Time_Read + 100 < millis())
  {
    if (digitalRead(PIN_B) == LOW)
    {
      ToLeft = true;
      Last_Time_Read = millis();
    }
    else
    {
      ToRight = true;
      Last_Time_Read = millis();
    }
  }
}
void Handle_Rotary() {
  if (ToLeft) {
    ToLeft = false;
    if (Current_Menu == 0 || Current_Menu == 1 || Current_Menu == 2) {
      Cursor_Position --;
    }
    else if (Current_Menu == 3 ) {
      Minimum_Distance --;
    }
    else if (Current_Menu == 4 ) {
      Maximum_Distance --;
    }
    else if (Current_Menu == 6)
    {
      Servo_Angle--;
    }
  }
  else if (ToRight) {
    ToRight = false;
    if (Current_Menu == 0 || Current_Menu == 1 || Current_Menu == 2) {
      Cursor_Position ++;
    }
    else if (Current_Menu == 3 ) {
      Minimum_Distance ++;
    }
    else if (Current_Menu == 4 ) {
      Maximum_Distance ++;
    }
    else if (Current_Menu == 6)
    {
      Servo_Angle ++;
    }
  }
  /****************************************************************/
  if (Cursor_Position < 0) {
    Cursor_Position = Lengths[Current_Menu];
  }
  if (Cursor_Position > Lengths[Current_Menu]) {
    Cursor_Position = 0;
  }
  if (Servo_Angle < 0) {
    Servo_Angle = 0;
  }
  if (Servo_Angle > 180) {
    Servo_Angle = 180;
  }
  if (Minimum_Distance < 0) {
    Minimum_Distance = 0;
  }
  if (Maximum_Distance < 0) {
    Maximum_Distance = 0;
  }
}
void Check_Button() {
  if (digitalRead(SELECT_BUTTON)) {
    Move_Select();
    Cursor_Position = 0;
    delay(200);
  }
}

void Set_Servo() {
  if (ToRotate == true) {
    Servo1.write(Servo_Angle);
    ToRotate = false;
    delay(100);
  }
}
void Set_All_Led() {

  if (ToMeasure == true) {
    if (Current_Distance > Minimum_Distance && Current_Distance <= (Minimum_Distance + Led_Value))
    {
      Ultrasonic_Led(1);
    }
    else if (Current_Distance > (Minimum_Distance + Led_Value) && Current_Distance <= (Minimum_Distance + (Led_Value * 2)))
    {
      Ultrasonic_Led(2);
    }
    else if (Current_Distance > (Minimum_Distance + (Led_Value * 2)) && Current_Distance <= (Minimum_Distance + (Led_Value * 3)) )
    {
      Ultrasonic_Led(3);
    }
    else if (Current_Distance > (Minimum_Distance + (Led_Value * 3)) && Current_Distance <= (Minimum_Distance + (Led_Value * 4)))
    {
      Ultrasonic_Led(4);
    }
    else if (Current_Distance > (Minimum_Distance + (Led_Value * 4)) && (Current_Distance <= Maximum_Distance) )
    {
      Ultrasonic_Led(5);
    }
    else if (Current_Distance > Maximum_Distance || Current_Distance <= Minimum_Distance ) {
      Ultrasonic_Led(0);
    }
  }
  else if (ToMeasure == false) {
    Ultrasonic_Led(0);
  }

  if (Current_Menu == 0) {
    digitalWrite(LEDs[5], LOW);
  }
  else if (Current_Menu == 1 || Current_Menu == 3 || Current_Menu == 4 || Current_Menu == 5) {
    Blinker(LedSpeed_1);
  }
  else if (Current_Menu == 2 || Current_Menu == 6) {
    Blinker(LedSpeed_2);
  }
}
void Ultrasonic_Led(int choice) {     // Hulp Methode
  if (choice == 0) {
    for (int i = 0; i < 5; i++) {
      digitalWrite(LEDs[i], LOW);
    }
  }
  else {
    choice -= 1;
    for (int i = 0; i < 5; i++) {
      if (choice == i) {
        digitalWrite(LEDs[i], HIGH);
      }
      else {
        digitalWrite(LEDs[i], LOW);
      }
    }
  }
}
void Blinker(int waitTime) {          // Hulp methode
  if (millis() - Last_Blink >= waitTime) {
    Last_Blink = millis();
    if (Blink == LOW) {
      Blink = HIGH;
    } else {
      Blink = LOW;
    }
    digitalWrite(LEDs[5], Blink);
  }
}

int Get_Distance() {
  digitalWrite(Trigger_Pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger_Pin, LOW);
  Pulse_Input = pulseIn(Echo_Pin, HIGH);
  Calculated_Distance = (Pulse_Input / 2) / 29.4;    /* Geluid gaat 0,034 cm per seconde, 1 second/0,034 = 29,411....*/
  return  Calculated_Distance;
}

void Set_Values() {
  if (ToMeasure == true) {
    Current_Distance = Get_Distance();
    Serial.println(Current_Distance);
  }
  Led_Value = ((Maximum_Distance - Minimum_Distance) / 5);
}

void Print(String Part1, int Number) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Part1);
  lcd.setCursor(0, 1);
  lcd.print(Number);
}
void Print(int C_Menu, int C_Pos) {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(Menu_Items[C_Menu][C_Pos]);
  lcd.setCursor(1, 1);

  if ((C_Pos + 1) > Lengths[C_Menu]) {                // Voor het doorlopende scroll effect
    lcd.print(Menu_Items[C_Menu][0]);
  }
  else {
    lcd.print(Menu_Items[C_Menu][C_Pos + 1]);
  }
  lcd.setCursor(0, 0);
  lcd.print(">");
}

void Update_Menu() {
  Set_Servo();
  
  if (millis() > Last_Time_Update + 100) {
    Last_Time_Update = millis();
    Set_All_Led();
    Set_Values();
    
    if (Current_Menu == 0 || Current_Menu == 1 || Current_Menu == 2) {
      Print(Current_Menu, Cursor_Position);
    }
    else if (Current_Menu == 3) {
      Print(Menu_Items[3][0], Minimum_Distance);
    }
    else if (Current_Menu == 4) {
      Print(Menu_Items[3][1], Maximum_Distance);
    }
    else if (Current_Menu == 5) {
      Print(Menu_Items[3][2], Led_Value);
    }
    else if (Current_Menu == 6) {
      Print(Menu_Items[4][0], Servo_Angle);
    }
  }
}
void Move_Select() {
  if (Current_Menu == 0) {
    switch (Cursor_Position)
    {
      case 0:
        Current_Menu = 1;
        break;
      case 1:
        Current_Menu = 2;
        break;
    }
  }
  else if (Current_Menu == 1) {
    switch (Cursor_Position) {
      case 0:
        ToMeasure = false;
        Current_Menu = 3;
        Temp_Minimum_Distance = Minimum_Distance;
        break;
      case 1:
        ToMeasure = false;
        Current_Menu = 4;
        Temp_Maximum_Distance = Maximum_Distance;
        break;
      case 2:
        Current_Menu = 5;
        break;
      case 3:
        if (Minimum_Distance <= (Maximum_Distance - 5)) {
          ToMeasure = !ToMeasure;
        }
        else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("CHECK VALUES");
          delay(5000);
        }
        break;
      case 4:
        Current_Menu = 0;
        break;
    }
  }
  else if (Current_Menu == 2) {
    switch (Cursor_Position) {
      case 0:
        Temp_Servo_Angle = Servo_Angle;
        Current_Menu = 6;
        break;
      case 1:
        ToRotate = true;
        break;
      case 2:
        Current_Menu = 0;
        break;
    }
  }
  else if (Current_Menu == 3) {
    if (Temp_Minimum_Distance != Minimum_Distance && ToEEPROM == true) {
      EEPROM.write(0, Minimum_Distance);
    }
    Current_Menu = 1;
  }
  else if (Current_Menu == 4) {
    if (Temp_Maximum_Distance != Maximum_Distance && ToEEPROM == true) {
      EEPROM.write(1, Maximum_Distance);
    }
    Current_Menu = 1;
  }
  else if (Current_Menu == 5) {
    Current_Menu = 1;
  }
  else if (Current_Menu == 6) {
    if (Temp_Servo_Angle != Servo_Angle && ToEEPROM == true) {
      EEPROM.write(2, Servo_Angle);
    }
    Current_Menu = 2;
  }
}


void loop() {
  Update_Menu();
  Check_Button();
  Handle_Rotary();
}
