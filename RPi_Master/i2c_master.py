# This program asks for data from an Arduino on an I2C bus and sends the data to a local webserver as an HTTP request.
# For more information contact Panteha Ahmadi.

import smbus
import urllib2
import time
import sys
import traceback

# 1 comes from i2cdetect -y 1 showing the device
I2C_DEVICE = 1
# keep all of the following constants in sync with arduino code
I2C_SLAVE_ADDRESS = 0x8
I2C_CMD_GET_TEMP = 0x2
I2C_CMD_GET_ULTRASONIC = 0x3

DS18B20_MIN_TEMPERATURE = -20.
DS18B20_MAX_TEMPERATURE = 60.
DS18B20_SCALE = 255.0/(DS18B20_MAX_TEMPERATURE-DS18B20_MIN_TEMPERATURE)

while True:
	try:
		bus = smbus.SMBus(I2C_DEVICE)
		record_temp_url = "http://localhost:5000/record_sensor/temperature/"
		record_ultra_url = "http://localhost:5000/record_sensor/ultrasonic/"

		while True:
			encoded_temperature = bus.read_byte_data(I2C_SLAVE_ADDRESS, I2C_CMD_GET_TEMP)
			temperature = encoded_temperature/DS18B20_SCALE + DS18B20_MIN_TEMPERATURE
			urllib2.urlopen(record_temp_url + str(temperature)).read()

			distance_from_gasholder = bus.read_byte_data(I2C_SLAVE_ADDRESS, I2C_CMD_GET_ULTRASONIC)
			urllib2.urlopen(record_ultra_url + str(distance_from_gasholder) + ".0").read()
			print 'temperature is', temperature
			print 'distance is', distance_from_gasholder

			time.sleep(30)
	except:
		print 'caught exception', sys.exc_info()[1]
		traceback.print_exc(file=sys.stdout)
		time.sleep(10)
