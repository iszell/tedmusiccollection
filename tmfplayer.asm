// TED Music File header
// =====================
.label	filestart	=	$1001
// Magic marker for TED Music
.label	signature	=	$1010
// Music data specification version
.label	version		=	signature + 8
// File offset of real music data (handle load address)
.label	dataoffset	=	version + 1
.label	dataaddress	=	dataoffset + 2
.label	initaddress	=	dataaddress + 2
.label	playaddress	=	initaddress + 2
.label	numberofsongs	=	playaddress + 2
.label	timing		=	numberofsongs + 1
.label	timer1value	=	timing + 1
.label	songflags	=	timer1value + 2
.label	fileflags	=	songflags + 1
.label	durationoffset	=	fileflags + 1
.label	title		=	signature + 48
.label	author		=	title + 32
.label	released	=	author + 32
.label	tracker		=	released + 32

	* = $1100

.label	srcptr	=	2
.label	dstptr	=	4
.label	screen	=	$0c00

	.encoding	"petscii_mixed"


	jsr $ff81	// VIDINIT
	jsr $ff84	// IOINIT
	lda #$ff	// hide cursor
	sta $ff0c
	sta $ff0d

	jsr	initscreen

	jsr	relocatedata

	sei
	lda	#<irq
	ldx	#>irq
	sta	$0314
	stx	$0315
	lda	#%00001000
	sta	$ff0a
	sta	$ff09
	jsr inittimer
	
	lda	#0
	sta	currentsong

	sta $ff3f
	jsr	init
	sta $ff3e

	cli
	
	jmp	*

init:	jmp	(initaddress)
play:	jmp	(playaddress)

irq:
	inc	$ff19
	sta	$ff3f
	jsr	play
	sta	$ff3e
	dec	$ff19
	lda	$ff09
	sta	$ff09
	pla
	tay
	pla
	tax
	pla
	rti

// //TODO: separate routine for movement _before_ dataoffset and after
// //TODO: relocate player code as well
// //TODO: allow moving to $1000
relocatedata:
	clc
	lda	dataoffset
	adc	#<(filestart-2)
	sta	srcptr
	lda	dataoffset+1
	adc	#>(filestart-2)
	sta	srcptr+1
	lda	dataaddress
	ldx	dataaddress+1
	sta	dstptr
	stx	dstptr+1
	ldy	#0
!:	lda	(srcptr),y
	sta	(dstptr),y
	iny
	bne	!-
	inc	srcptr+1
	inc	dstptr+1
	lda	$9e
	cmp	srcptr+1
	bcs	!-
	rts

initscreen:
	jsr	$ff4f
	.byte	$93, $0e, $08
	.text	"TED Music Collection standalone player"
	.byte	13, 13, 13, 13, 0
	lda	#$41
	sta	$053b
	jsr	$ff4f
	.text	"by"
	.byte	13, 13
	.text	"released"
	.byte	13, 13
	.text	"made with"
	.byte	13, 13, 13
	.text	"Features:"
	.byte	13, 13, 13
	.text	"Timing: "
	.byte	13, 13
	.text	"Song:"
	.byte	0

	ldx	#31
!:	lda	title,x
	jsr	convertpetscii
	sta	screen+3*40,x
	lda	author,x
	jsr	convertpetscii
	sta	screen+5*40,x
	lda	released,x
	jsr	convertpetscii
	sta	screen+7*40,x
	lda	tracker,x
	jsr	convertpetscii
	sta	screen+9*40,x
	dex
	bpl	!-

	ldy	#0
	ldx	#8
	lda	songflags
!:	lsr
	pha
	lda	#$00
	bcs	!+
	lda	#$61
!:	sta	$053b
!:	lda	flagtext,y
	beq	!+
	sta	screen+12*40,y
	lda	$053b
	sta	screen+12*40-$0400,y
	iny
	bne	!-
	
!:	iny
	pla
	dex
	bne	!----

	lda	timing
	asl
	asl
	sta	timing0
	asl
.label timing0	=	*+1
	adc	#0	// multiply by 10
	tay
	ldx	#0
!:	lda	timtab,y
	sta	screen+14*40+8,x
	iny
	inx
	cpx	#12
	bne	!-

	ldx	currentsong
	inx
	txa
	jsr	dectoasc
	ldx	#2
!:	lda	decstr,x
	sta	screen+16*40+6,x
	dex
	bpl	!-

	lda	#'/'
	sta	screen+16*40+9
	
	lda	numberofsongs
	jsr	dectoasc
	ldx	#2
!:	lda	decstr,x
	sta	screen+16*40+10,x
	dex
	bpl	!-
	
	ldx	#5
!:	.for	(var i=0; i<6; i++) {
	lda	logo+i*6,x
	sta	screen+(2+i)*40+34,x
	lda	#$6e
	sta	screen+(2+i)*40+34-$400,x
	}
	dex
	bpl	!-

!:	rts

convertpetscii:
	cmp	#$40
	bcc	!-
	cmp	#$80
	bcs	!+
	and	#$1f
	rts
!:	and	#$7f
	rts

dectoasc:
	ldx	#'0'
	stx	decstr
	stx	decstr+1
	
!:	cmp	#100
	bcc	!+
	inc	decstr
	sbc	#100
	bcs	!-
!:
!:	cmp	#10
	bcc	!+
	inc	decstr+1
	sbc	#10
	bcs	!-
!:	
	ora	#'0'
	sta	decstr+2
	rts

inittimer: {
	lda	$ff07
	and	#%01000000
	bne inittimerntsc

	ldy timing
	bne !+
// PAL VBlank
	lda #<17734
	ldx #>17734
	bne writetimer
!:  dey
	bne !+
// NTSC VBlank
	lda #<14779
	ldx #>14779
	bne writetimer
!:	dey
	bne !+
//  PAL timer
	lda	timer1value
	ldx	timer1value+1
	jmp writetimer
!:	dey
	bne !+
// NTSC timer (TODO! convert timer)
	beq inittimerend
!:	dey
	bne !+
// PAL timer by songflags
	beq inittimerend
!:	dey
	bne !+
// NTSC timer by songflags
	beq inittimerend
!:	dey
	bne !+
// PAL VBlank 2x
	lda #<8867
	ldx #>8867
	bne writetimer
!:  dey
	bne inittimerend
// NTSC VBlank 2x
	lda #<7389
	ldx #>7389
	bne writetimer
writetimer:
	sta	$ff00
	stx	$ff01
inittimerend:
	rts

inittimerntsc:
	ldy timing
	bne !+
// PAL VBlank
	lda #<17898
	ldx #>17898
	bne writetimer
!:  dey
	bne !+
// NTSC VBlank
	lda #<14915
	ldx #>14915
	bne writetimer
!:	dey
	bne !+
//  PAL timer
	lda	timer1value
	ldx	timer1value+1
	jmp writetimer
!:	dey
	bne !+
// NTSC timer (TODO! convert timer)
	beq inittimerend
!:	dey
	bne !+
// PAL timer by songflags
	beq inittimerend
!:	dey
	bne !+
// NTSC timer by songflags
	beq inittimerend
!:	dey
	bne !+
// PAL VBlank 2x
	lda #<8949
	ldx #>8949
	bne writetimer
!:  dey
	bne inittimerend
// NTSC VBlank 2x
	lda #<7457
	ldx #>7457
	bne writetimer
}

// Playback helper tables
// ----------------------
// Timer increase frequency (adjusted to current screen refresh)
timerjiffytab:
	.byte	50, 60 // PAL, NTSC

// ============================
// plus/4
//  PAL  timer clock = 886724
//  NTSC timer clock = 894886
// C64
//  PAL  timer clock = 985248
//  NTSC timer clock = 1022727
// ============================
// VBlank to timer value converter table ( key = current std * 2 + song std ) 
vblanktab:
	.word	17734, 14779	// PAL VBlank@PAL, NTSC VBLank@PAL
	.word	17898, 14915	// PAL VBlank@NTSC, NTSC VBlank@NTSC
	
// Conversion table for PAL/NTSC TED timer frequency conversion; 0 means no conversion neccessary (1 whole and 7 fraction bits)
// eight binary fraction digits (number/256)
tedtimerconvertertab:
	.byte	128, 127		// PAL@PAL=1; NTSC@PAL=0.991
	.byte	129, 128		// PAL@NTSC=1,009; NTSC@NTSC=1

decstr:
	.text	"000"
currentsong:
	.byte	0

logo:
	.byte	$ec, $e2, $e2, $7e, $ec, $e2
	.byte	$61, $fc, $6c, $61, $61, $20
	.byte	$61, $61, $7e, $61, $ec, $20
	.byte	$61, $61, $20, $61, $61, $20
	.byte	$61, $61, $20, $61, $61, $20
	.byte	$61, $61, $16, $31, $2e, $30

	.encoding	"screencode_mixed"

flagtext:
	.text	"Screen"
	.byte	$64
	.text	"off"
	.byte	0
	.text	"SID"
	.byte	0
	.text	"DigiBlaster"
	.byte	0
	.text	"AY"
	.byte	0
	.text	"FM"
	.byte	0
	.text	"C64"
	.byte	$64
	.text	"SID"
	.byte	0, 0, 0
		//0123456789a
timtab:	.text	"PAL VBlank  "
	.text	"NTSC VBlank "
	.text	"PAL Timer   "
	.text	"NTSC Timer  "
	.text	"PAL Timer   "
	.text	"NTSC Timer  "
	.text	"PAL 2X      "
	.text	"NTSC 2X     "
