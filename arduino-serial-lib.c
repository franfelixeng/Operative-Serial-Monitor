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

    fd = open(serialport, O_RDWR);

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

    //toptions.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

    toptions.c_cflag |= CREAD | CLOCAL;          // turn on READ & ignore ctrl lines
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw

    toptions.c_oflag &= ~OPOST; // make raw

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    //TODO
    toptions.c_cc[VMIN] = 1;
    toptions.c_cc[VTIME] = 0;
    // toptions.c_cc[VINTR] = 1;
    // toptions.c_cc[VQUIT] = 0;
    // toptions.c_cc[VSUSP] = 0;
    //toptions.c_cc[VTIME] = 20;

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

int serialport_read(int fd)
{
    char b[1]; // read expects an array, so we give it a 1-byte array
    //int i = 0;
    int n = 0;

    n = read(fd, b, 1); // read a char at a time
    if (n == -1)
    {
        printf("erro read -1\n");
        return -1; // couldn't read
    }
    else if (n == 0)
    {
        printf("fim eof\n");
    }

    return (int)b[0];
}

//
int serialport_read_until(int fd, char *buf, char until, int buf_max)
{
    char b[1]; // read expects an array, so we give it a 1-byte array
    int i = 0;
    do
    {
        int n = read(fd, b, 1); // read a char at a time
        if (n == -1)
            return -1; // couldn't read

        buf[i] = b[0];
        i++;
    } while (b[0] != until && i < buf_max);
    buf[i] = 0; // null terminate the string
    return 0;
}

int serialport_read_n_bytes(int fd, char *buf, int n)
{
    int state;
    state = read(fd, buf, n);
    return state;
}

int serialport_read_byte(int fd, char *c)
{
    int state;
    state = read(fd, c, 1);
    return state;
}

//
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
            ports[i] = calloc(9,sizeof(char));
            strcpy(ports[i],de->d_name);
            n_max--;
            i++;
        }
    }

    closedir(dr);
    return i;
}
