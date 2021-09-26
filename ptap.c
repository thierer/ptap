/* ptap - Plays back TAP files to datasette
          (C) 1999-2015 Markus Brenner
  
          V 0.23  - still pretty experimental
          V 0.24  - added delay after tape starts
          V 0.25  - changed calibration delay to 10000
                    and start recording 1s after hitting <Enter>
          V 0.26  - added TAP Version 1 support
          V 0.27  - bugfixes
          V 0.28  - bugfixes in Version 1 support
          V 0.29  - started work on X(E)1541 transfer
          V 0.30  - nearly complete rewrite (timer, LPT detection, XE-cables)
          V 0.31  - added VIC-20 PAL/NTSC switches
          V 0.32  - C16,C116,PLUS/4 support
          V 0.33  - Various improvements
          V 0.34  - Usage message updated
          V 0.35  - Fixes X(E)1541 support
          V 0.36  - added XA1541 support
          V 0.37  - fixed timer not initialized bug
                    (thousand thanks Piret Endre!)
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <pc.h>
#include <crt0.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 37
#define VERSION_PATCH 2

/* DAC Color Mask register, default: 0xff. palette_color = PELMASK & color_reg */
#define PELMASK 0x03c6

/* pins for direct (C64S interface) access of datasette */
#define read_pin  0x40
#define sense_pin 0x20
#define write_pin 0x02

/* The PC timer's frequency */
#define BASEFREQ 1193182        /* CTC (Counter/Timer Chip) frequency */

/* some more TAP format constants */
#define ZERO 2500               /* approx. 1/50s, length of a V0 '00'-pause */
#define HEADERLEN 20            /* TAP format header length */

#define CTC_DATA0 0x40
#define CTC_MODE  0x43

/* The following frequencies in [Hz] determine the length of one TAP unit */
#define C64PALFREQ  123156      /*  985248 / 8 */
#define C64NTSCFREQ 127841      /* 1022727 / 8 */
#define VICPALFREQ  138551      /* 1108405 / 8 */
#define VICNTSCFREQ 127841      /* 1022727 / 8 */
#define C16PALFREQ  110840      /*  886724 / 8 */
#define C16NTSCFREQ 111860      /*  894886 / 8 */

/* Machine entry used in extended TAP header */
#define C64 0
#define VIC 1
#define C16 2

/* Video standards */
#define PAL  0
#define NTSC 1

/* cable types */
#define ADAPTER 0               /* C64S Adapter */
#define X1541   1               /* X1541-cable  */
#define XE1541  2               /* XE1541-cable */
#define XA1541  3               /* XA1541-cable */

/* lpt port registers */
#define STAT_PORT (port+1)
#define CTRL_PORT (port+2)

/* X1541-cable input/output bits (CTRL_PORT) */
#define X_ATN	 0x01
#define X_CLK	 0x02
#define X_DATA	 0x08
#define X_IMASK  0x04           /* X1541 invert mask for writing */

/* XE1541-cable (input) bits (STAT_PORT) */
#define XE_ATN   0x10
#define XE_CLK	 0x20
#define XE_DATA	 0x80

/* XA1541-cable (input) bits (STAT_PORT) */
#define XA_ATN   0x10
#define XA_CLK   0x20
#define XA_DATA  0x40
#define XA_IMASK 0x0b           /* XA1541 invert mask for writing */

#define MAXSTRLEN 80

/* lock the allocated memory. Important, as interrupts are disabled */
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;


/* global TAP file variables  */
/* will later become a struct */
struct tap_image
{
    int version;
    int machine;
    int video_standard;
    unsigned long size;
    unsigned long frequency;
    unsigned char *data; 
};


/* global variables */
unsigned long timer_last;
int port, port_s;
int cable;                      /* cable type used */

/* the RCS id */
static char rcsid[] = "$Id: ptap.c,v 0.36 2002/06/08 09:09:39 brenner Exp $";


void usage(void)
{
    fprintf(stderr, "usage: ptap [-lpt<x>] [-i] [-x[e]] <tap file to play back>\n");
    fprintf(stderr, "  -lpt<x>   use port x\n");
    fprintf(stderr, "  -i        ignore 'tape running' check\n");
    fprintf(stderr, "  -x        use X1541  cable for transfer\n");
    fprintf(stderr, "  -xa       use XA1541 cable for transfer\n");
    fprintf(stderr, "  -xe       use XE1541 cable for transfer\n");
    fprintf(stderr, "  -vicpal   play back VIC-20 PAL  tape\n");
    fprintf(stderr, "  -vicntsc  play back VIC-20 NTSC tape\n");
    exit(1);
}


int freadbyte(FILE *fpin)
{
    /* read one byte from *fpin, abort if none available */
    int c;

    if ((c = fgetc(fpin)) == EOF)
    {
        fprintf(stderr, "unexpected End of File!\n");
        exit(6);
    }
    return c;
}


void set_file_extension(char *str, char *ext)
{
    /* sets the file extension of String *str to *ext */
    int i, len;

    for (i=0, len = strlen(str); (i < len) && (str[i] != '.'); i++);
    str[i] = '\0';
    strcat(str, ext);
}


unsigned int find_port(int lptnum)
{
    /* retrieve LPT port address from BIOS */
    unsigned char portdata[8];
    unsigned int port;

    if ((lptnum < 1) || (lptnum > 4)) return 0;
    _dosmemgetb(0x408, 8, portdata);
    port = portdata[2*(lptnum-1)] + portdata[2*(lptnum-1)+1] * 0x100;
    return port;
}


int argcmp(char *str1, char *str2)
{
    while ((*str1 != '\0') && (*str2 != '\0'))
        if (toupper(*str1++) != toupper(*str2++)) return (0);
    return (1);
}


void init_border_colors()
{
    /* Init border colors for the borderflasher */
    union REGS r;
    int color;

    /* colour table according to PC64 */

    int red[]   = {0x00,0x3f,0x2e,0x00,0x25,0x09,0x01,0x3f,0x26,0x14,0x3f,0x12,0x1c,0x17,0x15,0x2b};
    int green[] = {0x00,0x3f,0x00,0x30,0x00,0x33,0x04,0x3f,0x15,0x08,0x14,0x12,0x1c,0x3f,0x1c,0x2b};
    int blue[]  = {0x00,0x3f,0x05,0x2b,0x26,0x00,0x2c,0x00,0x00,0x05,0x1e,0x12,0x1c,0x15,0x3a,0x2b};

    for (color = 1; color < 16; color++)
    {
        r.h.ah = 0x10;       /* Video BIOS: set palette registers function */
        r.h.al = 0x10;       /* set individual DAC colour register */
        r.w.bx = color * 16; /* set border color entry */
        r.h.dh = red[color];
        r.h.ch = green[color];
        r.h.cl = blue[color];
        int86(0x10, &r, &r);
    }

    r.h.ah = 0x10;      /* Video BIOS: set palette registers function */
    r.h.al = 0x01;      /* set border (overscan) DAC colour register */
    r.h.bh = 0xf0;      /* set border color entry */
    int86(0x10, &r, &r);
}


void set_border_black()
{
    /* Reset border color to black */
    union REGS r;
    r.h.ah = 0x10;      /* Video BIOS: set palette registers function */
    r.h.al = 0x01;      /* set border (overscan) DAC colour register */
    r.h.bh = 0x00;      /* set border color entry */
    int86(0x10, &r, &r);
}


void sleep_init()
{
    /* init the 8253 CTC (Counter/Timer Chip) */
    /* we will use the Channel #2, ordinarily used for Audio generation */

    outp(CTC_MODE,0x14);/* Bits 7/6:   Channel = 0,
                                5/4:   Command/Access mode: lobyte only
                                3/2/1: Operating mode: Mode 2 (Rate Generator)
                                0:     16-bit binary */


    /* only once needed for lobyte-access only??? */
    outp(CTC_DATA0,0);  /* program the Reload registers:  rate = 65536 */
    outp(CTC_DATA0,0);  /* write 2 byte Reload value in low/high order */

    outp(CTC_MODE,0x00);/* Bits 7/6:   Channel = 0,
                                5/4:   Command/Access mode: latch count value
                                3/2/1: (not important for latch command)
                                0:     (not important for latch command) */

    timer_last = inp(CTC_DATA0);        /* read counter low  byte */
}


void sleep_usecs(unsigned long usec)
{
    /* wait usec microseconds */
    unsigned long timer_now;
    unsigned long ticks_left;
    unsigned long ticks_delta;
    unsigned long maxdel;
    unsigned long byte;

    ticks_left = usec * BASEFREQ / 1000000;
    while (ticks_left)
    {
        outp(CTC_MODE,0x00);               /* latch counter */
        timer_now = inp(CTC_DATA0);     /* read counter low  byte */

        ticks_delta = (timer_last + 0x100 - timer_now) & 0xff;
        timer_last = timer_now;
        if (ticks_left >= ticks_delta)
            ticks_left -= ticks_delta;
        else
            ticks_left = 0;
    }
}


int tape_running()
{
    switch (cable)
    {
        case ADAPTER:
            return (!(inp(STAT_PORT) & sense_pin));
        case X1541:
            return (inp(CTRL_PORT) & X_ATN);
        case XE1541:
	    return (!(inp(STAT_PORT) & XE_ATN));
        case XA1541:
	    return (!(inp(STAT_PORT) & XA_ATN));
        default:
            return (-1); /* invalid */
    }
}


void toggle_output(int value)
{
    /* toggle the output (write) bit */
    unsigned char portvalue;

    switch (cable)
    {
        case ADAPTER:
            outp(port,value?write_pin:0);
            break;
        case X1541:
        case XE1541:
            /* X_CLK is negative logic, so invert 'value'! */
            outp(CTRL_PORT, value ? (X_DATA ^ X_IMASK)
                                  : ((X_DATA|X_CLK) ^ X_IMASK));
            break;
        case XA1541:
            /* X_CLK is negative logic, so invert 'value'! */
            outp(CTRL_PORT, value ? ((XA_DATA >> 4) ^ XA_IMASK)
                                  : (((XA_DATA|XA_CLK) >> 4) ^ XA_IMASK));
            break;
    }
}


void *read_tap_data(FILE *fpin, struct tap_image *tap)
{
    unsigned long filelength;
    char inputstring[MAXSTRLEN];
    int i;

    /* figure out file length */
    fseek(fpin,0,SEEK_END);
    filelength = ftell(fpin);
    rewind(fpin);

    /* check "C64-TAPE-RAW" string */
    fgets(inputstring, 13, fpin);
    if(strncmp(inputstring+4,"TAPE-RAW",8) != 0)
    {
        fprintf(stderr, "invalid or corrupt TAP file!\n");
        exit(5);
    }

    /* read the TAP-file version */
    tap->version = freadbyte(fpin);
    if ((tap->version < 0) || (tap->version > 2))
    {
        fprintf(stderr, "TAP Version not (yet) supported, sorry!\n");
        exit(6);
    }

    /* read additional TAP info fields */
    tap->machine = freadbyte(fpin);
    tap->video_standard = freadbyte(fpin);
    freadbyte(fpin);

    switch(tap->machine)
    {
        case VIC:
            tap->frequency =
                (tap->video_standard == NTSC) ? VICNTSCFREQ : VICPALFREQ;
            break;
        case C16:
            tap->frequency =
                (tap->video_standard == NTSC) ? C16NTSCFREQ : C16PALFREQ;
            break;
        case C64:
        default:
            tap->frequency =
                (tap->video_standard == NTSC) ? C64NTSCFREQ : C64PALFREQ;
            break;
    }

    /* read the data length */
    for (tap->size = 0, i = 0; i < 4; i++)
    {
        tap->size >>= 8;
        tap->size += (freadbyte(fpin) << 24);
    }

    /* check if data length is valid */
    if (tap->size != (filelength-HEADERLEN))
    {
        fprintf(stderr, "File size doesn't match Header!\n");
        exit(6);
    }

    /* allocate buffer for TAP data */
    if ((tap->data = calloc(tap->size, sizeof(unsigned char))) == NULL)
    {
        fprintf(stderr, "Couldn't allocate buffer memory!\n");
        exit(7);
    }

    /* read in TAP data */
    if (fread(tap->data, sizeof(unsigned char), tap->size, fpin) != tap->size)
    {
        fprintf(stderr, "Couldn't read whole TAP file!\n");
        exit(8);
    }
}


int main(int argc, char **argv)
{
    FILE *fpin;                     /* TAP file for playback */
    char tap_file_name[MAXSTRLEN];  /* TAP file filename */
    unsigned char *buffer, *buffer_end;
    unsigned int tapbyte;
    unsigned long pause;
    unsigned long half_wave_time;
    int half_wave_level;
    int lptnum;
    int i;

    int flash=0;                    /* border color */
    struct tap_image tap;
    int ignore;

    printf("\nptap - Commodore TAP file player v%d.%02d.%d\n\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    ignore = 0;
    lptnum = 1;
    cable = ADAPTER;
    tap.machine = C64;
    tap.video_standard = PAL;
    tap.frequency = C64PALFREQ;
    while (--argc && (*(++argv)[0] == '-'))
    {
        switch (tolower((*argv)[1]))
        {
            case 'i':
                ignore = 1;
                break;
            case 'l':
                lptnum = atoi(*argv+4);
                printf("lptnum: %d\n", lptnum);
                if ((lptnum < 1) || (lptnum > 4)) usage();
                break;
            case 'v':
                if (argcmp(*argv,"-vicpal"))
                {
                    printf("playing back VIC-20 PAL TAP\n");
                    tap.frequency = VICPALFREQ;
                }
                else if (argcmp(*argv,"-vicntsc"))
                {
                    printf("playing back VIC-20 NTSC TAP\n");
                    tap.frequency = VICNTSCFREQ;
                }
                break;
            case 'x':
                if (tolower((*argv)[2]) == 'a')
                    cable = XA1541;
                else if (tolower((*argv)[2]) == 'e')
                    cable = XE1541;
                else
                    cable = X1541;
                break;
            default:
                break;
        }
    }
    if (argc < 1) usage();

    port = find_port(lptnum);
    port_s = port+1;
    if (port == 0)
    {
        fprintf(stderr, "LPT%d doesn't exist on your machine!\n", lptnum);
        exit(1);
    }
    else printf("Using port LPT%d at %04x\n",lptnum,port);

    /* try to open TAP file for playback */
    strcpy(tap_file_name, argv[0]);
    set_file_extension(tap_file_name, ".TAP");

    if ((fpin = fopen(tap_file_name,"rb")) == NULL)
    {
        fprintf(stderr, "Couldn't open TAP file %s!\n", tap_file_name);
        exit(2);
    }


    if (!read_tap_data(fpin, &tap))
    {
        fprintf(stderr, "Couldn't read TAP file %s!\n", tap_file_name);
        exit(3);
    }

    printf("TAP Version: %d\n", tap.version);
    printf("TAP size:    %x\n", tap.size);

    init_border_colors();

    if (!ignore)
    {
        /* put TAPSERV into SEND mode (for reliable PLAY-detection) */
        if ((cable == X1541) || (cable == XE1541))
            outp(CTRL_PORT, X_IMASK);
        else if (cable == XA1541)
            outp(CTRL_PORT, XA_IMASK);

        if (tape_running())
            printf("Please <STOP> the tape.\n");
        while (tape_running());
        /* put TAPSERV into RECEIVE mode now */
        if ((cable == X1541) || (cable == XE1541))
            outp(CTRL_PORT, X_DATA ^ X_IMASK);
        else if (cable == XA1541)
            outp(CTRL_PORT, (XA_DATA >> 4) ^ XA_IMASK);

        printf("Press <RECORD> AND <PLAY> on tape\n");
        while (!tape_running());
        printf("Tape now running, ");
        /* wait half a second for tape to settle */
        delay(500);
    }

    if (ignore)
    {
        /* put TAPSERV into RECEIVE mode */
        if ((cable == X1541) || (cable == XE1541))
            outp(CTRL_PORT, X_DATA ^ X_IMASK);
        else if (cable == XA1541)
            outp(CTRL_PORT, (XA_DATA >> 4) ^ XA_IMASK);
        /* start without checking for <RECORD> (batch mode) */
        /* sleep 2 seconds */
        delay(2000);
    }

    printf("Playing back: %s\n", tap_file_name);
    buffer_end = tap.data+tap.size;

    disable();
    sleep_init();
    half_wave_level = 0;
    /* the actual playback loop */
    for (buffer = tap.data; buffer < buffer_end; buffer++)
    {
        tapbyte = *buffer;

        if (tapbyte != 0x00)
            half_wave_time = (tapbyte * 1000000 / tap.frequency + 1) / 2;
        else if (tap.version == 0)
        {
            pause = ZERO;
            for (;(*(buffer+1) == 0) && ((buffer + 1) < buffer_end); buffer++)
                pause += ZERO;
            half_wave_time = (pause * 1000000 / tap.frequency + 1) / 2;
        }
        else if (tap.version > 1)
        {
            if ((buffer + 3) >= buffer_end) break;
            pause = 0;
            for (i = 0; i < 3; i++)
            {
                buffer++;
                pause >>= 8;
                pause += (*buffer << 16);
            }
            half_wave_time = (pause * 1000000 / tap.frequency + 8) / 16;
        }

	outp(PELMASK, (flash++ << 4) | 0x0f);
        if (tap.version != 2)
        {
            toggle_output(1);
            sleep_usecs(half_wave_time);
            toggle_output(0);
            sleep_usecs(half_wave_time);
            toggle_output(1);
        }
        else
        {
            half_wave_level ^= 1;
            half_wave_time *= 2;
            toggle_output(half_wave_level);
            sleep_usecs(half_wave_time);
        }
    }
    set_border_black();
    outp(PELMASK, 0xff);
    enable();

    /* cleanup */

    /* put TAPSERV into SEND mode, so TAPSERV can relax... */
    if ((cable == X1541) || (cable == XE1541))
        outp(CTRL_PORT, X_IMASK);
    else if (cable == XA1541)
        outp(CTRL_PORT, XA_IMASK);

    fclose(fpin);
    free(tap.data);
}
