#include "strutture.h"

/*Questa struttura va commentata se si utilizza OSX*/
union semun {
    int              val;    
    struct semid_ds *buf;
    unsigned short  *array;  
#if defined(__linux__)
    struct seminfo* __buf;
#endif
};
/*****************************************************/


/*****************************************************/
/********************DEFINIZIONI**********************/
/*****************************************************/

/************************IPCS*************************/
int initSemAvailable(int semId, int semNum);
int initSemInUse(int semId, int semNum);
int reserveSem(int semId, int semNum);
int releaseSem(int semId, int semNum);
int waitonSem(int semId, int semNum);
int creoInizializzoSemAvailable(int KEY, int nsems);
int creoInizializzoSemInUse(int KEY, int nsems);
void stampaSem(int semId, int semNum);
void checkSem(int semId, int semNum);
void killSemafori(int semID);
void killSharedmemory(int shmID);
void killCodaDiMessaggi(int msqID);

/***********************CELLE************************/
int converti_pos_matrice(int x,int y, int SO_BASE);
void stampa_matrix_dettagliata(Cella *matrix, int *players, Giocatore *info_players, int base, int altezza, int SO_NUM_G,int mutex);
void inizializza(Cella *matrix, int semID_mat, int SO_ALTEZZA, int SO_BASE);
int getScoreCella(Cella *matrice, Punto destinazione, int SO_ALTEZZA, int SO_BASE);
void setScoreCella(Cella *matrice, Punto destinazione, int valoreBandierina, int SO_ALTEZZA, int SO_BASE);
int getIdCella(Cella *matrice,int x,int y,int SO_ALTEZZA,int SO_BASE);
int getSemReferences(Cella *matrice,int x,int y,int SO_ALTEZZA,int SO_BASE);
int getPlayerReferences(Cella *matrice,int x,int y,int SO_ALTEZZA,int SO_BASE);
void setPlayerReferences(Cella *matrice,int x,int y,int valore,int SO_ALTEZZA,int SO_BASE);
int getValoreSemaforoCella(Cella *matrice, Punto destinazione, int SO_ALTEZZA, int SO_BASE);
void riservaSemaforoCella(Cella *matrice, Punto destinazione,int SO_ALTEZZA, int SO_BASE, int owner);
void rilasciaSemaforoCella(Cella *matrice, Punto destinazione, int SO_ALTEZZA, int SO_BASE);
int cellaLibera(Cella *matrice, Punto destinazione,int SO_ALTEZZA, int SO_BASE);
int getX(Punto p);
int getY(Punto p);
Punto setPunto(int riga,int colonna, int SO_ALTEZZA, int SO_BASE);
double distanzaPunto(Punto origine, Punto destinazione);
int equals(Punto a, Punto b);
Punto posizioneRand(int SO_ALTEZZA, int SO_BASE);

/*********************BANDIERINE**********************/
int randNumeroBandiere(int SO_FLAG_MAX, int SO_FLAG_MIN);
void valorizza_bandiere(Bandiera *fl,int dim,Cella *map, int SO_ROUND_SCORE, int SO_ALTEZZA, int SO_BASE,int mutex);
Bandiera *AggiornoVettBandiere(Bandiera *fl, Cella *matrix, int SO_ALTEZZA, int SO_BASE, int numero_bandiere,int mutex);
int ContBandiere(Cella *matrix, int SO_ALTEZZA, int SO_BASE,int mutex);
int BandierinaPiuVicina(Bandiera *flags,int dim_flags,Pedina p);

/***********************PEDINE************************/
void setPedina(Pedina *p, Punto posizione);
void PosizioneInizialePedina(Pedina *p,Punto posizione,int nr_mosse, int owner, Cella *matrice, int SO_ALTEZZA, int SO_BASE,int mutex);
Punto avvicinamento(Pedina p,Punto destinazione,Cella *map,int SO_ALTEZZA,int SO_BASE,int mutex);
int movAviable(Punto origin,Punto destinazione,Cella *matrice, int SO_ALTEZZA, int SO_BASE,int mutex);
int mov(Pedina *p,Punto destinazione,Cella *matrice, int SO_ALTEZZA, int SO_BASE, int SO_MIN_HOLD_NSEC, int SO_NUM_G, int SO_NUM_P,int mutex);

/**********************GENERICHE**********************/
void ImpostaVariabiliDiGioco(struct config* conf);
void CheckAllocazioneMemoria(void * ptr);



/*****************************************************/
/******************UTILITY SEMAFORI*******************/
/*****************************************************/


/*La funzione inizializza il semaforo semNum a 1*/
int initSemAvailable(int semId, int semNum) {
union semun arg;
arg.val = 1;
return semctl(semId, semNum, SETVAL, arg);
}

/*la funzione inizializza il semaforo semNum a 0*/
int initSemInUse(int semId, int semNum) {
union semun arg;
arg.val = 0;
return semctl(semId, semNum, SETVAL, arg);
}

/*La funzione decrementa il semaforo semNum di 1*/
int reserveSem(int semId, int semNum) {
struct sembuf sops;
sops.sem_num = semNum;
sops.sem_op = -1;
sops.sem_flg = 0;
checkSem(semId, semNum);
return semop(semId, &sops, 1);
}

/*La funzione incrementa il semaforo semNum di 1*/
int releaseSem(int semId, int semNum) {
struct sembuf sops;
sops.sem_num = semNum;
sops.sem_op = 1;
sops.sem_flg = 0;
checkSem(semId, semNum);
return semop(semId, &sops, 1);
}

/*La funzione mette il processo che lo invoca in attesa finché il semaforo semNum non raggiunge il valore 0*/
int waitonSem(int semId, int semNum) {
struct sembuf sops;
sops.sem_num=semNum;
sops.sem_flg=0;
sops.sem_op=0;
checkSem(semId, semNum);
return semop(semId, &sops, 1);
}

/*la funzione crea "nsems" semafori con la chiave KEY impostandoli tutti a valore 1*/
int creoInizializzoSemAvailable(int KEY, int nsems){
    int semID,i;
     semID = semget(KEY, nsems, IPC_CREAT | IPC_EXCL | 0600);
    if(semID == -1){
        perror("semget: errore creazione semafori\n");
        exit(EXIT_FAILURE);
    }
    
    for(i=0; i<nsems; i++){
        initSemAvailable(semID, i);
    }
    return semID;
}

/*la funzione crea "nsems" semafori con la chiave KEY impostandoli tutti a valore 0*/
int creoInizializzoSemInUse(int KEY, int nsems){
    int semID,i;
     semID = semget(KEY, nsems, IPC_CREAT | IPC_EXCL | 0600);
    if(semID == -1){
        perror("semget: errore creazione semafori\n");
        exit(EXIT_FAILURE);
    }
    for(i=0; i<nsems; i++){
        initSemInUse(semID, i);
    }
    return semID;
}

/*Stampa i valori dei semafori con chiave SemId, */
void stampaSem(int semId, int semNum){
    int i;
    for(i=0;i<semNum;i++)
    {
        printf("| %d | ",semctl(semId, i, GETVAL));
    }
    printf("\n");
}

/*La funziona controlla che tutti i semafori della coda abbiano un valore compreso tra 0 e 1 inclusi*/
void checkSem(int semId, int semNum){
    int i, val;
    for(i=0;i<semNum;i++)
    {
        val = semctl(semId, i, GETVAL);
        if(val < 0 || val > 1)
        {
            fprintf(stdout, "\nVALORE SEMAFORO ERRATO, semaforo %d ha un valore di %d\n\n", i, val);
            exit(EXIT_FAILURE);
        }
    }
}

/*la funzione cancella il semaforo con semId*/
void killSemafori(int semID){
    if(semctl(semID, 0, IPC_RMID) == -1)
    {
        perror("ERR: kill semafori. \n");
        exit(EXIT_FAILURE);
    }
    else
        fprintf(stdout, "Rimossa la coda di semafori %d \n", semID);
}


/*****************************************************/
/***************UTILITY SHARED MEMORY*****************/
/*****************************************************/

#define KEY 886317

/* la funzione cancella la shared memory con id shmID*/
void killSharedmemory(int shmID){
    if(shmctl(shmID, IPC_RMID, NULL) == -1)
    {
		perror("ERR: cancellazione shared memory. \n");
        exit(EXIT_FAILURE);
    }
    else
        fprintf(stdout, "Rimosso lo spazio condiviso %d \n", shmID);
}

/*****************************************************/
/**************UTILITY CODA DI MESSAGGI***************/
/*****************************************************/

#ifndef MAX_DIM
#define MAX_DIM 512
#endif

/*Struttura dedicata alla comunicazione tra player-->pedina*/
struct msgPlayerPedina {
            long mtype; /* Message type */
            /* Message body */
            Punto destinazione;
            int   partita_in_corso;
};

/*Struttura dedicata alla comunicazione tra pedina-->player*/
struct msgPedinaPlayer {
            long mtype; /* Message type */
            /* Message body */
            Punto nuova_posizione;
            int score_to_add;
};

/*Struttura dedicata alla comunicazione tra player-->master*/
struct msgPlayerMaster {
            long mtype; /*Message Type*/
            /*Message body*/
            Giocatore player;
            Punto bandierina_da_rimuovere;
};

/*Struttura dedicata alla comunicazione tra master-->player*/
struct msgMasterPlayer {
            long mtype; /*Message Type*/
            /*Message body*/
            int partita_in_corso;
};

/*La funzione cancella la coda di messaggi con id msqID*/
void killCodaDiMessaggi(int msqID){
    if(msgctl(msqID, IPC_RMID, NULL) == -1)
    {
		perror("ERR: cancellazione coda messaggi. \n");
        exit(EXIT_FAILURE);
    }
    else
        fprintf(stdout, "Rimossa la coda di messaggi %d\n", msqID);
}


/*****************************************************/
/*****************************************************/
/***************UTILITY PUNTO E CELLE*****************/
/*****************************************************/
/*****************************************************/

/*Dato x e y, converte la posozione della matrice in indirizzo monodimensionale*/
int converti_pos_matrice(int x,int y, int SO_BASE){
	return (SO_BASE*x)+y;
}

/*Viene stampata:
• una rappresentazione visuale della scacchiera in ASCII che illustra posizione delle pedine, distinguibili per
giocatore, e delle bandierine;
• il punteggio attuale dei giocatori e le mosse residue a disposizione.*/
void stampa_matrix_dettagliata(Cella *matrix, int *players, Giocatore *info_players, int base, int altezza, int SO_NUM_G,int mutex){
    int i,j,s;
    char riferimento_player;
    int pid;
    fprintf(stdout, "\n");
    waitonSem(mutex,0);
    releaseSem(mutex,0);
    for(i=0; i<altezza; i++){
        if(i == 0){
            for(s=0; s<base; s++){ 
            if(s == 0)fprintf(stdout, "   ");
            if(s>=0 && s<=8) fprintf(stdout, "%d  ", s);
            else if(s==9) fprintf(stdout, "%d  ", s);
            else if(s>=10 && s<=99) fprintf(stdout, "%d ", s);
            else if(s>=100 && s<=999) fprintf(stdout, "%d", s);            
            if(s == base-1) fprintf(stdout, "\n");
            }
        }
            for(j=0; j<base; j++){
                if(j == 0){
                    if(i>=0 && i<=9) fprintf(stdout, "%d  ", i);
                    else if(i>=10 && i<=99) fprintf(stdout, "%d ", i);
                    else if(i>=100 && i<=999) fprintf(stdout, "%d", i);
                    } 
                if(semctl(getIdCella(matrix,i,j,altezza,base),converti_pos_matrice(i,j, base), GETVAL) == 1){ /*La cella non contiene una pedina*/
                    if(((matrix+converti_pos_matrice(i,j,base))->score) == 0){ /*Se la cella non contiene una bandierina, stampo due spazi*/
                        fprintf(stdout, " . ");
                    } else if((matrix+converti_pos_matrice(i,j,base))->score > 0){ /*Se la cella contiene una bandierina, stampo il suo score*/
                        fprintf(stdout, " %d ", (matrix+converti_pos_matrice(i,j,base))->score);
                    } else {
                        fprintf(stdout, "Val. matrix[%d][%d].score minore di zero, impossibile!",i,j);
                    }
                } else if((semctl(getIdCella(matrix,i,j,altezza,base),converti_pos_matrice(i,j, base), GETVAL) == 0) && (getPlayerReferences(matrix,i,j,altezza,base) > 0)){ /*la cella contiene una pedina*/
                    if(getPlayerReferences(matrix,i,j,altezza,base) == players[0]){ fprintf(stdout, " %c ", 'A'); }
                    else if(getPlayerReferences(matrix,i,j,altezza,base) == players[1]){ fprintf(stdout, " %c ", 'B'); }
                    else if(getPlayerReferences(matrix,i,j,altezza,base) == players[2]){ fprintf(stdout, " %c ", 'C'); }
                    else if(getPlayerReferences(matrix,i,j,altezza,base) == players[3]){ fprintf(stdout, " %c ", 'D'); }
                    else{ fprintf(stdout, "Errore, numero giocatori superirore al limite"); }
                }
            }
        fprintf(stdout, "\n"); 
        }
    for(i=0; i<SO_NUM_G; i++){
        pid = players[i];
        if(pid == players[0]) riferimento_player ='A';
        else if(pid == players[1]) riferimento_player = 'B';
        else if(pid == players[2]) riferimento_player = 'C';
        else if(pid == players[3]) riferimento_player = 'D';
        else{fprintf(stdout, "Errore, numero giocatori superirore al limite");}
        fprintf(stdout, "# Giocatore %c\tMosse rimanenti: %d  Punteggio giocatore %d  #\n\n", riferimento_player, info_players[i].n_mosse_rimanenti, info_players[i].punteggio_giocatore);
    }
    reserveSem(mutex,0);
}

/*Inizializza la celle della matrice*/
void inizializza(Cella *matrix, int semID_mat, int SO_ALTEZZA, int SO_BASE){
    int i, j; 
    for(i=0;i<SO_ALTEZZA;i++){
        for(j=0;j<SO_BASE;j++){
            (matrix+converti_pos_matrice(i,j,SO_BASE))->score=0;    
            (matrix+converti_pos_matrice(i,j,SO_BASE))->semID_mat = semID_mat;
            (matrix+converti_pos_matrice(i,j,SO_BASE))->sem_reference = converti_pos_matrice(i,j, SO_BASE); /*numero del semaforo RIFERITO A QUELLA CELLA*/
            (matrix+converti_pos_matrice(i,j,SO_BASE))->player_reference = -1;
        }
    }
}



                                    /*FUNZIONI DI CONTROLLO SCORE DELLA CELLA*/

/*Ritorna il valore del campo score della cella in posizione x,y nella matrice*/
int getScoreCella(Cella *map, Punto destinazione, int SO_ALTEZZA, int SO_BASE){
    if(((map+converti_pos_matrice(getX(destinazione),getY(destinazione),SO_BASE))->score) < 0){
        perror("getScoreCella: score minore di 0 \n");
        exit(EXIT_FAILURE);
    } else {
        return (map+converti_pos_matrice(getX(destinazione),getY(destinazione),SO_BASE))->score;
    }
}

/*Imposta il valore del campo score della cella in posizione x,y nella matrice*/
void setScoreCella(Cella *map, Punto destinazione, int valoreBandierina, int SO_ALTEZZA, int SO_BASE){
        (map+converti_pos_matrice(getX(destinazione),getY(destinazione),SO_BASE))->score = valoreBandierina;
    
}

/*Ritorna il valore del campo semID_mat della cella in posizione x,y*/
int getIdCella(Cella *matrice,int x,int y,int SO_ALTEZZA,int SO_BASE){
    return (matrice+converti_pos_matrice(x,y,SO_BASE))->semID_mat;
}

/*Ritorna il valore del campo sem_reference della cella in posizione x,y*/
int getSemReferences(Cella *matrice,int x,int y,int SO_ALTEZZA,int SO_BASE){
    return (matrice+converti_pos_matrice(x,y,SO_BASE))->sem_reference;
}

/*Ritorna il valore del campo player_reference della cella in posizione x,y*/
int getPlayerReferences(Cella *matrice,int x,int y,int SO_ALTEZZA,int SO_BASE){
    return (matrice+converti_pos_matrice(x,y,SO_BASE))->player_reference;
}

/*Imposta il valore del campo player_reference della cella in posizione x,y*/
void setPlayerReferences(Cella *matrice,int x,int y,int valore,int SO_ALTEZZA,int SO_BASE){
    (matrice+converti_pos_matrice(x,y,SO_BASE))->player_reference=valore;
}

                                    /*FUNZIONI DI CONTROLLO SEMAFORI DELLA CELLA*/

/*Ritorna il valore del semaforo associato alla cella in posizione x,y*/
int getValoreSemaforoCella(Cella *matrice, Punto destinazione, int SO_ALTEZZA, int SO_BASE){
    int SemID, semValue;
    SemID = getIdCella(matrice,getX(destinazione),getY(destinazione),SO_ALTEZZA,SO_BASE);
	semValue = semctl(SemID, converti_pos_matrice(getX(destinazione),getY(destinazione), SO_BASE), GETVAL);
	if(semValue == -1){
        perror("getValoreSemaforoCella: errore nella semctl\n");
        exit(EXIT_FAILURE);
    } else {
        return semValue;
    }
}

/*Decrementa il valore del semaforo associato alla cella in posizione x,y*/
void riservaSemaforoCella(Cella *matrice, Punto destinazione,int SO_ALTEZZA, int SO_BASE, int owner){
    int SemID, semNum;
    SemID = getIdCella(matrice,getX(destinazione),getY(destinazione),SO_ALTEZZA,SO_BASE);
    semNum = converti_pos_matrice(getX(destinazione), getY(destinazione), SO_BASE);
    reserveSem(SemID, semNum);
    setPlayerReferences(matrice,getX(destinazione),getY(destinazione),owner,SO_ALTEZZA,SO_BASE);
}

/*Incrementa il valore del semaforo associato alla cella in posizione x,y*/
void rilasciaSemaforoCella(Cella *matrice, Punto destinazione, int SO_ALTEZZA, int SO_BASE){
    int SemID, semNum;
    SemID = getIdCella(matrice,getX(destinazione),getY(destinazione),SO_ALTEZZA,SO_BASE);
    semNum = converti_pos_matrice(getX(destinazione), getY(destinazione), SO_BASE);
    releaseSem(SemID, semNum);
}

/*Controlla se la cella in posizione "destinazione" è libera da altre pedine. Ritorna 1 nel caso in cui la cella sia libera, 0 altrimenti*/
int cellaLibera(Cella *matrice, Punto destinazione, int SO_ALTEZZA, int SO_BASE){
    int SemID, semValue;
    SemID = getIdCella(matrice,getX(destinazione),getY(destinazione),SO_ALTEZZA,SO_BASE);
	semValue = semctl(SemID, converti_pos_matrice(getX(destinazione),getY(destinazione), SO_BASE), GETVAL);
	if(semValue == -1) {
		perror("cellaLibera: errore nella semctl\n");
        exit(EXIT_FAILURE);
    } else if (semValue == 1) { /*se il semaforo è libero == 1*/
		return 1;
    } else if (semValue == 0){  /*se il semaforo è occupato == 0*/
        return 0;
    } else {
        perror("cellaLibera: valore del semaforo maggiore di 1! c'è qualcosa che non va \n");
        exit(EXIT_FAILURE);
    }    
}

/*Restituisce il campo x del Punto passato come parametro*/
int  getX(Punto p){
return p.x;
}

/*Restituisce il campo y del Punto passato come parametro*/
int  getY(Punto p){
   return p.y;
}

/*Crea un Punto, rispettando i limiti dati dalla base e dall'altezza della scacchiera*/
Punto setPunto(int riga,int colonna,int SO_ALTEZZA,int SO_BASE){
    Punto risultato;
    if((riga >= 0 && riga < SO_ALTEZZA) && (colonna >= 0 && colonna < SO_BASE)){
        risultato.x = riga;
        risultato.y = colonna;
    } else {
        risultato.x = 0;
        risultato.y = 0;
      }
   return risultato;
}
 
/*La funzione restituisce la distanza tra punto di origine e di destinazione. 
Restituisce -1 in caso di errore. Bisogna compilare con il flag "-lm" */
double distanzaPunto(Punto origine, Punto destinazione){
    double ox, oy, dx, dy, distance;
    ox = (double)getX(origine);
    oy = (double)getY(origine);
    dy = (double)getY(destinazione);
    dx = (double)getX(destinazione);
    distance = 0;
    if(((ox<0)&&(oy<0)) || ((dx<0)&&(dy<0)))
    {
        distance = -1;
    } else 
    {
    distance = sqrt(pow((dy-oy),2)+pow((dx-ox),2));
    }
return distance;
}

/*La funzione restituisce 0 se i punti sono uguali, -1 altrimenti*/
int equals(Punto a,Punto b){
   int result;
   result = -1;
   if((getX(a) == getX(b)) && (getY(a) == getY(b)))
   result = 0;
   return result;
}

/*Crea una posizione casuale all'interno della scacchiera*/
Punto posizioneRand(int SO_ALTEZZA, int SO_BASE){
    Punto risultato;
    srand(clock());
    risultato.y = (rand()%(SO_BASE-1));
    risultato.x = (rand()%(SO_ALTEZZA-1));
    return risultato;
}


/*****************************************************/
/*****************************************************/
/*****************UTILITY BANDIERINE******************/
/*****************************************************/
/*****************************************************/


/*Ritorna un intero casuale compreso tra SO_FLAG_MIN e SO_FLAG_MAX*/
int randNumeroBandiere(int SO_FLAG_MAX, int SO_FLAG_MIN){
    int risultato;
    srand(clock());
    risultato = (rand() % (SO_FLAG_MAX-SO_FLAG_MIN+1) + SO_FLAG_MIN);  
    return risultato;
}

/*Posiziona in modo casuale all'interno della scacchiera le bandierine e assegna un'punteggio aleatorio compreso tra 1 e SO_ROUND_SCORE*/
void valorizza_bandiere(Bandiera *fl,int dim,Cella *map, int SO_ROUND_SCORE, int SO_ALTEZZA, int SO_BASE,int mutex){
    int nr_div, i, j, punteggio_rimasto, sum, tot;
    Punto p;
    punteggio_rimasto = SO_ROUND_SCORE;
    srand(clock());
    for( i=0;i<dim;i++)
    {
        if(i==0)
        {
                do{
                p = posizioneRand(SO_ALTEZZA, SO_BASE);
                nr_div = cellaLibera(map, p, SO_ALTEZZA, SO_BASE);
                } while(nr_div == 0);
        
            (fl+i)-> posizione_bandiera=p;
            (fl+i)-> score = (rand() % (punteggio_rimasto) + (punteggio_rimasto/dim));
            punteggio_rimasto = punteggio_rimasto-(fl+i) -> score;
        }
        else
        {
            for(j=i-1;j>-1;j--)
            {
                do{
                p = posizioneRand(SO_ALTEZZA, SO_BASE);
                nr_div = cellaLibera(map, p, SO_ALTEZZA, SO_BASE);
                nr_div++;} while((equals(p, (fl+i) -> posizione_bandiera) == 0) || (nr_div == 0));
            }
            
            (fl+i) -> posizione_bandiera = p;
            }
    }
    sum = 0;
    tot = SO_ROUND_SCORE;     
    srand(clock());
        for(i=0; i<dim && tot>0; i++){
            if(i == dim-1){
                (fl+i) -> score = SO_ROUND_SCORE-sum;
            }
            else
            {
                do{
                j = rand() % tot + 1; /*calcolo in modo casuale un valore da scrivere nelle celle con index dim-i*/
                } while(j > (tot/dim));
                (fl+i) -> score = j;
                sum = sum + (fl+i) -> score;
                tot = tot - (fl+i) -> score;
            }       
        }
        waitonSem(mutex, 0);
        releaseSem(mutex, 0);
        for(i=0; i < dim; i++){
            setScoreCella(map, (fl+i) -> posizione_bandiera, (fl+i) -> score, SO_ALTEZZA, SO_BASE);
        }
        reserveSem(mutex, 0);
}

/*Aggiorna il vettore delle bandierine in cui sono salvate le posizioni di quest'ultime. Funziona solo con vettori creati dinamicamente*/
Bandiera *AggiornoVettBandiere(Bandiera *fl, Cella *matrix, int SO_ALTEZZA, int SO_BASE, int numero_bandiere,int mutex){
    int m,i,j;
    fl = realloc(fl, (sizeof(Bandiera) * numero_bandiere));
    m = 0;
    CheckAllocazioneMemoria(fl);
    for(i=0; i<SO_ALTEZZA; i++)
    {
    for(j=0; j<SO_BASE; j++)
        {
            if(getScoreCella(matrix, setPunto(i, j, SO_ALTEZZA, SO_BASE), SO_ALTEZZA, SO_BASE) > 0)
            {
                fl[m].posizione_bandiera.x = i;
                fl[m].posizione_bandiera.y = j;
                fl[m].score = getScoreCella(matrix, setPunto(i, j, SO_ALTEZZA, SO_BASE), SO_ALTEZZA, SO_BASE);
                m++;
            }
        }
    }
    return fl;
}

/*Ricava il numero di bandierine presenti in scacchiera in quell'istante*/
int ContBandiere(Cella *matrix, int SO_ALTEZZA, int SO_BASE,int mutex){
    int m , Sem_id , i, j;
    Sem_id = getSemReferences(matrix,1,1,SO_ALTEZZA,SO_BASE);
    m=0;
    for(i=0; i<SO_ALTEZZA; i++)
    {
    for(j=0; j<SO_BASE; j++)
        {
            if(getScoreCella(matrix, setPunto(i, j, SO_ALTEZZA, SO_BASE), SO_ALTEZZA, SO_BASE) > 0)
            {
                m++;
            }
            if(semctl(Sem_id, converti_pos_matrice(i, j, SO_BASE), GETVAL) == 0)
            {
                m--;
            }
        }
    }
    return m;
}

/*Calcola la posizione della bandierina più vicina alla pedina passata come parametro*/
int BandierinaPiuVicina(Bandiera *flags,int dim_flags,Pedina p){        
    double distanza_minima;
    int index_min,i;
    distanza_minima=distanzaPunto((flags)->posizione_bandiera,p.posizione);
    index_min=0;
    for(i=1;i<dim_flags;i++){
        if(distanza_minima>distanzaPunto((flags+i)->posizione_bandiera,p.posizione)){
            distanza_minima=distanzaPunto((flags+i)->posizione_bandiera,p.posizione);
            index_min=i;
        }
    }
    return index_min;
}  

/*Calcola la posizione della pedina più vicina alla bandierina passata come parametro*/
int PedinaPiuVicina(Pedina *info_pedine, int SO_NUM_P, Bandiera flag){
    double distanza;
    int index, i;
    distanza = 99999999;
    index = 0;
    for(i=0; i<SO_NUM_P; i++)
    {
        if(distanzaPunto((info_pedine+i)->posizione, flag.posizione_bandiera) < distanza)
        {
            distanza = distanzaPunto((info_pedine+i)->posizione, flag.posizione_bandiera);
            index = i;
        }
    }
    return index;
}

/*****************************************************/
/*****************************************************/
/*******************UTILITY PEDINE********************/
/*****************************************************/
/*****************************************************/

/*Imposta il valore della pedina con mosse e posizione*/
void setPedina(Pedina *p,Punto posizione){
   p->posizione=posizione;
}
  
/*Imposta la posizione iniziale della pedina sulla scacchiera. Restituisce 0 in caso di successo, -1 in caso di errore*/
void PosizioneInizialePedina(Pedina *p,Punto posizione,int nr_mosse, int owner, Cella *matrice, int SO_ALTEZZA, int SO_BASE,int mutex){
    if(posizione.x>=0 && posizione.y>=0&&nr_mosse>0){
        p->posizione = posizione;
        p->nr_mosse = nr_mosse;
        p->owner = owner;
        waitonSem(mutex,0);
        releaseSem(mutex,0);
        riservaSemaforoCella(matrice,p->posizione, SO_ALTEZZA, SO_BASE, owner);
        reserveSem(mutex,0);
    }
    else
    {
        fprintf(stdout, "Errore: posizionamento iniziale pedina\n");
        exit(EXIT_FAILURE);
    }
}

/*la funzione restiuisce un punto sulla stessa riga oppure sulla stessa colonna in caso di successo, altrimenti restituisce la posizione della pedina passata come parametro*/
Punto avvicinamento(Pedina p,Punto destinazione,Cella *map,int SO_ALTEZZA,int SO_BASE,int mutex){
   Punto risultato;
   /*Posso conquistare la bandierina?*/
    if(movAviable(p.posizione, destinazione, map, SO_ALTEZZA, SO_BASE,mutex)==0) 
            return destinazione;

   /*Se non va a buon fine, tento l'avvicinamento per riga*/
    risultato=setPunto(getX(p.posizione),getY(destinazione),SO_ALTEZZA,SO_BASE);
    if(movAviable(p.posizione,risultato,map,SO_ALTEZZA,SO_BASE,mutex)==0)        
        return risultato;
    else 
    {
    /*Altrimenti, tento l'avvicinamento tramite colonna*/
    risultato=setPunto(getX(destinazione),getY(p.posizione),SO_ALTEZZA,SO_BASE);
       
        if(movAviable(p.posizione,risultato,map,SO_ALTEZZA,SO_BASE,mutex)==0)
            return risultato;
        else
            return p.posizione;
    }
}

/*La funzione calcola se la strada tra punto di origine e punto di destinazione è libera da pedine. Restituisce 0 in caso affermativo, -1 altrimenti*/
int movAviable(Punto origin,Punto destinazione,Cella *matrice, int SO_ALTEZZA, int SO_BASE,int mutex){
int difx, dify, i, distanza;
Punto s; 
difx=getX(destinazione)-getX(origin);
dify=getY(destinazione)-getY(origin);
i=0;
distanza=(unsigned int)distanzaPunto(origin,destinazione);
s=setPunto(getX(origin),getY(origin), SO_ALTEZZA, SO_BASE);

    if(distanza==-1){
        perror("metodo mov_aviable problema con la distanza\n");
        exit(EXIT_FAILURE);
    }
    if(difx==0&&dify==0)
    return 0;
    if(dify==0&&difx!=0){
        /*movimenti orizzontali*/
        if(difx>0){
            for(i=1;i<=distanza;i++){                                    
                s=setPunto(getX(origin)+i,getY(origin), SO_ALTEZZA, SO_BASE);
                if(cellaLibera(matrice,s, SO_ALTEZZA, SO_BASE)==0){
                    i=distanza*10;
                }
            }
            i--;
        }
        else{
            /*vado a sinistra*/
            for(i=1;i<=distanza;i++){                                    
                s=setPunto(getX(origin)-i,getY(origin), SO_ALTEZZA, SO_BASE);
                if(cellaLibera(matrice,s, SO_ALTEZZA, SO_BASE)==0){
                    i=distanza*10;
                }
            }
            i--;
        }
    }else if(difx==0&&dify!=0){
        /*movimento in verticale*/
        if(dify>0){
            /*vado in alto*/
            for(i=1;i<=distanza;i++){                                    
                s=setPunto(getX(origin),getY(origin)+i, SO_ALTEZZA, SO_BASE);
                if(cellaLibera(matrice,s, SO_ALTEZZA, SO_BASE)==0){
                    i=distanza*10;
                }
            }
            i--;
        }
        else{
            /*vado in basso*/
            for(i=1;i<=distanza;i++){                                    
                s=setPunto(getX(origin),getY(origin)-i, SO_ALTEZZA, SO_BASE);
                if(cellaLibera(matrice,s, SO_ALTEZZA, SO_BASE)==0){
                    i=distanza*10;
                }
            }    
            i--;
        }
    }
return (i==distanza)?(0):(-1);
}

/*La funzione sposta la pedina nel punto "destinazione" passato come parametro, eseguirà una nanosleep di SO_MIN_HOLD_SEC nanosecondi se la pedina si è effettivamente mossa, altrimenti non la esegue.
Quindi rilascia il semaforo della cella riferita alla posizione in cui si trovava la pedina e riserva il semaforo della cella riferito alla nuova posizione. In ultima, restituisce il valore
del campo score della cella di destinazione*/
int mov(Pedina *p,Punto destinazione,Cella *matrice, int SO_ALTEZZA, int SO_BASE, int SO_MIN_HOLD_NSEC, int SO_NUM_G, int SO_NUM_P,int mutex){
    int ris;
    unsigned int distanza;
    long int time;
    struct timespec inf;
    
    ris = movAviable(p->posizione, destinazione, matrice, SO_ALTEZZA, SO_BASE, mutex);
    distanza = (unsigned int)distanzaPunto(p->posizione, destinazione);

    if(ris == 0)
    {
    if(distanza == 0)
        time = 0;
    else
        time = SO_MIN_HOLD_NSEC;

        waitonSem(mutex,0);
        releaseSem(mutex,0);
        rilasciaSemaforoCella(matrice,p->posizione, SO_ALTEZZA, SO_BASE);
        reserveSem(mutex,0);

            inf.tv_sec = 0;
            inf.tv_nsec = time;

            if(nanosleep(&inf,NULL)==-1)
            {
                perror("Nanosleep is not working \n");
                exit(EXIT_FAILURE);
            }

        waitonSem(mutex,0);
        releaseSem(mutex,0);
        riservaSemaforoCella(matrice,destinazione, SO_ALTEZZA, SO_BASE, p->owner);
        setPedina(p,destinazione);
        reserveSem(mutex,0);
        
        
        if(getScoreCella(matrice,destinazione, SO_ALTEZZA, SO_BASE) > 0)
        { 
            waitonSem(mutex,0);
            releaseSem(mutex,0);
            ris = getScoreCella(matrice,destinazione, SO_ALTEZZA, SO_BASE);
            reserveSem(mutex,0);                         
        }
        else
        {
            ris = 0;
        }
    }
    else if(ris == -1)
    {
        ris = 0;
    }
    
    return ris; 
}

/*****************************************************/
/*****************************************************/
/**********************GENERICHE**********************/
/*****************************************************/
/*****************************************************/


/*la funzione imposta i campi della struttura data come parametro con i valori letti nel file file.conf*/
void ImpostaVariabiliDiGioco(struct config* conf){
    FILE *fp;
    char buffer[100];
    int i;
    int dati [10];

    if((fp = fopen("file.conf", "r")) == NULL){
        perror("Fopen non andata a buon fine\n");
        exit(EXIT_FAILURE);
    }

    i=0;
    while((fgets(buffer, 100, fp) != NULL) && i<10){
       dati[i] = atoi(buffer);
       i++;
    }
    fclose(fp);

    if(dati[0] <= 0){ fprintf(stdout, "Giocatori insufficenti\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[1] <= 0){ fprintf(stdout, "Pedine insufficenti\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[2] < 1 ){ fprintf(stdout, "Tempo round insufficente\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[3] < 20){ fprintf(stdout, "Dimensione base insufficente\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[4] < 10){ fprintf(stdout, "Dimensione altezza insufficente\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[5] <= 0){ fprintf(stdout, "Numero minimo bandierine insufficente\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[6] < dati[5]){ fprintf(stdout, "Numero massimo bandierine più piccolo del numero minimo bandierine\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[7] < 10){ fprintf(stdout, "Score totale delle bandierine insufficente\n");
                      exit(EXIT_FAILURE);
                    }
    if(dati[8] < 1){ fprintf(stdout, "Mosse per pedina insufficenti\n");
                      exit(EXIT_FAILURE);
                    }
    

    conf->SO_NUM_G = dati[0];
    conf->SO_NUM_P = dati[1];
    conf->SO_MAX_TIME = dati[2];
    conf->SO_BASE = dati[3];
    conf->SO_ALTEZZA = dati[4];
    conf->SO_FLAG_MIN = dati[5];
    conf->SO_FLAG_MAX = dati[6];
    conf->SO_ROUND_SCORE = dati[7];
    conf->SO_N_MOVES = dati[8];
    conf->SO_MIN_HOLD_SEC = dati[9];

}
    
/*La funzione controlla che l'allocazione della memoria sia andata a buon fine*/
void CheckAllocazioneMemoria(void * ptr){
    if(ptr==NULL)
    {
        perror("Problemi con allocazione della memoria\n");
        exit(EXIT_FAILURE);
    }
}