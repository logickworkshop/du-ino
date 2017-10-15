#include <du-ino_function.h>
#include <du-ino_interface.h>
#include <TimerOne.h>

class DU_ADSR_Function : public DUINO_Function {
 public:
  DU_ADSR_Function(uint16_t sc) : DUINO_Function(sc) { }
  
  virtual void setup()
  {
    
  }

  virtual void loop(unsigned long dt)
  {

  }
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

void timer_isr()
{
  interface->timer_isr();
}

void setup() {
  function = new DU_ADSR_Function(0b0000000000);
  interface = new DU_ADSR_Interface();

  function->begin();
  interface->begin();

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timer_isr);
}

void loop() {
  function->run();
}
