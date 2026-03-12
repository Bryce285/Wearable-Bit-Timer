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

namespace Timer
{
  AlertMode cur_mode = AlertMode::ALL;
  bool active = false;
  bool done = false;

  bool input_received = false;

  unsigned int selected_mins = 0;
  unsigned long time_remaining = 0;
  unsigned long last_tick = 0;

  int light_read = -1;

  AlertMode next_mode(AlertMode mode)
  {
    switch (mode)
    {
      case AlertMode::BUZZ: 
        Serial.println("Light mode set");
        return AlertMode::LIGHT;
      case AlertMode::LIGHT:
        Serial.println("All mode set"); 
        return AlertMode::ALL;
      case AlertMode::ALL:
        Serial.println("Variable mode set"); 
        return AlertMode::VARIABLE;
      case AlertMode::VARIABLE:
        Serial.println("Buzz mode set"); 
        return AlertMode::BUZZ;
      default:
        Serial.println("All mode set"); 
        return AlertMode::ALL;
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

  void reset()
  {
    cur_mode = AlertMode::ALL;
    active = false;
    done = false;
    input_received = false;
    time_remaining = 0;
    last_tick = 0;
    light_read = -1;
  }

  void buzz_mode()
  {
    if (input_received) {
      noTone(BUZZER);
      reset();
      return;
    }

    if (cur_mode != AlertMode::ALL && (cur_mode != AlertMode::VARIABLE && analogRead(LIGHT_SENSOR) > 20)) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(LED4, LOW);
    }

    tone(BUZZER, 440);
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
      reset();
      return;
    }

    if (cur_mode != AlertMode::ALL) noTone(BUZZER);

    static unsigned long last_step = 0;
    static uint8_t pos = 0;

    if (millis() - last_step >= 500) {
      last_step = millis();
      LEDs::write_value((1 << (pos + 1)) - 1);
      pos = (pos + 1) % 5;
    }
  }

  void all_mode()
  {
    buzz_mode();
    light_mode();
  }

  void variable_mode()
  {
    if (light_read < 0) {
      light_read = analogRead(LIGHT_SENSOR);
    }

    if (light_read < 20) {
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
};

namespace Button
{
  unsigned long press_start = 0;
  unsigned long last_change_time = 0;
  bool last_state = HIGH;
  bool stable_state = HIGH;
  bool was_pressed = false;

  const unsigned long DEBOUNCE_MS = 50;
};

unsigned long last_read = millis();
int light;

int raw_button;
unsigned long now;
unsigned long press_duration;
unsigned long hold_duration;

void loop()
{
  if (millis() - last_read > 5000) {
    light = analogRead(LIGHT_SENSOR);
    Serial.print("Light sensor reading: ");
    Serial.println(light);
    last_read = millis();
  }

  if (Timer::active) {
    if (Timer::decrement()) {
      Timer::done = true;
    }

    if (Timer::done) {
      Timer::alert();
    }
  }

  Timer::input_received = false;

  if (!Timer::done) {
    LEDs::write_value(Timer::to_mins());
  }

  raw_button  = digitalRead(BUTTON);
  now = millis();

  if (raw_button != Button::last_state) {
    Button::last_change_time = now;
    Button::last_state = raw_button;
  }

  if (now - Button::last_change_time > Button::DEBOUNCE_MS) {
    if (Button::stable_state != raw_button) {
      Button::stable_state = raw_button;

      if (Button::stable_state == LOW) {
        Button::press_start = now;
        Button::was_pressed = true;
        Timer::input_received = true;
      }

      else if (Button::stable_state == HIGH && Button::was_pressed) {
        press_duration = now - Button::press_start;
        Button::was_pressed = false;

        if (press_duration < 1000) {
          Timer::cur_mode = Timer::next_mode(Timer::cur_mode);
        }
        else {
          Timer::set(Timer::selected_mins);
          Timer::active = true;
          Timer::done = false;
        }
      }
    }
  }

  if (Button::was_pressed && Button::stable_state == LOW) {
    hold_duration = now - Button::press_start;
    if (hold_duration > 1000) {
      Timer::selected_mins = hold_duration / 1000;
      if (Timer::selected_mins > 31) Timer::selected_mins = 31;
      LEDs::write_value(Timer::selected_mins);
    }
  }

  
  if (digitalRead(SWITCH) == LOW) {
    Timer::reset();
  }
}
