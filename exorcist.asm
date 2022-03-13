; TED Music Collection example file: Exorcist
; ===========================================

; BASIC header for standalone player
; ----------------------------------
	* = $1001
	.enc	none
;	.word	*
filestart
	.word	nextline
	.word	2020
	.byte	$9e
	.text	"4352"
	.byte	0
nextline	.word	0
	.align	$1010

; TED Music File header
; =====================
; Magic marker for TED Music
signature	.text	"tedmusic"
; Music data specification version
version
	.byte	1
; File offset of real music data (handle load address)
dataoffset
	.word	musicdata-filestart+2
dataaddress
	.word	$2c00
initaddress
	.word	$2f01
playaddress
	.word	$2f25
numberofsongs
	.byte	1
timing	.byte	2
timer1value
	.word	8867
songflags	.byte	0
fileflags	.byte	0
durationoffset
	.word	0
	.align	32
title	.text	"Exorcist"
	.align	32, " "
author	.text	"David Whittaker"
	.align	32, " "
released
	.text	"1984 Mr. Micro Ltd."
	.align	32, " "
tracker
	.text	" "
	.align	32, " "
	.align	$1100

	.binary	"tmfplayer.prg", 2

; Real music data
; ===============
musicdata	
	.binary	"exorcist_original.prg",2,$2f18-$2c00
	.byte $d0, $15
	.binary	"exorcist_original.prg",2+$2f18-$2c00+2, $2f32-$2f1a
	rts
	.binary	"exorcist_original.prg",2+$2f32-$2c00+1, $2f51-$2f33
	rts
	.binary	"exorcist_original.prg",2+$2f51-$2c00+1, $2fac-$2f52
	rts
	.binary	"exorcist_original.prg",2+$2fac-$2c00+1
