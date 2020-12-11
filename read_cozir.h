#include "esphome.h"

#define SEND_EVERY_X_MESSAGE 60 // cozir send a message ever 500ms, so 60 == 30 seconds update 

class UartReadLineSensor : public Component, public UARTDevice, public Sensor {
 public:
	UartReadLineSensor(UARTComponent *parent) : UARTDevice(parent) {}

	void setup() override {
	    write_str("K 1\r\n"); // set to streaming mode
	    write_str("@ 1.0 8.0\r\n"); // Autocalibration Intervals
	    write_str("S 8192\r\n"); //Altitude Compensation 0 meter
	    write_str("M 6\r\n"); // set co2 output filtered and unfiltered
	}

	int readline(int readch, char *buffer, int len)
	{
		static int readings_num = 0;
		static int pos = 0;
		int rpos;

		if (readch > 0) {
			switch (readch) {
				case ' ': // Ignore space
				case '\n': // Ignore new-lines
					break;
				case 'z': // lower case is end of filter co2
					rpos = pos;
					readings_num++;
					pos = 0;  // Reset position index ready for next time
					if (readings_num > SEND_EVERY_X_MESSAGE)
					{
						readings_num = 0;
						return rpos;
					}
					else
					{
						return -1;
					}
				case 'Z': // beginning of filtered co2
					pos = 0;
					break;
				default:
					if (pos < len-1) {
						buffer[pos++] = readch;
						buffer[pos] = 0;
					}
			}
		}
		// No end of line has been found, so return -1.
		return -1;
	}

	void loop() override {
		const int max_line_length = 80;
		static char buffer[max_line_length];
		int filter_co2 = 666;
		if (available() && readline(read(), buffer, max_line_length) > 0) {
			filter_co2 = atoi(buffer);
			// ESP_LOGI("cozir", "str `%s` = %d ppm", buffer, filter_co2);
			publish_state(filter_co2);
		}
	}
};