#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Return codes
#define SUCCESS			0
#define NOPARAM			1
#define UNKNOWN_OPTION		2
#define NOT_A_TMF		3
#define READERROR		10
#define WRONGLENGTH		11
#define WRITEERROR		20

// Modes
#define CREATE	0
#define EXTRACT	1
#define INFO	2

// Specification version
#define SPECVERSION	1
// Timing constants
#define PALVBLANK	0
#define NTSCVBLANK	1
#define PALTIMER	2
#define NTSCTIMER	3
#define PALCUSTOMTIMER	4
#define NTSCCUSTOMTIMER	5
// Song flags
#define SCREENOFF	1
#define SID		2
#define DIGIBLASTER	4
#define AY		8
#define FM		16
// File flags
#define DURATIONBLOCK	1

#include "defaulttmfplayer.h"

unsigned char *inputbuffer;
unsigned char *outputbuffer;
unsigned char *playercode = tmfplayer_prg;

int mode = CREATE;
int verbose = 0;

char *inputname = NULL;
char *outputname = NULL;
char *playername = NULL;
int inputlength;
int outputptr = 0;

int playerlength = sizeof(tmfplayer_prg)/sizeof(char);

int init = 0;
int play = 0;
int songs = 1;
int timing = PALVBLANK;
int timer = 0;
int songflags = 0;
int fileflags = 0;
int durations[255];

char *title = "";
char *author = "";
char *released = "";
char *tracker = "";

int fileheader[] = {
	0x01, 0x10, 0x0b, 0x10, 2022 & 0xff, 2022 >> 8, 0x9e, '4', '3', '5', '2', 0, 0, 0, 0, 0, 0,
	'T', 'E', 'D', 'M', 'U', 'S', 'I', 'C'
	};

void usage(char *name) {
	printf("%s 1.1 by Siz of Absence\n\n", name);
	printf("Usage:\n");
	printf("%s [<command>] (default: create)\n"
		"\t<options>\n"
		"\t<input file name> [<output fileaname>]\n\n", name);
	printf("%s [create] \n"
		"\t[--verbose]\n"
		"\t--init <initaddress>\n"
		"\t--play <playaddress>\n"
		"\t[--songs <number of songs>] (default: 1)\n"
		"\t[--timing palvblank|ntscvblank|paltimer|ntsctimer|palcustomtimer|ntsccustomtimer] (default: palvblank)\n"
		"\t[--timer <timer value for --timing paltimer and ntsctimer>]\n"
		"\t[--songflags <flags>] (flags is a space separated list of screenoff, sid, digiblaster, ay, fm)\n"
		"\t[--durations <duration in seconds for every song separated by spaces>\n"
		"\t--title <song title>\n"
		"\t[--author <song author>]\n"
		"\t[--released <release information>]\n"
		"\t[--tracker <tracker information>]\n"
		"\t[--player <file name of custom player routine>]\n"
		"\t<input name> <output name>\n", name);
	printf("(You can also use these convenience timing values: pal1x|pal2x|pal4x|ntsc1x|ntsc2x|ntsc4x)\n");
	printf("Example:\n"
		"%s --init 0x2f01 --play 0x2f25 --timing paltimer --timer 8867 --title \"Exorcist\" --author \"David Whittaker\" --released \"1984 Mr. Micro Ltd.\" exorcist.prg exorcist.tmf.prg\n", name);
	printf("\n%s extract \n"
		"\t[--verbose]\n"
		"\t<input name> <output name>\n", name);
	printf("Example:\n"
		"%s extract exorcist.tmf.prg exorcist.prg\n", name);
	printf("\n%s info \n"
		"\t<input name>\n", name);
	printf("Example:\n"
		"%s info exorcist.tmf.prg\n", name);
}

int readinput() {
	FILE *f = fopen(inputname, "rb");
	if(f == NULL) {
		printf("Can't read file %s\n", inputname);
		return READERROR;
	}
	inputbuffer = malloc(65536);
	inputlength = fread(inputbuffer, sizeof(unsigned char), 65536, f);
	if(verbose) {
		printf("Read %d bytes from input file %s\n", inputlength, inputname);
	}
	fclose(f);
	return SUCCESS;
}

int readplayer() {
	FILE *f = fopen(playername, "rb");
	if(f == NULL) {
		printf("Can't read file %s\n", playername);
		return READERROR;
	}
	playercode = malloc(65536);
	playerlength = fread(playercode, sizeof(unsigned char), 65536, f);
	if(verbose) {
		printf("Read %d bytes from input file %s\n", playerlength, playername);
	}
	fclose(f);
	return SUCCESS;
}

void writeoutb(int b) {
	outputbuffer[outputptr++] = b;
}

void writeoutw(int w) {
	writeoutb(w & 0xff);
	writeoutb(w >> 8);
}

void writeouts(char *str, int len) {
	int i, c;
	for( i = 0; i<len; i++) {
		if(strlen(str)<i+1) {
			writeoutb(' ');
		} else {
			c = str[i];
			if('a'<= c && c <= 'z') writeoutb(c-32);
			else if('A'<= c && c <= 'Z') writeoutb(c+128);
			else(writeoutb(c));
		}
	}
}

int writeoutput() {
	FILE *f = fopen(outputname, "wb");
	if(f == NULL) {
		printf("Can't write file %s\n", outputname);
		return WRITEERROR;
	}
	int writelength = fwrite(outputbuffer, sizeof(unsigned char), outputptr, f);
	fclose(f);
	if(verbose) {
		printf("Written %d bytes to output file %s\n", writelength, outputname);
	}
	if(writelength < outputptr) {
		printf("Bad number of bytes written (%d)\n", writelength);
		return WRONGLENGTH;
	}
	return SUCCESS;
}

int checktmf() {
	if(inputbuffer[0] != fileheader[0] || inputbuffer[1] != fileheader[1]) {
		return NOT_A_TMF;
	}
	int i;
	// Start of TMF header on a plus/4 - basic start + load address size
	for(i = 0x1010 - 0x1001 + 2; i < 0x1010 - 0x1001 + 2 + 8; i++) {
		if(inputbuffer[i] != fileheader[i]) {
			return NOT_A_TMF;
		}
	}
	return SUCCESS;
}

int getword(int address) {
	return (inputbuffer[address+1] << 8)+inputbuffer[address];
}

char * getstring(int address) {
	char *value = malloc(33);
	int i;
	for(i = 0; i < 32; i++) {
		int c = inputbuffer[address + i];
		if(c > 0xc0 && c < 0xe0) {
			c &= 0b01111111;
		} else if (c > 0x40 && c < 0x60) {
			c |= 0b00100000;
		}
		value[i] = c;
	}
	value[32] = 0;
	return value;
}

int create() {
	if(verbose) printf("Creating TED Music File\n");

	outputbuffer = malloc(65536);
	int i;
	for(i = 0; i < sizeof(fileheader)/sizeof(fileheader[0]); i++) {
		writeoutb(fileheader[i]);
	}
	// version
	writeoutb(SPECVERSION);
	if( verbose ) printf(". version: %d\n", SPECVERSION);
	// dataoffset
	int dataoffset = 0x0101 + playerlength - 2;
	if(fileflags & DURATIONBLOCK == 1) {
		dataoffset += songs*2;
	}
	writeoutw(dataoffset);
	if( verbose ) printf(". data offset: %d\n", dataoffset);
	// dataaddress
	writeoutb(inputbuffer[0]);
	writeoutb(inputbuffer[1]);
	if( verbose ) printf(". data address: %04x\n", inputbuffer[1]<<8+inputbuffer[0]);
	// init address
	writeoutw(init);
	if( verbose ) printf(". init address: %04x\n", init);
	// play address
	writeoutw(play);
	if( verbose ) printf(". play address: %04x\n", play);
	// number of songs
	writeoutb(songs);
	if( verbose ) printf(". number of songs: %d\n", songs);
	// timing
	writeoutb(timing);
	if( verbose ) printf(". timing: %d\n", timing);
	// timer value
	writeoutw(timer);
	if( verbose ) printf(". timer: %d\n", timer);
	// song flags
	writeoutb(songflags);
	if( verbose ) printf(". song flags: %d\n", songflags);
	// file flags
	writeoutb(fileflags);
	if( verbose ) printf(". file flags: %d\n", fileflags);
	// durtion offset
	int durationoffset = 0;
	if(fileflags & DURATIONBLOCK == 1) {
		durationoffset = 0x0101 + playerlength - 2;
	}
	writeoutw(durationoffset);
	if( verbose ) printf(". duration offset: %d\n", durationoffset);
	// reserved
	while(outputptr < 65) writeoutb(0);
	// title
	writeouts(title, 32);
	if( verbose ) printf(". title: %s\n", title);
	// author
	writeouts(author, 32);
	if( verbose ) printf(". author: %s\n", author);
	// released
	writeouts(released, 32);
	if( verbose ) printf(". released: %s\n", released);
	// tracker
	writeouts(tracker, 32);
	if( verbose ) printf(". tracker: %s\n", tracker);
	// reserved
	while(outputptr < 257) writeoutb(0);
	// copy player code
	for(i=2; i<playerlength; i++) {
		writeoutb(playercode[i]);
	}
	if( verbose ) printf(". player code block: %d bytes\n", playerlength - 2);
	// write duration block
	if(fileflags & DURATIONBLOCK == 1) {
		for(i=0; i<songs; i++) {
			writeoutw(durations[i]);
		}
		if( verbose ) printf(". duration block: %d bytes \n", songs*2);
	}
	// copy music data
	for(i=2; i<inputlength; i++) {
		writeoutb(inputbuffer[i]);
	}
	if( verbose ) printf(". music data block: %d bytes \n", inputlength - 2);
	return SUCCESS;
}

int extract() {
	if(verbose) printf("Extracting music data from TED Music File\n");
	if( checktmf() != SUCCESS) {
		return NOT_A_TMF;
	}

	outputbuffer = malloc(65536);

	int startpos = (inputbuffer[27] << 8)+inputbuffer[26];
	if(verbose) printf(". start offset: %04x\n", startpos);

	int startaddress = inputbuffer[29]*256+inputbuffer[28];
	writeoutw(startaddress);

	if(verbose) printf(". start address: %04x\n", startaddress);
	int i;

	for(i=startpos; i<inputlength; i++) {
		writeoutb(inputbuffer[i]);
	}

	if(verbose) printf(". data length: %d\n", inputlength - startpos);

	return SUCCESS;
}

int info() {
	printf("Extracting music info from TED Music File\n");
	if( checktmf() != SUCCESS) {
		return NOT_A_TMF;
	}

	int startpos = getword(26);
	printf(". data offset: \t\t%d\n", startpos);

	int startaddress = getword(28);

	printf(". data address:\t\t$%04x\n", startaddress);
	printf(". data length:\t\t%d\n", inputlength - startpos);
	printf(". init address:\t\t$%04x\n", getword(30));
	printf(". play address:\t\t$%04x\n", getword(32));
	printf(". number of songs:\t%d\n", inputbuffer[34]);
	printf(". timing:\t\t%d\n", inputbuffer[35]);
	printf(". timer 1 value:\t%d\n", getword(36));
	printf(". song flags:\t\t%02x\n", inputbuffer[38]);
	printf(". file flags:\t\t%02x\n", inputbuffer[39]);
	printf(". duration offset:\t%d\n", getword(40));
	printf(". title:\t\t%s\n", getstring(65));
	printf(". author:\t\t%s\n", getstring(97));
	printf(". released:\t\t%s\n", getstring(129));
	printf(". tracker:\t\t%s\n", getstring(161));

	return SUCCESS;
}

int main(int argc, char *argv[]) {
	if(argc==1) {
		usage(argv[0]);
		return NOPARAM;
	} else {
		int i;
		for(i=1; i<argc; i++) {
			if(strcmp("create", argv[i]) == 0) {
				mode = CREATE;
			} else if(strcmp("extract", argv[i]) == 0) {
				mode = EXTRACT;
			} else if(strcmp("info", argv[i]) == 0) {
				mode = INFO;
			} else if(strcmp("--verbose", argv[i]) == 0) {
				verbose = 1;
			} else if(strcmp("--init", argv[i]) == 0) {
				init = strtol(argv[++i], NULL, 16);
			} else if(strcmp("--play", argv[i]) == 0) {
				play = strtol(argv[++i], NULL, 16);
			} else if(strcmp("--songs", argv[i]) == 0) {
				songs = strtol(argv[++i], NULL, 16);
			} else if(strcmp("--timing", argv[i]) == 0) {
				if(strcmp("palvblank", argv[++i]) == 0) {
					timing = PALVBLANK;
				} else if(strcmp("ntscvblank", argv[i]) == 0) {
					timing = NTSCVBLANK;
				} else if(strcmp("paltimer", argv[i]) == 0) {
					timing = PALTIMER;
				} else if(strcmp("ntsctimer", argv[i]) == 0) {
					timing = NTSCTIMER;
				} else if(strcmp("palcustomtimer", argv[i]) == 0) {
					timing = PALCUSTOMTIMER;
				} else if(strcmp("ntsccustomtimer", argv[i]) == 0) {
					timing = NTSCCUSTOMTIMER;
				} else if(strcmp("pal1x", argv[i]) == 0) {
					timing = PALVBLANK;
				} else if(strcmp("pal2x", argv[i]) == 0) {
					timing = PALTIMER;
					timer = 8867;
				} else if(strcmp("pal4x", argv[i]) == 0) {
					timing = PALTIMER;
					timer = 4433;
				} else if(strcmp("ntsc1x", argv[i]) == 0) {
					timing = NTSCVBLANK;
				} else if(strcmp("ntsc2x", argv[i]) == 0) {
					timing = NTSCTIMER;
					timer = 7457;
				} else if(strcmp("ntsc4x", argv[i]) == 0) {
					timing = NTSCTIMER;
					timer = 3728;
				} else {
					printf("ERROR! Unknown timing value: %s.\n", argv[i]);
					return UNKNOWN_OPTION;
				}
			} else if(strcmp("--timer", argv[i]) == 0) {
				timer = atoi(argv[++i]);
			} else if(strcmp("--songflags", argv[i]) == 0) {
				int j;
				for(j = i+1 ; j<argc; j++) {
					if(argv[j][0] == '-' && argv[j][1] == '-') {
						break;
					}
					if(strcmp("screenoff", argv[j]) == 0) {
						songflags |= SCREENOFF;
						i++;
					} else if(strcmp("sid", argv[j]) == 0) {
						songflags |= SID;
						i++;
					} else if(strcmp("digiblaster", argv[j]) == 0) {
						songflags |= DIGIBLASTER;
						i++;
					} else if(strcmp("ay", argv[j]) == 0) {
						songflags |= AY;
						i++;
					} else if(strcmp("fm", argv[j]) == 0) {
						songflags |= FM;
						i++;
					}
				}
			} else if(strcmp("--durations", argv[i]) == 0) {
				fileflags |= DURATIONBLOCK;
				int j;
				for(j = 0 ; j<songs; j++) {
					durations[j] = atoi(argv[++i]);
				}
			} else if(strcmp("--title", argv[i]) == 0) {
				title = argv[++i];
			} else if(strcmp("--author", argv[i]) == 0) {
				author = argv[++i];
			} else if(strcmp("--released", argv[i]) == 0) {
				released = argv[++i];
			} else if(strcmp("--tracker", argv[i]) == 0) {
				tracker = argv[++i];
			} else if(strcmp("--player", argv[i]) == 0) {
				playername = argv[++i];
			} else if((i == argc - 2 && mode != INFO)||(i == argc - 1 && mode == INFO)) {
				inputname = argv[i];
			} else if(i == argc - 1 && mode != INFO) {
				outputname = argv[i];
			} else {
				printf("ERROR! Unknown option: %s.\n", argv[i]);
				return UNKNOWN_OPTION;
			}
		}
	}
	if(inputname == NULL || (outputname == NULL && mode != INFO) ||
		(mode == CREATE && ( init == 0 || play == 0 || songs == 0))) {
		usage(argv[0]);
		return UNKNOWN_OPTION;
	}

	if(verbose) {
		printf("mode: %d, verbose: %d, inputname=%s, outputname=%s\n", mode, verbose, inputname, outputname);
	}

	int result;
	result = readinput();
	if(result != SUCCESS) {
		printf("Error: %d\n", result);
	} else {
		if( mode == CREATE ) {
			if(playername != NULL) {
				result = readplayer();
			}
			if(result == SUCCESS) {
				result = create();
			}
		} else if( mode == EXTRACT ) {
			result = extract();
		} else if( mode == INFO ) {
			result = info();
		}
		if(result == SUCCESS) {
			if(mode != INFO) {
				result = writeoutput();
			}
		} else {
			printf("Error: %d\n", result);
		}
	}
	return result;
}
