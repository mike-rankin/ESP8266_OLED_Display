I did not create the code for this project. Just the hardware.

Web Site: http://blog.squix.ch/2015/06/esp8266-weather-station-v2-code.html
Code: https://github.com/squix78/esp8266-projects/tree/master/arduino-ide/weather-station-v2

For my ESP-01 the i2c display is on pins 0 and 2 so I changed one line to this: SSD1306 display(0x3c, 0, 2);