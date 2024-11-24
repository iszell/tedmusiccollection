KICKASS	=	java -jar $(KICKASSPATH)/KickAss.jar 

all: exorcist.tmf.prg

clean:
	$(RM) tmftool tmftool.exe defaulttmfplayer.h tmfplayer.prg exorcist.tmf.prg *.lst

tmftool: tmftool.c defaulttmfplayer.h
	$(CC) tmftool.c -o tmftool

defaulttmfplayer.h: tmfplayer.prg
	xxd -i tmfplayer.prg defaulttmfplayer.h

tmfplayer.prg: tmfplayer.asm
	$(KICKASS) -bytedumpfile tmfplayer.lst tmfplayer.asm

exorcist.tmf.prg: tmftool exorcist_patched.prg
	./tmftool --init 0x2f01 --play 0x2f25 --timing pal2x --title "Exorcist" --author "David Whittaker" --released "1984 Mr. Micro Ltd." exorcist_patched.prg exorcist.tmf.prg
