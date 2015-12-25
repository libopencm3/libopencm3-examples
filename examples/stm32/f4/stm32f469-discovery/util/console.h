/*
 * Our simple console definitions
 */

void console_putc(char c);
char console_getc(int wait);
void console_puts(char *s);
int console_gets(char *s, int len);
void console_setup(int baud);
void console_baud(int baudrate);

/* this is for fun, if you type ^C to this example it will reset */
#define RESET_ON_CTRLC

