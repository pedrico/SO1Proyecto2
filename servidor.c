//SHMServer.C
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>          /* errno, ECHILD            */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>      

#include "ncurses.h"
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pthread.h>

#define MAXSIZE     1024
 
void die(char *s)
{
    perror(s);
    exit(1);
}

void *do_smth_periodically(void *data)
{
    //se usan posiciones 1005,1006 y 1007 para minutos, decenas desegundo y segundo
    int shmid3;
    key_t key3;
    char *shm3, *smundo3;
    key3 = 5679; 
    if ((shmid3 = shmget(key3, MAXSIZE, IPC_CREAT | 0666)) < 0)
        die("shmget");
 
    if ((shm3 = shmat(shmid3, NULL, 0)) == (char *) -1)
        die("shmat");

    //Setear valores
    smundo3 = shm3;


      int segundos = 0;
      int minutos = 0;
      int horas = 0;
      int minAux = 0;

      int interval = *(int *)data;
      while(1)
      {
        if (segundos<10)
        {
            smundo3[1005] = minutos;
            smundo3[1006] = 0;
            smundo3[1007] = segundos;
            //printf("%d%d:%d%d\n",0,minutos,0,segundos );
        }else if(minutos<10)
        {
            smundo3[1005] = minutos;
            smundo3[1006] = segundos/10;
            smundo3[1007] = segundos%10;
            //printf("%d%d:%d\n",0,minutos,segundos);
        }
        
        sleep(1);
        segundos++;
        minAux = segundos % 60;
        if (minAux == 0) //ya hay minutos
        {
            minutos++;
            segundos = 0; 
        }
      } 
  
}



bool kbhit()
{
    struct termios term;
    tcgetattr(0, &term);

    struct termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}
 
int main()
{
    sem_t *sem;
    sem_t *sem2;                   /*      synch semaphore         *//*shared */
    unsigned int value;           /*      semaphore value         */ 
    

    initscr(); //Crea stdscr
    raw();

    char c;
    int shmid;
    key_t key;
    char *shm, *smundo;
    int valor;    
    char jugador;
    
    //989 espacios de memoria ocupados por el tablero
    int sizey = 23;
    int sizex = 40;
    int x, y, yi;
    char world[sizey][sizex];
    char player = 'A';
    char playerLaser = '^';
    char enemy = '*';
    char enemyLeader = 'V';
    char enemyShielded1 = 'O';
    char enemyShielded2 = '2';
    char enemyShielded3 = '3';
    char enemyShielded4 = '4';
    char enemyShielded5 = '5';
    char enemyLaser = 'U';
    char explosion = 'X';
    int score = 0;
    int victory = 1;
    int laserReady = 1;
    int enemyReady = 0;
    int contJugadores; //Cuenta cuantos jugadores hay conectados.
    char rol = 'x';
    int vidasd = 5;
    int vidasa = 5;
    int punteoMaximo = 100;

    srand(time(NULL));    

    while(1){
    /*welcome screen*/
    clear();
    printw("\n \n        ¡Bienvenido! \n \n \n \n");
    refresh();
    napms(500);
    printw("       Preparando escenario. \n \n \n \n");
    refresh();
    napms(500);
    printw(" Preparado para ganar la batalla?. \n \n \n \n");
    refresh();
    napms(500);
    printw("   ¡Que empiece la batalla!.");
    refresh();
    napms(500);
    printw("\n \n \n \n Ingrese la letra 'a' para seleccionar al atacante o 'd' para seleccionar al defensor.\n");
    rol = getch();
    //printw(&rol);
    //refresh();
    //napms(500);

    key = 5679; 
    if ((shmid = shmget(key, MAXSIZE, IPC_CREAT | 0666)) < 0)
        die("shmget");
 
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1)
        die("shmat");

    //Setear valores
    smundo = shm;

    //Verifico si jugador 1 ya entró
    contJugadores = smundo[1001];
    if (contJugadores != 1)
    {
        smundo[1001] = 1;
        if (rol != 'a' && rol != 'd')//si el usuario ingresa otra letra
        {
            rol = 'd'; //defensor            
        }        
        smundo[1012] = rol;
        sem_unlink ("pSem"); //El sietema operativo destruye pSem si nadie lo esta utilizando
        int valueSemJugadores = 0;
        sem = sem_open ("pSem", O_CREAT | O_RDWR, 0644, valueSemJugadores);         
        printw(" Esperando a jugador 2 \n.");
        refresh();
        sem_wait (sem);   /*Down semaforo. P operation */                 
    }else if(contJugadores == 1)
    {
        contJugadores++;
        smundo[1001] = 0;
        if (smundo[1012] == 'd')//si el primer usuario seleccionó defensor
        {
            rol = 'a'; //atacante
        }  
        else
        {
           rol = 'd'; //defensor
        }
        sem = sem_open ("pSem", 0);         
        sem_post (sem); /* Up semaforo. V operation */                        
    }

    /* initialize semaphores for shared processes */
    sem_unlink ("pSem2"); 
    value = 1;// Inicio el semaforo en rojo
    sem2 = sem_open ("pSem2", O_CREAT | O_RDWR, 0644, value); 
    /* name of semaphore is "pSem", semaphore is reached using this name */
    //sem_unlink ("pSem");      
    /* unlink prevents the semaphore existing forever */
    /* if a crash occurs during the execution         */

    //Se prepara el mundo
    /*initialise world*/
    int totalEnemies = 0;
    for (x = 0; x < sizex; x ++) {
        for (y = 0; y < sizey; y ++) {
            if ((y+1) % 2 == 0 && y < 7 && x > 4
            && x < sizex - 5 && x % 2 ==0) {
                if (y == 5 && x == 14)
                {
                    //Enemigo disparador 2
                    world[y][x] = enemyShielded3;
                }else if (y == 3 && x == 20)
                {
                    //Enemigo disparador 3
                    world[y][x] = enemyShielded4;
                }else if (y == 1 && x == 32)
                {
                    //Enemigo disparador 4
                    world[y][x] = enemyShielded5;
                }
                else {
                    world[y][x] = enemy;                        
                }                    
                totalEnemies ++;
            }
            else if ((y+1) % 2 == 0 && y >= 7 && y < 9 && x > 4
            && x < sizex - 5 && x % 2 ==0){
                if(x == 6){
                    //Enemigo disparador 1
                    world[y][x] = enemyShielded2;
                }else{
                    world[y][x] = enemyShielded1;
                }
                totalEnemies = totalEnemies + 2;
            }
            else {
                world[y][x] = ' ';
            }
        }
    }
    world[0][sizex / 2] = enemyLeader;
    world[sizey - 1][sizex / 2] = player;
    int i = 1;
    char direction = 'l';
    char keyPress;
    int currentEnemies = totalEnemies;

    //Escribro la matriz del juego en la memoria compartida
    smundo = shm;
    for (x = 0; x < sizex; x++)
    {
        for (y = 0; y < sizey; y++)
        {
            smundo[x*sizey + y] = world[y][x];                
        }
    }

    //victory sera la posicion 1000
    smundo[1000] = 1;
    smundo[1002] = vidasd;//vidas defensor
    smundo[1003] = vidasa;//vidas atacante
    smundo[1004] = score;
    //1008 1009 y 1010 para DekkerV5
    smundo[1008] = 0; //Proceso1QuiereEntrar
    smundo[1009] = 0; //Proceso2QuiereEntrar
    smundo[1010] = 1; //Turno

    pthread_t thread;
    int interval = 5000;
    pthread_create(&thread, NULL, do_smth_periodically, &interval);
    
    /*Logica*/
    while(currentEnemies > 0 && smundo[1000] == 1) {
        int drop = 0;
        int enemySpeed = 1 + 10 * currentEnemies / totalEnemies;
        enemySpeed = 30;
        laserReady ++;
        
        smundo = shm;
        /*display world*/
        /*limpiar pantalla*/
        clear();
        printw("Punteo: %d Vidas Defensor: %d Vidas Atacante: %d", smundo[1004], smundo[1002], smundo[1003]);
        printw("\n");
        printw("Tiempo: %d%d:%d%d", 0, smundo[1005], smundo[1006], smundo[1007]);
        printw("\n");
        for (y = 0; y < sizey; y ++) {
            printw("|");
                for (x = 0; x < sizex; x ++) {
                    printw("%c",smundo[x * sizey + y]);
                }
            printw("|");
            printw("\n");
        }

        //sem_wait (sem2);   /*Down semaforo. P operation */        

        if(rol == 'a'){
            //Rol Atacante

            //Inicio de algoritomo de DekkerV5                                  (DekkerV5)
            smundo[1008] = 1; //Proceso1QuiereEntrar                            (DekkerV5)
            while(smundo[1009] == 1) //Proceso2QuiereEntrar                     (DekkerV5)
            {            
                if(smundo[1010] != 1) //Turno <> 1                              (DekkerV5)
                {
                    smundo[1008] = 0; //P1QuiereEntrar = F                      (DekkerV5)
                    while (smundo[1009] == 1){ //while P2QuiereEntrar Esperar   (DekkerV5)
                        //Esperar
                    }
                }            
            }
            //Region critica        



            /*laser time*/
            for (x = 0; x < sizex; x ++) {
                for (y = sizey-1; y >= 0; y --) {
                    //Borro balas al llegar hasta arriba
                    if(y == 0 && smundo[x * sizey + y] == playerLaser) {
                        smundo[x * sizey + y] = ' ';
                    }
                    else{
                        if (i%2 == 0 && smundo[x * sizey + y] == enemyLaser
                        && (smundo[x * sizey + y+1] != enemy & smundo[x * sizey + y+1] != enemyShielded1
                            & smundo[x * sizey + y+1] != enemyShielded2
                            & smundo[x * sizey + y+1] != enemyShielded3
                            & smundo[x * sizey + y+1] != enemyShielded4
                            & smundo[x * sizey + y+1] != enemyShielded5
                            & smundo[x * sizey + y+1] != player)){//Verifico que la casilla de abajo no sea el defensor
                            /*Bajar disparo*/
                            if (y != sizey-1)
                            {
                                smundo[x * sizey + y+1] = enemyLaser;
                            }
                            smundo[x * sizey + y] = ' ';
                        }
                        else if (i%2 == 0 && smundo[x * sizey + y] == enemyLaser
                        && (smundo[x * sizey + y+1] == enemy | smundo[x * sizey + y+1] == enemyShielded1
                            | smundo[x * sizey + y+1] == enemyShielded2
                            | smundo[x * sizey + y+1] == enemyShielded3
                            | smundo[x * sizey + y+1] == enemyShielded4
                            | smundo[x * sizey + y+1] == enemyShielded5
                            | smundo[x * sizey + y+1] == player)){
                            smundo[x * sizey + y] = ' ';
                        }
                    }
                }
            }
            for (x = 0; x < sizex; x ++) {
                for (y = 0; y < sizey; y ++) {
                    /*Velocidad de las valas %15*/
                    if ((i % 30) == 0 
                        && (smundo[x * sizey + y] == enemyShielded1 | smundo[x * sizey + y] == enemyShielded2
                            | smundo[x * sizey + y] == enemyShielded3
                         | smundo[x * sizey + y] == enemy)


                     && (rand() % 15) > 13
                    && smundo[x * sizey + y+1] != playerLaser) {
                        for (yi = y+1; yi < sizey; yi ++) {
                            if (smundo[x * sizey + yi] == enemy
                            | smundo[x * sizey + yi] == enemyShielded1
                            | smundo[x * sizey + yi] == enemyShielded2
                            | smundo[x * sizey + yi] == enemyShielded3
                            | smundo[x * sizey + yi] == enemyShielded4
                            | smundo[x * sizey + yi] == enemyShielded5) {
                                enemyReady = 0;
                                break;
                            }
                            enemyReady = 1;
                        }
                        if (enemyReady) {
                            /*Disparar*/
                            //smundo[x * sizey + y+1] = enemyLaser;
                        }
                    }
                    if (smundo[x * sizey + y] == playerLaser && smundo[x * sizey + y-1] == enemyLeader) {//Bala a atacante
                        smundo[x * sizey + y] = ' ';
                        smundo[1003] = smundo[1003] - 1; //Resto vidas a atacante
                        if (smundo[1003] == 0)//si vidas defensor == 0
                        {
                            //victory
                            smundo[1000] = 0;
                        }
                    }else if (smundo[x * sizey + y] == playerLaser && smundo[x * sizey + y-1] == enemy) {
                        smundo[x * sizey + y] = ' ';
                        smundo[x * sizey + y-1] = explosion;
                        currentEnemies --;
                        //Score
                        smundo[1004] = smundo[1004] + 10;
                        if (smundo[1004] > punteoMaximo)
                        {
                            //victory
                            smundo[1000] = 0;
                        }
                    }
                    else if (smundo[x * sizey + y] == playerLaser
                    && (smundo[x * sizey + y-1] == enemyShielded1
                        | smundo[x * sizey + y-1] == enemyShielded2
                        | smundo[x * sizey + y-1] == enemyShielded3
                        | smundo[x * sizey + y-1] == enemyShielded4
                        | smundo[x * sizey + y-1] == enemyShielded5)
                    ) {
                        //Score
                        if (smundo[x * sizey + y-1] == enemyShielded2
                            ||smundo[x * sizey + y-1] == enemyShielded3
                            ||smundo[x * sizey + y-1] == enemyShielded4
                            ||smundo[x * sizey + y-1] == enemyShielded5)
                        {
                            smundo[1004] = smundo[1004] + 15;//sumo 15 puntos a las naves que disparan
                        }else
                        {
                            smundo[1004] = smundo[1004] + 10;
                        }
                        if (smundo[1004] > punteoMaximo)
                        {
                            //victory
                            smundo[1000] = 0;
                        }

                        smundo[x * sizey + y] = ' ';
                        smundo[x * sizey + y-1] = enemy;
                        currentEnemies --;
                    }
                    else if (smundo[x * sizey + y] == playerLaser
                    && smundo[x * sizey + y-1] == enemyLaser) {
                        smundo[x * sizey + y] = ' ';
                    }
                    else if (smundo[x * sizey + y] == explosion) {
                        smundo[x * sizey + y] = ' ';
                    }
                    else if ((i+1) % 2 == 0 && smundo[x * sizey + y] == enemyLaser
                    && smundo[x * sizey + y+1] == player) {//Bala a defensor
                        smundo[1002] = smundo[1002] - 1;//Resto vidas a defensor

                        //smundo[x * sizey + y+1] = explosion;
                        //smundo[x * sizey + y] = ' ';                        
                        if (smundo[1002] == 0)//si vidas defensor == 0
                        {
                            //victory
                            smundo[1000] = 0;
                        }
                        
                    }
                    else if (smundo[x * sizey + y] == playerLaser
                    && smundo[x * sizey + y-1] != enemyLaser) {
                            smundo[x * sizey + y] = ' ';
                            smundo[x * sizey + y-1] = playerLaser;
                    }
                }
            }

            /*update enemy direction*/
            for (y = 0; y < sizey; y ++) {
                if (smundo[0 * sizey + y] == enemy) {
                    direction = 'r';
                    drop = 1;
                    break;
                }
                if (smundo[(sizex-1) * sizey + y] == enemy){
                    direction = 'l';
                    drop = 1;
                    break;
                }
            }

            /*update board*/
            if (i % enemySpeed == 0) {
                if (direction == 'l') {
                    for (x = 0; x < sizex - 1; x ++) {
                        for (y = 0; y < sizey; y ++) {
                            if (drop && (smundo[(x+1) * sizey + y-1] == enemy
                                || smundo[(x+1) * sizey + y-1] == enemyShielded1
                                || smundo[(x+1) * sizey + y-1] == enemyShielded2
                                || smundo[(x+1) * sizey + y-1] == enemyShielded3
                                || smundo[(x+1) * sizey + y-1] == enemyShielded4
                                || smundo[(x+1) * sizey + y-1] == enemyShielded5)){
                                smundo[x * sizey + y] = smundo[(x+1) * sizey + y-1];
                                smundo[(x+1) * sizey + y-1] = ' ';
                            }
                            else if (!drop && (smundo[(x+1) * sizey + y] == enemy
                                || smundo[(x+1) * sizey + y] == enemyShielded1
                                || smundo[(x+1) * sizey + y] == enemyShielded2
                                || smundo[(x+1) * sizey + y] == enemyShielded3
                                || smundo[(x+1) * sizey + y] == enemyShielded4
                                || smundo[(x+1) * sizey + y] == enemyShielded5)) {
                                smundo[x * sizey + y] = smundo[(x+1) * sizey + y];
                                smundo[(x+1) * sizey + y] = ' ';
                            }
                        }
                    }
                }
                else {
                    for (x = sizex; x > 0; x --) {
                        for (y = 0; y < sizey; y ++) {
                            if (drop && (smundo[(x-1) * sizey + y-1] == enemy
                                || smundo[(x-1) * sizey + y-1] == enemyShielded1
                                || smundo[(x-1) * sizey + y-1] == enemyShielded2
                                || smundo[(x-1) * sizey + y-1] == enemyShielded3
                                || smundo[(x-1) * sizey + y-1] == enemyShielded4
                                || smundo[(x-1) * sizey + y-1] == enemyShielded5)) {
                                smundo[x * sizey + y] = smundo[(x-1) * sizey + y-1];
                                smundo[(x-1) * sizey + y-1] = ' ';
                            }
                            else if (!drop && (smundo[(x-1) * sizey + y] == enemy
                                || smundo[(x-1) * sizey + y] == enemyShielded1
                                || smundo[(x-1) * sizey + y] == enemyShielded2
                                || smundo[(x-1) * sizey + y] == enemyShielded3
                                || smundo[(x-1) * sizey + y] == enemyShielded4
                                || smundo[(x-1) * sizey + y] == enemyShielded5)) {
                                smundo[x * sizey + y] = smundo[(x-1) * sizey + y];
                                smundo[(x-1) * sizey + y] = ' ';
                            }
                        }
                    }
                }
                //Si los atacantes llegan hasta el defensor
                for (x = 0; x < sizex; x ++) {
                    if (smundo[x * sizey + sizey-1] == enemy) {
                        //victory
                        smundo[1000] = 0;
                    }
                }
            }

            /*control player*/
            if(kbhit()){
                keyPress = getch();
            }
            else {
                keyPress = ' ';
            }
            
            /*Movimientos del enemigo*/
            if (keyPress == 'j') {
                for (x = 0; x < sizex; x = x+1) {
                    if ( smundo[(x+1) * sizey + 0] == enemyLeader) {
                        smundo[x * sizey] = enemyLeader;
                        smundo[(x+1) * sizey + 0] = ' ';
                    }
                }
            }

            if (keyPress == 'l') {
                for (x = sizex - 1; x > 0; x = x-1) {
                    if ( smundo[(x-1) * sizey + 0] == enemyLeader) {
                        smundo[x * sizey] = enemyLeader;
                        smundo[(x-1) * sizey + 0] = ' ';
                    }
                }
            }
            /*EnemyLaser*/
            if (keyPress == 'k' && laserReady > 2) {
                for (x = 0; x < sizex; x = x+1) {
                    if ( smundo[x * sizey + 0] == enemyLeader) {
                        smundo[x * sizey + 1] = enemyLaser;
                        //laserReady = 0;
                    }    
                }
            }
            
            for (x = 0; x < sizex; x++) {
                for (y = 0; y < sizey; y++) {
                    if ( smundo[x * sizey + y] == enemyShielded2 && keyPress == '2') {
                        smundo[x * sizey + y + 1] = enemyLaser;                    
                    }
                    else if (smundo[x * sizey + y] == enemyShielded3 && keyPress == '3')
                    {
                        smundo[x * sizey + y + 1] = enemyLaser;                    
                    }
                    else if (smundo[x * sizey + y] == enemyShielded4 && keyPress == '4')
                    {
                        smundo[x * sizey + y + 1] = enemyLaser;                    
                    }
                    else if (smundo[x * sizey + y] == enemyShielded5 && keyPress == '5')
                    {
                        smundo[x * sizey + y + 1] = enemyLaser;
                    }
                }    
            }
            
            /*if (keyPress == '5' && laserReady > 2) {
                for (x = 0; x < sizex; x++) {
                    for (y = 0; y < sizey; y++) {
                        if ( smundo[x * sizey + y] == enemyShielded5) {
                            smundo[x * sizey + y + 1] = enemyLaser;
                            //laserReady = 0;
                        }
                    }    
                }
            }*/

            //Fin de algoritomo de DekkerV5         (DekkerV5)  
            smundo[1008] = 0; //P1QuiereEntrar = F  (DekkerV5)
            smundo[1010] = 2; //Turno = 2           (DekkerV5)
        }
        else{
            //Rol Defensor

            //Inicio de algoritomo de DekkerV5                                  (DekkerV5)
            smundo[1009] = 1; //Proceso2QuiereEntrar                            (DekkerV5)
            while(smundo[1008] == 1) //Proceso1QuiereEntrar                     (DekkerV5)
            {            
                if(smundo[1010] != 2) //Turno <> 2                              (DekkerV5)
                {
                    smundo[1009] = 0; //P2QuiereEntrar = F                      (DekkerV5)
                    while (smundo[1008] == 1){ //while P1QuiereEntrar Esperar   (DekkerV5)
                        //Esperar
                    }
                }            
            }
            //Region critica      

            /*control player*/
            if(kbhit()){
                keyPress = getch();
            }
            else {
                keyPress = ' ';
            }
            if (keyPress == 'a') {
                for (x = 0; x < sizex; x = x+1) {
                    if ( smundo[(x+1) * sizey + sizey-1] == player) {
                        smundo[x * sizey + sizey-1] = player;
                        smundo[(x+1) * sizey + sizey-1] = ' ';
                    }
                }
            }

            if (keyPress == 'd') {
                for (x = sizex - 1; x > 0; x = x-1) {
                    if ( smundo[(x-1) * sizey + sizey-1] == player) {
                        smundo[x * sizey + sizey-1] = player;
                        smundo[(x-1) * sizey + sizey-1] = ' ';
                    }
                }
            }
            if (keyPress == 'm' && laserReady > 2) {
                for (x = 0; x < sizex; x = x+1) {
                    if ( smundo[x * sizey + sizey-1] == player) {
                        smundo[x * sizey + sizey-2] = playerLaser;
                        laserReady = 0;
                    }
                }
            }

            //Fin de algoritomo de DekkerV5         (DekkerV5)  
            smundo[1009] = 0; //P2QuiereEntrar = F  (DekkerV5)
            smundo[1010] = 1; //Turno = 2           (DekkerV5)
        }

        i ++;
        //sem_post (sem2);           /* V operation */ 
        

        refresh();
        napms(50);
    }
     /*limpiar pantalla*/
    clear();
    if (smundo[1000] == 0) {
        printw("\n \n \n \n \n \n                    Game Over! \n \n \n");
        refresh();
        napms(1000);
        printw("\n \n               Puntaje defensor: %d", smundo[1004]);
        refresh();
        napms(1000);
        
        printw("\n \n               Vidas defensor: %d", smundo[1002]);
        refresh();
        napms(1000);        
        printw("\n \n               Vidas atacante %d", smundo[1003]);
        refresh();
        napms(1000);                
        printw("\n \n               Tiempo %d%d:%d%d", 0,smundo[1005],smundo[1006],smundo[1007]);
        refresh();
        napms(1000);                
        getch();
    }
   

    /* shared memory detach */
    shmdt (shm);
    shmctl (shmid, IPC_RMID, 0); //se limpia la memoria compartida

    /* cleanup semaphores */
    printf("sem_destroy return value:%d\n",sem_destroy (sem));
    printf("sem_destroy return value:%d\n",sem_destroy (sem2));

    }
    endwin();
}

