# Lightweight solar variometer for free flight

## Purpose
This project is about buiding a simple, lightweight, energey self-sufficient and helmet embbeded variometer. It is small enough to remain in the helmet's inner coat and draws its energy from the sun. A small battery can provide power during shaded times up to 10 hours.

This repo containsthe sources, pcb design files (eagle format), gerber files and a few instructions to build it, including pictures and bill of materials. The firmware must be loaded into the pic, so you will need a programmer such as a PICKIT or some arduino DIY one.

## Electronic parts needed
The variometer transcribes vertical speed to audio signal and must contain a power manager with a solar panel and a backup battery. Here are the comonent nedded :

* **MS5611** : absolute pressure sensor. [reference](http://www.te.com/usa-en/product-CAT-BLPS0036.html)

* **PIC24F16KA101** : this microcontroller is the brain of the vario. It communicates with the pressure sensor, does all the calculations and generates the audio signal. You mest get the 20 leads SSOP package. [reference](https://www.microchip.com/wwwproducts/en/PIC24F16KA101)

* **MCP4542** : Microchip digital I2C potentiometer used to adjust audio volume. [reference](https://www.microchip.com/wwwproducts/en/MCP4542)

* **ADP160** : Analog Device ultra low quiescent current ldo regulator. It supplies stabilized power to electronics. Get the 2.8 or 3V version. [reference](http://www.analog.com/en/products/power-management/ldo-linear-regulators/positive-linear-regulators/adp160.html)

* **LTC4071** : power manager. It havests solar energey and manages system load supplying/battery charging/power path handling. The current shunt limit is 50mA, so make sure your solar panel maximum current is under this value. [reference here](http://www.analog.com/media/en/technical-documentation/data-sheets/4071fc.pdf)

* **solar cell** : power supply. The system is designed for 6V/50mA max solar panel. I use two flexible 3V/12mA solar cells (ref [sp3-12](http://www.flexsolarcells.com/index_files/OEM_Components/Flex_Cells/pages/PowerFilm-Solar-OEM-00-Solar-Cell-Module-SP3-12.php)).

* **buzzer** : i like the kingstate's [KPEG006](http://uk.farnell.com/kingstate/kpeg006/receiver-piezo-leads/dp/1299873). It is flat and easy to fit under inner coat.

* **shottky diode** : used to protect solar panel.

* **3.6 lithium ion battery**. [I use a LIR2016](http://www.eemb.com/public/image/download/LIR2016.pdf)

* some capacitors and resistors (0603 package)

## PCB
![alt text](doc/_TOP.png | width=100)

