/************************************************
Familia printf no lpc2378 com gcc
Marcos Augusto Stemmer
Modificado em sexta 13 de marco de 2015
Pode escrever zeros a esquerda nos formatos de ponto flutuante
*************************************************
A funcao mprintf e'parecida com a fprintf da biblioteca padrao
O primeiro parametro e' o nome da funcao putchar que deve ser usada para escrever.
Os codigos de formato sao parecidos com os da funcao printf padrao:
Existem difierncas: so alguns formatos sao suportados.
Suporta os formatos:
O modificador '-' significa alinhamento a esquerda.
O modificador '0' faz preencher o campo com zeros em vez de espacos
[campo]	numero de caracteres de largura total do campo
	Se campo for omitido assume 0; Se campo inicia com 0 preenche com zeros.
[frac]	Numero de caracteres da fracao do formato f; pode ser omitido.
	%[campo]d ou i	Escreve inteiro decimal com sinal
	%[campo]u	Decimal sem sinal
	%[campo]x ou X	Hexadecimal
	%[campo]o ou q	(letra o ou letra que): Escreve como octal.
	%[campo]c	Escreve inteiro como caractere ASCII
	%[campo]s	String
	%[campo].[frac]f	Escreve numero tipo double com ponto fixo
	%[campo].[frac]e	Escreve double em notacao cientifica
	%[campo].[frac]g	Ponto fixo comutando automaticamente para 
notacao cientifica se o valor for maior que 1e10 ou menor que a precisao.
	Os transformadores de tipo h, l, L, hh ou ll sao ignorados.
Caracteres de formato maiusculos e minusculos dao o mesmo resultado.
Retorna o numero de caracteres escritos.

A funcao sprintf e' similar ao sprintf da biblioteca padrao.
Este modulo e' independente do hardware
*/
#include <stdarg.h>
#include <ctype.h>
#define edigito(c) ((c)>='0' && (c)<='9')
/* Comentando a definicao de FPSUPPORT pode-se diminuir o codigo gerado eliminando 
o suporte aos formatos de ponto flutuante (float e double) */
//#define DEBUG
//#define FPSUPPORT
/* Local Function prototypes */
int u2str(char *buf, unsigned int num, int base);
int matoi(char *str);
int mprintf(void (*putc)(int), const char *formato, ... );
int va_printf(void (*putc)(int), const char *formato, va_list va);
void sputchar(int c);
int sprintf(char *buf, const char *formato, ... );

/* uso interno: Escreve o numero de tras para a frente */
int u2str(char *buf, unsigned int num, int base)
{
int nd=0, c;
do	{
	c = (num % base) + '0';
	if(c > '9') c+=7;
	buf[nd++] = c;
	num /= base;
	} while(num);
return nd;
}

/* Converte string para numero (similar ao atoi da biblioteca padrao);
 Usa-se assim:
char buf[20];	// Para armazenar o texto lido
int i;		// Vai ler o numero i
U0gets(buf,20);	// Le como texto
i = atoi(buf);	// Converte para numero
*/

int ngscanf;


int matoi(char *str)
{
int x, d, s;
x=s=0;
while(!edigito(*str)) { s |= (*str++ == '-'); ngscanf++; }
do	{
	d = *str++;
	if(!edigito(d)) break;
	x = x*10 + d - '0';
	ngscanf++;
	} while(1);
return s? -x: x;
}

int padchar;

#ifdef FPSUPPORT
/* Converte string para numero tipo double (com notacao cientifica)*/
double atod(char *str)
{
double x, p;
int d, s;
p=1.0;
x=0.0; s=0;
while(*str == ' ') str++;
if(*str == '-') { s=1; str++; }
do	{
	d = *str++;
	if(d=='.') { d = *str++; s |= 2; }
	if(d < '0' || d > '9') break;
	x = x*10 + d - '0';
	if(s & 2) p *=10.;
	} while(1);
if(d=='e' || d=='E'){
	if(*str == '-') { str++; s |= 4; }
	if(*str == '+') str++;
	d=0;
	while(edigito(*str)) d = 10*d + (*str++) - '0';
	if(s & 4) while(d--) p *= 10.0;
	else while(d--) x *= 10.0;
	}
if(s & 1) x = -x;
return x/p;
}

/**** dprint Escreve double ******
x	Numero double que sera escrito
campo	Espaco de campo total (largura em caracteres)
frac	Numero de digitos depois do ponto
putc	Nome da funcao que escreve caractere 
retorna o numero de caracteres escritos  */
int dprint(double x, int campo, int frac, void (*putc)(int))
{
int dig, k, sinal=0, npr=1;
double ar;
if(x < 0) { sinal=1; x=-x; campo--; }
for(ar=0.5, k=0; k<frac; k++) ar/=10.0;
x += ar;
k=0;
campo -= (frac+1);
while(x >= 1.0) { x/=10.0; k++; campo--; }
if(!k) { k=1; campo--; x/=10.0; }
frac += k;
if(padchar == 0x130 && sinal) {
	putc('-'); npr++;
	}
while(campo-- > 0) { putc(padchar); npr++; }
if(padchar == 0x120 && sinal) {
	putc('-'); npr++;
	}
npr += frac;
while(frac--) {
	x *= 10;
	dig = x;
	x -= dig;
	putc(dig+'0');
	if(!(--k)) { putc('.'); k=800; }
	}
return npr;
}
#endif

int va_printf(void (*putc)(int), const char *formato, va_list va)
{
char buf[16];
char *s, *ps;
int c,  frac, n, sinal, base, nprinted;
int campo;
#ifdef FPSUPPORT
double x, y;
#endif
s=(char *)formato;
nprinted=campo=0;
while((c=*s++)) {
	if(c != '%') { putc(c); nprinted++; continue;} 
	c=*s++;
	sinal=frac=campo=0;
	padchar = ((c=='0')? '0' + 0x100:' ' + 0x100);
	if(c=='-') { padchar =' '; c=*s++; }
	while(edigito(c)) { campo=10*campo+c-'0'; c=*s++; }
	if(c=='.') {
		c=*s++;
		while(edigito(c)) { frac=10*frac+c-'0'; c=*s++; }
		}
	while(c=='l' || c=='L' || c=='h') c=*s++;
	base=10;
	if(c >= 'A' && c <= 'Z') c ^= 0x20;
	switch(c){
		case 'o': case 'q': base=8; goto lbl_u;
		case 'x': base=16;
		case 'u':
		lbl_u:
			n = u2str(buf, va_arg(va, unsigned int), base);
		lbl_escrevei:
		nprinted += n;
		campo -= (n+sinal);
		if(campo < 0) campo=0;
		nprinted +=campo;
			if(padchar & 0x100) while(campo--) putc(padchar & 0xff);
			if(sinal) putc('-');
			while(n--) putc(buf[n]);
			break;
		case 'd': case 'i':
			base=va_arg(va, int);
			if(base < 0) { sinal=1; base=-base; }
			n = u2str(buf, base, 10);
			goto lbl_escrevei;
		case 'c':
			if(campo < 0) campo=0;
			nprinted +=campo;
			if(padchar & 0x100) while(campo--) putc(padchar);
			putc(va_arg(va, int)); nprinted++; break;
		case 's':
			ps = va_arg(va, char *);
			for(n=0; ps[n]; n++);
			nprinted += n;
			campo -= n;
			if(campo < 0) campo=0;
			nprinted +=campo;
			if(padchar & 0x100) while(campo--) putc(padchar);
			while(*ps) { putc(*ps); ps++; }
			break;
#ifdef FPSUPPORT
		case 'e': case 'f': case 'g':
			n=0;
			x=y=va_arg(va, double);
			if(x<0) { x=-x; sinal=1; }
			while((x > 1.0)) { x /= 10.0; n++; }
			if(x) while((x < 1.0)) { x *= 10.0; n--; }
			if(c=='e' || ((c=='g') && 
				((n > 9) || ((-n >= frac)&& frac)))){
				y = (sinal ? -x: x);
				sinal = 2;
				campo -=4;
				if(frac<2) frac=2;
				}
			c = dprint(y, (padchar & 0x100)?campo:0, frac, putc);
			campo -=c;
			nprinted +=c;
			if(sinal & 2){
				putc('E');
				if(n < 0) { putc('-'); n=-n; }
				else putc('+');
				if(n>99) { putc((n/100) + '0'); n %= 100; }
				putc((n / 10) + '0');
				putc((n % 10) + '0');
				}
			break;
#endif
		case '%': putc(c); nprinted++; break;
		}
	if(campo < 0) campo=0;
	nprinted +=campo;
	if(!(padchar & 0x100)) while(campo--) putc(padchar);
	}
return nprinted;
}

/*
O primeiro parametro e' o nome da funcao usada para escrever caracteres 
os demais parametros funcionam como no printf */
int mprintf(void (*putc)(int), const char *formato, ... )
{
va_list va;
va_start(va,formato);
return va_printf(putc, formato, va);
}

char *gp_buf;
void sputchar(int c) { *gp_buf++=c; }

/* Atendendo a pedidos, o meu sprintf: */
int sprintf(char *buf, const char *formato, ... )
{
va_list va;
int n;
va_start(va,formato);
gp_buf=buf;
n=va_printf(sputchar, formato, va);
*gp_buf='\0';
return n;
}
