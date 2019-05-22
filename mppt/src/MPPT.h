#include "mbed.h"

float I, V, P; // Variables for Current, Voltage and Power

Serial pc2(USBTX, USBRX);

/* Maximum Power Point Tracker Class
Control the duty cycle of a DC-DC converter switch using PWM,
to optimize the power flow.
Power is calculated using current and voltage sensors.
*/
class MPPT{
  private:
    AnalogIn *CurrentSensor;
    AnalogIn *VoltageSensor;
    PwmOut *PwmOutput;
    AnalogOut *DutyOutput;
    float PreviousPower; // Power measured previous iteration
    float DutyCycle; // Duty cycle of PWM output signal
    float Perturbation; // Change in the duty cycle every iteration

  public:
    MPPT(PinName I_pin,PinName V_pin,PinName PWM_pin,PinName D_pin);
    float readI(); // read from current sensor
    float readV(); // read from voltage sensor
    float readP(); // return last calculated power

    /* Perturb and Observe MPP Tracking algorithm.
    Optional target power argument in case a lower power point needs to be tracked.*/
    void PerturbObserve(float Target); 

    void pause(); // stop tracking power point, open PWM switch
};

MPPT::MPPT(PinName I_pin,PinName V_pin,PinName PWM_pin,PinName D_pin){
  CurrentSensor = new AnalogIn(I_pin);
  VoltageSensor = new AnalogIn(V_pin);
  PwmOutput = new PwmOut(PWM_pin);
  DutyOutput = new AnalogOut(D_pin);

  Perturbation = 0.01;// the change in the duty cycle
  PwmOutput->period_us(100); // set PWM frequency to 10kHz (1/100us = 10kHz)
};

float MPPT::readI(){
  return CurrentSensor->read(); // return read current
}

float MPPT::readV(){
  return VoltageSensor->read(); // return read voltage
}

float MPPT::readP(){
  return PreviousPower; // return last calculated power
}

void MPPT::PerturbObserve(float Target = 0){
  //observe
  I = CurrentSensor->read(); // read current sensor
  V = VoltageSensor->read(); // read voltage sensor
  P = I*V; // calculate power

  /*perturb*/
  if (Target == 0) { // No target given -> Track MPPT
    if (P < PreviousPower) { // if previous perturbation resulted in loss of power:
      Perturbation = -Perturbation; // reverse perturbation direction
    }
    
  }
  else { // Track target power
    pc2.printf("Target power: %d mW\r\n",static_cast<int>(Target*1000));
    if (P>Target){
      Perturbation = -abs(Perturbation); // set perturbation to negative
    }
    else {
      Perturbation = abs(Perturbation); // set perturbation to positive
    }  
  }
  DutyCycle = DutyCycle+Perturbation; // Apply Perturbation
  /*end perturb*/

  // Constrain duty cycle to be in between 0 and 1
  if (DutyCycle < 0) {DutyCycle = 0;}
  else if (DutyCycle > 1){DutyCycle = 1;}
  
  DutyOutput->write(DutyCycle); // write Duty Cycle
  PwmOutput->write(DutyCycle); // write PWM
  
  PreviousPower = P; // store calculated power  
}

void MPPT::pause(){
  DutyCycle = 0; // Set duty cycle to zero
  DutyOutput->write(DutyCycle); // write Duty Cycle
  PwmOutput->write(DutyCycle); // Open MOSFET
  I = CurrentSensor->read(); // read current sensor
  V = VoltageSensor->read(); // read voltage sensor
  P = I*V; // calculate power (should be zero)
  PreviousPower = P;
}
