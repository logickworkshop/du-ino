/*
 * ####                                                ####
 * ####                                                ####
 * ####                                                ####      ##
 * ####                                                ####    ####
 * ####  ############  ############  ####  ##########  ####  ####
 * ####  ####    ####  ####    ####  ####  ####        ########
 * ####  ####    ####  ####    ####  ####  ####        ########
 * ####  ####    ####  ####    ####  ####  ####        ####  ####
 * ####  ####    ####  ####    ####  ####  ####        ####    ####
 * ####  ############  ############  ####  ##########  ####      ####
 *                             ####                                ####
 * ################################                                  ####
 *            __      __              __              __      __       ####
 *   |  |    |  |    [__)    |_/     (__     |__|    |  |    [__)        ####
 *   |/\|    |__|    |  \    |  \    .__)    |  |    |__|    |             ##
 *
 *
 * DU-INO ADSR Envelope & VCA Function
 * Aaron Mavrinac <aaron@logick.ca>
 */

#include <du-ino_function.h>
#include <du-ino_interface.h>
#include <TimerOne.h>

struct DU_ADSR_Values {
  uint16_t A;  // ms
  uint16_t D;  // ms
  float S;    // V
  uint16_t R;  // ms
};

volatile DU_ADSR_Values adsr_values;
volatile bool gate;

void gate_isr();
void timer_isr();

class DU_ADSR_Function : public DUINO_Function {
 public:
  DU_ADSR_Function() : DUINO_Function(0b0000000100) { }
  
  virtual void setup()
  {
    gate_time = 0;
    gate = false;
    gt_attach_interrupt(GT3, gate_isr, CHANGE);
  }

  virtual void loop()
  {
    if(gate_time)
    {
      if(gate)
      {
        uint16_t elapsed = millis() - gate_time;
        if(elapsed < adsr_values.A)
        {
          // attack
          cv_current = (float(elapsed) / float(adsr_values.A)) * 10.0;
        }
        else if(elapsed < adsr_values.A + adsr_values.D)
        {
          // decay
          cv_current = adsr_values.S + (1.0 - float(elapsed - adsr_values.A) / float(adsr_values.D))
                       * (10.0 - adsr_values.S);
        }
        else
        {
          // sustain
          cv_current = adsr_values.S;
        }
      }
      else
      {
        if(release_time)
        {
          uint16_t elapsed = millis() - release_time;
          if(elapsed < adsr_values.R)
          {
            // release
            cv_current = (1.0 - float(elapsed) / float(adsr_values.R)) * cv_released;
          }
          else
          {
            cv_current = 0.0;
            release_time = 0;
            gate_time = 0;
          }
        }
        else
        {
          release_time = millis();
          cv_released = cv_current;
        }
      }

      cv_out(CO1, cv_current);
    }
    else if(gate)
    {
      gate_time = millis();
    }
  }

 private:
  unsigned long gate_time;
  unsigned long release_time;
  float cv_current;
  float cv_released;
};

class DU_ADSR_Interface : public DUINO_Interface {
 public:
  virtual void setup()
  {
    
  }

  virtual void timer()
  {
    
  }
};

DU_ADSR_Function * function;
DU_ADSR_Interface * interface;

void gate_isr()
{
  gate = function->gt_read(GT3);
}

void timer_isr()
{
  interface->timer_isr();
}

void setup() {
  function = new DU_ADSR_Function();
  interface = new DU_ADSR_Interface();

  function->begin();
  interface->begin();

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timer_isr);
}

void loop() {
  function->loop();
}
