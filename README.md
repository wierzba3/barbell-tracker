# barbell-tracker
Arduino code for a device that measures the velocity of retractable string, to be used for measuring velocity of a barbell.

The device uses a retractable string, quadrature rotary encoder and arduino microcontroller to read and interpret the gray code emitted by the rotary encoder. This code allows me to determine when, how much, and which direction the rotary encoder spins. After some calibration, it is possible to determine how many state changes equal a unit of length (centimeters in my case). With the length measurement and time samples, velocity, acceleration, and power can be derived.

Video of it in action: 
https://youtu.be/WLRWjXfTbiQ


![Alt text](/barbell-tracker-design.png?raw=true "Circuit design")
