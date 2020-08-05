/* mtap - Markus' TAP generator

   (C) 1998-2002 Markus Brenner
  
  
          V 0.1  - still pretty experimental
          V 0.11 - free memory
          V 0.12 - Header corrected:
                    'MTAP' removed
                    filelength entry corrected
                    Cassette stop correction
          V 0.13 - correct '00' bytes
          V 0.13a- various long pulses corrections
                   force filename extension to '.tap'
          V 0.14   Pauses should be mostly correct now
                   now converts V and Boulder Dash CS correctly
                   prints program name and version number
                   always sets .TAP extension
          V 0.15   Borderflasher added
                   short pulse after longpulse added to longpulse
          V 0.16   Choice of LPT port
                   delay play key
          V 0.17   according to Andreas Boose set 'ZERO' to 2500
                   buffersize can be set by command line
          V 0.18   a minor cosmetic change (usage) :)
          V 0.19   extension for X(E)1541 transfer
          V 0.20   bugfixes  for X1541
          V 0.21   added TAP Version 1 support
          V 0.22   fix first pulse after pause
          V 0.23   made Version 1 the default
          V 0.24   fixed Version 1 pauses
          V 0.25   fixed Version 1 pauses (Ben's 8x bug)
          V 0.26   added VIC-20 switch (for VIC-20 PAL tapes)
          V 0.27   added VIC-20 NTSC switch
          V 0.28   changed to new TAPSERV standard
          V 0.29   added LPT detection, improved program structure
          V 0.30   C16,C116,PLUS/4 support
          V 0.31   experimental halfwave support
          V 0.32   fixed bug in argcmp()
          V 0.33   fixed severe overflow bug in calculation of pauses
          V 0.34   improved conversion by using long long integers
          V 0.35   warn user if output file exists, improved rounding
          V 0.36   added XA1541 support
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <pc.h>
#include <crt0.h>


#define VERSION_MAJOR 0
#define VERSION_MINOR 36

#define STAT_PORT (port+1)
#define CTRL_PORT (port+2)

#define X_ATN   0x01
#define X_CLK   0x02
#define X_DATA  0x08

#define XE_ATN  0x10
#define XE_CLK  0x20
#define XE_DATA 0x80

/* defines for XA_ATN and XA_CLK identical with XE_XXX */
#define XA_ATN  0x10
#define XA_CLK  0x20
#define XA_DATA 0x40

/* DAC Color Mask register, default: 0xff. palette_color = PELMASK & color_reg */
#define PELMASK 0x03c6
#define READ_PIN  0x40
#define SENSE_PIN 0x20
#define BUFFERSIZE 0x400000	/* buffer defaults to 4 Megabyte */
#define BASEFREQ 1193182
#define ZERO 2500		/* approx. 1/50 s */

/* The following frequencies in [Hz] determine the length of one TAP unit */
#define C64PALFREQ  123156	/*  985248 / 8 */
#define C64NTSCFREQ 127841	/* 1022727 / 8 */
#define VICPALFREQ  138551	/* 1108405 / 8 */
#define VICNTSCFREQ 127841	/* 1022727 / 8 */
#define C16PALFREQ  110840	/*  886724 / 8 */
#define C16NTSCFREQ 111860	/*  894886 / 8 */

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

/* globals */
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;
unsigned long int buffersize = BUFFERSIZE;
int tapvers;
int cable;

/* the RCS id */
static char rcsid[] = "$Id: mtap.c,v 0.36 2002/06/09 10:44:26 brenner Exp $";


void usage(void)
{
    fprintf(stderr, "usage: mtap [-lpt] [-x[e]] [-buffer <size>] [-v] <tap output file>\n");
    fprintf(stderr, "  -lpt<x>:  use parallel port x (default: lpt1)\n");
    fprintf(stderr, "  -x:       use X1541  cable for transfer\n");
    fprintf(stderr, "  -xa:      use XA1541 cable for transfer\n");
    fprintf(stderr, "  -xe:      use XE1541 cable for transfer\n");
    fprintf(stderr, "  -b:       increase buffer size (default: 4 MB)\n");
    fprintf(stderr, "  -h:       halfwaves\n");
    fprintf(stderr, "  -r:       write raw sample data to <output file>.raw (for debugging)\n");
    fprintf(stderr, "  -v:       record Version 0 TAP\n");
    fprintf(stderr, "  -vicntsc: record VIC-20 NTSC tape\n");
    fprintf(stderr, "  -vicpal:  record VIC-20 PAL  tape\n");
    fprintf(stderr, "  -c16ntsc: record C16 NTSC tape\n");
    fprintf(stderr, "  -c16pal:  record C16 PAL  tape\n");
    fprintf(stderr, "  -c64ntsc: record C64 NTSC tape\n");
    fprintf(stderr, "  -c64pal:  record C64 PAL  tape\n");
    exit(1);
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
    while ((*str1 != '\0') || (*str2 != '\0'))
        if (toupper(*str1++) != toupper(*str2++)) return (0);
    return (1);
}


void init_border_colors()
{
    /* Init border colors for borderflasher */
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


unsigned char *record_adapter(int port, unsigned char *bufferp)
{
    int waitflag;
    int overflow;
    int flash; /* border color */
    int lb, hb;

    printf("Recording from C64S interface\n");

    if (!(inp(STAT_PORT) & SENSE_PIN))
        printf("Please <STOP> the tape.\n");
    while (!(inp(STAT_PORT) & SENSE_PIN));

    printf("Press <PLAY> on tape!\n");
    while (inp(STAT_PORT) & SENSE_PIN);

    printf("Tape now running, recording...\n");

    delay(500); /* delay for half a second, so signal settles */


    disable();

    overflow = 0;
    waitflag = 0;
    flash = 0;

    /* init counter */
    outp(0x43,0x34);
    outp(0x40,0); /* rate = 65536 */
    outp(0x40,0);

    /* wait for 1->0 transition */
    while ((inp(STAT_PORT) & (READ_PIN | SENSE_PIN)) == READ_PIN);

    /* store initial timer value */
    outp(0x43,0x00);        /* latch counter 0 output */
    *bufferp++ = inp(0x40); /* read CTC channel 0 (lb) */
    inp(0x61);              /* dummy instruction */
    *bufferp = inp(0x40);
    overflow = *bufferp++;


    do
    {
        /* wait for 0 -> 1 transition */
        while (!(inp(STAT_PORT) & (READ_PIN | SENSE_PIN)))
        {
            outp(0x43,0x00); /* latch counter 0 output */
            inp(0x40);       /* read CTC channel 0 (lb) */
            inp(0x61);       /* dummy instruction */
            if (inp(0x40) == overflow)	
            {
                if (waitflag)
                {
                    *bufferp++ = 0xff;     /* lb */
                    *bufferp++ = overflow; /* hb */
                    waitflag = 0;
                    outp(PELMASK, (flash++ << 4) | 0x0f);
                }
            }
            else waitflag = 1;
        }
        if (tapvers == 2) /* record halfwaves */
        {
            outp(0x43,0x00);        /* latch counter 0 output */
            lb = inp(0x40);         /* read CTC channel 0 */
            inp(0x61);              /* dummy instruction */
            hb = inp(0x40);         /* high byte */
            if ((hb == overflow) && waitflag)
            {
                *bufferp++ = 0xff;     /* lb */
                *bufferp++ = overflow; /* hb */
                waitflag = 0;
                lb = 0xfe;
            }
            *bufferp++ = lb;
            *bufferp++ = hb;
            overflow = hb;
            waitflag = 0;
            outp(PELMASK, (flash++ << 4) | 0x0f);
        }

        /* wait for 1 -> 0 transition */
        while ((inp(STAT_PORT) & (READ_PIN | SENSE_PIN)) == READ_PIN)
        {
            outp(0x43,0x00); /* latch counter 0 output */
            inp(0x40);       /* read CTC channel 0 */
            inp(0x61);       /* dummy instruction */
            if (inp(0x40) == overflow)	
            {
                if (waitflag)
                {
                    *bufferp++ = 0xff;
                    *bufferp++ = overflow;
                    waitflag = 0;
                    outp(PELMASK, (flash++ << 4) | 0x0f);
                }
            }
            else waitflag = 1;
        }
        outp(0x43,0x00);        /* latch counter 0 output */
        lb = inp(0x40);         /* read CTC channel 0 */
        inp(0x61);              /* dummy instruction */
        hb = inp(0x40);         /* high byte */
        if ((hb == overflow) && waitflag)
        {
            *bufferp++ = 0xff;     /* lb */
            *bufferp++ = overflow; /* hb */
            waitflag = 0;
            lb = 0xfe;
        }
        *bufferp++ = lb;
        *bufferp++ = hb;
        overflow = hb;
        waitflag = 0;
        outp(PELMASK, (flash++ << 4) | 0x0f);
    } while (!(inp(STAT_PORT) & SENSE_PIN));

    enable();
    return bufferp;
}


unsigned char *record_x1541(int port, unsigned char *bufferp)
{
	int laststate;
	int waitflag;
	int overflow;
	int flash; /* border color */
	int outputvalue;

	printf("Recording from X1541 interface\n");

	/* prepare X1541 interface for reading */
	outp(CTRL_PORT, 0x04);

	if (inp(CTRL_PORT) & X_ATN)
		printf("Please <STOP> the tape.\n");
	while (inp(CTRL_PORT) & X_ATN);

	printf("Press <PLAY> on tape!\n");
	while (!(inp(CTRL_PORT) & X_ATN));

	printf("Tape now running, recording...\n");

	delay(500);	/* delay for half a second, so signal settles */

	outp(0x43,0x34);
	outp(0x40,0);	// rate = 65536
	outp(0x40,0);

	disable();

	overflow = 0;
	waitflag = 0;
	flash = 0;

	laststate= (inp(CTRL_PORT) & (X_CLK | X_ATN));

	do
	{
		while ((inp(CTRL_PORT) & (X_CLK | X_ATN)) == laststate)
		{
			outp(0x43,0x00);	// latch counter 0 output
			inp(0x40);		// read CTC channel 0
			inp(0x61); // dummy instruction
			if (inp(0x40) == overflow)	
			{
				if (waitflag)
				{
					*bufferp++ = 0xff;
					*bufferp++ = overflow;
					waitflag = 0;
					outp(PELMASK, (flash++ << 4) | 0x0f);
				}
			}
			else waitflag = 1;
		}
		outp(0x43,0x00);	// latch counter 0 output
		*bufferp++ = inp(0x40);		// read CTC channel 0
		inp(0x61); // dummy instruction
		*bufferp = inp(0x40);
		overflow = *bufferp++;
		waitflag = 0;
		outp(PELMASK, (flash++ << 4) | 0x0f);
		laststate ^= X_CLK;
	} while (inp(CTRL_PORT) & X_ATN);

	enable();
	return bufferp;
}


unsigned char *record_xe1541(int port, unsigned char *bufferp)
{
    int laststate;
    int waitflag;
    int overflow;
    int flash; /* border color */
    int outputvalue;

    if (cable == XE1541)
    {
        printf("Recording from XE1541 interface\n");
        /* prepare XE1541 interface for reading */
        outp(CTRL_PORT, 0x04);
    }
    else /* cable == XA1541 */
    {
        printf("Recording from XA1541 interface\n");
        /* prepare XA1541 interface for reading */
        outp(CTRL_PORT, 0x0b);
    }

    if (!(inp(STAT_PORT) & XE_ATN))
        printf("Please <STOP> the tape.\n");
    while (!(inp(STAT_PORT) & XE_ATN));

    printf("Press <PLAY> on tape!\n");
    while (inp(STAT_PORT) & XE_ATN);

    printf("Tape now running, recording...\n");

    delay(500);	/* delay for half a second, so signal settles */

    outp(0x43,0x34);
    outp(0x40,0);	// rate = 65536
    outp(0x40,0);

    disable();

    overflow = 0;
    waitflag = 0;
    flash = 0;

    laststate= (inp(STAT_PORT) & (XE_CLK | XE_ATN));

    do
    {
        while ((inp(STAT_PORT) & (XE_CLK | XE_ATN)) == laststate)
        {
            outp(0x43,0x00);	// latch counter 0 output
            inp(0x40);		// read CTC channel 0
            inp(0x61); // dummy instruction
            if (inp(0x40) == overflow)	
            {
                if (waitflag)
                {
                    *bufferp++ = 0xff;
                    *bufferp++ = overflow;
                    waitflag = 0;
                    outp(PELMASK, (flash++ << 4) | 0x0f);
                }
            }
            else waitflag = 1;
        }
        outp(0x43,0x00);	// latch counter 0 output
        *bufferp++ = inp(0x40);		// read CTC channel 0
        inp(0x61); // dummy instruction
        *bufferp = inp(0x40);
        overflow = *bufferp++;
        waitflag = 0;
        outp(PELMASK, (flash++ << 4) | 0x0f);
        laststate ^= XE_CLK;
    } while (!(inp(STAT_PORT) & XE_ATN));

    enable();
    return bufferp;
}


void write_longpulse(long long int longpulse, FILE *fpout)
{
    long long int zerot;
    int i;
    int count;

    if (tapvers == 0)
    {
        for (zerot = longpulse; zerot > 0; zerot -= ZERO)
            fputc(0, fpout);
    }
    else if (tapvers == 1)
    {
        longpulse = (longpulse << 3); /* Version 1 longpulses *not* times 8 */
        while (longpulse != 0)
        {
            if (longpulse >= 0x1000000)
                zerot = 0xffffff;    /* zerot =  maximum */
            else
                zerot = longpulse;	
            longpulse -= zerot;

            fputc(0, fpout);
            for (i=0; i < 3; i++)
            {
                fputc(zerot % 256, fpout);
                zerot /= 256;
            }
        }
    }
    else if (tapvers == 2)  /* halfwaves - make even number of pulses */
    {
        longpulse = (longpulse << 3); /* Version 1 longpulses *not* times 8 */
        count = 0;
        while (longpulse != 0)
        {
            count++;
            if (longpulse >= 0x1000000)
                zerot = 0xffffff;    /* zerot =  maximum */
            else if (count & 1)
                zerot = longpulse; /* odd number of pulses */
            else
                zerot = longpulse/2; /* even number of pulses, split last */
            longpulse -= zerot;

            fputc(0, fpout);
            for (i=0; i < 3; i++)
            {
                fputc(zerot % 256, fpout);
                zerot /= 256;
            }
        }
    }
}


void write_raw_samples(char *outname, unsigned char *buffer, long long int len)
{
    char rawname[100];
    FILE *rawout;

    strcpy(rawname, outname);
    set_file_extension(rawname, ".RAW");

    if ((rawout = fopen(rawname, "wb")) == NULL)
    {
        fprintf(stderr, "Couldn't open raw sample file %s!\n", rawname);
        return;
    }

    if (fwrite(buffer, 1, len, rawout) != len)
    {
        fprintf(stderr, "Error writing raw samples!\n");
    }

    fclose(rawout);
}


int main(int argc, char **argv)
{
    FILE *fpout;
    long int pulsbytes, databytes;
    unsigned long int time, lasttime, deltatime;
    long long int newpulse;
    long long int longpulse;
    unsigned long int frequency;
    unsigned char *buffer, *bufferp;
    char outname[100];
    char input[100];
    int i;
    int lptnum;
    int port;
    int machine;
    int video_standard;
    int write_raw;

    printf("\nmtap - Commodore TAP file Generator v%d.%02d\n\n", VERSION_MAJOR, VERSION_MINOR);

    lptnum = 1;
    cable = ADAPTER;
    tapvers = 1;
    frequency = C64PALFREQ;
    machine = C64;
    video_standard = PAL;
    write_raw = 0;
    while (--argc && (*(++argv)[0] == '-'))
    {
        switch (tolower((*argv)[1]))
        {
            case 'b':
                buffersize = atoi(*(++argv));
                argc--;
                if (buffersize < 1 || buffersize > 128)
                {
                    fprintf(stderr, "Illegal Buffersize spezified!\n");
                    exit(3);
                }
                buffersize *= 0x100000;
                break;
            case 'c':
                if (argcmp(*argv,"-c64pal"))
                {
                    printf("recording from C64 PAL tape\n");
                    frequency = C64PALFREQ;
                    machine = C64;
                    video_standard = PAL;
                }
                else if (argcmp(*argv,"-c64ntsc"))
                {
                    printf("recording from C64 NTSC tape\n");
                    frequency = C64NTSCFREQ;
                    machine = C64;
                    video_standard = NTSC;
                }
                else if (argcmp(*argv,"-c16pal"))
                {
                    printf("recording from C16 PAL tape\n");
                    frequency = C16PALFREQ;
                    machine = C16;
                    video_standard = PAL;
                }
                else if (argcmp(*argv,"-c16ntsc"))
                {
                    printf("recording from C16 NTSC tape\n");
                    frequency = C16NTSCFREQ;
                    machine = C16;
                    video_standard = NTSC;
                }
                break;
            case 'h':
                tapvers = 2; /* enable halfwave recording */
                break;
            case 'l':
                lptnum = atoi(*argv+4);
                printf("lptnum: %d\n", lptnum);
                if ((lptnum < 1) || (lptnum > 4)) usage();
                break;
            case 'r':
                write_raw = 1; /* write raw sample data to *.RAW file */
                break;
            case 'v':
                if (argcmp(*argv,"-vicpal"))
                {
                    printf("recording from VIC-20 PAL tape\n");
                    frequency = VICPALFREQ;
                    machine = VIC;
                    video_standard = PAL;
                }
                else if (argcmp(*argv,"-vicntsc"))
                {
                    printf("recording from VIC-20 NTSC tape\n");
                    frequency = VICNTSCFREQ;
                    machine = VIC;
                    video_standard = NTSC;
                }
                else if ((*argv)[2] == '\0')
                    tapvers = 0;
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
    if (port == 0)
    {
        fprintf(stderr, "LPT%d doesn't exist on your machine!\n", lptnum);
        exit(1);
    }
    else printf("Using port LPT%d at %04x\n",lptnum,port);

    strcpy(outname, argv[0]);
    set_file_extension(outname, ".TAP");


    /* check, if the file exists */
    if ((fpout = fopen(outname,"rb")) != NULL)
    {
        fclose(fpout);
        fprintf(stderr, "TAP file %s already exists!\n", outname);
        fprintf(stderr, "Overwrite? (Y/N) ");
        gets(input);
        if (toupper(input[0]) != 'Y') exit(0);
    }

    if ((fpout = fopen(outname,"wb")) == NULL)
    {
        fprintf(stderr, "Couldn't open output file %s!\n", outname);
        exit(2);
    }

    if ((buffer = calloc(buffersize, sizeof(char))) == NULL)
    {
        fprintf(stderr, "Couldn't allocate buffer memory!\n");
        exit(3);
    }

    init_border_colors();

    /* do the actual recording */
    if (cable == ADAPTER)
        bufferp = record_adapter(port, buffer);
    else if (cable == X1541)
        bufferp = record_x1541(port, buffer);
    else /* (cable == XE1541) || (cable == XA1541) */
        bufferp = record_xe1541(port, buffer);
	

    set_border_black();
    outp(PELMASK, 0xff);

    // first pulselen is second value, discard <STOP> pulse
    pulsbytes = (bufferp-buffer)/2 - 2;
	
    if (pulsbytes < 1)
    {
        fprintf(stderr, "No pulses recorded\n");
        exit(4);
    }
    if (write_raw)
        write_raw_samples(outname, buffer, bufferp-buffer);

    printf("pulses recorded = %x\n", pulsbytes);
    if (machine == C16)
        fprintf(fpout,"C16-TAPE-RAW");
    else
        fprintf(fpout,"C64-TAPE-RAW");
    fprintf(fpout,"%c%c%c%c",tapvers,machine,video_standard,0);
    fprintf(fpout,"%c%c%c%c",0,0,0,0);	/* filelength */


    bufferp = buffer;
    longpulse = 0;
    lasttime = *bufferp++;
    lasttime += (*bufferp++)*256;

    for (i = 1; i <= pulsbytes; i++)
    {
        time = *bufferp++;
        time += (*bufferp++)*256;

        if (lasttime <= time)
            lasttime += 0x10000;
        deltatime = (lasttime-time);

	/* calculate pulse length */
        newpulse =
            (((long long int)deltatime) * frequency + BASEFREQ/2) / BASEFREQ;

        /* assert minimal pulse length of 1 */
        if (newpulse == 0) newpulse = 1;

        if (newpulse < 0x100)
        {
            if (longpulse) 
            {
                longpulse += newpulse;
                write_longpulse(longpulse, fpout);
                longpulse = 0;
            }
            else
            {
                fputc((newpulse & 0xff), fpout);
            }
        }
        else if (deltatime < 0xff00)
        {
            longpulse += newpulse;
            write_longpulse(longpulse, fpout);
            longpulse = 0;
        }
        else
        {
            longpulse += newpulse;
        }
        lasttime = time;
    }
    /* write trailing Zeroes, if any */
    if (longpulse > 0)
        write_longpulse(longpulse, fpout);


    /* determine data length and set it in header */
    databytes = ftell(fpout) - 20;
    fseek(fpout, 16, SEEK_SET);

    for (i=0; i < 4; i++)
    {
        fputc(databytes % 256, fpout);
        databytes /= 256;
    }

    fclose(fpout);
    free(buffer);
}
