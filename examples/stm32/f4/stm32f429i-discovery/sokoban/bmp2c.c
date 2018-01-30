/******************************************************
 * Native PC program to convert BMP image to a C header
 * with raw data of an image 320x240 565 color format
 * by Marcos A. Stemmer
 * Command line:
 * bmp2c arquivo-bitmap > bmp.c
 * ****************************************************/
#include <stdio.h>
#include <string.h>

struct bmp_header {
	unsigned int filesize;
	int res1;
	unsigned int dataoffset;
	int ihsize;
	int width;
	int hight;
	short planes;
	short nbits;
	int compression;
	int imagesize;
	int colorinfo[4];
	};

/* Convert RGB color components RGB to 16 bits 565 */
/* Swap bytes because Discovery dislpay is big endian: */
/* G2G1G0B4B3B2B1B0 R4R3R2R1R0G5G4G3	*/
int cor565(int r, int g, int b)
{
return (((g & 0x1c) << 11) | ((b & 0xF8) << 5) |
	(r & 0xf8) | ((g & 0xe0) >> 5));
}

void justname(char *filename, char *dest)
{
	char *p, *sp;
	int k, kp;
	for(p = sp = filename; *p; p++) {
		if(*p == ':' || *p == '\\' || *p =='/') sp = p+1;
	}
	kp = strlen(sp);
	for(k=0; sp[k] && (k < 40); k++) {
		if(sp[k]=='.') kp = k;
		dest[k] = sp[k];
	}
	dest[k] = '\0';
	dest[kp] = '\0';
}

int bmp2c(char *bmp_name)
{
	FILE *file1;
	unsigned nb, kb;
	unsigned char lbmp[240*3];
	char arrayname[48];
	struct bmp_header bmh;
	int x, y;
	int altura, largura;
	file1 = fopen(bmp_name, "rb");
	if(file1 == NULL) {
		return -1;
		}
	nb = fread(lbmp, 1, 2, file1);
	if(lbmp[0]!='B' || lbmp[1]!='M' || nb != 2) return -2;
	nb = fread(&bmh, sizeof(struct bmp_header), 1, file1);
	if(nb != 1) {
		fclose(file1);
		return -1;
		}
	if((bmh.nbits != 24) || (bmh.compression != 0)) {
		fclose(file1);
		return -1;
		}
	largura = bmh.width < 240? bmh.width:240;
	altura = bmh.hight < 320? bmh.hight: 320;
	justname(bmp_name, arrayname);
	printf("\nstatic const unsigned short %s_img[] = { %d, %d,\n", arrayname, largura, altura); 
	kb = 16;
	largura *= 3;
	for(y = 0; y < altura; y++) {
		fseek(file1, bmh.dataoffset + ((3*bmh.width + 7) & -8)*y, SEEK_SET);
		nb = fread(lbmp, 1, largura, file1);
		for(x=0; x < nb; x+=3) {
			printf("0x%04X,", cor565(lbmp[x+2], lbmp[x+1], lbmp[x]));
			if(kb-- == 0) {
				printf("\n");
				kb = 16;
				}
			}
		if(nb != largura) break;
		}
	printf("0 };\n");
	fclose(file1);
	return 0;
}

int main(int argc, char **argv)
{
int k;
for(k = 1; k < argc; k++) {
	if(bmp2c(argv[k])) {
		perror(argv[k]);
		}
	}
return 0;
}
