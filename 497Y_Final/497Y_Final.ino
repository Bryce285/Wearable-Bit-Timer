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

/*
    The setup function runs once at the start of the programs execution.
    Here we set the pin modes, enable the pullup resistor for the switch and the button, 
    and setup serial communication.
*/
void setup() 
{
  Serial.begin(9600);

  /* Enabling the pullup resistor for the switch and the button prevents unknown values from being read */
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

/* 
    We create an enum class to store the various alert modes so that we
    can easily reference them by name later in the code.
*/
enum class AlertMode 
{
  BUZZ,         // alert with buzzer only
  LIGHT,        // alert with LEDs only
  ALL,          // alert with both buzzer and LEDs
  VARIABLE      // alert based on current light level
};

/*
    This namespace contains all functions/variables relating to the LEDs
*/
namespace LEDs
{

  /*
      The write_value function takes a value of up to 31 and displays that value as a binary
      number using the LEDs. The max value that can be displayed is 31 because that is the
      maximum value that can be represented by 5 bits.
  */
  void write_value(unsigned int value)
  {
    /* Error handling in case a value greater than 31 is passed to the function */
    if (value > 31) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(LED4, LOW);
      return;
    }

    /* Mask the target bit and move it to the ones place */
    unsigned int ones_bit = value & 1U;
    unsigned int twos_bit = (value & (1U << 1)) >> 1;
    unsigned int fours_bit = (value & (1U << 2)) >> 2;
    unsigned int eights_bit = (value & (1U << 3)) >> 3;
    unsigned int sixteens_bit = (value & (1U << 4)) >> 4;

    /* Check each bit and turn the led on if it is set */
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

/*
    This namespace contains functions/variables related to the timing and alerting
*/
namespace Timer
{
  /* Initialize variables that will be used for the timing and alert logic and set default values */
  AlertMode cur_mode = AlertMode::ALL;
  bool active = false;
  bool done = false;
  bool input_received = false;
  unsigned int selected_mins = 0;
  unsigned long time_remaining = 0;
  unsigned long last_tick = 0;
  int light_read = -1;

  /* 
      Take an alert mode and return the next mode in the sequence.
      This is a helper function used to change the mode when the user pushes the button.
  */
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
      Take a value in minutes and convert it to seconds.
      This function is used to set the initial value of the timer.
  */
  void set(unsigned int start_val_mins)
  {
    unsigned long start_val_minsUL = static_cast<unsigned long>(start_val_mins);
    time_remaining = start_val_minsUL * 60UL;
    last_tick = millis();
  }

  /*
      Decrement one second from the timer and set the done flag if the timer has
      reached zero.
      
      This function uses "ticks" to track the time. A tick is not guaranteed to
      be exactly equal to one second but it is close enough for this project.
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

  /*
      Get the value in minutes of the time_remaining variable, which
      stores time in seconds.

      This function is used to get the time value that will be displayed 
      by the LEDs.
  */
  unsigned long to_mins()
  {
    if (time_remaining < 60) return 0;
    return (time_remaining / 60UL);
  }

  /*
      Reset all the namespace variables to their defaults.

      This function is used when the user flips the switch and
      when the user pushes the button when the timer is alerting.
  */
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

  /*
      Turns on the buzzer.

      This function is used when the timer is alerting in a mode that uses the buzzer.
  */
  void buzz_mode()
  {
    if (input_received) {
      noTone(BUZZER);
      reset();
      return;
    }

    /* Make sure that the LEDs are off if the current alert mode does not involve them */
    if (cur_mode != AlertMode::ALL && (cur_mode != AlertMode::VARIABLE && analogRead(LIGHT_SENSOR) > 20)) {
      digitalWrite(LED0, LOW);
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      digitalWrite(LED4, LOW);
    }

    tone(BUZZER, 440);
  }

  /*
      Turns on the LED alert sequence.

      This function is used when the timer is alerting in a mode that uses the LEDs.
  */
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

    /* Make sure the buzzer is off if the current alert mode does not use it */
    if (cur_mode != AlertMode::ALL && (cur_mode != AlertMode::VARIABLE && analogRead(LIGHT_SENSOR) < 20)) {
      noTone(BUZZER);
    }

    static unsigned long last_step = 0;
    static uint8_t pos = 0;

    /* LEDs turn on sequentially every half second */
    if (millis() - last_step >= 500) {
      last_step = millis();
      LEDs::write_value((1 << (pos + 1)) - 1);
      pos = (pos + 1) % 5;
    }
  }

  /*
      Activate both the buzzer and the LED alert sequence.
  */
  void all_mode()
  {
    buzz_mode();
    light_mode();
  }

  /*
      Activate either the buzzer or the LED alert sequence based
      on the light level that the light sensor read when the alert
      began.

      We cannot take a constant reading because the light from the
      LEDs may interfere.
  */
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

  /*
      Call the alert mode function based on the currently
      selected mode.
  */
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

/*
    This namespace contains variables relating to the button logic.
*/
namespace Button
{
  /* Initialize variables to default values */
  unsigned long press_start = 0;
  unsigned long last_change_time = 0;
  bool last_state = HIGH;
  bool stable_state = HIGH;
  bool was_pressed = false;

  int raw_button = 0;
  unsigned long now = 0;
  unsigned long press_duration = 0;
  unsigned long hold_duration = 0;

  const unsigned long DEBOUNCE_MS = 50;
};

/* 
    Some variables that are used to make debug prints regarding the 
    light sensor readings to the serial monitor.

    We initialize them outside of the loop function to avoid 
    having initialization overhead in every loop.
*/
unsigned long last_read = millis();
int light;

/*
    The loop function runs continuously as long as the program is executing.
*/
void loop()
{
  /* Print a light sensor reading to the serial monitor every 5 seconds */
  if (millis() - last_read > 5000) {
    light = analogRead(LIGHT_SENSOR);
    Serial.print("Light sensor reading: ");
    Serial.println(light);
    last_read = millis();
  }

  /*
      Decrement the timer and start the alert if it is done.
  */
  if (Timer::active) {
    if (Timer::decrement()) {
      Timer::done = true;
    }

    if (Timer::done) {
      Timer::alert();
    }
  }

  Timer::input_received = false;

  /* As long as the timer is not done, we write its value in minutes to the LEDs */
  if (!Timer::done) {
    LEDs::write_value(Timer::to_mins());
  }

  Button::raw_button = digitalRead(BUTTON);
  Button::now = millis();

  /* 
      Detect a change in the buttons state and when that state change occured.
      This information is used as part of the debouncing process.
  */
  if (Button::raw_button != Button::last_state) {
    Button::last_change_time = Button::now;
    Button::last_state = Button::raw_button;
  }

  /* Check that the debounce threshold has been met */
  if (Button::now - Button::last_change_time > Button::DEBOUNCE_MS) {
    
    /* 
        The button is now stable, so if its stable state is different than its
        raw state, we want to set the stable state to the raw state
     */
    if (Button::stable_state != Button::raw_button) {
      Button::stable_state = Button::raw_button;

      /* This is the case for if the button has been pressed */
      if (Button::stable_state == LOW) {
        Button::press_start = Button::now;
        Button::was_pressed = true;
        Timer::input_received = true;
      }

      /* This is the case for if the button was released after being pressed/held */
      else if (Button::stable_state == HIGH && Button::was_pressed) {
        Button::press_duration = Button::now - Button::press_start;
        Button::was_pressed = false;

        /* 
            If the button is pressed for less than one second, the mode is changed, 
            otherwise the timer is set according to how many seconds it was held 
            down for (1 second hold corresponds to a timer length of 1 minute, 
            31+ second hold corresponds to a timer length of 31 minutes. The timer
            can only be set to a whole number of minutes).
        */
        if (Button::press_duration < 1000) {
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

  /*
      This is the case for if the button is being held down
  */
  if (Button::was_pressed && Button::stable_state == LOW) {
    Button::hold_duration = Button::now - Button::press_start;
    if (Button::hold_duration > 1000) {
      Timer::selected_mins = Button::hold_duration / 1000;
      if (Timer::selected_mins > 31) Timer::selected_mins = 31;

      /* 
          Provide some feedback to the user by writing the currently 
          selected timer value to the LEDs.
      */
      LEDs::write_value(Timer::selected_mins);
    }
  }

  /* Flipping the switch resets the timer */
  if (digitalRead(SWITCH) == LOW) {
    Timer::reset();
  }
}
