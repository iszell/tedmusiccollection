	#importonce
	.macro	loadtmf(filename) {
		.print	"Loading TMF "+filename
		.const	TMF_TEMPLATE = "DataOffset=26, DataAddress=28, InitAddress=30, PlayAddress=32"
		.var music = LoadBinary(filename, TMF_TEMPLATE)
		.label musicDataOffset  = (music.getDataOffset(1) &$ff)*256+(music.getDataOffset(0) &$ff)
		.label musicDataAddress = (music.getDataAddress(1)&$ff)*256+(music.getDataAddress(0)&$ff)
		.label musicInitAddress = (music.getInitAddress(1)&$ff)*256+(music.getInitAddress(0)&$ff)
		.label musicPlayAddress = (music.getPlayAddress(1)&$ff)*256+(music.getPlayAddress(0)&$ff)

		* = musicDataAddress
		.fill music.getSize()-musicDataOffset, music.get(musicDataOffset+i)

		.print " Music start address: " + toHexString(musicDataAddress, 4)
		.print " Music end address  : " + toHexString(*, 4)
		.print " Music init address : " + toHexString(musicInitAddress, 4)
		.print " Music play address : " + toHexString(musicPlayAddress, 4)
	}
