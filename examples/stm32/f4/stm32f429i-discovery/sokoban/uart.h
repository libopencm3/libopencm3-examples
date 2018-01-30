

int U1available(void);
int U1getchar(void);
void U1putchar(int c);
int U1getchare(void);
void U1puts(char *txt);
void U1gets(char *txt, int nmax);
int U1getnum(char *prompt, unsigned base);
void usart_setup(void);
unsigned xatoi(char *str, unsigned base);
int mprintf(void (*putc)(int), const char *formato, ... );

#define TAM_FILA_RX	64
void dump(void);
void copia(void);
void verifica(void);
