//====================================================
// kindle clock - large numeral display on kindle
// derived from geekmaster's dither demo
// ---------------------------------------------------
// demo - kindle eink dynamic dither demo for tcc
// Copyright (C) 2012 by geekmaster, with MIT license:
// http://www.opensource.org/licenses/mit-license.php
//----------------------------------------------------
// The speed is limited by the eink device drivers.
// Newer kindle models are faster, but need delays.
// This was tested on DX,DXG,K3,K4(Mini),K5(Touch).
//----------------------------------------------------

#include <stdio.h>      // printf
#include <stdlib.h>     // malloc, free
#include <string.h>     // memset, memcpy
#include <unistd.h>     // usleep
#include <fcntl.h>      // open, close, write
#include <time.h>       // time
#include <sys/mman.h>   // mmap, munmap
#include <sys/ioctl.h>  // ioctl
#include <linux/fb.h>   // screeninfo

#define ED6 570000      // k4 eink delay best quality
#define ED5 500000      // k4 eink delay good quality
#define ED4 230000      // k4 eink delay K3 speed
#define ED3 100000      // k4 eink delay okay
#define ED2 80000       // k4 eink delay fast
#define ED1 0           // k4 eink delay none, bad
#define K4DLY ED2       // k4 eink delay

enum eupd_op { EUPD_OPEN,EUPD_CLOSE,EUPD_UPDATE };
typedef unsigned char u8;
typedef unsigned int u32;

// function prototypes
void inline setpx(int,int,int);
void box(int,int,int,int);
int eupdate(int);
void draw_from_array(int[7][5],int,int,int);
void num_zero(int,int,int);
void num_one(int,int,int);
void num_two(int,int,int);
void num_three(int,int,int);
void num_four(int,int,int);
void num_five(int,int,int);
void num_six(int,int,int);
void num_seven(int,int,int);
void num_eight(int,int,int);
void num_nine(int,int,int);

// global vars
u8 *fb0=NULL;           // framebuffer pointer
int fdFB=0;             // fb0 file descriptor
u32 fs=0;               // fb0 stride
u32 MX=0;               // xres (visible)
u32 MY=0;               // yres (visible)
u8 blk=0;               // black
u8 wht=0;               // white
u8 pb=0;                // pixel bits

void kindleclock(void) {
    int x,y;
    struct fb_var_screeninfo screeninfo;
    fdFB=open("/dev/fb0",O_RDWR); // eink framebuffer

    // calculate model-specific vars
    ioctl(fdFB,FBIOGET_VSCREENINFO,&screeninfo);
    MX=screeninfo.xres; // max X+1
    MY=screeninfo.yres; // max Y+1
    pb=screeninfo.bits_per_pixel; // pixel bits
    fs=screeninfo.xres_virtual*pb/8; // fb0 stride
    blk=pb/8-1; // black
    wht=~blk; // white
    fb0=(u8 *)mmap(0,MY*fs,PROT_READ|PROT_WRITE,MAP_SHARED,fdFB,0); // map fb0
    eupdate(EUPD_OPEN); // open fb0 update proc

    // calculate days and load into char array
    srand(time(NULL));
    time_t epoch=time(NULL);
    int days=((2849756400 - (int)time(NULL))/(60*60*24));
    char arr[8];
    sprintf(arr, "%d", days);

    // display block numerals on screen
    int x_offset=75;
    int y_offset=(rand() % 665);
    int size=15;
    int i;
    for (i = 0; i < 5; ++i) {
        if (arr[i] == '0') {
            num_zero(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '1') {
            num_one(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '2') {
            num_two(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '3') {
            num_three(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '4') {
            num_four(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '5') {
            num_five(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '6') {
            num_six(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '7') {
            num_seven(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '8') {
            num_eight(x_offset+(90*i),y_offset+15,size);
        } else if (arr[i] == '9') {
            num_nine(x_offset+(90*i),y_offset+15,size);
        }
    }

    // cleanup - close and free resources
    eupdate(EUPD_UPDATE); // update display
    eupdate(EUPD_CLOSE);  // close fb0 update proc port
    munmap(fb0,fs*(MY+1)); // unmap fb0
    close(fdFB); // close fb0
}

//===============================
// eupdate - eink update display
// op (open, close, update)
//-------------------------------
int eupdate(int op) {
    static int fdUpdate=-1;
    if (EUPD_OPEN==op) { fdUpdate=open("/proc/eink_fb/update_display",O_WRONLY);
    } else if (EUPD_CLOSE==op) { close(fdUpdate);
    } else if (EUPD_UPDATE==op) {
        if (-1==fdUpdate) { system("eips ''"); usleep(K4DLY);
        } else { write(fdUpdate,"1\n",2); }
    } else { return -1; }
    return fdUpdate;
}

//========================================
// setpx - draw pixel using ordered dither
// x,y: screen coordinates, c: color(0-64).
// (This works on all eink kindle models.)
//----------------------------------------
void inline setpx(int x,int y,int c) {
    // dither table
    static int dt[64] = { 1,33,9,41,3,35,11,43,49,17,57,25,51,19,59,27,13,45,5,
    37,15,47,7,39,61,29,53,21,63,31,55,23,4,36,12,44,2,34,10,42,52,20,60,28,50,
    18,58,26,16,48,8,40,14,46,6,38,64,32,56,24,62,30,54,22 };

    // geekmaster formula 42
    fb0[pb*x/8+fs*y]=((128&(c-dt[(7&x)+8*(7&y)]))/128*(blk&(240*(1&~x)|
        15*(1&x)|fb0[pb*x/8+fs*y])))|((128&(dt[(7&x)+8*(7&y)]-c))/128*wht|
        (blk&((240*(1&x)|15*(1&~x))&fb0[pb*x/8+fs*y])));
}

//======================
// box - simple box draw
//----------------------
void box(int x,int y,int d,int c) {
    int i;
    int j;
    for (i=0;i<d;++i) {
        for (j=0;j<d;++j) {
            setpx(x+i,y+j,c);
        }
    }
}

void draw_from_array(int arr[7][5], int x, int y, int m) {
    int i;
    int j;
    for (i=0;i<7;++i) {
        for (j=0;j<5;++j) {
            if (arr[i][j] == 1) {
                box(x+(m*j), y+(m*i), m, 64);
            }
        }
    }
}

void num_zero(int x, int y, int m) {
    int arr[7][5] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,1,1},
        {1,0,1,0,1},
        {1,1,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_one(int x, int y, int m) {
    int arr[7][5] = {
        {0,0,1,0,0},
        {0,1,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,1,1,1,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_two(int x, int y, int m) {
    int arr[7][5] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {1,1,1,1,1}
    };
    draw_from_array(arr, x, y, m);
}

void num_three(int x, int y, int m) {
    int arr[7][5] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,1,1,0},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_four(int x, int y, int m) {
    int arr[7][5] = {
        {0,0,0,1,0},
        {0,0,1,1,0},
        {0,1,0,1,0},
        {1,0,0,1,0},
        {1,1,1,1,1},
        {0,0,0,1,0},
        {0,0,0,1,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_five(int x, int y, int m) {
    int arr[7][5] = {
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,1,1,1,0},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_six(int x, int y, int m) {
    int arr[7][5] = {
        {0,0,1,1,0},
        {0,1,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_seven(int x, int y, int m) {
    int arr[7][5] = {
        {1,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_eight(int x, int y, int m) {
    int arr[7][5] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0}
    };
    draw_from_array(arr, x, y, m);
}

void num_nine(int x, int y, int m) {
    int arr[7][5] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,1,1,0,0}
    };
    draw_from_array(arr, x, y, m);
}

int main(void) {
    kindleclock();
    return 0;
}
