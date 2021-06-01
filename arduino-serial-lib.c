//
// arduino-serial-lib -- simple library for reading/writing serial ports
//
// 2006-2013, Tod E. Kurt, http://todbot.com/blog/
//

#include "arduino-serial-lib.h"

#include <stdio.h>   // Standard input/output definitions
#include <unistd.h>  // UNIX standard function definitions
#include <fcntl.h>   // File control definitions
#include <errno.h>   // Error number definitions
#include <termios.h> // POSIX terminal control definitions
#include <string.h>  // String function definitions
#include <sys/ioctl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

uint8_t is_color_mode = 0;

// uncomment this to debug reads
//#define SERIALPORTDEBUG

// takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
// and a baud rate (bps) and connects to that port at that speed and 8N1.
// opens the port in fully raw mode so you can send binary data.
// returns valid fd, or -1 on error
int serialport_init(const char *serialport, int baud)
{
    struct termios toptions;
    int fd;

    fd = open(serialport, O_RDWR, O_NOCTTY);

    if (fd == -1)
    {
        perror("serialport_init: Unable to open port ");
        return -1;
    }

    //int iflags = TIOCM_DTR;
    //ioctl(fd, TIOCMBIS, &iflags);     // turn on DTR
    //ioctl(fd, TIOCMBIC, &iflags);    // turn off DTR

    if (tcgetattr(fd, &toptions) < 0)
    {
        perror("serialport_init: Couldn't get term attributes");
        return -1;
    }
    speed_t brate = baud; // let you override switch below if needed
    switch (baud)
    {
    case 4800:
        brate = B4800;
        break;
    case 9600:
        brate = B9600;
        break;
#ifdef B14400
    case 14400:
        brate = B14400;
        break;
#endif
    case 19200:
        brate = B19200;
        break;
#ifdef B28800
    case 28800:
        brate = B28800;
        break;
#endif
    case 38400:
        brate = B38400;
        break;
    case 57600:
        brate = B57600;
        break;
    case 115200:
        brate = B115200;
        break;
    }
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);

    // 8N1
    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;
    // no flow control
    toptions.c_cflag &= ~CRTSCTS;
    //toptions.c_cflag |= CRTSCTS;

    //toptions.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

    toptions.c_cflag |= CREAD | CLOCAL;          // turn on READ & ignore ctrl lines
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    // Disable any special handling of received bytes
    toptions.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);

    toptions.c_oflag &= ~OPOST;

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html

    toptions.c_cc[VMIN] = 50;
    toptions.c_cc[VTIME] = 1; //needed to solve problems like \r\n for example

    tcsetattr(fd, TCSANOW, &toptions);
    if (tcsetattr(fd, TCSAFLUSH, &toptions) < 0)
    {
        perror("init_serialport: Couldn't set term attributes");
        return -1;
    }

    return fd;
}

//
int serialport_close(int fd)
{
    return close(fd);
}

//
int serialport_writebyte(int fd, uint8_t b)
{
    int n = write(fd, &b, 1);
    if (n != 1)
        return -1;
    return 0;
}

//
int serialport_write(int fd, const char *str)
{
    int len = strlen(str);
    int n = write(fd, str, len);
    if (n != len)
    {
        perror("serialport_write: couldn't write whole string\n");
        return -1;
    }
    return 0;
}

//0 or 1 (false or true)
void serialport_color_mode(uint8_t color_mode)
{
    is_color_mode = color_mode;
}

/*
    Blocks the process, until it has characters.
    Returns null in case of an error, or an empty string if the usb connection is lost.
    If all goes well it returns to the statically allocated buffer;
*/

char *serialport_read(int fd)
{
    static char buf[60];

    int n = 0;
    int i = 0;

    n = read(fd, buf, 50);

    if (n < 0)
    {
        return NULL;
    }

    // for the caracter \r\n which uses two bytes
    if (n == 50)
    {
        if (buf[49] == '\r')
        {
            n += read(fd, &buf[50], 1);
        }
        else if (is_color_mode)
        {
            //it is necessary to take care not to break the special color strings;

            for (i = 44; i < 47; i++)
            {
                if (buf[i] == '\033')
                {

                    if (strspn(&buf[i], LOG_COLOR_INIT) == 4)
                    {
                        n += read(fd, &buf[50], i - 43);
                    }
                    break;
                }
            }

            if (n <= 50)
            {
                for (i = 47; i < 50; i++)
                {
                    if (buf[i] == '\033')
                    {
                        n += read(fd, &buf[50], i - 46);
                        break;
                    }
                }
            
                if (buf[i + 3] == ';' && n > 50)
                {
                    n += read(fd, &buf[i + 4], 3);
                }
            }
        }
    }

    buf[n] = '\0';

    return buf;
}

int serialport_flush(int fd)
{
    //TODO corrigir  flush
    sleep(2); //required to make flush work, for some reason
    return tcflush(fd, TCIOFLUSH);
}

//return n ports
int serialport_devices_ports(char **ports, int n_max)
{
    int i = 0;
    struct dirent *de; // Pointer for directory entry

    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir("/dev");

    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory");
        return -1;
    }
    while ((de = readdir(dr)) != NULL && n_max)
    {
        if (strncmp(de->d_name, "ttyUSB", 6) == 0)
        {
            ports[i] = calloc(9, sizeof(char));
            strcpy(ports[i], de->d_name);
            n_max--;
            i++;
        }
    }

    closedir(dr);
    return i;
}
