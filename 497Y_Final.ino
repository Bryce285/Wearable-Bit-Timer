/*
    TODO - DOUBLE CHECK THAT THESE PIN ASSIGNMENTS WORK WITH CIRCUIT LAYOUT BEFORE CONSTRUCTION
*/
const int SWITCH = 10;
const int BUTTON = A9;
const int LIGHT_SENSOR = A8;
const int BUZZER = A7;

/*
    LEDs are labeled according to the bit significance of their position
    on the garment.
    LED0 is in the place of the least significant bit while LED5
    is in the place of the most significant bit.
*/
const int LED0 = A4;
const int LED1 = A5;
const int LED2 = 6;
const int LED3 = A3;
const int LED4 = A2;

void setup() 
{
  pinMode(SWITCH, INPUT);
  digitalWrite(SWITCH, HIGH);

  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);

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

struct Timer
{
  AlertMode cur_mode = AlertMode::ALL;
  bool active = false;
  bool done = false;

  bool input_received = false;

  unsigned long time_remaining = 0;
  unsigned long last_tick = 0;

  AlertMode next_mode(AlertMode mode)
  {
    switch (mode)
    {
      case AlertMode::BUZZ: return AlertMode::LIGHT;
      case AlertMode::LIGHT: return AlertMode::ALL;
      case AlertMode::ALL: return AlertMode::VARIABLE;
      case AlertMode::VARIABLE return AlertMode::BUZZ;
    }

    return AlertMode::ALL;
  }

  /*
      This function essentially stores the time remaining for the timer in seconds
  */
  void set(unsigned int start_val_hrs)
  {
    unsigned long start_val_hrsUL = static_cast<unsigned long>(start_val_hrs);
    time_remaining = start_val_hrsUL * 60UL * 60UL;
  }

  /*
      A tick is approximately one second 
      It's not exact but it's close enough for this project
  */
  bool decrement()
  {
    if (millis() - last_tick >= 1000) {
      last_tick = millis();
      time_remaining--;

      if (time_remaining == 0) {
        done = true;
        return true;
      }
    }

    return false;
  }

  unsigned long to_hours()
  {
    if (time_remaining < 3600) return 0;
    return (time_remaining / 60UL) / 60UL;
  }

  void buzz_mode()
  {
    if (input_received) {
      digitalWrite(BUZZER, LOW);
      timer.active = false;
      return;
    }

    digitalWrite(BUZZER, HIGH);
  }

  /* This implementation makes the lights blink without blocking user input */
  void light_mode(unsigned long call_time_millis)
  {
    unsigned long time = call_time_millis;
    while (millis - time <= 250) {
      if (input_received) {
        digitalWrite(LED0, LOW);
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);
        digitalWrite(LED4, LOW);

        timer.active = false;
        return;
      }

      digitalWrite(LED0, HIGH);
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, HIGH);
      digitalWrite(LED4, HIGH);
    }

    time = millis();
    while (millis - time <= 250) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(LED4, LOW);

      if (input_received) {
        timer.active = false;
        return;
      }
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
      light_mode(millis());
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
      light_mode(millis());
    }
    else if (cur_mode == AlertMode::ALL) {
      buzz_mode();
      light_mode(millis());
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

struct Button
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

struct LEDs
{
  void write_value(unsigned int value)
  {
    if (value > 24) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(LED4, LOW);
      return;
    }

    unsigned int ones_bit = value & 1U;
    unsigned int twos_bit = (value & (1U << 1)) >> 1;
    unsigned int threes_bit = (value & (1U << 2)) >> 2;
    unsigned int fours_bit = (value & (1U << 3)) >> 3;
    unsigned int fives_bit = (value & (1U << 4)) >> 4;

    if (ones_bit == 1) digitalWrite(LED0, HIGH);
    else digitalWrite(LED0, LOW);

    if (twos_bit == 1) digitalWrite(LED1, HIGH);
    else digitalWrite(LED1, LOW);

    if (threes_bit == 1) digitalWrite(LED2, HIGH);
    else digitalWrite(LED2, LOW);

    if (fours_bit == 1) digitalWrite(LED3, HIGH);
    else digitalWrite(LED3, LOW);

    if (fives_bit == 1) digitalWrite(LED4, HIGH);
    else digitalWrite(LED4, LOW);
  }
};

Timer timer;
Button button;
LEDs leds;

void loop()
{
  if (timer.active && timer.decrement() == true) {
    timer.alert();
  }

  leds.write_value(timer.to_hours());

  /*
      Holding the button down increments the starting timer value each second up to a
      starting value of 24 hours.
      Pressing the button for less than 1 second cycles through the alert modes.
  */
  unsigned int val_hrs = 0;
  if ((digitalRead(BUTTON) == LOW) && button.was_pressed) {
    button.cur_press_duration = millis() - button.press_start;

    /*check current press duration and light up LEDs accordingly to show
    the starting timer value */
    if (button.cur_press_duration > 1000) {
      val_hrs = button.cur_press_duration / 1000;
      if (val_hrs > 24) val_hrs = 24;

      leds.write_value(val_hrs);
    }
  }
  else if ((digitalRead(BUTTON) == LOW) && !button.was_pressed) {
    button.press_start = millis();
    button.was_pressed = true;
    timer.input_received = true;
  }
  else if ((digitalRead(BUTTON) == HIGH) && button.was_pressed) {
    button.final_press_duration = millis() - button.press_start;
    button.was_pressed = false;

    if (button.final_press_duration < 1000) {
      timer.cur_mode = timer.next_mode(timer.cur_mode);
    }
    else if (button.final_press_duration > 1000) {
      timer.active = true;
      timer.done = false;
      timer.set(val_hrs);
    }

    button.reset();
    timer.input_received = false;
  }

  /* the switch lets users cancel the current timer */
  if (digitalRead(SWITCH) == LOW) {
    timer.reset();
  }
}
