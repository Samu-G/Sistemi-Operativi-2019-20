#include <stdio.h>   
#include <stdlib.h>   
#include <unistd.h>   
#include <bits/types.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <sys/sem.h>  
#include <sys/msg.h>
#include <signal.h> 
#include <errno.h>     
#include <math.h>
#include <time.h>
#include "funzioni.h"
#ifndef KEY_MUTEX    
#define KEY_MUTEX 23158817
#endif