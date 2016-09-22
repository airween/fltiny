#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#define PORT "/dev/fsk"

int fd;

int open_port(void) {

    struct termios options;
    int _fd;

    _fd = open(PORT, O_RDWR | O_NOCTTY, O_NDELAY);
    if (_fd == -1) {
        return -1;
    }
    else {
        tcgetattr(_fd, &options);
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        tcsetattr(_fd, TCSANOW, &options);
    }
    return _fd;
}

void serial_read() {
    int rb;
    char buffer[255];

    memset(buffer, '\0', 255);

    rb = read(fd, buffer, 255);
    if (rb == -1) {
        switch (errno) {
            case EAGAIN:	printf("EAGAIN\n");
                    break;
            case EBADF:		printf("EBADF\n");
                    break;
            case EFAULT:	printf("EFAULT\n");
                    break;
            case EINTR:		printf("EINTR\n");
                    break;
            case EINVAL:	printf("EINVAL\n");
                    break;
            case EIO:		printf("EIO\n");
                    break;
            case EISDIR:	printf("EISDIR\n");
                    break;
        }
    }

}

int serial_write(char c) {
    char buffer[2];
    buffer[0] = c;
    buffer[1] = '\0';
    return write(fd, buffer, 1);
}

int serial_init() {

    fd = -1;

    fd = open_port();

    if (fd < 0) {
        return -1;
    }

    serial_read();
    return 0;
}

int serial_close() {
    close(fd);
    return 0;
}
