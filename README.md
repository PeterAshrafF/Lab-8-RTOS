# Lab-8-RTOS

This project was made on Proteus simulation tool, using PlatformIO for software programming
The video showcases simulation in action for the task

steps to recreate the project:
1. Make a project on platformio choosing suitable board and framework, then choose the directory if not default
2. write software code
3. build the code ( you will find the HEX file generally under ".pio/build/<environment_name>/firmware.hex")
4. make a project on proteus and connect as suggested in the video
5. start simulation and tinker around

IF you cloned the repo instead, DO the following:
1. open the folder named "RTOS Lab" on platformio as a project
2. build the code file under folder named "src" and find the file named "main.cpp" ( you will find the HEX file generally under ".pio/build/<environment_name>/firmware.hex")
3. open the proteus project file named "RTOS Lab.pdsprj"
4. double click on the mcu board, then choose the path of the hex file on the MCU to flash the code on it (the hex file u compiled from step 2)
5. run simulation and change the temperature on the lm35 sensor (threshold is 25 C)

this is the link for the video:
https://drive.google.com/file/d/1djCMPJeBOxcIx8loIzUhJPMIkfEws3Jt/view?usp=sharing

<img width="1679" height="945" alt="image" src="https://github.com/user-attachments/assets/8538ac21-ac88-41b7-8bf4-e65e80cf8b3b" />
