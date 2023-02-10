del tmfplayer.prg tmfplayer.lst tmftool.exe exorcist.prg exorcist.tmf.prg

call ka -bytedumpfile tmfplayer.lst tmfplayer.asm
xxd -i tmfplayer.prg defaulttmfplayer.h
gcc -o tmftool.exe tmftool.c
call tass -o exorcist.prg exorcist.asm

tmftool --init 0x2f01 --play 0x2f25 --timing pal2x --title "Exorcist" --author "David Whittaker" --released "1984 Mr. Micro Ltd." exorcist_patched.prg exorcist.tmf.prg
start exorcist.tmf.prg
