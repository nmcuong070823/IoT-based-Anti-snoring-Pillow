# IoT-based-Anti-snoring-Pillow
Abstract: Snoring is a common phenomenon that not only causes discomfort to others but can also be an indicator of Obstructive Sleep Apnea (OSA) — a sleep-related breathing disorder that reduces sleep quality and may lead to various health issues.
This project focuses on developing a solution to mitigate the risk of sleep apnea by reducing the frequency of snoring. The proposed system is low-cost, user-friendly, and non-invasive. It utilizes a deep learning algorithm to detect snoring, along with hardware components including the ESP32 microcontroller, INMP441 sound sensor, an air pump, and an inflatable air cushion. The system inflates the pillow to adjust the sleeper’s head position, interrupting snoring and thereby reducing the risk of sleep apnea.
## Theoretical basis
Snoring sound comes from the vibration of the pharyngeal walls when the airflow passes by due to the narrow of the upper airway. If left untreated, it can leads to Obstructive Sleep Apnea, which can causes several health problems and also economy loss.  
![Capture](https://github.com/user-attachments/assets/15ab5a61-96e1-4909-a845-9be1bc716eda)
Nowadays, many products with different solutions have made in order to mitigate the problems due to sleep disorders problems including snoring but there's a still plenty of drawbacks such as lack of comfort, no actual urgent solutions or high cost.
The proposed system uses one of the deep learning algorithms is Convolutional Neural Network (CNN) to improve the accuracy of snoring detection in different sound environments as well as on different individuals.  
![Capture2](https://github.com/user-attachments/assets/051f74f8-3f92-4a2b-bedd-5070751fff31)
The proposed system is based on the solution from Smart Nora
![image](https://github.com/user-attachments/assets/826d13e8-9d6b-4424-90d0-e409df105baa)
## Design and Implementation
### Snoring Detection Model
![Capture3](https://github.com/user-attachments/assets/00c6f997-545f-4684-991b-90256f213ca6)
### Block diagram of System
The proposed system consists of 2 modules: Snoring detector device using I2S microphone and Inflate/Deflate Device using relay to control the air pump and electronics valve according to the designed flowchart mentioned later
The 2 modules communicates each other using ESP-NOW protocol.
The Inflate/Deflate Device also uses another ESP32 for connecting to Web Server to visualize real-time data received from UART.
![Capture4](https://github.com/user-attachments/assets/1073108f-263d-4bf2-8b5c-822fa107dd66)
The detailed block diagram of Inflate/Deflate Device
![Capture6](https://github.com/user-attachments/assets/8f9fed9c-2fa8-4a44-865a-4a9e50721ebd)
The detailed block diagram of Snoring Detector device 
![Capture5](https://github.com/user-attachments/assets/3d96e16c-d188-444b-873c-3252275ec831)
### System flowchart and Web Server Dashboard design
Web Server Dashboard design
![Capture8](https://github.com/user-attachments/assets/7498e7eb-21fb-4a0c-aae7-08759f525497)
System flowchart
![Capture7](https://github.com/user-attachments/assets/7369ee27-811e-4723-a693-1b9dca38df3c)
## Results
Prototype System
![image](https://github.com/user-attachments/assets/e0599dcd-2a9b-4e6f-8ed0-8956d604e3d9)
![image](https://github.com/user-attachments/assets/a35e931d-8828-4b34-a939-6f2d1a095b4c)
Experimental Result
![image](https://github.com/user-attachments/assets/b1ce47cb-b0e6-47da-a024-222681971b21)
## REFERENCES


