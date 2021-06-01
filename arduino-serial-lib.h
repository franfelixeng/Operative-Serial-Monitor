//
// arduino-serial-lib -- simple library for reading/writing serial ports
//
// 2006-2013, Tod E. Kurt, http://todbot.com/blog/
//

#ifndef __ARDUINO_SERIAL_LIB_H__
#define __ARDUINO_SERIAL_LIB_H__

#include <stdint.h> // Standard types

#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31" //ERROR
#define LOG_COLOR_GREEN   "32" //INFO
#define LOG_COLOR_YELLOW  "33" //WARNING
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_MAGENTA "35"
#define LOG_COLOR_CYAN    "36" //DEBUG
#define LOG_COLOR_GRAY    "37" //VERBOSE
#define LOG_COLOR_WHITE   "38"

#define LOG_COLOR_INIT "\033[0;"
# define LOG_RESET_COLOR "\033[0m"


int serialport_init(const char *serialport, int baud);
int serialport_close(int fd);
int serialport_writebyte(int fd, uint8_t b);
int serialport_write(int fd, const char *str);
char *serialport_read(int fd);
int serialport_flush(int fd);
int serialport_devices_ports(char **ports, int n_max);
void serialport_color_mode(uint8_t color_mode);

#endif
