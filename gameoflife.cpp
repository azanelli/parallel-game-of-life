/*!
  \file gameoflife.cpp
  \brief Implementazione parallela del gioco della vita
  \author Andrea Zanelli
  \date 09-03-2010
*/

#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include "Muesli.h"
#include "Matrix.h"
#include "Block.h"
#include "Vector.h"

using gameoflife::Matrix;
using gameoflife::Block;
using gameoflife::Vector;


// Matrice che rappresenta il "Gioco della vita"
Matrix* GAME_OF_LIFE_MATRIX;
unsigned int ROWS;
unsigned int COLUMNS;

// Densita' della matrice
float DENSITY;

// Numero di iterazioni
unsigned int ITERATIONS;

// True se si deve stampare la matrice iniziale e finale
bool PRINT_MATRIX;

// True se devono essere stampati i tempi di computazione
bool PRINT_CTIMES;

// MPI workers comunicator
MPI_Comm MPI_COMM_WORKERS;

// Informazioni sui workers
int N_WORKERS;
int* WORKERS_ID;

// Tempi di computazione
timeval T_START, T_END;
clock_t C_START, C_END;

// Dichiarazioni delle funzioni
Block* init(Empty);
Block* compute(Block*);
void fin(Block*);
void initWorkers();
void createMpiCommWorkes();
void discoverNeighbors(unsigned int, ProcessorNo*, ProcessorNo*);
void workersSynch(Block*, ProcessorNo, ProcessorNo);
inline void startTimer();
inline void stopTimer();
bool getParameters(int, char**);
void printProgramInfo();
void printComputationTimes();
void printHelp();


/*!
  \fn int main(int argc, char **argv)
  \param argv array con gli argomenti
  \param argc numero di argomenti  
  \return 0: ok
  \return 1: errore

  Esegue i seguenti passi:
    - Inizializza gli skeleton della libreria Muesli.
    - Legge ed inizializza i parametri dell'applicazione.
    - Cotruisce la farm utilizzando la libreria Muesli.
    - Avvia l'esecuzione della farm.
    - Se non si sono verificati errori termina la libreria Muesli e termina
      l'esecuzione.
*/
int main(int argc, char* argv[]) {
  try {
    // Inizializza la libreria muesli
    InitSkeletons(1, argv);
    
    // Prende e inizializza i parametri dell'applicazione
    if(!getParameters(argc, argv)) {
      TerminateSkeletons();
      return 1;
    }
    
    // Inizializza i workers
    initWorkers();

    // Il primo processo stampa le informazioni e crea la matrice iniziale
    if(MSL_myId == 0) {
      printProgramInfo();
      startTimer();
      GAME_OF_LIFE_MATRIX = new Matrix(ROWS, COLUMNS, DENSITY);
    }
    
    // Costruisce la farm
    Initial<Block> in(init);
    Atomic<Block, Block> atomic(compute, 1);
    Farm<Block, Block> farm(atomic, N_WORKERS);
    Final<Block> out(fin);
    Pipe pipe(in, farm, out);

    // Avvia l'esecuzione della farm
    if(MSL_myId == MSL_numOfTotalProcs-1)
      startTimer();
    pipe.start();

    if(PRINT_CTIMES) {
      usleep(MSL_myId*100000);
      printComputationTimes();
    }

    // Termina la libreria Muesli
    TerminateSkeletons();
    
   } catch(Exception& ex) {
     std::cerr <<"The execution is terminate for the exception: ";
     std::cerr <<ex  <<std::endl;
     return 1;
   } // end of try-catch

  return 0;
} // end of function main

/*!
  \fn Block* init(Empty)
  \brief Funzione eseguita dallo stage iniziale
  
  Divide la matrice in N_WORKERS blocchi e restituisce un blocco alla volta
  ogni volta che viene invocata. Ogni blocco restituito sara' l'input di un
  worker. Quando ha restituito tutti i blocchi, ritorna NULL.
*/
Block* init(Empty) {
  static int count = 0;
  if(PRINT_MATRIX && count == 0)
    std::cout <<std::endl <<(*GAME_OF_LIFE_MATRIX);
  if(count == N_WORKERS) {
    stopTimer();
    return NULL;
  }
  Block* block = GAME_OF_LIFE_MATRIX->getBlock(N_WORKERS, count);
  count = count + 1;
  return block;
} // end of function init

/*!
  \fn Block* compute(Block* input)
  \brief Funzione eseguita dai workers
  \param input blocco da elaborare
  
  Esegue ITERATIONS iterazioni sul blocco ricevuto in input e restituisce il
  blocco elaborato. L'output di questa funzione sara' l'input della funzione
  fin.
*/
Block* compute(Block* input) {
  startTimer();
  ProcessorNo leftNeigh, rightNeigh;
  // Cerca i processi "vicini"
  discoverNeighbors(input->getN(), &leftNeigh, &rightNeigh);
  // Esegue le iterazioni sul blocco, sincronizzandosi alla fine di ognuna.
  for(int i=0; i < ITERATIONS; ++i) {
    input->compute();
    workersSynch(input, leftNeigh, rightNeigh);
  } // end for i
  stopTimer();
  return input;
} // end of function compute

/*!
  \fn void fin(Block* input)
  \brief Funzione eseguita dallo stage finale
  \param input blocco elaborato
  
  Riceve i blocchi elaborati dai workers e li ricompone in modo ordinato per
  formare la matrice finale.
*/
void fin(Block* input) {
  static int count = 1;
  if(count == 1)
    GAME_OF_LIFE_MATRIX = new Matrix(ROWS, COLUMNS);
  GAME_OF_LIFE_MATRIX->setBlock(input);
  delete input;
  if(count == N_WORKERS) {
    stopTimer();
    if(PRINT_MATRIX) {
      std::cout <<std::endl <<(*GAME_OF_LIFE_MATRIX);
      usleep(100000); // Aspetta per non sovrapporre le stampe
    }
  } // end if(count == N_WORKERS)
  count = count + 1;
  return;
} // end of function fin

/*!
  \fn void initWorkers()
  \brief Inizializza i workers
  
  Inizializza il numero di worker (N_WORKERS), gli ID dei workers (WORKERS_ID),
  e crea il comunicator (MPI_COMM_WORKERS) utilizzato per la comunicazione tra
  workers.
*/
void initWorkers() {
  N_WORKERS = MSL_numOfTotalProcs-2;
  WORKERS_ID = new int[N_WORKERS];
  for(int i=0; i < N_WORKERS; ++i)
    WORKERS_ID[i] = i+1;
  createMpiCommWorkes();
  return;
} // end of function initWorkers

/*!
  \fn void createMpiCommWorkes()
  \brief Crea un comunicator MPI per i workers
*/
void createMpiCommWorkes() {
  MPI_Group mpiGroupWorld, mpiGroupWorkers;
  MPI_Comm_group(MPI_COMM_WORLD, &mpiGroupWorld);
  MPI_Group_incl(mpiGroupWorld, N_WORKERS, WORKERS_ID, &mpiGroupWorkers);
  MPI_Comm_create(MPI_COMM_WORLD, mpiGroupWorkers, &MPI_COMM_WORKERS); 
  return;
} // end of function createMpiCommWorkes

/*!
  \fn void discoverNeighbors(unsigned int N, ProcessorNo* left, 
                             ProcessorNo* right)
  \brief Ricerca i vicini di un processo
  \param N numero del blocco (IN)
  \param right restituisce l'ID del vicino destro (OUT)
  \param left restituisce l'ID del vicino sinistro (OUT)
  
  Si scambia con tutti i workers un vettore di coppie <ID, Blocco>, in cui
  e' indicato l'ID del processo e il numero del blocco che gli e' stato
  assegnato. Attraverso questo vettore risale all'ID dei processi vicini e
  li restituisce nei parametri left e right.
*/
void discoverNeighbors(unsigned int N, ProcessorNo* left, ProcessorNo* right) {
  int mycoord[2] = {N, MSL_myId};
  int recvbuf[2 * N_WORKERS];
  // Broadcast del vettore con le coordinate
  MPI_Allgather(mycoord, 2, MPI_INT, recvbuf, 2, MPI_INT, MPI_COMM_WORKERS);
  int nLeft = (N == 0 ? N_WORKERS : N) - 1; // numero del blocco precedente
  int nRight = (N+1 == N_WORKERS ? 0 : N+1); // numero del blocco successivo
  // Ricerca i vicini sul vettore ricevuto
  for(int i=0; i < 2*N_WORKERS; i=i+2) {
    if(recvbuf[i] == nLeft) *left = recvbuf[i+1];
    if(recvbuf[i] == nRight) *right = recvbuf[i+1];
  }
  return;
} // end of funciton discoverNeighbors

/*!
  \fn void workersSynch(Block* block, ProcessorNo left, ProcessorNo right)
  \brief Esegue la fase di sincronizzazione con i worker vicini
  \param block blocco da aggiornare
  \param left ID del vicino sinistro
  \param right ID del vicino destro
  
  Aggiorna i vettori del blocco passato ricevendo i bordi dai worker vicini,
  ed invia i bordi del blocco ai worker vicini.
*/
void workersSynch(Block* block, ProcessorNo left, ProcessorNo right) {
  Vector* leftb = block->getLeftBoundary();
  Vector* rightb = block->getRightBoundary();

  // Caso di un solo worker
  if(N_WORKERS == 1) {
    block->setLeftVector(rightb);
    block->setRightVector(leftb);
    return;
  }

  MPI_Status status;
  Vector* leftv = new Vector();
  Vector* rightv = new Vector();
  
  // I processi con un blocco pari spediscono per primi, quelli con un blocco
  // dispari ricevono per primi.
  if( (block->getN() % 2) == 0 ) {
    // Spedisce il bordo destro al vicino destro
    MSL_Send(right, rightb, 1);
    // Riceve il vettore sinistro dal vicino sinistro
    MSL_Receive(left, leftv, 1, &status);
    // Spedisce il bordo sinistro al vicino sinistro
    MSL_Send(left, leftb, 1);
    // Riceve il vettore destro dal vicino destro
    MSL_Receive(right, rightv, 1, &status);
  }
  else {
    // Riceve il vettore sinistro dal vicino sinistro
    MSL_Receive(left, leftv, 1, &status);
    // Spedisce il bordo destro al vicino destro
    MSL_Send(right, rightb, 1);
    // Riceve il vettore destro dal vicino destro
    MSL_Receive(right, rightv, 1, &status);
    // Spedisce il bordo sinistro al vicino sinistro
    MSL_Send(left, leftb, 1);
  }
  
  // Aggiorna i vettori del blocco
  block->setLeftVector(leftv);
  block->setRightVector(rightv);
  delete leftb;
  delete rightb;
  
  return;
} // end of function workersSynch

/*!
  \fn void void startTimer()
  \brief Memorizza i tempi iniziali
*/
inline void startTimer() {
  gettimeofday(&T_START, NULL);
  C_START = clock();
  return;
} // end of function startTimer

/*!
  \fn void void stopTimer()
  \brief Memorizza i tempi finale
*/
inline void stopTimer() {
  C_END = clock();
  gettimeofday(&T_END, NULL);
  return;
} // end of function stopTimer

/*!
  \fn void void stopTimer()
  \brief Legge i parametri ed inizializza le variabili globali 
  \param argv array con i parametri
  \param argc numero di parametri
  \return true ok
  \return false error
  
  Preleva i parametri e imposta i valori delle variabili globali, in caso di
  errore (es. un parametro ha valore errato) restituisce false.
*/
bool getParameters(int argc, char* argv[]) {
  // Valori di default
  ROWS = 0;
  COLUMNS = 0;
  DENSITY = 0;
  ITERATIONS = 1;
  PRINT_MATRIX = false;
  PRINT_CTIMES = false;

  // Preleva i parametri
  extern char *optarg;
  extern int optopt;
  bool rflg=0, cflg=0, dflg=0, errflg=0;
  int opt;
  while ((opt = getopt(argc, argv, ":r:c:d:i:pth")) != -1) {
    switch(opt) {
      case 'r':
        rflg = 1;
        ROWS = atoi(optarg);
        break;
      case 'c':
        cflg = 1;
        COLUMNS = atoi(optarg);
        break;
      case 'd':
        dflg = 1;
        DENSITY = atof(optarg);
        break;
      case 'i':
        ITERATIONS = atoi(optarg);
        break;
      case 'p':
        PRINT_MATRIX = true;
        break;
      case 't':
        PRINT_CTIMES = true;
        break;
      case 'h':
        errflg = 1;
        break;
      case ':':
        if(MSL_myId == 0)
          std::cout <<"Option -" <<char(optopt) <<" requires an operand. \n";
        errflg = 1;
        break;
      case '?':
        if(MSL_myId == 0)
          std::cout<<"Unrecognized option: -" <<char(optopt) <<".\n";
        errflg = 1;
        break;
    } // end switch
  } // end while

  // Controlla i parametri
  if(errflg) {
    if(MSL_myId == 0) printHelp();
    return false;
  }
  if(!rflg || !cflg || !dflg) {
    if(MSL_myId == 0) {
      std::cout <<"Missing parameters." <<std::endl;
      printHelp();
    }
    return false;
  }
  if(COLUMNS < 2 || ROWS < 2) {
    if(MSL_myId == 0)
      std::cout <<"Matrix dimensions MUST BE at least 2x2." <<std::endl;
    return false;
  }
  if(DENSITY < 0 || DENSITY > 1) {
    if(MSL_myId == 0)
      std::cout <<"Density must be a number between 0 and 1." <<std::endl;
    return false;
  }
  if(MSL_numOfTotalProcs < 3 || MSL_numOfTotalProcs-2 > COLUMNS) {
    if(MSL_myId == 0)
      std::cout <<"Attention, the number of processes MUST BE greater or "
          <<"equals to 3 and at most " <<COLUMNS+2 <<" (i.e. number of "
          <<"columns plus two processes)." <<std::endl;
    return false;
  }
  if(ITERATIONS <= 0) ITERATIONS = 1;
  
  return true;
} // end of function getParameters

/*!
  \fn void printProgramInfo()
  \brief Stampa su standard output i parametri del programma
*/
void printProgramInfo() {
  std::cout <<MSL_numOfTotalProcs <<" process elements, "
      <<N_WORKERS <<" workers." <<std::endl
      <<ROWS <<"x" <<COLUMNS <<" matrix, with density " <<DENSITY <<"."
      <<std::endl
      <<ITERATIONS <<" iterations to compute." <<std::endl;
  return;
} // end of function printProgramInfo

/*!
  \fn void printComputationTimes()
  \brief Stampa su standard output i tempi di computazione
*/
void printComputationTimes() {
  double t1 = T_START.tv_sec+(T_START.tv_usec/1000000.0);
  double t2 = T_END.tv_sec+(T_END.tv_usec/1000000.0);
  std::cout <<"PE" <<MSL_myId <<": "
      <<"finish in " <<t2-t1 <<" seconds - "
      <<"cpu usage " <<((C_END-C_START)/double(CLOCKS_PER_SEC)) <<" seconds" 
      <<std::endl;
  return;
} // end of function printComputationTimes

/*!
  \fn void printHelp(
  \brief Stampa su standard output l'help del programma
*/
void printHelp() {
  std::cout <<"Usage: \n"
      <<"  mpiexec -n <procs> ./game-of-life OPTIONS \n"
      <<"Options:\n"
      <<"  <procs>        number of processes for use in computation. "
      <<"At least 3: initial\n"
      <<"                 stage, final stage, and at least one worker.\n"
      <<"  -r <rows>      number of rows in the matrix (at least 2).\n"
      <<"  -c <cols>      number of columns in the matrix (at least 2).\n"
      <<"  -d <dens>      density of live cells in the matrix. Number between "
      <<"0 and 1.\n"
      <<"  [-i <iters>]   number of iterations (generations in the Game of "
      <<"Life) to\n"
      <<"                 execute on the matrix. 1 is the default value.\n"
      <<"  [-p]           prints on standard output the initial and final "
      <<"matrix.\n"
      <<"  [-t]           calculates and prints on standard output the times "
      <<"of\n"
      <<"                 computations of each processes.\n"
      <<"  [-h]           prints this help message."
      <<std::endl;
  return;
} // end of function printHelp

