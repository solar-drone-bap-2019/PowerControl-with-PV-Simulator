#include "mbed.h"
#include "MPPT.h"
#include "ChargeController.h"

/*
Tracks the maximum power point for multiple solar arrays (solar cell groups),
And controls the power flow in the system, to maximize solar power usage, without overcharging the batteries.
*/

/* Control signal to be read from ChargeController.
0 = track MPPT
1 = pause MPPT
2 = track Pload
*/
int Control; 

float Target; // Target variable to be sent to MPPTs in case they need to track a lower power point

// Pins for tracking group 1
#define I1 PA_0 // Current sensor
#define V1 PA_1 // Voltage sensor
#define PWM1 PA_3 // Pulse width modulation output
#define D1 PA_4 // Duty Cycle

//Pins for charge Controller
//#define Vbat PA_5 // Battery voltage sensor               two more analog pins were not achieveable for this simulation
//#define Iload PA_6 // Load current sensor                 so Vbat and Iload values are set via the software
#define PvSw PA_7 // Pin out to switches connecting Solar cells to MPPTs
#define BatSw PA_8 // Switch which can enable battery charging

// define the Serial object
Serial pc(USBTX, USBRX);

DigitalOut myled(LED1);

MPPT MPPT1(I1,V1,PWM1,D1); // Create Maximum Power Point Tracker 1

ChargeController CC(PvSw,BatSw); // Create Charge Controller object


int main() {
    while(1) {
      pc.printf("\e[1;1H\e[2J"); // clear console
      myled = 1;
        CC.Ppv = MPPT1.readP();
        CC.run();
        Control = CC.readControl();
        if (Control == 0) { // Track MPPT       
            MPPT1.PerturbObserve(); // Execute the P&O algorithm for tracking group 1

            pc.printf("control = 0: Tracking MPP\r\n");
        }
        else if (Control == 1) { // Pause the MPPT algorithm 
              
            MPPT1.pause(); 

            pc.printf("control = 1: Pausing MPPT\r\n"); 
        }
        else if (Control == 2) { // Track Target power
            Target = CC.readPload(); // Set Pload as target power.
            MPPT1.PerturbObserve(Target);

            pc.printf("control = 2: Tracking %d percent power\r\n",static_cast<int>(Target*100));     
        }  
   
        // MPPT doesn't need to happen very fast   
        wait_ms(10); // Track the power point every 10 ms
    }
}
