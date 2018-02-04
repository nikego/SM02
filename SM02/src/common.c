/*****************************************************************************
 * common.c
 *
 *    common functions - egl initialize, at least
 *
 ****************************************************************************/
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include "common.h"
#include <sys/time.h>
#include <termios.h>

int gQuit = 0;
void signalHandler(int signum) 
{
    (void)signum;
    gQuit = 1;
}

int nextp2(int x)
{
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x + 1;
}

unsigned char read_console()
{ // считываем данные с консоли
    unsigned char rb;
    struct timeval tv;
    int retval;
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt); // открываем терминал для реакции на клавиши без эха
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds); // 0 - стандартный вход
    tv.tv_sec = 0;
    tv.tv_usec = 1000; // ждем 0.001с
    retval = select(1, &rfds, NULL, NULL, &tv);
    if (!retval)
        rb = 0;
    else {
        if (FD_ISSET(STDIN_FILENO, &rfds))
            rb = getchar();
        else
            rb = 0;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return rb;
}

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec * 10000LL + te.tv_usec / 100; // caculate 0.1 milliseconds
    return milliseconds;
}

void print_matrixf(GLfloat* m)
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            printf("%.2f\t", m[i * 4 + j]);
        }
        printf("\n");
    }
}

/***************************************************************************
* CRC functions 
*/

static uint32_t wCRC_Table[256] = {0};

const uint32_t CRCPOL =  0xA001;

void MakeCRCTable(void)
{
#if 1
	const uint32_t CRC_POLY = 0xEDB88320;
    uint32_t i, j, r;
    for (i = 0; i < 256; i++){
		for (r = i, j = 8; j; j--)
			r = r & 1? (r >> 1) ^ CRC_POLY: r >> 1;
        wCRC_Table[i] = r;
    }
#else
	UINT wTable[8];
	UINT i, j, k, wRemainder, temp;

	wRemainder = CRCPOL;
	for (i = 0; i < 8; i++)
	{
		wTable[i] = wRemainder;
		temp = wRemainder & 1;
		wRemainder = wRemainder >> 1;
		if (temp == 1)
			wRemainder = wRemainder ^ CRCPOL;
	}
	for (i = 0; i < 256; i++)
	{
		wRemainder = 0;
		k = i;
		for (j = 0; j < 8; j++)
		{
			if ( (k & 0x80) == 0x80 )
            wRemainder = wRemainder ^ wTable[j];
			k = k << 1;
		}
		wCRC_Table[i] = wRemainder;
	}
#endif
}

// End: MakeCRCTable
void CRC16 (uint8_t cData, uint32_t *rest)
{
	uint32_t x;
	if ( wCRC_Table[5] == 0 )
		MakeCRCTable();

	x = (cData ^ *rest) & 0xFF;
	*rest = *rest >> 8;
	*rest = *rest ^ wCRC_Table[x];
}

// End: CRC16
uint32_t CalcCRC(uint8_t *pdata,int size)
{
#if 1 
	uint32_t crc = 0xFFFFFFFF;
	if ( wCRC_Table[5] == 0 )
		MakeCRCTable();
        
	while (size--) {
        crc = wCRC_Table[( crc & 0xFF ) ^ *pdata++] ^ (crc >> 8);
	}
	crc ^= 0xFFFFFFFF;
	return crc;
#else
	uint32_t CRCword = 0xFFFF;

	for (int i = 0; i < size; i++)
		CRC16 (pdata[i], &CRCword);

	return CRCword;
#endif
}
