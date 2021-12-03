#include "definizioni.h"

int main(int argc, char **argv){
    int                      ShmID;                   /*ID memoria condivisa*/
    int                      SemID_player, SemID_pedine, SemID_mutex; /*ID semafori*/
    int                      MsqID_Pedina;            /*ID coda di messaggi player<-->pedina*/
    int                      IdentificatoreP, guadagno, prima_direttiva, partita_in_corso, numero_bandiere, mtype, is_a_new_round; /*Variabili di vario uso*/
    struct msgPlayerPedina   MessaggiDaPlayer;        /*Struttura messaggi player-->pedina*/
    struct msgPedinaPlayer  *MessaggiPerPlayer;       /*Struttura messaggi pedina-->player*/
    struct config            conf;                    /*Struttura dati per configurazione*/
    Punto                    destinazione;            /*Struttura "Punto" temporanea*/
    Pedina                   QuestaSonoIo;            /*Struttura di riferimento per questa pedina*/
    Cella                   *matrix;                  /*Matrice di gioco*/

    ImpostaVariabiliDiGioco(&conf);

    IdentificatoreP     = atoi(argv[1]);
    SemID_pedine        = semget(getppid()+16, 1, 0600);

/*Attendo il consenso sullo '0' dal mio giocatore*/
    waitonSem(SemID_pedine, IdentificatoreP);
/*Incremento il mio semaforo...*/
    releaseSem(SemID_pedine, IdentificatoreP);

    MessaggiPerPlayer   = (struct msgPedinaPlayer *)malloc(sizeof(struct msgPedinaPlayer));
    ShmID               = atoi(argv[2]);
    matrix              = (Cella *)shmat(ShmID, NULL, 0);
    SemID_player        = semget(getppid(), 1, 0600);
    SemID_mutex         = semget(KEY_MUTEX, 1, 0600);
    MsqID_Pedina        = msgget(getppid(), 0);

    CheckAllocazioneMemoria(MessaggiPerPlayer);

    if((msgrcv(MsqID_Pedina, &MessaggiDaPlayer, MAX_DIM, 0, 0)) == -1)
    {
        perror("Msgrcv fallita!\n");
		exit(EXIT_FAILURE);
	}

    mtype = MessaggiDaPlayer.mtype;    
    destinazione = MessaggiDaPlayer.destinazione;

/*Qui viene posizionata la pedina sulla scacchiera*/
    PosizioneInizialePedina(&QuestaSonoIo, destinazione, conf.SO_N_MOVES, getppid(), matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex);
    
/*Consento l'esecuzione del mio giocatore (sinc player<->pedina)*/
    reserveSem(SemID_player, 0);
/*...e attendo il consenso sullo '0' dal mio giocatore*/
    waitonSem(SemID_pedine, IdentificatoreP);
/*Incremento il mio semaforo...*/
    releaseSem(SemID_pedine, IdentificatoreP);

    partita_in_corso = 1;
    is_a_new_round = 1;

    while(partita_in_corso)
    {

        if(is_a_new_round == 1)
        {
            if((msgrcv(MsqID_Pedina, &MessaggiDaPlayer, MAX_DIM, 0, 0)) == -1)
            {		
                fprintf(stdout,"msgrcv fallita. Giocatore: %d Pedina: %d\n", getppid(), getpid());
                exit(EXIT_FAILURE);
            }
        mtype = MessaggiDaPlayer.mtype;
        destinazione = MessaggiDaPlayer.destinazione;
    
    /*Consento l'esecuzione del mio giocatore (sinc player<->pedina)*/        
        reserveSem(SemID_player, 0);
    /*...e attendo il consenso sullo '0' dal mio giocatore*/
        waitonSem(SemID_pedine, IdentificatoreP);
    /*Incremento il mio semaforo...*/
        releaseSem(SemID_pedine, IdentificatoreP);

        prima_direttiva = 1;
        is_a_new_round = 0;
        }

        
        if(prima_direttiva == 0) 
        {
            if((msgrcv(MsqID_Pedina, &MessaggiDaPlayer, MAX_DIM, 0, 0)) == -1)
            {		
                fprintf(stdout,"msgrcv fallita. Giocatore: %d Pedina: %d\n", getppid(), getpid());
                exit(EXIT_FAILURE);
            }
                
            mtype = MessaggiDaPlayer.mtype;
            destinazione = MessaggiDaPlayer.destinazione;              
        }

        prima_direttiva = 0;

        guadagno = mov(&QuestaSonoIo, destinazione, matrix, conf.SO_ALTEZZA, conf.SO_BASE, conf.SO_MIN_HOLD_SEC, conf.SO_NUM_G, conf.SO_NUM_P, SemID_mutex);
            
        MessaggiPerPlayer->mtype = 1;
        MessaggiPerPlayer->nuova_posizione = QuestaSonoIo.posizione;
        MessaggiPerPlayer->score_to_add = guadagno;
            
        if(msgsnd(MsqID_Pedina, MessaggiPerPlayer, sizeof(struct msgPedinaPlayer)-sizeof(long), 0) == -1)
        {
            perror("Msgsend in pedina");
            exit(EXIT_FAILURE);
        }

    /*Consento l'esecuzione del mio giocatore (sinc player<->pedina)*/        
        reserveSem(SemID_player, 0);
    /*...e attendo il consenso sullo '0' dal mio giocatore*/
        waitonSem(SemID_pedine, IdentificatoreP);
    /*Incremento il mio semaforo...*/
        releaseSem(SemID_pedine, IdentificatoreP);

        /*Conto le bandiere per capire se il round è giunto al termine*/
            if((numero_bandiere = ContBandiere(matrix, conf.SO_ALTEZZA, conf.SO_BASE,SemID_mutex)) == 0)
            {
            is_a_new_round = 1;

        /*Consento l'esecuzione del mio giocatore (sinc player<->pedina)*/        
            reserveSem(SemID_player, 0);
        /*...e attendo il consenso sullo '0' dal mio giocatore*/
            waitonSem(SemID_pedine, IdentificatoreP);
        /*Incremento il mio semaforo...*/
            releaseSem(SemID_pedine, IdentificatoreP);
            
            }
    }
    exit(EXIT_SUCCESS);
}