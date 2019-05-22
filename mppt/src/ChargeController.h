#include "mbed.h"

#define open 0
#define close 1

float Ibat, Vload, Pbat; // Variables for current, voltage for battery and load, and power from battery
float SoC; // Variable for State of Charge of the battery pack, given in percentages
const float VbMax = 12; // Maximum Battery voltage (SoC = 100)   //// Need to verify value
const int SoCMax = 95; // Maximum allowed battery State of Charge //// Need to verify value
const int SoCDanger = 97; // Dangerously high SoC //// Need to verify value

const float ChargingConst = 0.1; // Arbitrary constant used for calculating change in battery voltage


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
        */
        int Control; 

        float Pload; // Power used by the load
        float Vbat; // Battery voltage
        float Iload; // Load current

        DigitalOut *PvSwitch; // This switch can disconnect the PV system from the batteries and load
        DigitalOut *BatSwitch; // This switch can disconnect the Batteries from the PV system, not allowing them to charge

    public:
        float Ppv; // Incoming power from solar panels
        ChargeController(PinName PvSwPin, PinName BatSwPin);
        void run(); // Execute charge controller algorithm
        int readControl(); // Allows the control signal to be read
        float readPload(); // Allows Pload to be read

};

ChargeController::ChargeController(PinName PvSwPin, PinName BatSwPin){
    PvSwitch = new DigitalOut(PvSwPin);
    BatSwitch = new DigitalOut(BatSwPin);
    
    /* In the final code, these two values should be read via sensors
    However for this simulation, their values are set through software*/
    Iload = 0.01; // [A] 
    Vbat = 11.1; // [V]
};

int ChargeController::readControl(){
    return Control; // Return control signal
}

float ChargeController::readPload(){
    return Pload; // Return Pload
}

void ChargeController::run(){

    /*  read battery voltage  */

    SoC = Vbat/VbMax*100; // Determine state of charge //// This formula is incorrect, need to change

    /*  read load current    */

    Vload = Vbat; // Load is in parallel with the batteries   
    Pload = Iload*Vload; // Calculate power used by the load

    if (SoC >= SoCDanger) { // Battery charge is dangerously high
        Control = 1; // Pause MPPT (open MOSFET and pause algorithm)
        // open both switches
        PvSwitch->write(open); // PV power must be cut off,
        BatSwitch->write(open); // As the battery should quickly discharge to a safe level      
    }
    else if (SoC < SoCMax && PvSwitch->read()==open) // Battery has discharged to safe level, but PV is still disconnected
    {
        Control = 0; // Track MPPT
        PvSwitch->write(close); // Connect PV
        BatSwitch->write(close); // Close Battery switch in case PV power happens to be higher than load power       
    }


    else if (SoC >= SoCMax && Ppv >= Pload) { // Both SoC and Ppv are too high
        Control = 2; // Tell main loop to read Pload. MPPTs must try to set Ppv equal to Pload
        PvSwitch->write(close); // Keep PV connected to supply load power
        BatSwitch->write(close); // Keep Battery connected to absorb excess current        
    }

    else if (SoC >= SoCMax && Ppv < Pload) { // SoC is too high but PV power is too low (so battery is discharging)
        Control = 0; // Track MPPT
        PvSwitch->write(close); // Keep PV connected to supply as much power as possible
        BatSwitch->write(open); // PV power is too low to charge the batteries       
    }

    else if (SoC < SoCMax && Ppv >= Pload) { // Battery can be charged, and PV is supplying enough power to do so
        Control = 0; // Track MPPT
        PvSwitch->write(close); // Connect PV
        BatSwitch->write(close); // Enable battery charging
    }

    else if (SoC < SoCMax && Ppv < Pload) { // Battery can be charged, but PV is not supplying enough power to do so
        Control = 0; // Track MPPT
        PvSwitch->write(close); // Connect PV
        BatSwitch->write(open); // PV power is too low to charge the batteries
    }

    /* Debugging */
    if (PvSwitch->read() == open){pc1.printf("PvSwitch: open\r\n");}
    else {pc1.printf("PvSwitch: closed\r\n");}
    if (BatSwitch->read() == open){pc1.printf("BatSwitch: open\r\n");}
    else {pc1.printf("BatSwitch: closed\r\n");}         
    pc1.printf("Ppv: %d mW Pload: %d mW\r\n",static_cast<int>(Ppv*1000),static_cast<int>(Pload*1000)); 
    pc1.printf("SoC: %d\r\n",static_cast<int>(SoC)); 
    /*          */


    // Calculate battery voltage
    if (PvSwitch->read() && BatSwitch->read()){ // If charging is enabled
        Vbat = Vbat + (Ppv-Pload)*ChargingConst;
    }
    else
    {
        Vbat = Vbat - (Pload-Ppv)*ChargingConst;
    }

}
