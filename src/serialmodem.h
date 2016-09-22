/*
 * serialmodem.h
 * fltiny serial modem interface header
 */

#define SERIALMODEM_H 1

void serial_read();
int serial_init();
int serial_write(char c);
int serial_close();
