CC=gcc
CF1=-std=c89
CF2=-pedantic
CF3=-lm
CF4=-o
compila_pedina:Pedina.c
	$(CC) Pedina.c $(CF1) $(CF2) $(CF3) $(CF4) Pedina
compila_player:Player.c
	$(CC) Player.c $(CF1) $(CF2) $(CF3) $(CF4) Player
compila_master:Master.c
	$(CC) Master.c $(CF1) $(CF2) $(CF3) $(CF4) Master
compila:compila_pedina compila_player compila_master 
	make compila_pedina compila_player compila_master
