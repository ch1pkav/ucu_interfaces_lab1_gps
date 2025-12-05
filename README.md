# Lab 1: GPS UART
## Building:
```bash
mkdir build
```
```bash
cd build
```
```bash
cmake ..
```
```bash
cmake --build .
```
## Flashing
```bash
cd build; cmake --build --target flash
```
## Components and pinout:
- `STM32F411EDISCOVERY`
- `GY-GPS6MV2` - PA10: RX, PA15: TX
## Available commands
All output is in csv format. USB CDC baud rate is 115200.
- GET POS

<img width="180" height="78" alt="image" src="https://github.com/user-attachments/assets/8497f8a4-4ccc-46d9-b3d9-d7f2c52a0027" />

- GET TIME

<img width="180" height="78" alt="image" src="https://github.com/user-attachments/assets/82260b4e-0df4-46e7-9f33-3d727d88ca2c" />

- VER

<img width="180" height="78" alt="image" src="https://github.com/user-attachments/assets/e0ca7359-1a5f-4fe6-866d-0968c3290485" />

- RAW ON - seen below
- RAW OFF - self-explanatory

Example of running commands:

<img width="682" height="786" alt="image" src="https://github.com/user-attachments/assets/cf76599f-369e-4123-9260-509f763295ac" />
