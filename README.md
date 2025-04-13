# Ride the CAN BUS 🚌

Data logging framework for motorsports based on CAN BUS logs.

![gh_preview](https://github.com/user-attachments/assets/a003a675-0640-4710-9c1d-1a5abd53c6a0)


## What you need

- ESP32 microcontroller
- CAN transceiver

I used the ESP32-C6 and SN65HVD230 transceiver.

![image](https://github.com/user-attachments/assets/34557a51-719e-4c3c-bb2e-2540423653c5)


## Programming for your car

The project contains a header file [include/operators.h](include/operators.h) for defining custom message parsers. The original project was based on the A90 Supra. I found most of the formulas [online](https://thesecretingredient.neocities.org/bmw/can/g29/).

## Contributing

Contributions are welcome! Ridethecanbus is based on the [esp-idf](https://github.com/espressif/esp-idf) framework. Please reference it for development information.
