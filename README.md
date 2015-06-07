# emeir
Monitor the counter of an electricity meter with Arduino and infrared light sensor

![Electricity meter with infrared light barrier](http://www.kompf.de/tech/images/countemeir.jpg)


The software consists of two parts:

* Data acquisition part running on an Arduino Nano. It controls the infrared light barrier, detects trigger levels and communicates with a master computer over USB serial.
* Data recording part running on a the master computer (Raspberry Pi). It retrieves the data from the Arduino over USB serial and stores counter and consumption values into a round robin database.

