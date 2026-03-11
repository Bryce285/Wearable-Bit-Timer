/*
    TODO - DOUBLE CHECK THAT THESE PIN ASSIGNMENTS WORK WITH CIRCUIT LAYOUT BEFORE CONSTRUCTION
*/
const int SWITCH = 10;
const int BUTTON = 11;
const int LIGHT_SENSOR = A2;
const int BUZZER = A3;

/*
    LEDs are labeled according to the bit significance of their position
    on the garment.
    LED0 is in the place of the least significant bit while LED4
    is in the place of the most significant bit.
*/
const int LED0 = A4;
const int LED1 = A5;
const int LED2 = 6;
const int LED3 = A7;
const int LED4 = A8;

void setup() 
{
  Serial.begin(9600);

  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);

  pinMode(LIGHT_SENSOR, INPUT);
  
  pinMode(BUZZER, OUTPUT);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
}

enum class AlertMode 
{
  BUZZ,         // alert with buzzer only
  LIGHT,        // alert with LEDs only
  ALL,          // alert with both buzzer and LEDs
  VARIABLE      // alert based on current light level
};

namespace Timer
{
  AlertMode cur_mode = AlertMode::ALL;
  bool active = false;
  bool done = false;

  bool input_received = false;

  unsigned int selected_mins = 0;
  unsigned long time_remaining = 0;
  unsigned long last_tick = 0;

  AlertMode next_mode(AlertMode mode)
  {
    switch (mode)
    {
      case AlertMode::BUZZ: return AlertMode::LIGHT;
      case AlertMode::LIGHT: return AlertMode::ALL;
      case AlertMode::ALL: return AlertMode::VARIABLE;
      case AlertMode::VARIABLE: return AlertMode::BUZZ;
      default: return AlertMode::ALL;
    }

    return AlertMode::ALL;
  }

  /*
      This function converts minutes to seconds for the timer
  */
  void set(unsigned int start_val_mins)
  {
    unsigned long start_val_minsUL = static_cast<unsigned long>(start_val_mins);
    time_remaining = start_val_minsUL * 60UL;
    last_tick = millis();
  }

  /*
      A tick is approximately one second 
      It's not exact but it's close enough for this project
  */
  bool decrement()
  {
    if (millis() - last_tick >= 1000) {
      last_tick = millis();

      if (time_remaining > 0) {
        time_remaining--;
      }

      if (time_remaining == 0) {
        done = true;
        return true;
      }
    }

    return false;
  }

  unsigned long to_mins()
  {
    if (time_remaining < 60) return 0;
    return (time_remaining / 60UL);
  }

  void buzz_mode()
  {
    if (input_received) {
      digitalWrite(BUZZER, LOW);
      active = false;
      return;
    }

    digitalWrite(BUZZER, HIGH);
  }

  /* This implementation makes the lights blink without blocking user input */
  void light_mode()
  {
    if (input_received) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(LED4, LOW);
      active = false;
      return;
    }

    static unsigned long last_blink = 0;
    static bool state = false;

    if (millis() - last_blink >= 250) {
      last_blink = millis();
      state = !state;

      digitalWrite(LED0, state);
      digitalWrite(LED1, state);
      digitalWrite(LED2, state);
      digitalWrite(LED3, state);
      digitalWrite(LED4, state);
    }
  }

  void all_mode()
  {
    buzz_mode();
    light_mode();
  }

  void variable_mode()
  {
    if (analogRead(LIGHT_SENSOR) < 20) {
      light_mode();
    }
    else {
      buzz_mode();
    }
  }

  void alert()
  {
    if (cur_mode == AlertMode::BUZZ) {
      buzz_mode();
    }
    else if (cur_mode == AlertMode::LIGHT) {
      light_mode();
    }
    else if (cur_mode == AlertMode::ALL) {
      buzz_mode();
      light_mode();
    }
    else {
      variable_mode();
    }
  }

  void reset()
  {
    cur_mode = AlertMode::ALL;
    active = false;
    done = false;
    input_received = false;
    time_remaining = 0;
    last_tick = 0;
  }
};

namespace Button
{
  unsigned long press_start = 0;
  unsigned long cur_press_duration = 0;
  unsigned long final_press_duration = 0;
  bool was_pressed = false;

  void reset()
  {
    press_start = 0;
    cur_press_duration = 0;
    final_press_duration = 0;
    was_pressed = false;
  }
};

namespace LEDs
{
  void write_value(unsigned int value)
  {
    if (value > 31) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(LED4, LOW);
      return;
    }

    unsigned int ones_bit = value & 1U;
    unsigned int twos_bit = (value & (1U << 1)) >> 1;
    unsigned int fours_bit = (value & (1U << 2)) >> 2;
    unsigned int eights_bit = (value & (1U << 3)) >> 3;
    unsigned int sixteens_bit = (value & (1U << 4)) >> 4;

    if (ones_bit == 1) digitalWrite(LED0, HIGH);
    else digitalWrite(LED0, LOW);

    if (twos_bit == 1) digitalWrite(LED1, HIGH);
    else digitalWrite(LED1, LOW);

    if (fours_bit == 1) digitalWrite(LED2, HIGH);
    else digitalWrite(LED2, LOW);

    if (eights_bit == 1) digitalWrite(LED3, HIGH);
    else digitalWrite(LED3, LOW);

    if (sixteens_bit == 1) digitalWrite(LED4, HIGH);
    else digitalWrite(LED4, LOW);
  }
};

void loop()
{
  if (Timer::active) {
    if (Timer::decrement()) {
      Timer::done = true;
    }

    if (Timer::done) {
      Timer::alert();
    }
  }

  LEDs::write_value(Timer::to_mins());

  /*
      Holding the button down increments the starting timer value each second up to a
      starting value of 31 minutes.
      Pressing the button for less than 1 second cycles through the alert modes.
  */
  if ((digitalRead(BUTTON) == LOW) && Button::was_pressed) {
    Serial.println("Button pressed");
    Button::cur_press_duration = millis() - Button::press_start;

    /*check current press duration and light up LEDs accordingly to show
    the starting timer value */
    if (Button::cur_press_duration > 1000) {
      Timer::selected_mins = Button::cur_press_duration / 1000;
      if (Timer::selected_mins > 31) Timer::selected_mins = 31;

      LEDs::write_value(Timer::selected_mins);
    }
  }
  else if ((digitalRead(BUTTON) == LOW) && !Button::was_pressed) {
    Serial.println("Button pressed");
    Button::press_start = millis();
    Button::was_pressed = true;
    Timer::input_received = true;
  }
  else if ((digitalRead(BUTTON) == HIGH) && Button::was_pressed) {
    Button::final_press_duration = millis() - Button::press_start;
    Button::was_pressed = false;

    if (Button::final_press_duration < 1000) {
      Timer::cur_mode = Timer::next_mode(Timer::cur_mode);
    }
    else if (Button::final_press_duration > 1000) {
      Timer::active = true;
      Timer::done = false;
      Timer::set(Timer::selected_mins);
    }

    Button::reset();
    Timer::input_received = false;
  }

  /* the switch lets users cancel the current timer */
  if (digitalRead(SWITCH) == LOW) {
    Serial.println("Switch flipped");
    Timer::reset();
  }
}
