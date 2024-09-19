# AutoCatFeeder

This project was designed to automatically feed my cat, who tends to get very vocal and insistent around feeding time. The goal was to create a reliable system to control feeding portions and schedules using an Arduino and various sensors and components. However, after building it, I decided not to use it because I wasn’t sure it would be safe enough for my cat in the long run.

Automated Feeding: The system uses a servo motor to rotate a peddle wheel that dispenses portions of food based on a pre-programmed schedule.
Manual Feeding Option: In addition to scheduled feedings, the feeder can also be activated manually.
Custom Feeding Schedules: Two distinct feeding times can be set, each with adjustable portion sizes.
Fail-safe Mechanisms: A Hall sensor monitors each 60-degree turn of the feeder, ensuring accurate portion control. Backup systems prevent overfeeding if the sensor fails.
LCD Display: Displays feeding schedules, current time, and settings on a 16x2 LCD display.
User Interface: Simple button controls for setting feed times, portions, and manual overrides.
Components
Arduino: The brain of the system, controlling the feeding mechanism and managing the feeding schedule.
Servo Motor: Used to rotate the feeder to dispense food portions.
Hall Sensor: Monitors the rotation of the feeder to ensure portions are accurate.
LCD Display: Provides real-time information about feeding times and settings.
Buttons: Used for navigating the menu and manually activating feedings.
RTC Module: Keeps track of time for feeding schedules.
LED Indicators: Show the status of feeding and potential issues like sensor failures.

Why I Didn't Use It:
While the system worked well in testing, I ultimately decided not to use it for my cat. I wasn't confident that it would be entirely safe, as there could be risks related to overfeeding or mechanical issues that could harm my cat. While the project was a fun exercise in automation and embedded systems, my cat’s safety comes first.

Setup and Usage
Wiring: Connect all components to the Arduino according to the schematic.
Upload the Code: Use the Arduino IDE to upload the provided sketch to the Arduino.
Set Feeding Schedule: Use the buttons to configure feeding times and portion sizes.
Testing: Run some tests without food to ensure the system works as expected.
Monitor for Issues: Keep an eye on the system for any mechanical or software issues.
Future Improvements
If I were to revisit this project, I would focus on improving the safety features, perhaps adding sensors to detect the cat’s presence near the feeder and ensuring more robust fail-safes to avoid overfeeding.

