# TED Music Collection
## What's this?
TED Music Collection is an initiative to create a standard music data format with metadata for the Commodore 264 series computers.
## Why?
Because we wanted to have a collection which can be played back using a generic music player on the real machine.
## But we already have HVTC!
HVTC is missing several key features we think is very important:
* it contains every music in two flavors: a standalone player which can be started from BASIC (which is good) and the raw player routine and data from the original program (which in fact cannot be used for anything)
* music playback from HVTC is a real pain in the ass on the real machine: you extract the collection to your trusted SD2IEC, browse to the folder, load the music, play it then reset the computer which resets SD2IEC to the root folder so you can start all over
* it does not have metadata for the music such as author, title, release date, music playback frequency, init and play entry addresses and so on which prevents playing back from a standardized player
* TED Music Collection is the natural evolution of HVTC and we wanted a new name to reflect the fundamental differences. (High Voltage is the name of a C64 demo group who started collecting C64 music and they are not related to the plus/4 scene in any way)
## What are the key features of TED Music Collection? 
TED Music Collection (TMC in short) is
* aimed to be played back from a music player
* but we keep what's good in HVTC: the standalone player: you can load and play a music straight from BASIC
* has metadata fields to display information about the music
# Creating TED Music Files
To create a TED Music File you need to

* Extract music data and player code from the original program
* Patch the player code (or create a wrapper) that allows you to
  * init the music with the number of song passed in register A
  * does the playback by a single call to the play routine
* Create the file using the TMF Tool
# TMFTool description
# TED Music File description
## General information
* files are standard Commodore .prg files with load address of $1001 (Commodore 264 series BASIC startup)
* for BASIC RUN command they start the standalone player
* word type fields are little endian: just like on a Commodore 6502 based computer
* text fields are encoded in US PETSCII lower/upper case character set
* reserved areas should be filled with zeros to allow backwards compatible changes in the specifications
* in flag fields 0 means the flag is off and 1 means on. all not specified bits should be set to 0.
## File layout
|Offset (decimal)|Offset (hexadecimal)|Address when loaded|Type|Length (decimal)|Name|Mandatory|Value|Remarks|
|-|-|-|-|-|-|-|-|-|
|0|0|none|word|2|loadaddress|yes|$1001|not checked|
|2|2|$1001|word|2|basicnextline|yes|$100b|not checked|
|4|4|$1003|word|2|basiclinenumber|yes|2020|not checked|
|6|6|$1005|byte|1|basicsys|yes|$9e|not checked|
|7|7|$1006|text|4|basicsysparam|yes|4352|not checked|
|11|B|$100A|byte|1|basiclineend|yes|0|not checked|
|12|C|$100B|word|2|basicend|yes|$0000|not checked|
|14|E|$100D|byte|3||yes||filler|
|17|11|$1010|text|8|signature|yes|$54 $45 $44 $4d $55 $53 $49 $43|„tedmusic”|
|25|19|$1018|byte|1|version|yes|0|0: Preliminary specification version|
|26|1A|$1019|word|2|dataoffset|yes||The offset where song data starts from the beginning of the file (load address included)|
|28|1C|$101B|word|2|dataaddress|yes||This is the memory address where the real music data should be loaded (in case of BASIC load this is the address where the music should be relocated to)|
|30|1E|$101D|word|2|initaddress|yes||The address of the routine which can be called to start a song. The number of the song is passed in register A.<br/>The init must not change interrupt enable ($ff0a) register and interrupt vectors (neither $0314 nor $fffe)|
|32|20|$101F|word|2|playaddress|yes||The address of the routine which can be called to play the song. CPU I flag should not be manipulated and interrupt request ($ff09) must not be acknowledged|
|34|22|$1021|byte|1|numberofsongs|yes|||
|35|23|$1022|byte|1|timing|yes|<ul><li>0: PAL VBlank (50Hz),</li><li>1: NTSC VBlank (60Hz),</li><li>2: PAL timer1 value in next field,</li><li>3: NTSC timer1 value in next field,</li><li>4: PAL timer1 value set by init routine,</li><li>5: NTSC timer1 value set by init routine</li><li>6: PAL 2x</li><li>7: NTSC 2x</li></ul>|Value 4 and 5 should be avoided whenever possible because it prevents playing back PAL music on NTSC machines and vice versa.|
|36|24|$1023|word|2|timer1value|no||<p>The value put in TED timer#1</p> <p>Special timer values:</p> <ul> <li>PAL 1/frame (VBlank, 1x): 17784</li> <li>PAL 2/frame (2x): 8892</li> <li>PAL 4/frame (4x): 4446</li> <li>NTSC 1/frame: 14934</li> <li>NTSC 2/frame: 7467</li> <li>NTSC 4/frame: 3734</li> </ul>|
|38|26|$1025|byte|1|songflags|yes|<ul> <li>Bit 0: screen turned off during playback</li> <li>Bit 1: uses SID</li> <li>Bit 2: uses DigiBlaster</li> <li>Bit 3: uses AY (DIGIMUZ)</li> <li>Bit 4: uses FM</li><li>Bit 5: SID is used at C64 frequency</li></ul>|Screen can be turned of during playback but the song still should handle interrupts gracefully.<br/>Music should indicate the used additional sound sources in order to allow music players to mute them after playback|
|39|27|$1026|byte|1|fileflags|yes|<ul> <li>Bit 0: file has song duration block</li> </ul>||
|40|28|$1027|word|2|durationoffset|no||Offset of duration block if present|
|42|2A|$1029|byte|23|reserved|no||Reserved for future use (padding to align text field to addresses dividable by $10)|
|65|41|$1040|text|32|title|yes|||
|97|61|$1060|text|32|author|no|||
|129|81|$1080|text|32|released|no|||
|161|A1|$10A0|text|32|tracker|no||Name of the tracker/composer used to create song|
|193|C1|$10C0|byte|64||yes||Reserved for future use|
|257|101|$1100|||player|yes||A built-in player (and optional relocator) routine that allows playing the music on a 264 series machine after a BASIC DLOAD|
||durationoffset||word|2*numberofsongs|durationblock|no||An optional array of words for every song in the file. The durations are stored in seconds and rounded up (for example 1 minute 3.2 seconds should be stored as 64)|
||dataoffset||||data|yes||The music data|
