#include "definizioni.h"

int main (int argc, char **argv) {
    int                      i,j,k;                   /*Variabili di vario uso*/
    int                      ShmID;                   /*ID memoria condivisa*/
    int                      SemID_master, SemID_players, SemID_player, SemID_pedine, SemID_mutex; /*ID semafori*/
    int                      MsqID_Player, MsqID_Master; /*ID code di messaggi player<-->pedina e player<-->master*/
    int                      IdentificatoreG, bandierina_piu_vicina, cell_is_not_free, direttiva_data, pedina_di_turno;   /*Variabili di vario uso*/
    int                      partita_in_corso, numero_bandiere, mtype, score_to_add, messaggio_ricevuto, primo_ciclo, is_a_new_round; /*Variabili di vario uso*/
    pid_t                   *pedine;                  /*PID processi pedina*/
    struct msgPlayerPedina  *MessaggiPerPedina;       /*Struttura messaggi player-->pedina*/
    struct msgPlayerMaster  *MessaggiPerMaster;       /*Struttura messaggi player-->master*/
    struct msgPedinaPlayer   MessaggiDaPedina;        /*Struttura messaggi pedina-->player*/
    struct msgMasterPlayer   MessaggiDaMaster;        /*Struttura messaggi master-->player*/
    struct config            conf;                    /*Struttura dati per configurazione*/
    char                     IdentificatoreP [16];    /*Identificatore pedina*/
    Punto                    temp;                    /*Struttura "Punto" temporanea*/
    Cella                   *matrix;                  /*Matrice di gioco*/
    Bandiera                *bandierine;              /*Vettore di bandierine*/
    Pedina                  *info_pedine;             /*Vettore di informazioni pedine*/
    Giocatore                QuestoSonoIo;            /*Struttura di riferimento per questo giocatore*/

    ImpostaVariabiliDiGioco(&conf);

    IdentificatoreG     = atoi(argv[1]);
    SemID_players       = semget(getppid()+16, 1, 0600);

    waitonSem(SemID_players, IdentificatoreG); /*ATTENDO IL MIO TURNO*/
    
/*Inizializzo strutture dati, ricavo key IPCS*/
    pedine              = (pid_t *)malloc(sizeof(pid_t)*conf.SO_NUM_P);
    info_pedine         = (Pedina *)malloc(sizeof(Pedina)*conf.SO_NUM_P);
    MessaggiPerPedina   = (struct msgPlayerPedina *)malloc(sizeof(struct msgPlayerPedina));
    MessaggiPerMaster   = (struct msgPlayerMaster *)malloc(sizeof(struct msgPlayerMaster));
    bandierine          = (Bandiera *)malloc(sizeof(Bandiera) * numero_bandiere);
    ShmID               = atoi(argv[2]);
    matrix              = (Cella *)shmat(ShmID, NULL, 0);
    SemID_master        = semget(getppid(), 1, 0600);
    SemID_player        = creoInizializzoSemAvailable(getpid(), 1);
    SemID_pedine        = creoInizializzoSemAvailable(getpid()+16, conf.SO_NUM_P);
    SemID_mutex         = semget(KEY_MUTEX, 1, 0600);
    MsqID_Master        = msgget(getppid(), 0);
    MsqID_Player        = msgget(getpid(), IPC_CREAT | IPC_EXCL | 0600);

    CheckAllocazioneMemoria(pedine);
    CheckAllocazioneMemoria(info_pedine);
    CheckAllocazioneMemoria(MessaggiPerPedina);
    CheckAllocazioneMemoria(MessaggiPerMaster);
    CheckAllocazioneMemoria(bandierine);

    QuestoSonoIo.n_mosse_rimanenti = conf.SO_N_MOVES*conf.SO_NUM_P;
    QuestoSonoIo.punteggio_giocatore = 0;

/*Creo i processi pedina*/
    for(i=0; i<conf.SO_NUM_P; i++)
    {
    sprintf(IdentificatoreP, "%d", i);
        switch(pedine[i] = fork())
        {
        case -1:
            perror("FORKING PEDINE\n");
            exit(EXIT_FAILURE);
        break;
            
        case 0:
            execlp("./Pedina","Pedina", IdentificatoreP, argv[2], (char *)NULL);
        break;
        }        
    }

/*Posiziono le pedine sulla scacchiera*/
    for(i=0; i<conf.SO_NUM_P; i++)
    {
        cell_is_not_free = 1;
            
            while(cell_is_not_free)
            {   
                temp = posizioneRand(conf.SO_ALTEZZA, conf.SO_BASE);
                
                if(cellaLibera(matrix, temp, conf.SO_ALTEZZA, conf.SO_BASE) == 1)
                    cell_is_not_free = 0;
            }
            
        MessaggiPerPedina->mtype = 1;
        MessaggiPerPedina->destinazione = temp;
        MessaggiPerPedina->partita_in_corso = 1;

        info_pedine[i].nr_mosse = conf.SO_N_MOVES;
        info_pedine[i].owner = getpid();
        info_pedine[i].posizione = temp;

    /*Invio messaggio a pedina con la sua coordinata iniziale*/
        if((msgsnd( MsqID_Player, MessaggiPerPedina, (sizeof(struct msgPlayerPedina)-sizeof(long)), 0 )) == -1)
        {
            perror("Msgsend fallita!\n");
            exit(EXIT_FAILURE); 
        }

    /*Consento l'esecuzione del processo pedina i-esimo*/
        reserveSem(SemID_pedine, i);
    /*...attendo quindi il consenso sullo '0'(sinc player<->pedina) dal processo pedina i-esimo*/   
        waitonSem(SemID_player, 0);
    /*Incremento il mio semaforo(sinc player<->pedina)...*/      
        releaseSem(SemID_player, 0);                   
    /*Incremento il mio semaforo(sinc master<->player)...*/
        releaseSem(SemID_players, IdentificatoreG);
    /*Consento l'esecuzione del processo master*/
        reserveSem(SemID_master, 0);
    /*...attendo quindi il consenso sullo '0'(sinc master<->player) dal processo master*/   
        waitonSem(SemID_players, IdentificatoreG);
    }

/*Incremento il mio semaforo(sinc master<->player)...*/
    releaseSem(SemID_players, IdentificatoreG);

    partita_in_corso = 1;
    is_a_new_round = 1;
    messaggio_ricevuto = 0;

    while(partita_in_corso)
    {

        if(is_a_new_round==1)
        {
        /*Do la prima direttiva su dove voglio spostare la mia pedina*/
            for(i=0; i<conf.SO_NUM_P; i++)
            {
            /*Ricavo il vettore di bandierine, cacolo la bandiera più vicina alla pedina*/
                    numero_bandiere = ContBandiere(matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex);
                    bandierine = AggiornoVettBandiere(bandierine, matrix, conf.SO_ALTEZZA, conf.SO_BASE, numero_bandiere,SemID_mutex);
                    bandierina_piu_vicina = BandierinaPiuVicina(bandierine, numero_bandiere,info_pedine[i]);  

                    MessaggiPerPedina->mtype = 1;
                    temp = bandierine[bandierina_piu_vicina].posizione_bandiera;
                    MessaggiPerPedina->destinazione = avvicinamento(info_pedine[i], temp, matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex);

                    if((msgsnd( MsqID_Player, MessaggiPerPedina, (sizeof(struct msgPlayerPedina)-sizeof(long)), 0 )) == -1)
                        {
                            perror("Msgsend fallita!\n");
                            exit(EXIT_FAILURE); 
                        }

                /*Consento l'esecuzione del processo pedina i-esimo*/
                    reserveSem(SemID_pedine, i);
                /*...attendo quindi il consenso sullo '0'(sinc player<->pedina) dal processo pedina i-esimo*/   
                    waitonSem(SemID_player, 0);
                /*Incremento il mio semaforo(sinc player<->pedina)...*/      
                    releaseSem(SemID_player, 0);                   
                /*Consento l'esecuzione del processo master*/
                    reserveSem(SemID_master, 0);
                /*...attendo quindi il consenso sullo '0'(sinc master<->player) dal processo master*/   
                    waitonSem(SemID_players, IdentificatoreG);
                /*Incremento il mio semaforo(sinc master<->player)...*/
                    releaseSem(SemID_players, IdentificatoreG);
            }

        direttiva_data = 1;
        is_a_new_round = 0;
            
        }


    i=0;
        for(i=0; i<conf.SO_NUM_P && partita_in_corso && !is_a_new_round; i++)
        {

            if(!messaggio_ricevuto)
            {
            pedina_di_turno = i;
            primo_ciclo = 1;

            /*Mi aggiorno con master se il timer della partita è scaduto*/
                if(msgrcv(MsqID_Master, &MessaggiDaMaster, MAX_DIM, 0, 0) == -1)
                {
                    perror("Msgrcv non andata a buon fine\n");
                    exit(EXIT_FAILURE); 
                }

            mtype = MessaggiDaMaster.mtype;
            partita_in_corso = MessaggiDaMaster.partita_in_corso;
            
                if(partita_in_corso == 0)
                {
                    /*Scommentare se si vuole visualizzare le mosse rimanenti per le varie pedine*/
                    /*
                    fprintf(stdout, "Giocatore %d\n", getpid());
                    for(j=0; j<conf.SO_NUM_P; j++)
                    {
                    fprintf(stdout, "mosse rimanenti pedina %d = %d in posizione (%d,%d)\n",j,info_pedine[j].nr_mosse, info_pedine[j].posizione.x, info_pedine[j].posizione.y);
                    }
                    */

                    for(j=0; j<conf.SO_NUM_P; j++)
                    {
                        kill(pedine[j], SIGINT);
                    }

                fprintf(stdout, "\nI %d processi pedina del giocatore %d sono stati killati\n", j, getpid());
                fprintf(stdout, "Killo il processo giocatore PID %d e i suoi strumenti IPCS\n", getpid());
                    
                free(pedine);
                free(info_pedine);
                free(MessaggiPerPedina);
                free(MessaggiPerMaster);

                killSemafori(SemID_player);
                killSemafori(SemID_pedine);
                killCodaDiMessaggi(MsqID_Player);
                }
            
        /*Consento l'esecuzione del processo master*/
            reserveSem(SemID_master, 0);
        /*...attendo quindi il consenso sullo '0'(sinc master<->player) dal processo master*/
            waitonSem(SemID_players, IdentificatoreG);
        /*Incremento il mio semaforo(sinc master<->player)...*/ 
            releaseSem(SemID_players, IdentificatoreG);

            messaggio_ricevuto = 1;
            }


            if(info_pedine[i].nr_mosse > 0 && QuestoSonoIo.n_mosse_rimanenti)
            {
                if(direttiva_data == 0)
                { 
            /*Ricavo il vettore di bandierine, cacolo la bandiera più vicina alla pedina*/
                numero_bandiere = ContBandiere(matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex); 
                bandierine = AggiornoVettBandiere(bandierine, matrix, conf.SO_ALTEZZA, conf.SO_BASE, numero_bandiere,SemID_mutex);
                bandierina_piu_vicina = BandierinaPiuVicina(bandierine, numero_bandiere,info_pedine[i]);

            
                if((info_pedine[PedinaPiuVicina(info_pedine, conf.SO_NUM_P, bandierine[bandierina_piu_vicina])].nr_mosse == 0) || 
                (equals(avvicinamento(info_pedine[PedinaPiuVicina(info_pedine, conf.SO_NUM_P, bandierine[bandierina_piu_vicina])],  
                bandierine[bandierina_piu_vicina].posizione_bandiera, matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex),bandierine[bandierina_piu_vicina].posizione_bandiera) == -1))
                {
            /*Se la pedina più vicina non ha mosse disponibili oppure se la pedina più vicina non ha la possibilità di conquistare la bandierina, 
            avvicino la pedina che voglio muovere alla bandierina*/
                    MessaggiPerPedina->mtype = 1;
                    temp = bandierine[bandierina_piu_vicina].posizione_bandiera;
                    MessaggiPerPedina->destinazione = avvicinamento(info_pedine[i], temp, matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex);   

                }
                else if((PedinaPiuVicina(info_pedine, conf.SO_NUM_P, bandierine[bandierina_piu_vicina]) == i) || 
                equals(avvicinamento(info_pedine[i],  bandierine[bandierina_piu_vicina].posizione_bandiera, matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex),bandierine[bandierina_piu_vicina].posizione_bandiera) == 0)
                {
            /*Se invece la pedina più vicina è proprio quella che voglio muovere, oppure se ho la possibilità di conquistare la bandierina "a colpo sicuro", 
            mi avvicino/conquisto la bandierina*/
                    MessaggiPerPedina->mtype = 1;
                    temp = bandierine[bandierina_piu_vicina].posizione_bandiera;
                    MessaggiPerPedina->destinazione = avvicinamento(info_pedine[i], temp, matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex);   
                }
                else
                {
            /*Altrimenti decido di lasciare la pedina al suo posto, senza muoverla*/
                    MessaggiPerPedina->mtype = 1;
                    temp = info_pedine[i].posizione;
                    MessaggiPerPedina->destinazione = temp;
                }
                

                /*Spedisco il messaggio a pedina con la sua destinazione designata dal giocatore*/
                    if((msgsnd(MsqID_Player, MessaggiPerPedina, (sizeof(struct msgPlayerPedina)-sizeof(long)), 0 )) == -1)
                    {
                        perror("Msgsend fallita! \n");
                        exit(EXIT_FAILURE);
                    }                
                }

        /*Consento l'esecuzione del processo pedina i-esimo*/
            reserveSem(SemID_pedine, i);
        /*...attendo quindi il consenso sullo '0'(sinc player<->pedina) dal processo pedina i-esimo*/   
            waitonSem(SemID_player, 0);
        /*Incremento il mio semaforo(sinc player<->pedina)...*/      
            releaseSem(SemID_player, 0);                   

                if(msgrcv(MsqID_Player, &MessaggiDaPedina, MAX_DIM, 0, 0) == -1)
                {
                    perror("Msgrcv errore nel player\n");
                    exit(EXIT_FAILURE);
                }


            temp = MessaggiDaPedina.nuova_posizione;
            score_to_add = MessaggiDaPedina.score_to_add;
                
        /*Controllo se la posizione della pedina è cambiata, decrementando o meno il numero di mosse*/
            if(equals(info_pedine[i].posizione, temp) == 0)
            {
                info_pedine[i].nr_mosse = info_pedine[i].nr_mosse;
                QuestoSonoIo.n_mosse_rimanenti = QuestoSonoIo.n_mosse_rimanenti;
            }
            else
            {
                info_pedine[i].nr_mosse = info_pedine[i].nr_mosse - 1;
                QuestoSonoIo.n_mosse_rimanenti = QuestoSonoIo.n_mosse_rimanenti - 1;
            }    
            
            info_pedine[i].posizione = temp;
            QuestoSonoIo.punteggio_giocatore = QuestoSonoIo.punteggio_giocatore + score_to_add;

            MessaggiPerMaster->mtype = 1;
            MessaggiPerMaster->player.n_mosse_rimanenti = QuestoSonoIo.n_mosse_rimanenti;
            MessaggiPerMaster->player.punteggio_giocatore = QuestoSonoIo.punteggio_giocatore;
            MessaggiPerMaster->bandierina_da_rimuovere = temp;


                if(msgsnd(MsqID_Master, MessaggiPerMaster, sizeof(struct msgPlayerMaster)-sizeof(long), 0) == -1)
                {
                    perror("Msgsnd in player\n");
                    exit(EXIT_FAILURE);
                }

            if(i==conf.SO_NUM_P-1) direttiva_data = 0;

            messaggio_ricevuto = 0;

        /*Consento l'esecuzione del processo master*/
            reserveSem(SemID_master, 0);
        /*...attendo quindi il consenso sullo '0'(sinc master<->player) dal processo master*/   
            waitonSem(SemID_players, IdentificatoreG);
        /*Incremento il mio semaforo(sinc master<->player)...*/ 
            releaseSem(SemID_players, IdentificatoreG);

            }
            

            if(i == pedina_di_turno && primo_ciclo == 0)
            {
        /*Se nessuna delle mie pedine ha mosse residue, passo il comando a master*/
                messaggio_ricevuto = 0;
                
                    MessaggiPerMaster->mtype = 1;
                    MessaggiPerMaster->player.n_mosse_rimanenti = QuestoSonoIo.n_mosse_rimanenti;
                    MessaggiPerMaster->player.punteggio_giocatore = QuestoSonoIo.punteggio_giocatore;
                    MessaggiPerMaster->bandierina_da_rimuovere = info_pedine[i].posizione;


                    if(msgsnd(MsqID_Master, MessaggiPerMaster, sizeof(struct msgPlayerMaster)-sizeof(long), 0) == -1)
                    {
                        perror("Msgsnd in player\n");
                        exit(EXIT_FAILURE);
                    }

            /*Consento l'esecuzione del processo master*/
                reserveSem(SemID_master, 0);
            /*...attendo quindi il consenso sullo '0'(sinc master<->player) dal processo master*/  
                waitonSem(SemID_players, IdentificatoreG);
            /*Incremento il mio semaforo(sinc master<->player)...*/ 
                releaseSem(SemID_players, IdentificatoreG);
            }

            primo_ciclo = 0;

        /*Conto le bandiere per capire se il round è giunto al termine*/
            if((numero_bandiere = ContBandiere(matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex)) == 0)
            {
                for(i=0; i<conf.SO_NUM_P; i++)
                {
                /*Consento l'esecuzione del processo pedina i-esimo*/
                    reserveSem(SemID_pedine, i);
                /*...attendo quindi il consenso sullo '0'(sinc player<->pedina) dal processo pedina i-esimo*/   
                    waitonSem(SemID_player, 0);
                /*Incremento il mio semaforo(sinc player<->pedina)...*/      
                    releaseSem(SemID_player, 0);                   
                }
                
            /*Consento l'esecuzione del processo master*/
                reserveSem(SemID_master, 0);
            /*...attendo quindi il consenso sullo '0'(sinc master<->player) dal processo master*/  
                waitonSem(SemID_players, IdentificatoreG);
            /*Incremento il mio semaforo(sinc master<->player)...*/ 
                releaseSem(SemID_players, IdentificatoreG);

                is_a_new_round = 1;
                messaggio_ricevuto = 0;
            }
        }
    }
    exit(EXIT_SUCCESS);
}