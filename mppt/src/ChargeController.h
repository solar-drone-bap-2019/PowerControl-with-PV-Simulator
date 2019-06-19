#include "mbed.h"

#define open 0
#define close 1

float Ibat, Vload, Pbat; // Variables for current, voltage for battery and load, and power from battery
float SoC; // Variable for State of Charge of the battery pack, given in percentages
const float VbMax = 8.4; // Maximum Battery voltage (SoC = 100)   //// Need to verify value
const int SoCMax = 95; // Maximum allowed battery State of Charge //// Need to verify value
const int SoCDanger = 97; // Dangerously high SoC //// Need to verify value
const int SoCMin = 20; // Minimum allowed battery SoC //// Need to verify value
const float BatCap = 3800; // [mAh] //// Need to verify value
const float IbatDanger = 2*BatCap/1000; // [A] maximum safe charging current
const float IbatMax = IbatDanger*0.8;

const float ChargingConst = 0.001; // Arbitrary constant used for calculating change in battery voltage

const float LOADCURRENT_SENSOR_MAX = 11; // [A] maximum current the sensor is able to sense properly
const float BATVOLTAGE_SENSOR_MAX = 9; // [V] maximum voltage the sensor is able to sense properly

Serial pc1(USBTX, USBRX);

/*
Control the power flow in the system using switches
*/
class ChargeController{
    private: 
        /* Control signal to be read by the main code. Will tell it how to control the MPPT.
        0 = track MPPT
        1 = pause MPPT
        2 = track Pload
        3 = track Pload + PbatMax (overcurrent protection)
        4 = reset MPPT 
        */
        int Control; 

        float Pload; // Power used by the load
        float Vbat; // Battery voltage
        float Iload; // Load current

        float PbatMax; // Maximum allowed battery charge power

        DigitalOut *BatSwitch; // This switch can disconnect the Batteries, not allowing them to charge or discharge

    public:
        float Ppv; // Incoming power from solar panels
        ChargeController(PinName BatSwPin);
        void run(); // Execute charge controller algorithm
        int readControl(); // Allows the control signal to be read
        float readPload(); // Allows Pload to be read
        float readPbatMax(); // Allows PbatMax to be read

};

ChargeController::ChargeController(PinName BatSwPin){
    BatSwitch = new DigitalOut(BatSwPin);
    
    /* In the final code, these two values should be read via sensors
    However for this simulation, their values are set through software*/
    //Iload = 0.05; // [A] 
    Vbat = 7.4; // [V]
};

int ChargeController::readControl(){
    return Control; // Return control signal
}

float ChargeController::readPload(){
    return Pload; // Return Pload
}

float ChargeController::readPbatMax(){
    return PbatMax; 
}

void ChargeController::run(){

    /*  read battery voltage  */

    SoC = Vbat/VbMax*100; // Determine state of charge //// This formula is incorrect, need to change

    /*  read load current    */


    Ibat = (Ppv-Pload)/Vbat;

    PbatMax = IbatMax*Vbat; // Set maximum allowed battery charging power; to be read by main loop

    Vload = Vbat; // Load is in parallel with the batteries   
    Pload = 20; // Calculate power used by the load (hard set to 20W in this sim)

    /*  Might need to implement some control algorithm in case of low voltage/
    if (SoC <= SoCMin){ // Battery charge is too low
        LoadSwitch->write(open);
    }
    */

    if(Ibat >= IbatDanger){ // Battery charging current is dangerously high
        Control = 4; // Reset MPPT (open MOSFET and reset duty cycle to zero)
        BatSwitch->write(open); // Disconnect battery 

    }

    if (SoC >= SoCDanger) { // Battery charge is dangerously high 
    
        Control = 1; // Pause MPPT (open MOSFET and pause algorithm)
        BatSwitch->write(open); // Disconnect battery    
    }

    else if (SoC >= SoCMax && Ppv >= Pload) { // Both SoC and Ppv are too high
        Control = 2; // Tell main loop to read Pload. MPPTs must try to set Ppv equal to Pload
        BatSwitch->write(close); // Keep battery connected to absorb excess current        
    }

    else if (Ibat >= IbatMax){ // Battery charging current is very high, but within limits
        Control = 3; // Track lower power point
        BatSwitch->write(close);  // charge battery
    }


    else 
    {
        Control = 0; // Track MPP
        BatSwitch->write(close);   // Charge battery 
    }


    /* Debugging */
    pc1.printf("IbatMax = %d A\r\n",static_cast<int>(IbatMax));
    pc1.printf("Ibat = %d A\r\n",static_cast<int>(Ibat));
    if (BatSwitch->read() == open){pc1.printf("BatSwitch: open\r\n");}
    else {pc1.printf("BatSwitch: closed\r\n");}         
    pc1.printf("Ppv: %d W Pload: %d W\r\n",static_cast<int>(Ppv),static_cast<int>(Pload)); 
    pc1.printf("SoC: %d\r\n",static_cast<int>(SoC)); 
    /*          */


    // Calculate battery voltage
    if (BatSwitch->read()){ // If charging is enabled
        Vbat = Vbat + (Ppv-Pload)*ChargingConst;
    }
    else
    {
        Vbat = Vbat - (Pload-Ppv)*ChargingConst;
    }

}
