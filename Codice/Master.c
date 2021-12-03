#include "definizioni.h"

int main (int argc, char **argv) {
    int                      i,j,dim_band, numero_bandiere, partita_in_corso, numero_round,indice_mono; /*Variabili di vario uso*/
    int                      visualizza_partita_in_real_time, mtype, is_a_new_round; /*Variabili di vario uso*/
    int                      ShmID;                   /*ID memoria condivisa*/
    int                      SemID_matrice, SemID_master, SemID_players,SemID_mutex; /*ID semafori*/
    int                      MsqID_master;            /*ID coda di messaggi master <--> player*/
    pid_t                   *players;                 /*PID processi giocatore*/
    struct msgPlayerMaster   MessaggiDaPlayer;        /*Struttura messaggi player --> master*/
    struct msgMasterPlayer  *MessaggiPerPlayer;       /*Struttura messaggi master --> player*/
    struct config            conf;                    /*Struttura dati per configurazione*/
    char                     SHAREDshmID[10];         /*ID per shared memory*/
    char                     identificatoreG[10];     /*identificatore player*/
    Cella                   *matrix;                  /*Matrice di gioco*/
    size_t                   sizeMatrix;              /*Dimensione memoria matrice di gioco*/
    Bandiera                *bandierine;              /*Vettore di bandierine*/
    Giocatore               *info_players;            /*Vettore per informazioni giocatori*/
    time_t                   Timer, TimerDiControllo; /*Timer di controllo partita*/
    Punto                    temp;                    /*Utilizzato per rimuovere la bandierina*/

/*Chiedo all'utente se vuole vedere la partita in real-time*/
    fprintf(stdout, "Desidera guardare la partita in real-time?\n1 = SI\t0 = NO;\n");
    fscanf(stdin, "%d", &visualizza_partita_in_real_time);
        if(visualizza_partita_in_real_time < 0 || visualizza_partita_in_real_time > 1)
        {
            fprintf(stdout, "Perfavore, inserisca un valore compreso tra 0 e 1. Riavvii il gioco \n");
            exit(EXIT_FAILURE);
        }

    ImpostaVariabiliDiGioco(&conf);
    
/*Inizializzo strumenti IPCS e strutture dati*/
    numero_round        = 1;
    dim_band            = randNumeroBandiere(conf.SO_FLAG_MAX, conf.SO_FLAG_MIN);
    bandierine          = (Bandiera *)malloc(sizeof(Bandiera)*dim_band); 
    players             = (pid_t *)malloc(sizeof(pid_t)*conf.SO_NUM_G);
    MessaggiPerPlayer   = (struct msgMasterPlayer *)malloc(sizeof(struct msgMasterPlayer));
    info_players        = (Giocatore *)malloc(sizeof(Giocatore)*conf.SO_NUM_G);
    SemID_matrice       = creoInizializzoSemAvailable(KEY, conf.SO_ALTEZZA*conf.SO_BASE);
    SemID_master        = creoInizializzoSemAvailable(getpid(), 1);
    SemID_players       = creoInizializzoSemAvailable(getpid()+16, conf.SO_NUM_G);
    SemID_mutex         = creoInizializzoSemInUse(KEY_MUTEX, 1);    
    MsqID_master        = msgget(getpid(), IPC_CREAT | IPC_EXCL | 0600);    
    sizeMatrix          = conf.SO_ALTEZZA*conf.SO_BASE*sizeof(Cella);
    ShmID               = shmget(KEY, sizeMatrix, IPC_CREAT | IPC_EXCL | 0600);
    
    CheckAllocazioneMemoria(bandierine);
    CheckAllocazioneMemoria(players);
    CheckAllocazioneMemoria(MessaggiPerPlayer);
    CheckAllocazioneMemoria(info_players);

    for(i=0; i<conf.SO_NUM_G; i++)
    {
    info_players[i].n_mosse_rimanenti = conf.SO_N_MOVES*conf.SO_NUM_P;
    info_players[i].punteggio_giocatore = 0;
    }

/*Inizializzo e metto in memoria condivisa la scacchiera*/
    matrix = (Cella *)shmat(ShmID, NULL, 0);
    inizializza(matrix, SemID_matrice, conf.SO_ALTEZZA, conf.SO_BASE);

/*Creo i processi giocatore che posizionano a turno le loro pedine*/
    sprintf(SHAREDshmID, "%d", ShmID);
    for(i=0; i<conf.SO_NUM_G; i++)
    {
    sprintf(identificatoreG, "%d", i);
        switch(players[i] = fork())
        {
        case -1:
            perror("FORKING PLAYERS");
            exit(EXIT_FAILURE);
        break;
                
        case 0:
            execlp("./Player","Player", identificatoreG, SHAREDshmID, (char *)NULL);
        break;
        }        
    }

    for(j=0; j<conf.SO_NUM_P; j++)
    {
    for(i=0; i<conf.SO_NUM_G; i++)
        {
    /*Consento l'esecuzione del processo giocatore i-esimo*/
        reserveSem(SemID_players, i);
    /*...attendo quindi il consenso sullo '0'*/   
        waitonSem(SemID_master, 0);
    /*Incremento il mio semaforo...*/
        releaseSem(SemID_master, 0);
        }
    }

    is_a_new_round = 1;
    partita_in_corso = 1;

    while(partita_in_corso)
    {

        if(is_a_new_round==1)
        {
        /*Il processo gestore posiziona le bandierine sulla scacchiera*/
            valorizza_bandiere(bandierine, dim_band, matrix, conf.SO_ROUND_SCORE, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex);

        /*A turno, i processi giocatore danno la direttiva alla pedina*/
            for(j=0; j<conf.SO_NUM_P; j++)
            {
                for(i=0; i<conf.SO_NUM_G; i++)
                {
            /*Consento l'esecuzione del processo giocatore i-esimo*/
                reserveSem(SemID_players, i);
            /*...attendo quindi il consenso sullo '0'*/
                waitonSem(SemID_master, 0);
            /*Incremento il mio semaforo...*/
                releaseSem(SemID_master, 0);
                }
            }

        /*Viene stampato lo stato della scacchiera all'inizio di ogni round*/
            fprintf(stdout, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            fprintf(stdout, "# # # # # # # # # # # # # # # # # # INIZIO DEL %d ROUND # # # # # # # # # # # # # # # # # # # # #\n", numero_round);
            stampa_matrix_dettagliata(matrix, players, info_players, conf.SO_BASE, conf.SO_ALTEZZA, conf.SO_NUM_G,SemID_mutex);
            fprintf(stdout, "# # # # # # # # # # # # # # # # 5 SEC. ALL'INIZIO DEL ROUND # # # # # # # # # # # # # # # # # # #\n");
            fprintf(stdout, "\n\n\n\n\n\n\n\n\n");
            sleep(5);            
            fprintf(stdout, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

            if(!visualizza_partita_in_real_time)
                fprintf(stdout, "partita in corso, i giocatori stanno muovendo le loro pedine.......\n");

            is_a_new_round = 0;
            time(&Timer);
            time(&TimerDiControllo);
        }

/*Round in corso*/

    i=0, j=0;
 
        for(j=0; j<conf.SO_NUM_P && partita_in_corso && !is_a_new_round; j++)
        {
            for(i=0; i<conf.SO_NUM_G && partita_in_corso && !is_a_new_round; i++)
            {

                if(_ERRNO_H != 1) {
                    perror("\n");
                    exit(EXIT_FAILURE);
                }

                time(&TimerDiControllo);

                if(difftime(TimerDiControllo,Timer) > (float)conf.SO_MAX_TIME)
                {
            /*Il tempo è scaduto, vengono avvisati tutti i giocatori*/
                
                fprintf(stdout, "# # # # # # # # # # # # # # # # # IL TEMPO E' SCADUTO # # # # # # # # # # # # # # # # # # # #\n");

                    for(i=0; i<conf.SO_NUM_G; i++)
                    {
                            
                        MessaggiPerPlayer->mtype = 1;
                        MessaggiPerPlayer->partita_in_corso = 0;

                            if(msgsnd(MsqID_master, MessaggiPerPlayer, sizeof(struct msgMasterPlayer)-sizeof(long), 0) == -1)
                            {
                                perror("Msgsnd in master\n");
                                exit(EXIT_FAILURE); 
                            }

                    /*Consento l'esecuzione del processo giocatore i-esimo*/
                        reserveSem(SemID_players, i);
                    /*...attendo quindi il consenso sullo '0'*/
                        waitonSem(SemID_master, 0);
                    /*Incremento il mio semaforo...*/
                        releaseSem(SemID_master, 0);
                        
                    }
                
                /*Termino i processi giocatori...*/
                    for(i=0; i<conf.SO_NUM_G; i++)
                    {
                        kill(players[i], SIGINT);
                    }

                /*...e termino la partita*/
                partita_in_corso = 0;

                fprintf(stdout, "\n# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #\n");

                }
                else
                {
                /*altrimenti, il tempo non è scaduto e viene avvisato il giocatore che sta giocando in quel momento*/
                
                    MessaggiPerPlayer->mtype = 1;
                    MessaggiPerPlayer->partita_in_corso = partita_in_corso;

                    if(msgsnd(MsqID_master, MessaggiPerPlayer, sizeof(struct msgMasterPlayer)-sizeof(long), 0) == -1)
                    {
                        perror("Msgsnd in master");
                        exit(EXIT_FAILURE); 
                    }
                            
                /*Consento l'esecuzione del processo giocatore i-esimo*/
                    reserveSem(SemID_players, i);
                /*...attendo quindi il consenso sullo '0'*/
                    waitonSem(SemID_master, 0);
                /*Incremento il mio semaforo...*/
                    releaseSem(SemID_master, 0);

                }
            
            /*Il processo giocatore una volta avvisato dello stato partita, viene sbloccato nuovamente*/
                if(partita_in_corso)
                {

                /*Consento l'esecuzione del processo giocatore i-esimo*/
                    reserveSem(SemID_players, i);
                /*...attendo quindi il consenso sullo '0'*/
                    waitonSem(SemID_master, 0);
                /*Incremento il mio semaforo...*/
                    releaseSem(SemID_master, 0);
                    
                        if(msgrcv(MsqID_master, &MessaggiDaPlayer, MAX_DIM, 0, 0) == -1) 
                        { 
                            perror("Msgrcv in master");
                            exit(EXIT_FAILURE);
                        }
                            
                     mtype = MessaggiDaPlayer.mtype;
                    info_players[i].n_mosse_rimanenti = MessaggiDaPlayer.player.n_mosse_rimanenti;
                    info_players[i].punteggio_giocatore = MessaggiDaPlayer.player.punteggio_giocatore;
                    temp = MessaggiDaPlayer.bandierina_da_rimuovere;
                        
                    /*Master cancella la bandierina in cui si trova la pedina*/
                        waitonSem(SemID_mutex,0);
                        releaseSem(SemID_mutex,0);
                        setScoreCella(matrix,temp,0,conf.SO_ALTEZZA,conf.SO_BASE);
                        reserveSem(SemID_mutex,0);

                    if(info_players[i].n_mosse_rimanenti > 0)
                    {
                    
                        if(visualizza_partita_in_real_time)
                        {
                            fprintf(stdout, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                            stampa_matrix_dettagliata(matrix, players, info_players, conf.SO_BASE, conf.SO_ALTEZZA, conf.SO_NUM_G,SemID_mutex);
                            fprintf(stdout, "\n\n\n\n\n\n\n\n\n");                       
                        }
                    }

                    numero_bandiere = ContBandiere(matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex); 

                    if(numero_bandiere == 0)
                    {
                        numero_round++;
                        is_a_new_round = 1;

                        for(i=0; i<conf.SO_NUM_G; i++)
                        {
                        /*Consento l'esecuzione del processo giocatore i-esimo*/
                            reserveSem(SemID_players, i);
                        /*...attendo quindi il consenso sullo '0'*/
                            waitonSem(SemID_master, 0);
                        /*Incremento il mio semaforo...*/
                            releaseSem(SemID_master, 0);
                        }

                        fprintf(stdout, "\2a\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                        fprintf(stdout, "# # # # # # # # # # # # # # # # # TUTTE LE BANDIERINE CONQUISTATE # # # # # # # # # # # # # # # # # # # #\n");
                        stampa_matrix_dettagliata(matrix, players, info_players, conf.SO_BASE, conf.SO_ALTEZZA, conf.SO_NUM_G,SemID_mutex);
                        fprintf(stdout, "\n\n\n\n\n\n\n\n\n");                       
                        sleep(5);
                    }       
                }   
            }
        }
    }

/*La partita è giunta al termine, vengono stampate lo stato della scacchiera e le statistiche di gioco*/
    stampa_matrix_dettagliata(matrix, players, info_players, conf.SO_BASE, conf.SO_ALTEZZA, conf.SO_NUM_G,SemID_mutex);
    fprintf(stdout,"Numero round giocati = %d \n", numero_round);
    fprintf(stdout,"Numero bandierine rimanenti = %d \n", numero_bandiere);

    killCodaDiMessaggi(MsqID_master);
    killSemafori(SemID_mutex);
    killSemafori(SemID_master);
    killSemafori(SemID_matrice);
    killSemafori(SemID_players);
    killSharedmemory(ShmID);

    free(bandierine);
    free(players);
    free(MessaggiPerPlayer);
    free(info_players);

    exit(EXIT_SUCCESS);
}