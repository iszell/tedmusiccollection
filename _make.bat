gcc -o tmftool.exe tmftool.c
call ka tmfplayer.asm
call tass -o exorcist.prg exorcist.asm

tmftool --init 0x2f01 --play 0x2f25 --timing pal2x --title "Exorcist" --author "David Whittaker" --released "1984 Mr. Micro Ltd." --player tmfplayer.prg exorcist_patched.prg exorcist.tmf.prg
start exorcist.tmf.prg
