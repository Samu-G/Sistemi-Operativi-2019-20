/*PUNTO*/
typedef struct{
	int x;
	int y;
} Punto;


/*CELLA*/
typedef struct {
	int score;
	int semID_mat;
	int sem_reference; /*numero del semaforo RIFERITO A QUELLA CELLA*/
	int player_reference;
} Cella;


/*PEDINA*/
typedef struct {
	Punto  posizione;
	int nr_mosse;
	int owner;
} Pedina;


/*BANDIERA*/
typedef struct{
	Punto posizione_bandiera;
	int score;
} Bandiera;

/*GIOCATORE*/
typedef struct{
	int punteggio_giocatore;
	int n_mosse_rimanenti;
} Giocatore;

/*CONFIGURAZIONE*/
struct config{
    int SO_NUM_G;
    int SO_NUM_P;
    int SO_MAX_TIME;
    int SO_BASE;
    int SO_ALTEZZA;
    int SO_FLAG_MIN;
    int SO_FLAG_MAX;
    int SO_ROUND_SCORE;
    int SO_N_MOVES;
    int SO_MIN_HOLD_SEC;
}; 

#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC 1

  /*UTILITY NANOSLEEP*/
  struct timespec
  {
    __time_t tv_sec;
    __syscall_slong_t tv_nsec;
  };

#endif
