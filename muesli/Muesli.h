#ifndef MUESLI_H
#define MUESLI_H

//***********************************************************************************
//*                                                                                 *
//*         C++ Draft Proposal of MueSLi v1.79 (Muenster Skeleton Library)          *
//*                                                                                 *
//*                   (c) Michael Poldner   and   Herbert Kuchen                    *
//*                        {poldner|kuchen}@uni-muenster.de                         *
//*                                  Nov. 2001-2008                                 *
//*                                                                                 *
//* including several improvements by Joerg Striegnitz <J.Striegnitz@fz-juelich.de> *
//*        and a proper formating of the source code by Philipp Ciechanowicz        *
//*                                                                                 *
//***********************************************************************************

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

#include "Collection.h"
#include "Curry.h"
#include "Exception.h"
#include "mpi.h"
#include "omp.h"

#ifdef DEBUG
	static int Debug = DEBUG;
#else
	static int Debug = 0;
#endif

#ifdef DEBUG
#  if DEBUG > 0
#    define dbg(x) x
#  else
#    define dbg(x)
#  endif
#else
#  define dbg(x)
#endif

//***********************************************************************************************************
//* global constants
//***********************************************************************************************************

#define MSL_VERSION 79	// 1.xx

#define MSLT_ANY_TAG MPI_ANY_TAG	// MPI_ANY_TAG = -1; DO NOT ASSIGN -1 to ANY OTHER TAG!!!

#define MSLT_MYTAG 1 				// is used for ordinary messages containing data.
#define MSLT_STOP 2					// is used to stop the following process;
#define MSLT_TERMINATION_TEST 3
#define MSLT_TOKEN_TAG 4
#define MSLT_PROBLEM_TAG 5
#define MSLT_BBINCUMBENT_TAG 6
#define MSLT_LB_TAG 7
#define MSLT_SOLUTION 8
#define MSLT_SUBPROBLEM 9
#define MSLT_WORKREQUEST 10
#define MSLT_REJECTION 11
#define MSLT_SENDREQUEST 12
#define MSLT_READYSIGNAL 13
#define MSLT_BB_TERMINATIONTOKEN 14			// Token zur Termination Detection
#define MSLT_BB_PROBLEM 15					// Problem, dass während Lastverteilung versendet wird
#define MSLT_BB_INCUMBENT 16				// aktuell gefundene beste Lösung
#define MSLT_BB_LOADBALANCE 17				// Nachricht mit der aktuell besten Abschätzung
#define MSLT_BB_LOADBALANCE_REJECTION 18				// Nachricht mit der aktuell besten Abschätzung
#define MSLT_BB_INCUMBENT_SENDREQUEST 19	// zur Anfrage, ob ein Incumbent gesendet werden darf
#define MSLT_BB_INCUMBENT_READYSIGNAL 20	// Antwort, dass das Incumbent gesendet werden darf
#define MSLT_BB_PROBLEM_SENDREQUEST 21		// zur Anfrage, ob ein Problem gesendet werden darf
#define MSLT_BB_PROBLEM_READYSIGNAL 22		// Antwort, das das Problem gesendet werden darf
#define MSLT_BB_PROBLEM_SOLVED 23
#define MSLT_BB_STATISTICS 24

#define MSLT_ALLGATHER 50			// tag used by the MSL_Allgather function
#define MSLT_BROADCAST 51			// tag used by the MSL_Broadcast function
#define MSLT_BROADCAST_SERIAL 52	// tag used by the MSL_BroadcastSerial function; currently not in use
#define MSLT_ROTATE 53				// tag used by the rotate function

static const int MSL_BB_TOPOLOGY_ALLTOALL = 1;
static const int MSL_BB_TOPOLOGY_HYPERCUBE = 2;
static const int MSL_BB_TOPOLOGY_RING = 3;

/* deprecated */ static const bool MSL_NOT_SERIALIZED = false;		 // deaktiviert serialisierten Kommunikationmodus (Arg. für InitSkeletons)
/* deprecated */ static const bool MSL_SERIALIZED = true; 			 // aktiviert serialisierten Kommunikationmodus (Arg. für InitSkeletons)
/* deprecated */ static bool MSL_COMMUNICATION = MSL_NOT_SERIALIZED; // Default

static const int 	MSL_RANDOM_DISTRIBUTION = 1;
static const int 	MSL_CYCLIC_DISTRIBUTION = 2;
static const int 	MSL_DEFAULT_DISTRIBUTION = MSL_CYCLIC_DISTRIBUTION;
static int 			MSL_DISTRIBUTION_MODE; 							// Modus wird initSkeletons als Parameter bergeben
static const bool 	MSL_TIMER = false; 								// aktiviert/deativiert die Zeitmessung
static const int 	MSL_UNDEFINED = -1;

static int numP = 0, numS = 0, numPF = 0, numSF = 0; // TODO DELETE IN DCSOLVER TODO

typedef int ProcessorNo; 											// Typ einer ProcessorID
/* deprecated */ static ProcessorNo MSL_myEntrance = MSL_UNDEFINED;	// = -1 // verweist auf den Prozessor, der den Eingang eines Skeletts darstellt
/* deprecated */ static ProcessorNo MSL_myExit = MSL_UNDEFINED;  	// verweist auf den Prozessor, der den Ausgang eines Skeletts darstellt
class Process; 														// Vorwaertsdeklaration
static Process* 	MSL_myProcess = NULL; 							// Zeiger auf die Prozessklasse zu dem auch MSL_myId gehört
static ProcessorNo 	MSL_runningProcessorNo = 0; 					// Anzahl der zu einem Zeitpunkt arbeitender/integrierter Prozessoren (dynamisch)
static ProcessorNo 	MSL_myId = MSL_UNDEFINED;  						// ID des betrachteten Processes
static int 			MSL_numOfLocalProcs = 1; 						// number of processes used by atomic skeleton where MSL_myId collaborates
static int 			MSL_numOfTotalProcs;    						// total number of processors used

// ***********************************************************************************************************

// Oberklasse aller serialisierbaren Klassen
class MSL_Serializable {

// Message vererbt den Zeiger "this" (4 Byte)
public:

	MSL_Serializable() {
	}

	virtual ~MSL_Serializable() {
		/* std::cout << "Message-Destruktor aufgerufen" << std::endl; */
	}

	virtual int getSize() = 0;
	virtual void reduce(void* pBuffer, int bufferSize) = 0;
	virtual void expand(void* pBuffer, int bufferSize) = 0;

};

// ***********************************************************************************************************

// liefert true, wenn U öffentlich von T erbt oder wenn T und U den gleichen Typ besitzen
#define MSL_IS_SUPERCLASS(T, U) (MSL_Conversion<const U*, const T*>::exists && !MSL_Conversion<const T*, const void*>::sameType)

// MSL_Conversion erkennt zur Compilezeit, ob sich T nach U konvertieren lässt.
// exists = true, wenn sich T nach U konvertieren lässt, sonst false
template<class T, class U>
class MSL_Conversion {

private:

	// sizeof(Small) = 1
	typedef char Small;
	
	// sizeof(Big) > 1
	class Big {
		char dummy[2];
	};

	static Small Test(U);			// Compiler wählt diese Funktion, wenn er eine Umwandlung von T nach U findet
	static Big Test(...); 			// sonst nimmer er diese
	static T MakeT(); 				// Erzeugt ein Objekt vom Typ T, selbst wenn der Konstruktor als private deklariert wurde

public:

	enum {
		exists = sizeof(Test(MakeT())) == sizeof(Small)
	};

	enum {
		sameType = false
	};

};

// Überladung von MSL_Conversion, um den Fall T = U zu erkennen
template<class T>
class MSL_Conversion<T, T> {

public:

	enum {exists = true, sameType = true
	};

};

// MSL_Int2Type erzeugt aus ganzzahligen Konstanten einen eigenen Typ.
// wird benötigt, damit der Compiler zur Compilezeit die korrekte MSL_Send Methode auswählen kann.
template<int v>
struct MSL_Int2Type {

	enum {
		value = v
	};

};

// ***********************  auxiliary functions  *********************

/* Function used to determine the number of non-zero elements
   of the distributed sparse matrix (cf. getElementCount and
   MSL_Allreduce).

   author: Philipp
*/
template<typename T>
T add(T a, T b) {
	return a + b;
}

// assumption: n > 0
inline int log2(int n) {
	int i;
	
	for(i = 0; n > 0; i++)
		n /= 2;
	
	return i - 1;
}

inline void throws(const Exception& e) {
	std::cout << MSL_myId << ": " << e << std::endl << std::flush;
	#if(! defined(__KCC	) && ! defined(_CRAYC) ) // ???
    //  if(! Debug) throw e;
    // if(! Debug) throw SkeletonException();
	#endif
}

template<class C1, class C2>
inline C1 proj1_2(C1 a, C2 b) {
	return a;
}

template<class C1, class C2>
inline C2 proj2_2(C1 a, C2 b) {
	return b;
}

// needed in nullary argument functions of skeletons;  otherwise
struct Empty {
};

template<class C> // wofür soll das gut sein ???
inline C extend(C (*f)(), Empty dummy) {
	return (*f)();
}

template<class F>
inline static int auxRotateRows(const Fct1<int, int, F>& f, int blocks, int row, int col) {
	return (col + f(row) + blocks) % blocks;
}

template<class F>
inline static int auxRotateCols(const Fct1<int, int, F>& f, int blocks, int row, int col) {
	return (row + f(col) + blocks) % blocks;
}

inline bool MSL_isSerialized() {
	return MSL_COMMUNICATION == MSL_SERIALIZED;
}

// ***********************************************************************************************************
// MSL_Send

// Implementierung von MSL_Send für zu serialisierende Objekte
template<class Data>
inline void MSL_Send(ProcessorNo destination, Data* pData, int tag, MSL_Int2Type<true>) {
	// std::cout << "MSL_Send für zu serialisierende Objekte" << std::endl;
   	int size = pData->getSize();
   	void* buffer = malloc(size + 10);

	if(buffer == NULL)
		std::cout << "OUT OF MEMORY ERROR in MSL_Send: malloc returns NULL" << std::endl;

	pData->reduce(buffer,size);
//	std::cout << MSL_myId << ": MSL_Send - verschicke Nachricht an " << destination << " ... " << std::endl;
	MPI_Send(buffer, size, MPI_BYTE, destination, tag, MPI_COMM_WORLD);
//	std::cout << MSL_myId << ": MSL_Send - fertig" << std::endl;
	free(buffer); 														//std::cout << "MSL_Send: Puffer gelöscht" << std::endl;
}

// Implementierung von MSL_Send für bereits serialisierte Objekte
template<class Data>
inline void MSL_Send(ProcessorNo destination, Data* pData, int tag, MSL_Int2Type<false>) {
	// std::cout << "MSL_Send für bereits serialisierte Objekte" << std::endl;
	MPI_Send(pData, sizeof(Data), MPI_BYTE, destination, tag, MPI_COMM_WORLD);
}

// Allg. Send für serialisierte und nicht serlialisierte Objekte.
// Diese Methode kann innerhalb der Skelette aufgerufen werden. Der Compiler sucht sich beim Compilieren automatisch
// die zu Data passende MSL_Send-Implementierung raus.
template<class Data>
inline void MSL_Send(ProcessorNo destination, Data* pData, int tag = MSLT_MYTAG) {
	if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

	MSL_Send(destination, pData, tag, MSL_Int2Type<MSL_IS_SUPERCLASS(MSL_Serializable, Data)>() );
}

// ***********************************************************************************************************
// MSL_Receive

// Implementierung von MSL_Receive für zu serialisierende Objekte
template<class Data>
inline void MSL_Receive(ProcessorNo source, Data* pData, int tag, MPI_Status* pStatus, MSL_Int2Type<true>) {
	// std::cout << "MSL_Receive für zu serialisierende Objekte" << std::endl;
	MPI_Probe(source, tag, MPI_COMM_WORLD, pStatus);
    int size = 1;
    MPI_Get_count(pStatus, MPI_BYTE, &size);
	void* buffer = malloc(size); 		// TODO: Optimierung: Buffer wird wiederverwendet -> InitSkeleton(..., int bufSize)
	
	if(buffer == NULL)
		std::cout << "OUT OF MEMORY ERROR in MSL_Receive: malloc returns NULL" << std::endl;

	//std::cout << MSL_myId << ": MSL_Recv - empfange Nachricht von " << source << " ... " << std::endl;
    MPI_Recv(buffer, size, MPI_BYTE, source, tag, MPI_COMM_WORLD, pStatus);
	//std::cout << MSL_myId << ": MSL_Recv - fertig" << std::endl;
    pData->expand(buffer,size);
    free(buffer);
}

// Implementierung von MSL_Receive für bereits serialisierte Objekte
template<class Data>
inline void MSL_Receive(ProcessorNo source, Data* pData, int tag, MPI_Status* pStatus, MSL_Int2Type<false>) {
	// std::cout << "MSL_Receive für bereits serialisierte Objekte" << std::endl;
	MPI_Recv(pData, sizeof(Data), MPI_BYTE, source, tag, MPI_COMM_WORLD, pStatus);
}

// Allg. Send für serialisierte und nicht serlialisierte Objekte.
// Diese Methode kann innerhalb der Skelette aufgerufen werden. Der Compiler sucht sich beim Compilieren automatisch
// die zu Data passende MSL_Receive-Implementierung raus.
template<class Data>
inline void MSL_Receive(ProcessorNo source, Data* pData, int tag, MPI_Status* pStatus) {
	if(source == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

	MSL_Receive(source, pData, tag, pStatus, MSL_Int2Type<MSL_IS_SUPERCLASS(MSL_Serializable, Data)>() );
}

// ***********************************************************************************************************
// MSL_SendTag / MSL_ReceiveTag

// Verschickt Nachrichten ohne Inhalt, die mit einem bestimmten Tag versehen sind. (Protokoll)
inline void MSL_SendTag(ProcessorNo destination, int tag) {
   if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

   int dummy;
   MPI_Send(&dummy, sizeof(dummy), MPI_BYTE, destination, tag, MPI_COMM_WORLD);
}

// Empfängt Nachrichten ohne Inhalt, die mit einem bestimmten Tag versehen sind. (Protokoll)
inline void MSL_ReceiveTag(ProcessorNo source, int tag) {
   if(source == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

   int dummy;
   MPI_Status status;
   MPI_Recv(&dummy, sizeof(int), MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
}

// ***********************************************************************************************************

/* deprecated */  // blockierendes, asynchrones Senden eines Tokens im rahmen des Termination Detection Alg.
inline void sendLBInfo(int value, ProcessorNo destination) {
   if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());
   //std::cout <<"sendToken: "<< MSL_myId << " schickt Nachricht mit Wert=" << value << " und TAG="<<MSLT_TOKEN_TAG<<" an " << destination << std::endl;
   MPI_Send(&value, sizeof(int), MPI_BYTE, destination, MSLT_LB_TAG, MPI_COMM_WORLD);
}

/* deprecated */ // -> MSL_Send
template<class C>
inline void sendProblem(ProcessorNo destination, C* valref, int size) {
   if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

   dbg(std::cout << MSL_myId << " sending  message of size " << size << " to " << destination << std::endl << std::flush);
   MPI_Send(valref, size, MPI_BYTE, destination, MSLT_PROBLEM_TAG, MPI_COMM_WORLD);
}

/* deprecated */ // -> MSL_Send
template<class C> // blockierendes, asynchrones Send
inline void sendIncumbent(ProcessorNo destination, C* valref, int size) {
	if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

	dbg(std::cout << MSL_myId << " sending  message of size " << size << " to " << destination << std::endl << std::flush);
	MPI_Send(valref, size, MPI_BYTE, destination, MSLT_BBINCUMBENT_TAG, MPI_COMM_WORLD);
}

/* deprecated */ 	// blockierendes, asynchrones Senden eines Tokens im rahmen des Termination Detection Alg.
inline void sendToken(int value, ProcessorNo destination) {
   if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

   //std::cout <<"sendToken: "<< MSL_myId << " schickt Nachricht mit Wert=" << value << " und TAG="<<MSLT_TOKEN_TAG<<" an " << destination << std::endl;
   MPI_Send(&value, sizeof(int), MPI_BYTE, destination, MSLT_TOKEN_TAG, MPI_COMM_WORLD);
}

/* deprecated */	// blockierendes, asynchrones Send eines MSLT_STOP an den Prozessor mit der Processornummer "destination"
inline void sendStop(ProcessorNo destination) {
    if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

    dbg(std::cout << MSL_myId << " sending  stop message "  << " to " << destination << std::endl << std::flush);
    int dummy;
    MPI_Send(&dummy, sizeof(dummy), MPI_BYTE, destination, MSLT_STOP, MPI_COMM_WORLD);
}

/* deprecated */	// blockierendes, asynchrones Send eines MSLT_TERMINATION_TEST an den Prozessor mit der Processornummer "destination"
inline void sendTerminationTest(ProcessorNo destination) {
   	if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

   	dbg(std::cout << MSL_myId << " sending termination test message " << " to " << destination << std::endl << std::flush);
   	int dummy;
	MPI_Send(&dummy, sizeof(dummy), MPI_BYTE, destination, MSLT_TERMINATION_TEST, MPI_COMM_WORLD);
}

/* deprecated */
inline void MSL_sendWorkRequest(ProcessorNo destination) {
   if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

   int dummy;
   MPI_Send(&dummy, sizeof(dummy), MPI_BYTE, destination, MSLT_WORKREQUEST, MPI_COMM_WORLD);
}

/* deprecated */	// verschickt eine Absage (von was auch immer)
inline void MSL_sendRejection(ProcessorNo destination) {
   if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

   //std::cout <<"sendToken: "<< MSL_myId << " schickt Nachricht mit Wert=" << value << " und TAG="<<MSLT_TOKEN_TAG<<" an " << destination << std::endl;
   int dummy = 0;
   MPI_Send(&dummy, sizeof(int), MPI_BYTE, destination, MSLT_REJECTION, MPI_COMM_WORLD);
}

/* deprecated */
inline void MSL_receiveRejection(ProcessorNo source) {
   if(source == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

   int dummy;
   MPI_Status status;
   MPI_Recv(&dummy, sizeof(int), MPI_BYTE, source, MSLT_REJECTION, MPI_COMM_WORLD, &status);
}

/* deprecated */ // -> MSL_Send
// blockierendes, asynchrones Send / Receive
template<class C> // @deprecated
inline void MSL_send(ProcessorNo destination, C* valref, int size) {
    if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

    dbg(std::cout << MSL_myId << " sending  message of size " << size << " to " << destination << std::endl << std::flush);
	MPI_Send(valref, size, MPI_BYTE, destination, MSLT_MYTAG, MPI_COMM_WORLD);
}
/* deprecated */ // -> MSL_Send
template<class C> // versieht Nachricht mit dem übergebenen Tag // @deprecated
inline void MSL_send(ProcessorNo destination, C* valref, int size, int tag) {
    if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

    dbg(std::cout << MSL_myId << " sending  message of size " << size << " to " << destination << std::endl << std::flush);
	MPI_Send(valref, size, MPI_BYTE, destination, tag, MPI_COMM_WORLD);
}

/* deprecated */ // -> MSL_Receive
template<class C>
inline void MSL_receive(ProcessorNo source, C* valref, int size, MPI_Status* stat) {
	if(source == MSL_UNDEFINED)
		throws(UndefinedSourceException());

    dbg(std::cout << MSL_myId << " waiting  for message of size " << size << " from " << source << std::endl << std::flush);
    MPI_Recv(valref, size, MPI_BYTE, source, MPI_ANY_TAG, MPI_COMM_WORLD, stat);
    dbg(std::cout << MSL_myId << " received  message of size "  << size << " with tag " << (*stat).MPI_TAG << " from " << source << std::endl << std::flush);
}
/* deprecated */ // -> MSL_Receive
template<class C> // empfängt nur Nachrichten mit dem übergebenen Tag
inline void MSL_receive(ProcessorNo source, C* valref, int size, int tag, MPI_Status* stat) {
	if(source == MSL_UNDEFINED)
		throws(UndefinedSourceException());

    dbg(std::cout << MSL_myId << " waiting  for message of size " << size << " from " << source << std::endl << std::flush);
    MPI_Recv(valref, size, MPI_BYTE, source, tag, MPI_COMM_WORLD, stat);
    dbg(std::cout << MSL_myId << " received  message of size "  << size << " with tag " << (*stat).MPI_TAG << " from " << source << std::endl << std::flush);
}

/* deprecated */ // -> MSL_Send
// nichtblockierendes, asynchrones Send / Receive
template<class C>
inline void  nonblockingSend(ProcessorNo destination, C* valref, int size, MPI_Request* req) {
    if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

    dbg(std::cout << MSL_myId << " sending  message of size " << size << " to " << destination << std::endl << std::flush);
    MPI_Isend(valref, size, MPI_BYTE, destination, MSLT_MYTAG, MPI_COMM_WORLD, req);
}
/* deprecated */ // -> MSL_Receive
template<class C>
inline void nonblockingReceive(ProcessorNo source, C* valref, int size, MPI_Request* req) {
    if(source == MSL_UNDEFINED)
		throws(UndefinedSourceException());

    //        buffer, numOfElem, Datatype, source, tag,         Communicator  , Request-Object
	MPI_Irecv(valref, size, MPI_BYTE, source, MPI_ANY_TAG, MPI_COMM_WORLD, req);
}

// **********************************************************************************************************************

/* deprecated */ // -> MSL_Send
// Serialisiertes Send / Receive
// MSL_SendPacked fordert Speicherplatz an, in welchen das Objekt serialisiert werden kann.
// Nach dem Versenden wird der Speicherbereich wieder freigegeben.
template<class C>
inline void MSL_SendPacked(ProcessorNo destination, C* valref) {
   	if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

   	int size = valref->getSize();
   	void* buffer = malloc(size + 10);

	if(buffer == NULL)
		std::cout << "ERROR: malloc returns NULL" << std::endl;
	else
		std::cout << "MSL_SendPacked: " << size << " bytes erfolgreich angefordert." << std::endl;

   	valref->reduce(buffer,size);
	std::cout << "MSL_SendPacked: Nachricht gepackt" << std::endl;
   	MPI_Send(buffer, size, MPI_BYTE, destination, MSLT_MYTAG, MPI_COMM_WORLD);
	std::cout << "MSL_SendPacked: Nachricht verschickt" << std::endl;
   	free(buffer); // Freigabe möglich, da MPI_Send benutzt
	std::cout << "MSL_SendPacked: Puffer gelöscht" << std::endl;
}

/* deprecated */ // -> MSL_Receive
template<class C>
inline void MSL_ReceivePacked(ProcessorNo source, C* valref, MPI_Status* status) {
    if(source == MSL_UNDEFINED)
		throws(UndefinedSourceException());

	MPI_Probe(source, MPI_ANY_TAG, MPI_COMM_WORLD, status);
    int size = 1;
    MPI_Get_count(status, MPI_BYTE, &size);
	void* buffer = malloc(size); // TODO: Optimierung: Buffer wird wiederverwendet -> InitSkeleton(..., int bufSize)
	
	if(buffer == NULL)
		std::cout << "ERROR: malloc returns NULL" << std::endl;

    MPI_Recv(buffer, size, MPI_BYTE, source, MPI_ANY_TAG, MPI_COMM_WORLD, status);
	std::cout << "MSL_ReceivePacked: Nachricht empfangen." << std::endl;
    valref->expand(buffer,size);
	std::cout << "MSL_ReceivePacked: Nachricht entpackt." << std::endl;
    free(buffer); // Freigabe möglich, da MPI_Send benutzt; TODO: Optimierung: Buffer wird wiederverwendet
	std::cout << "MSL_ReceivePacked: Puffer gelöscht" << std::endl; // Problem???
}

/* deprecated */ // -> MSL_Send
template<class C> // versendet Nachricht mit dem übergebenen Tag
inline void MSL_SendPacked(ProcessorNo destination, C* valref, int tag) {
   	if(destination == MSL_UNDEFINED)
		throws(UndefinedDestinationException());

   	//std::cout << "sendPacked+TAG" << std::endl;
   	// alt:
   	//int size = 0;
   	//void* buffer = valref->reduce(&size);

   	// neu:
    int size = valref->getSize();
   	//int size = 262144;
    // std::cout << "valref->getSize(): " << size << " byte" << std::endl;
   	void* buffer = malloc(size);
	
	if(buffer == NULL)
		std::cout << "ERROR: malloc returns NULL" << std::endl;

   	valref->reduce(buffer,size);
   	MPI_Send(buffer, size, MPI_BYTE, destination, tag, MPI_COMM_WORLD);
   	free(buffer);
}

/* deprecated */ // -> MSL_Receive
template<class C> // empfängt nur Nachrichten mit dem übergebenen Tag
inline void MSL_ReceivePacked(ProcessorNo source, C* valref, int tag, MPI_Status* status) {
   	//std::cout << "recvPacked+TAG" << std::endl;
    if(source == MSL_UNDEFINED)
		throws(UndefinedSourceException());

    // Warte darauf, dass eine Nachricht ankommt
	MPI_Probe(source, MPI_ANY_TAG, MPI_COMM_WORLD, status);
	// berechne die Größe der Nachricht und fordere Speicherplatz an
    int size = 1;
//  int size = 262144;
    MPI_Get_count(status, MPI_BYTE, &size);
    std::cout << "MPI_Get_count(status, MPI_BYTE, &size): " << size << " byte" << std::endl;
	void* buffer = malloc(size*2); // TODO: Optimierung: Buffer wird wiederverwendet -> InitSkeleton(..., int bufSize)
	
	if(buffer == NULL)
		std::cout << "ERROR: malloc returns NULL" << std::endl;

    // Nachricht entgegennehmen und entpacken
    MPI_Recv(buffer, size, MPI_BYTE, source, tag, MPI_COMM_WORLD, status);
    valref->expand(buffer,size);
    // Speicher feigeben
    free(buffer); 				 // TODO: Optimierung: Buffer wird wiederverwendet
}

// blockierendes synchrones Send mit blockierendem asynchronen recieve
template<class C>
inline void sendReceive(ProcessorNo destination, C* valref1, C* valref2, int size) {  //  synchronous!
	if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

	MPI_Status stat;
	// original attempt: (causes deadlock)
	// MPI_Sendrecv(valref1, size, MPI_BYTE, destination, MSLT_MYTAG, valref2, size, MPI_BYTE, MSL_myId,  MSLT_MYTAG, MPI_COMM_WORLD, &stat);
	// 2nd attempt: (works)
	if(destination > MSL_myId) {
	   syncSend(destination, valref1, size);
	   MSL_receive(destination, valref2, size, &stat);
	}
	else {
	   MSL_receive(destination, valref2, size, &stat);
	   syncSend(destination, valref1, size);
	}
}

// blockierendes, synchrones Send
template<class C>
inline void syncSend(ProcessorNo destination, C* valref, int size) {
	if(destination == MSL_UNDEFINED)
	   throws(UndefinedDestinationException());

	dbg(std::cout << MSL_myId << " sending  message of size " << size << " synchronously to " << destination << std::endl << std::flush);
	MPI_Ssend(valref, size, MPI_BYTE, destination, MSLT_MYTAG, MPI_COMM_WORLD);
}

// **********************************************************************************************************
// *********************  classes and auxiliary functions for object serialization **************************
// **********************************************************************************************************

// **** Senden und Empfangen von gepackten Daten

// Sendepuffer
// #define MSL_BUFFERSIZE 67108864 // 64 MB
// void* MSL_Buffer;

// Pack-Methoden zur Serialisierung
inline void MSL_Pack(signed short int* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_SHORT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(signed int* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_INT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(signed char* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_CHAR, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(signed long int* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_LONG, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(unsigned char* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_UNSIGNED_CHAR, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(unsigned short* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_UNSIGNED_SHORT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(unsigned int* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_UNSIGNED, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(unsigned long int* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_UNSIGNED_LONG, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(float* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_FLOAT, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(double* val, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(val, 1, MPI_DOUBLE, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

inline void MSL_Pack(long double* value, void* pBuffer, int bufferSize, int* position) {
	MPI_Pack(value, 1, MPI_LONG_DOUBLE, pBuffer, bufferSize, position, MPI_COMM_WORLD);
}

// Unpack-Methoden zur Serialisierung
inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed short int* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_SHORT, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed int* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_INT, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed char* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_CHAR, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, signed long int* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_LONG, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned char* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned short* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED_SHORT, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned int* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, unsigned long int* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, float* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_FLOAT, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, double* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_DOUBLE, MPI_COMM_WORLD);
}

inline void MSL_Unpack(void* pBuffer, int bufferSize, int* position, long double* val) {
	MPI_Unpack(pBuffer, bufferSize, position, val, 1, MPI_LONG_DOUBLE, MPI_COMM_WORLD);
}

// ***************************************************************************************************
// *** 									         FRAME          								   ***
// ***************************************************************************************************

// Ein Frame für das interne Versenden von serialisierten Objekten, die mit weiteren Informationen ausgestattet werden müssen
// Idee: Reserviere Speicherbereich, der den kompletten Frame inkl. serialisieren Datenteil aufnehmen kann.
// An Anfang stehen die Frame-internen Daten, dann folgen die Nutzerdaten.
// Die Adresse, ab der die Nutzdaten stehen können, wird der reduce-Methode übergeben, die der Nutzer implementieren muss.
template<class Data>
class Frame: public MSL_Serializable {

private:

	long id;			// ID eines (Teil)Problems
	long rootNodeID;	// ID eines abgegebenen Teilproblems, das beim Empfänger als Wurzelknoten interpretiert wird
	long originator;	// Absender eines abgegebenen Teilproblems
	long poolID;		// ProzessorID des Initialproblems, aus dem das Teilproblem hervorgegangen ist

	Data* pData; 		// void* pData; ??? -> Nein, da auf dem Data-Objekt reduce/expand aufgerufen werden muss, was sonst nicht geht!

public:
	
	Frame(long myId, long origID, long snd, long pool, Data* myData):
	id(myId), rootNodeID(origID), originator(snd), poolID(pool), pData(myData) {
	}

	// used in the scope of serialization
	Frame(): id(-1), rootNodeID(-1), originator(-1), poolID(-1), pData(NULL) {
	}

	~Frame(void) {
		// std::cout << MSL_myId << ": (~Frame) deleting frame" << std::endl;
		// delete pData;
	}

	inline long getID() {
		return id;
	}

	inline long getRootNodeID() {
		return rootNodeID;
	}

	inline long getOriginator() {
		return originator;
	}

	inline long getPoolID() {
		return poolID;
	}

	inline Data* getData() {
		return pData;
	}

	inline void setID(long i) {
		id = i;
	}

	inline void setRootNodeID(long oid) {
		rootNodeID = oid;
	}

	inline void setOriginator(int o) {
		originator = (long)o;
	}

	inline void setPoolID(int id) {
		poolID = (long)id;
	}

	inline void setData(Data* d) {
		pData = d;
	}

	// liefert die Größe des Frames in Bytes im serialisierten Zustand
	inline int getSize() {
		// Wenn die Kommunikation serialisiert erfolgt, dann ist Data von MSL_Serializable abgeleitet und ein reduce/expand/getSize existiert
		if(MSL_isSerialized()) return 4*sizeof(long) + pData->getSize();
		else /* !serialized */  return 4*sizeof(long) + sizeof(Data);
	}

	// Serialisiert den Frame
	void reduce(void* pBuffer, int bufferSize) {
		//std::cout << "serialisiere Frame" << std::endl;
		// frameinterne Dinge kopieren
		long* pos = (long*) memcpy(pBuffer, &(this->id), sizeof(long));
		pos++;
		pos = (long*) memcpy(pos, &(this->rootNodeID), sizeof(long));
		pos++;
		pos = (long*) memcpy(pos, &(this->originator), sizeof(long));
		pos++;
		pos = (long*) memcpy(pos, &(this->poolID), sizeof(long));
		pos++;

		// user data kopieren
		if(MSL_isSerialized())
			pData->reduce(pos, bufferSize - (4 * sizeof(long)));
		else
			memcpy(pos, pData, sizeof(Data)); // void* memcpy(void* dest, const void* source, size_t num);
	}

	// entpackt den Frame
	void expand(void* pBuffer, int bufferSize) {
		//std::cout << "deserialisiere Frame" << std::endl;
		// frameinterne Daten entpacken
		long* pos = (long*) pBuffer;
		id = *pos; 			pos++;
		rootNodeID = *pos; 	pos++;
		originator = *pos; 	pos++;
		poolID = *pos; 		pos++;
		// address of user data
		pData = new Data(); // Platz für die Solution/das Problem schaffen

		// Userdata kopieren
		if(MSL_isSerialized())
			pData->expand(pos, bufferSize - 4 * sizeof(long));
		else
			memcpy(pData, pos, sizeof(Data)); // void* memcpy(void* dest, const void* source, size_t num);
	}

}; // Frame

/**
 * Generische Klasse, die ein Objekt des angegebenen Datentyps kapselt.  Die Klasse kann serialisiert versendet werden
 * und initiiert dabei zugleich die Serialisierung der gekapselten Daten, sofern diese ebenfalls
 * von der Klasse MSL_Serializable abgeleitet sind.
 * */
template<class Data>
class BBFrame: public MSL_Serializable {

private:

	unsigned long id;			// ID eines (Teil)Problems
	BBFrame* parentProblem;	// Zeiger auf das Vaterproblem
	long originator;	// Absender eines abgegebenen Teilproblems
	int numOfSubProblems;		// Anzahl der erzeugten Kindprobleme
	int numOfSubProblemsSolved; // Anzahl der gelösten Kindprobleme
	Data* pData; 		// void* pData; ??? -> Nein, da auf dem Data-Objekt reduce/expand aufgerufen werden muss, was sonst nicht geht!
	static const int internalSize = sizeof(long) + sizeof(long) + 2 * sizeof(int) + sizeof(BBFrame*);

public:

	BBFrame(long myId, BBFrame* parent, long snd, int numSub, Data* myData):
	id(myId), parentProblem(parent), originator(snd), numOfSubProblems(numSub), pData(myData) {
	}

	BBFrame(): id(0), parentProblem(NULL), originator(-1), numOfSubProblems(-1), numOfSubProblemsSolved(-1), pData(NULL) {
	}

	~BBFrame(void) {
		// delete pData;
	}

	inline unsigned long getID() {
		return id;
	}

	inline BBFrame* getParentProblem() {
		return parentProblem;
	}

	inline long getOriginator() {
		return originator;
	}

	inline int getNumOfSubProblems() {
		return numOfSubProblems;
	}

	inline int getNumOfSolvedSubProblems() {
		return numOfSubProblemsSolved;
	}

	inline Data* getData() {
		return pData;
	}

	inline void setID(unsigned long i) {
		id = i;
	}

	inline void setParentProblem(BBFrame* parent) {
		parentProblem = parent;
	}

	inline void setOriginator(int o) {
		originator = (long)o;
	}

	inline void setNumOfSubProblems(int num) {
		numOfSubProblems = num;
	}

	inline void setNumOfSolvedSubProblems(int num) {
		numOfSubProblemsSolved = num;
	}

	inline void setData(Data* d) {
		pData = d;
	}

	// liefert die Größe des Frames in Bytes im serialisierten Zustand
	inline int getSize() {
		//std::cout << "Frame getSize(); MSL_isSerialized; " << (MSL_isSerialized()) << "sizeof(data) " << sizeof(Data) << " MSL_COMMUNICTAIO " << MSL_COMMUNICATION << " MSL_SERIAL " << MSL_SERIALIZED << std::endl;

		// Wenn die Kommunikation serialisiert erfolgt, dann ist Data von MSL_Serializable abgeleitet und ein reduce/expand/getSize existiert
		if(pData == NULL)
			return internalSize;
		else if(MSL_isSerialized())
			return internalSize + (reinterpret_cast<MSL_Serializable*>(pData))->getSize();
		else /* !serialized */
			return internalSize + sizeof(Data);
	}

	// Serialisiert den Frame
	void reduce(void* pBuffer, int bufferSize) {
		long* pos = (long*) memcpy(pBuffer, &(this->id), sizeof(long));
		pos+=2; // long ist doppelt so groß wie long!
		pos = (long*) memcpy(pos, &(this->parentProblem), sizeof(parentProblem));
		pos++; // TODO check
		pos = (long*) memcpy(pos, &(this->originator), sizeof(long));
		pos++;
		pos = (long*) memcpy(pos, &(this->numOfSubProblems), sizeof(int));
		pos++;
		pos = (long*) memcpy(pos, &(this->numOfSubProblemsSolved), sizeof(int));
		pos++;

		// user data kopieren, falls welches zu senden ist
		if(bufferSize == internalSize)
			return;

		if(MSL_isSerialized())
			(reinterpret_cast<MSL_Serializable* >(pData))->reduce(pos,
			bufferSize - (sizeof(long) + sizeof(long) + 2 * sizeof(int) + sizeof(parentProblem))); // TODO: 2mal oder 3mal
		else
			memcpy(pos, pData, sizeof(Data)); // void* memcpy(void* dest, const void* source, size_t num);
	}

	// entpackt den Frame
	void expand(void* pBuffer, int bufferSize) {
		// frameinterne Daten entpacken
		long* pos = (long*) pBuffer;
		memcpy(&(this->id), pos, sizeof(long));
		pos+=2; // long ist doppelt so groß!
		memcpy(&(this->parentProblem), pos, sizeof(parentProblem));
		pos++;
		memcpy(&(this->originator), pos, sizeof(originator));
		pos++;
		memcpy(&(this->numOfSubProblems), pos, sizeof(numOfSubProblems));
		pos++;
		memcpy(&(this->numOfSubProblemsSolved), pos, sizeof(numOfSubProblemsSolved));
		pos++;

		if(bufferSize==internalSize) return;
		// address of user data
		pData = new Data(); // Platz für die Solution/das Problem schaffen
		// Userdata kopieren
		if(MSL_isSerialized())
			(reinterpret_cast<MSL_Serializable* >(pData))->expand(pos,
			bufferSize - (sizeof(long) + sizeof(long) + 2 * sizeof(int) + sizeof(parentProblem)));
		else
			memcpy(pData, pos, sizeof(Data)); // void* memcpy(void* dest, const void* source, size_t num);
	}

}; // BBFrame

// ***************************************************************************************************
// *** 									DATA PARALLEL SKELETONS  								   ***
// ***************************************************************************************************

// ***************************************************************************************************
// Distributed Arrays with data parallel skeletons map, fold, zip, gather, scan, permute, broadcast...
// ***************************************************************************************************

class Process; // forward declaration needed in constructor DistributedArray

template<class E>
class DistributedArray {

	int n;             // total number of elements of DistributedArray
  	E* a;              // E a[localsize];
  	int localsize;

  	// assumption: block division of array
  	int localposition; // position of processor in data parallel group of processors
  	int first;         // first index in local partition; assuming division mode: block
  	int nextfirst;     // first index in next partition


  private:

  	inline void init() {
    	if((MSL_myExit == MSL_UNDEFINED) || (MSL_myEntrance == MSL_UNDEFINED))
			throws(MissingInitializationException());

      	// for simplicity: assuming MSL_numOfLocalProcs is power of 2
      	localsize = n/MSL_numOfLocalProcs;  // for simplicity assuming: MSL_numOfLocalProcs divides n
      	localposition = MSL_myId - MSL_myEntrance;
      	first = localposition * localsize;
      	nextfirst = first + localsize;
      	a = new E[localsize];
	}

  public:

  	DistributedArray(int size): n(size) {
    	init();
	}

  	DistributedArray(int size, E initial): n(size) {
    	init();

		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = initial;
	}

	// The following constructor is in fact scatter. Since non-distributed data structures
	// are replicated on every collaborating processor, there is no need for communication here.
  	DistributedArray(int size, const E b[]): n(size) {
    	init();

		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = b[i + first];
	}

  	DistributedArray(int size, E (*f)(int)): n(size) {
    	init();
		
		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = f(i + first);
	}

  	template<class F>
  	DistributedArray(int size, const Fct1<int, E, F>& f): n(size) {
    	init();

		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = f(i + first);
	}

	/* Copy constructor.

	   author: Philipp
	*/
  	DistributedArray(const DistributedArray<E>& b): n(b.getSize()) {
    	init();

		#pragma omp parallel for
		for(int i = 0; i < localsize; i++) {
			a[i] = b.getLocal(i);
		}
	}

  	inline int getFirst() const {
		return first;
	}

  	inline int getSize() const {
		return n;
	}

  	inline int getLocalSize() const {
		return localsize;
	}

  	inline bool isLocal(int i) const {
		return (i >= first) && (i < nextfirst);
	}

  	inline void setLocal(int i, E v) { // only applicable to locally available elements
    	 // using local index; assuming 0 <= i < localsize
		 a[i] = v;
	}

  	inline void set(int i, E v) {
		// only allowed for locally available elements; using global index
     	if(!isLocal(i))
			throws(NonLocalAccessException());

     	a[i - first] = v;
	}

  	inline E get(int i) const {
		// only allowed for locally available elements; using global index
     	if(!isLocal(i))
			throws(NonLocalAccessException());

     	return a[i - first];
	}

  	inline E getLocal(int i) const {
		// only applicable to locally available elements;
    	// using local index; assuming 0 <= i < localsize
		return a[i];
	}

	// ******************** map and variants *************************************

	template<class R, class F>
	// requires no communication
  	inline DistributedArray<R> map(const Fct1<E, R, F>& f) const {
    	DistributedArray<R> b(n);

		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			b.setLocal(i, f(a[i]));

    	return b;
	}

  	template<class R>
  	inline DistributedArray<R> map(R (*f)(E)) const {
		return map(curry(f));
	}

  	template<class R, class F>
	// requires no communication
  	inline DistributedArray<R> mapIndex(const Fct2<int, E, R, F>& f) const {
    	DistributedArray<R> b(n);

		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			b.setLocal(i, f(i + first, a[i]));

    	return b;
	}

  	template<class R>
  	inline DistributedArray<R> mapIndex(R (*f)(int, E)) const {
		return mapIndex(curry(f));
	}

  	template<class F>
	// requires no communication
  	inline void mapInPlace(const Fct1<E, E, F>& f) {
		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = f(a[i]);
	}

  	inline void mapInPlace(E (*f)(E)) {
		mapInPlace(curry(f));
	}

  	template<class F>
  	inline void mapIndexInPlace(const Fct2<int, E, E, F>& f) {
		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = f(i + first, a[i]);
	}

  	inline void mapIndexInPlace(E (*f)(int, E)) {
		mapIndexInPlace(curry(f));
	}

  	template<class F>
  	inline void mapPartitionInPlace(const Fct1<E*, void, F>& f) {
		f(a);
	}

  	inline void mapPartitionInPlace(void (*f)(E*)) {
		f(a);
	}

	// ******************** zipWith and variants *************************************

  	template<class E2, class R, class F>
	// requires no communication
  	inline DistributedArray<R> zipWith(const DistributedArray<E2>& b,
		const Fct2<E, E2, R, F> f) const {
    	DistributedArray<R> c(n);

		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			c.setLocal(i, f(a[i], b.getLocal(i)));

    	return c;
	}

  	template<class E2, class R>
  	inline DistributedArray<R> zipWith(const DistributedArray<E2>& b, R (*f)(E, E2)) const {
    	return zipWith(b, curry(f));
	}

	// requires no communication
  	template<class E2, class F>
  	inline void zipWithInPlace(const DistributedArray<E2>& b, const Fct2<E, E2, E, F> f) {
		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = f(a[i], b.getLocal(i));
	}

	// fill gap at the end with dummy
  	template<class E2>
  	inline void zipWithInPlace(DistributedArray<E2>& b, E (*f)(E, E2)) {
    	zipWithInPlace(b, curry(f));
	}

	// requires no communication
  	template<class E2, class R, class F>
  	inline DistributedArray<R> zipWithIndex(const DistributedArray<E2>& b,
	const Fct3<int, E, E2, R, F>& f) const {
    	DistributedArray<R> c(n);

		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			c.setLocal(i, f(i + first, a[i], b.getLocal(i)));

    	return c;
	}

  	template<class E2, class R>
  	inline DistributedArray<R> zipWithIndex(const DistributedArray<E2>& b,
	R (*f)(int, E, E2)) const {
    	return zipWithIndex(b, curry(f));
	}

	// requires no communication
  	template<class E2, class F>
  	inline void zipWithIndexInPlace(const DistributedArray<E2>& b,
	const Fct3<int, E, E2,E, F>& f) {
		#pragma omp parallel for
    	for(int i = 0; i < localsize; i++)
			a[i] = f(i + first, a[i], b.getLocal(i));
	} // fill gap at the end with dummy

  	template<class E2>
  	inline void zipWithIndexInPlace(const DistributedArray<E2>& b, E (*f)(int, E, E2)) {
    	zipWithIndexInPlace(b, curry(f));
	}

	// *************************  gather  ********************************************

  	void gather(E b[]) const { // E b[n]
    	int i;
    	for(i = 0; i < localsize; i++)
			b[i + first] = a[i];
    	int power = 1;
    	int neighbor;
    	int log2noprocs = log2(MSL_numOfLocalProcs);
    	int *indexstack = new int[n];    // can be avoided by massive index calculations
    	indexstack[0] = first;
    	int top = 0;
    	int oldtop;
    	for(i = 0; i < log2noprocs; i++) {
      		neighbor = MSL_myEntrance + (localposition ^ power);
      		power *= 2;
      		oldtop = top;
      		for(int j = 0; j <= oldtop; j++) {
				// important: communication has to be synchronous!
        		sendReceive(neighbor, &indexstack[j], &indexstack[++top], sizeof(int));
        		sendReceive(neighbor, &b[indexstack[j]], &b[indexstack[top]], sizeof(E) * localsize);
			}
		}

    	delete [] indexstack;
	}

	// **************************  fold  *********************************************

	template<class F>
  	E fold(const Fct2<E, E, E, F>& f) const { // result on every collaborating processor
    	int i;
    	// assumption: f is associative
    	// parallel prefix-like algorithm
    	// step 1: local fold
    	E result = a[0];

    	for(i = 1; i < localsize; i++)
    		result = f(result, a[i]);

    	// step 2: global folding
    	int power = 1;
    	int neighbor;
    	E result2;
    	int log2noprocs = log2(MSL_numOfLocalProcs);

    	for(i = 0; i < log2noprocs; i++) {
      		neighbor = MSL_myEntrance + (localposition ^ power);
      		power *= 2;
      		sendReceive(neighbor, &result, &result2, sizeof(E));
			// important: this has to be synchronous!
      		if(MSL_myId < neighbor)
				result = f(result, result2);
      		else
				result = f(result2, result);
		}

    	return result;
	}

  	inline E fold(E (*f)(E, E)) const {
		return fold(curry(f));
	}

	/* g is used to map, f is used to fold.
	*/
  	template<class F1, class F2>
  	E mapIndexInPlaceFold(const Fct2<int, E, E, F1>& g, const Fct2<E, E, E, F2>& f) const {
    	// result on every collaborating processor
    	int i;
    	// assumption: f is associative
    	// parallel prefix-like algorithm
    	// step 1: local fold
    	E result = g(first, a[0]);

    	for(i = 1; i < localsize; i++)
    		result = f(result, g(i + first, a[i]));

    	// step 2: global folding
    	int power = 1;
    	int neighbor;
    	E result2;
    	int log2noprocs = log2(MSL_numOfLocalProcs);
    	for(i = 0; i < log2noprocs; i++) {
      		neighbor = MSL_myEntrance + (localposition ^ power);
      		power *= 2;
      		sendReceive(neighbor, &result, &result2, sizeof(E)); // important: this has to be synchronous!
      		if(MSL_myId < neighbor)
				result = f(result, result2);
      		else
				result = f(result2, result);
		}

    	return result;
	}

	/* Variant expecting two function pointers.
	*/
  	inline E mapIndexInPlaceFold(E (*g)(int, E), E (*f)(E, E)) const {
		return mapIndexInPlaceFold(curry(g), curry(f));
	}

	/* Variant expectiong a function pointer for g and a partial application for f.
	*/
  	template<class F>
  	inline E mapIndexInPlaceFold(E (*g)(int, E), const Fct2<E, E, E, F>& f) const {
		return mapIndexInPlaceFold(curry(g), f);
	}

	/* Variant expectiong a partial application for g and a function pointer for f.
	*/
  	template<class F>
  	inline E mapIndexInPlaceFold(const Fct2<int, E, E, F>& g, E (*f)(E, E)) const {
		return mapIndexInPlaceFold(g, curry(f));
	}

	// ****************************  scan  ********************************************

  	template<class F>
  	void scan(const Fct2<E, E, E, F>& f) const { // result on every collaborating processor
    	int i;
    	// assumption: f is associative
    	// step 1: local scan
    	for(i = 1; i < localsize; i++)
      		a[i]  = f(a[i - 1], a[i]);
    	E sum = a[localsize - 1];
    	// step 2: global scan
    	int power = 1;
    	int neighbor;
    	E neighborSum;
    	int log2noprocs = log2(MSL_numOfLocalProcs);
    	for(i = 0; i < log2noprocs; i++) {
      		neighbor = MSL_myEntrance + (localposition ^ power);
      		power *= 2;
      		sendReceive(neighbor, &sum, &neighborSum, sizeof(E));
      		if(MSL_myId > neighbor) {
        		for(int j = 0; j < localsize; j++)
					a[j] = f(neighborSum, a[j]);
        		sum = f(neighborSum, sum);
			}
      		else
				sum = f(sum,neighborSum);
		}
	}

  	inline E scan(E (*f)(E, E)) const {
		scan(curry(f));
	}

  	template<class F>
  	void mapIndexInPlaceScan(const Fct2<int, E, E, F>& g, const Fct2<E, E, E, F>& f) const {
    	// result on every collaborating processor
    	int i;
    	// assumption: f is associative
    	// step 1: local scan
    	a[0] = g(first, a[0]);
    	for(i = 1; i < localsize; i++)
      		a[i]  = f(a[i - 1], g(i+first, a[i]));
    	E sum = a[localsize - 1];
    	// step 2: global scan
    	int power = 1;
    	int neighbor;
    	E neighborSum;
    	int log2noprocs = log2(MSL_numOfLocalProcs);
    	for(i = 0; i < log2noprocs; i++) {
      		neighbor = MSL_myEntrance + (localposition ^ power);
      		power *= 2;
      		sendReceive(neighbor, &sum, &neighborSum, sizeof(E));
      		if(MSL_myId > neighbor) {
        		for(int j = 0; j < localsize; j++) a[j] = f(neighborSum, a[j]);
        		sum = f(neighborSum, sum);
			}
      		else sum = f(sum,neighborSum);
		}
	}

	/* Variant expection a function pointer for each argument.
	*/
  	inline E mapIndexInPlaceScan(E (*g)(int, E), E (*f)(E, E)) const {
		scan(curry(g), curry(f));
	}
	
	/* Variant expection a function pointer for g and a partial application for f.
	*/
  	template<class F>
  	inline E mapIndexInPlaceScan(E (*g)(int, E), const Fct2<E, E, E, F>& f) const {
		scan(curry(g), f);
	}

	/* Variant expection a partial application for g and a function pointer for f.
	*/
  	template<class F>
  	inline E mapIndexInPlaceScan(const Fct2<int, E, E, F>& g, E (*f)(E, E)) const {
		scan(g, curry(f));
	}

	// **************************  permute  ********************************************

	template<class F>
  	void permutePartition(const Fct1<int, int, F>& f) {
    	int i;
    	MPI_Request req;
		MPI_Status stat;
    	int receiver = MSL_myEntrance + f(localposition);

    	if((receiver < MSL_myEntrance) || (receiver >= MSL_myEntrance + MSL_numOfLocalProcs))
			throws(IllegalPartitionException());

    	int sender = MSL_UNDEFINED;

    	for(i = 0; i <  MSL_numOfLocalProcs; i++) // determine sender by computing invers of f
       		if(f(i) + MSL_myEntrance == MSL_myId)
         		if(sender == MSL_UNDEFINED)
					sender = MSL_myEntrance + i;
         		else
					throws(IllegalPermuteException()); // f is not bijective

    	if(sender == MSL_UNDEFINED)
			throws(IllegalPermuteException()); // f is not bijective

    	if(receiver != MSL_myId) {
      		E* b = new E[localsize];

      		for(i = 0; i < localsize; i++)
				b[i] = a[i];

			// assumption: messages with same sender and receiver
      		nonblockingSend(receiver, b, sizeof(E) * localsize, &req);
      		//receive(sender,a, sizeof(E)*localsize, &stat); // don't pass each other
			MSL_Receive(sender, a, sizeof(E) * localsize, &stat); // don't pass each other
      		MPI_Wait(&req, &stat);
      		delete[] b;
		}
	}

  	inline void permutePartition(int (*f)(int)) {
		return permutePartition(curry(f));
	}

  	template<class F>
  	void permute(const Fct1<int, int, F>& f) {
    	E* b = new E[localsize];
    	int* f_inv = new int[n];
    	MPI_Status stat; MPI_Request req;
    	int receiver;
    	int sender;
    	int newposition;
    	int l, k;

    	for(k = 0; k < n; k++)
			f_inv[k] = MSL_UNDEFINED;

    	for(k = 0; k < n; k++) {
			// check, if f is permutation (Exception, if not)
      		int dest = f(k);

      		if((dest < 0) || (dest >= n) || (f_inv[dest] != MSL_UNDEFINED))
        		throws(IllegalPermuteException());

      		f_inv[dest] = k;
		}

    	for(k = 0; k < localsize; k++) {
			// send and receive local elements (with minimal buffer space)
      		newposition = f(first + k);
      		receiver = MSL_myEntrance + newposition / localsize;

      		if(receiver != MSL_myId)
				nonblockingSend(receiver, &a[k], sizeof(E), &req);
      		else
				b[newposition-first] = a[k];

      		for(l = k; l < n; l += localsize) {
        		sender = MSL_myEntrance + l / localsize;
        		newposition = f(l);

        		if((newposition >= first) && (newposition < nextfirst) && (sender != MSL_myId))
          			//receive(sender, &b[newposition -first], sizeof(E), &stat);
					MSL_receive(sender, &b[newposition - first], sizeof(E), &stat);
			}

      		if(receiver != MSL_myId)
				MPI_Wait(&req, &stat);
		}

    	for(k = 0; k < localsize; k++)
      		a[k] = b[k];

    	delete[] b;
		delete[] f_inv;
	}

  	inline void permute(int (*f)(int)) {
		return permute(curry(f));
	}

  	template<class F1, class F2>
  	void mapIndexInPlacePermutePartition(const Fct2<int, E, E,F1>& f, const Fct1<int, int,F2>& g) {
    	int i;
    	int receiver = MSL_myEntrance + g(localposition);

    	if((receiver < MSL_myEntrance) || (receiver >= MSL_myEntrance + MSL_numOfLocalProcs))
			throws(IllegalPartitionException());

    	int sender = MSL_UNDEFINED;
    	
		for(i = 0; i <  MSL_numOfLocalProcs; i++)
			// determine sender by computing invers of g
       		if(g(i) + MSL_myEntrance == MSL_myId)
         		if(sender == MSL_UNDEFINED)
					sender = MSL_myEntrance + i;
         		else // g is not bijective
					throws(IllegalPermuteException());

    	if(sender == MSL_UNDEFINED)
			throws(IllegalPermuteException()); // g is not bijective
    	
		if(receiver != MSL_myId) {
      		MPI_Request* req = new MPI_Request[4];
      		MPI_Status* stat = new MPI_Status[4];
      		int half = localsize / 2;
			int half2 = localsize - half;
      		E* b1= new E[half];
      		E* b2= new E[half2];

      		for(i = 0; i < half; i++)
				b1[i] = f(i + first, a[i]);

			// assumption: messages with same 	sender and receiver don't pass each other
      		nonblockingSend(receiver, b1, sizeof(E)*half, &req[0]);
      		nonblockingReceive(sender, a, sizeof(E)*half, &req[1]);
      		int j = 0;

      		for(i = half; i < localsize; i++)
				b2[j++] = f(i + first, a[i]);

      		// assumption: messages with same 	sender and receiver
			nonblockingSend(receiver, b2, sizeof(E) * half2, &req[2]);
      		nonblockingReceive(sender, &a[half], sizeof(E) * half2, &req[3]); // don't pass each other
      		MPI_Waitall(4, req, stat);
      		
			delete[] b1;
			delete[] b2;
			delete[] req;
			delete[] stat;
		}
	}

  	inline void mapIndexInPlacePermutePartition(E (*f)(int, E), int (*g)(int)) {
    	return mapIndexInPlacePermutePartition(curry(f), curry(g));
	}

	// **************************  broadcast  ********************************************

  	void broadcastPartition(int block) {
    	if((block < 0) || (block >= n/localsize))
			throws(IllegalPartitionException());

    	unsigned int neighbor;
     	unsigned int power = 1;
     	unsigned int mask = 1073741822; // 2^30-2
     	MPI_Status stat;
     	int log2noprocs = log2(MSL_numOfLocalProcs);

     	for(int i = 0; i < log2noprocs; i++) {
      		if((localposition & mask) == (block & mask)) {
        		neighbor = MSL_myEntrance + (localposition ^ power);

        		if((localposition & power) == (block & power))
          			syncSend(neighbor, a, sizeof(E) * localsize);
        		else
					MSL_receive(neighbor, a, sizeof(E) * localsize, &stat);
			}

      		power *= 2;
      		mask &= ~power;
		}
	}

  	void broadcast(int index) {
    	int i;
     	int block = index / localsize;
     	int localindex = index % localsize;

     	if((block < 0) || (block > n/localsize))
			throws(IllegalPartitionException());

     	unsigned int neighbor;
     	unsigned int power = 1;
     	unsigned int mask = 1073741822; // 2^30-2
     	MPI_Status stat;
     	int log2noprocs = log2(MSL_numOfLocalProcs);

     	for(i = 0; i < log2noprocs; i++) {
      		if((localposition & mask) == (block & mask)) {
        		neighbor = MSL_myEntrance + (localposition ^ power);

        		if((localposition & power) == (block & power))
          			syncSend(neighbor, &a[localindex], sizeof(E));
        		else
					MSL_receive(neighbor, &a[localindex], sizeof(E), &stat);
			}

      		power *= 2;
      		mask &= ~power;
		}

     	for(i = 0; i < localsize; i++)
			a[i] = a[localindex];
	}

	// ************************   allToAll  *****************************************

  	/* allToAll: each collaborating processor sends a block of elements to every
	   other processor; the beginnings of all blocks are specified by an index
	   array; the received blocks are concatenated without gaps in arbitrary order.
    */

  	void allToAll(const DistributedArray<int *>& Index, E dummy) {
                // DistributedArray<int [MSL_numOfLocalProcs+1]> Index(MSL_numOfLocalProcs)
    	MPI_Status stat;
    	E* b = new E[localsize];
    	int i, current;
    	int start = (Index.get(localposition))[localposition];
    	int end = (Index.get(localposition))[localposition + 1];
    	int no1 = end - start;
    	int no2;

		// keep own block
    	for(current = 0; current < no1; current++)
			b[current] = a[start + current];
    	
		// exchange blocks with all others
		for(i = 1; i < MSL_numOfLocalProcs; i++) {
    		start = (Index.get(localposition))[localposition^i];
      		end = (Index.get(localposition))[(localposition^i)+1];
      		no1 = end-start;
      		sendReceive(MSL_myId^i, &no1, &no2, sizeof(int));

      		if(current + no2 > localsize)
				throws(IllegalAllToAllException());

      		if(MSL_myId > (MSL_myId ^ i)) {
        		if(no1 > 0)
					syncSend(MSL_myId ^ i, &a[start], sizeof(E) * no1);
        		
				if(no2 > 0)
					MSL_receive(MSL_myId ^ i, &b[current], sizeof(E) * no2, &stat);
			}
      		else {
        		if(no2 > 0)
					MSL_receive(MSL_myId ^ i, &b[current], sizeof(E) * no2, &stat);

        		if(no1 > 0)
					syncSend(MSL_myId ^ i, &a[start], sizeof(E) * no1);
			}

      		current += no2;
		}

    	for(i = 0; i <  current; i++)
			a[i] = b[i];
		
		// fill gap at the end with dummy
    	for(i = current; i < localsize; i++)
			a[i] = dummy;

    	delete[] b;
	}

	// ******************************  copy  **************************************

  	inline DistributedArray<E> copy() const {
		return new DistributedArray<E>(this);
	}

  	inline DistributedArray<E> copyWithGap(int size, E dummy) const {
    	DistributedArray<E> C(size, dummy); // gaps are filled with dummy
    	int number = (localsize < C.getLocalSize()) ? localsize : C.getLocalSize();

		#pragma omp parallel for
    	for(int i = 0; i < number; i++)
			C.setLocal(i, a[i]);

    	return C;
	}

	// **************************  show  ********************************************

  	inline void show() const { // alternatively use << (see below)
		//#if Output == 1
    	E* b =  new E[n];
    	gather(b);

    	if(MSL_myId == MSL_myEntrance) {
      		std::cout <<"{";
      	
			for(int i = 0; i < n-1; i++)
				std::cout << b[i] << ",";

      		std::cout  << b[n - 1] << "}" << std::endl << std::flush;
		}

    	delete[] b;
		//#endif
  	}
};

// *********  end of class DistributedArray *************

template<class E, class F>
void multiMapIndexInPlace(DistributedArray<E>& A, const Fct2<E, E, E, F>& f,
DistributedArray<E>& B, const Fct2<E, E, E, F>& g) {
    int first = A.getFirst();
    int localsize = A.getLocalSize();

    if((first != B.getFirst()) || (localsize != B.getLocalSize()))
		throws(NonLocalAccessException());
    
	for(int i = 0; i < localsize; i++) {
		A.setLocal(i, f(i + first, A.getLocal(i)));
		A.setLocal(i, g(i + first, A.getLocal(i)));
	}
}

template<class E>
std::ostream& operator<<(std::ostream& os, const DistributedArray<E>& A) {
	#if Output == 1
	int n = A.getSize();
	E* b =  new E[n];
	A.gather(b);
	
	if(MSL_myId == MSL_myEntrance) {
		os << "{";

		for(int i = 0; i < n - 1; i++)
			os << b[i] << ",";

		os << b[n - 1] << "}" << std::endl;
	}

	delete[] b;
	return os;
	#endif
}

// ***************************************************************************************************
// * Distributed Matrix  with data parallel skeletons map, fold, zip, gather, permute, broadcast...  *
// ***************************************************************************************************

template<class E>
class DistributedMatrix {

	int n;                // number of rows
	int m;                // number of columns
	E** a;                // E a[localRows][localCols];
	int localRows;        // number of local rows
	int localCols;        // number of local columns
	int localsize;        //  = localRows*localCols;
	int blocksInRow;      // number of partitions per row
	int blocksInCol;      // number of partitions per column

	// assumption: block division of matrix
	int localColPosition; // X position of processor in data parallel group of processors
	int localRowPosition; // Y position of processor in data parallel group of processors
	int localPosition;    // position of processor in data parallel group of processors
	int firstRow;         // first column index in local partition; assuming division mode: block
	int firstCol;         // first row index in local partition; assuming division mode: block
	int nextRow;          // index of first row in next partition
	int nextCol;          // index of first column in next partition

private:

	// initializes distributed matrix (used in constructors)
	void init(int rows, int cols) { 
		if((MSL_myExit == MSL_UNDEFINED) || (MSL_myEntrance == MSL_UNDEFINED))
			throws(MissingInitializationException());

		blocksInRow = cols;
		blocksInCol = rows;
		
		// for simplicity: assuming MSL_numOfLocalProcs is a power of 2
		if(cols * rows != MSL_numOfLocalProcs)
			throws(PartitioningImpossibleException());

		localRows = n / rows;  // for simplicity: assuming rows divides n
		localCols = m / cols;  // for simplicity: assuming cols divides m
		localsize =  localRows * localCols;
		localColPosition = (MSL_myId - MSL_myEntrance) % cols; // blocks assigned row by row
		localRowPosition = (MSL_myId - MSL_myEntrance) / cols;
		localPosition = localRowPosition * cols + localColPosition;
		firstRow = localRowPosition * localRows;
		firstCol = localColPosition * localCols;
		nextRow = firstRow + localRows;
		nextCol = firstCol + localCols;
		a = new E*[localRows];

		for(int i = 0; i < localRows; i++)
			a[i] = new E[localCols];
	}

public:

	DistributedMatrix(int n0, int m0, int rows, int cols): n(n0), m(m0) {
		init(rows, cols);
	}

	DistributedMatrix(int n0, int m0, E initial, int rows, int cols): n(n0), m(m0) {
		init(rows, cols);

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				a[i][j] = initial;
	}

	// The following constructor is in fact scatter. Since non-distributed data structures
	// are replicated on every collaborating processor, there is no need for communication here.
	DistributedMatrix(int n0, int m0, const E** b, int rows, int cols):
	n(n0), m(m0) { //E b[n][m]
		init(rows, cols);

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				a[i][j] = b[i + firstRow][j + firstCol];
	}

	DistributedMatrix(int n0, int m0, E (*f)(int, int), int rows, int cols):
	n(n0), m(m0) {
		init(rows, cols);

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				a[i][j] = f(i + firstRow, j + firstCol);
	}

	template<class F>
	DistributedMatrix(int n0, int m0, const Fct2<int, int, E, F>& f, int rows, int cols):
	n(n0), m(m0) {
		init(rows, cols);

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
			  a[i][j] = f(i + firstRow, j + firstCol);
	}

	/* Copy constructor.

	 author: Philipp
	*/
	DistributedMatrix(const DistributedMatrix<E>& b):
	n(b.getLocalRows()), m(b.getLocalCols()) {
		init(b.getRows(), b.getCols());

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++) {
			for(int j = 0; j < localCols; j++) {
				a[i][j] = b.getLocal(i, j);
			}
		}
	}

	inline int getRows() const {
		return n;
	}

	inline int getCols() const {
		return m;
	}

	inline int getFirstRow() const {
		return firstRow;
	}

	inline int getFirstCol() const {
		return firstCol;
	}

	inline int getLocalRows() const {
		return localRows;
	}

	inline int getLocalCols() const {
		return localCols;
	}

	inline int getBlocksInRow() const {
		return blocksInRow;
	}

	inline int getBlocksInCol() const {
		return blocksInCol;
	}
	
	// only applicable to locally available elements
	inline void setLocal(int i, int j, E v) {
		a[i][j] = v;
	}

	// using local index; assuming 0 <= i < localRows; 0 <= j < localCols
	inline bool isLocal(int i, int j) const {
		return (i >= firstRow) && (i < nextRow) && (j >= firstCol) && (j < nextCol);
	}

	// only allowed for locally available elements; using global index
	inline E get(int i, int j) const {
		if(!isLocal(i, j))
			throws(NonLocalAccessException());

		return a[i - firstRow][j - firstCol];
	}

	// only applicable to locally available elements;
	inline E getLocal(int i, int j) const {
		// using local index; assuming 0 <= i < localRows; 0 <= j < localCols
		return a[i][j];
	}

	// j is global index; i is local index ;
	inline E getLocalGlobal(int i, int j) const {
		if((j < firstCol) || (j >= nextCol))
			throws(NonLocalAccessException());
	
		// assuming 0 <= i < localRows;
		return a[i][j - firstCol];
	}

	// i is global index; j is local index ;
	inline E getGlobalLocal(int i, int j) const {
		if((i < firstRow) || (i >= nextRow))
			throws(NonLocalAccessException());

		// assuming 0 <= j < localCols;
		return a[i - firstRow][j];
	}

	// ******************** map and variants *************************************

	// requires no communication
	template<class R, class F>
	inline DistributedMatrix<R> map(const Fct1<E, R, F>& f) const {
		// causes implicit call of constructor
		DistributedMatrix<R> b(n, m, blocksInRow, blocksInCol);
		
		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				b.setLocal(i, j, f(a[i][j]));
		
		return b;
	}

	template<class R>
	inline DistributedMatrix<R> map(R (*f)(E)) const {
		return map(curry(f));
	}

	// requires no communication
	template<class R, class F>
	inline DistributedMatrix<R> mapIndex(const Fct3<int, int, E, R, F>& f) const {
		DistributedMatrix<R> b(n, m, blocksInRow, blocksInCol);

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				b.setLocal(i, j, f(i + firstRow, j + firstCol, a[i][j]));

		return b;
	}

	template<class R>
	inline DistributedMatrix<R> mapIndex(R (*f)(int, int, E)) const {
		return mapIndex(curry(f));
	}

	template<class F>
	inline void mapInPlace(const Fct1<E, E, F>& f) { // requires no communication
		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				a[i][j] = f(a[i][j]);
	}

	inline void mapInPlace(E (*f)(E)) {
		mapInPlace(curry(f));
	}

	// requires no communication
	template<class F>
	inline void mapIndexInPlace(const Fct3<int, int, E, E, F>& f) {
		#pragma omp parallel for
		for(int i = 0; i < localRows; ++i)
			for(int j = 0; j < localCols; ++j)
				a[i][j] = f(i + firstRow, j + firstCol, a[i][j]);
	}

	inline void mapIndexInPlace(E (*f)(int, int, E)) {
		mapIndexInPlace(curry(f));
	}

	template<class F>
	inline void mapPartitionInPlace(const Fct1<E**, void, F>& f) {
		f(a);
	}

	inline void mapPartitionInPlace(void (*f)(E**)) {
		f(a);
	}

	// ******************** zipWith and variants *************************************

	// requires no communication
	template<class E2, class R, class F>
	inline DistributedMatrix<R> zipWith(const DistributedMatrix<E2>& b,
	const Fct2<E, E2, R, F>& f) const {
		DistributedMatrix<R> c(n, m, blocksInRow, blocksInCol);

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				c.setLocal(i, j, f(a[i][j], b.getLocal(i, j)));

		return c;
	}

	template<class E2, class R>
	inline DistributedMatrix<R> zipWith(const DistributedMatrix<E2>& b, R (*f)(E, E2)) const {
		return zipWith(b, curry(f));
	}

	// requires no communication
	template<class E2, class F>
	inline void zipWithInPlace(const DistributedMatrix<E2>& b, const Fct2<E, E2, E, F>& f) {
		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				a[i][j] = f(a[i][j], b.getLocal(i, j));
	}

	template<class E2>
	inline void zipWithInPlace(const DistributedMatrix<E2>& b, E (*f)(E, E2)) {
		zipWithInPlace(b, curry(f));
	}

	// requires no communication
	template<class E2, class R, class F>
	inline DistributedMatrix<R> zipWithIndex(const DistributedMatrix<E2>& b,
	const Fct4<int, int, E, E2, R, F>& f) const {
		DistributedMatrix<R> c(n, m, blocksInRow, blocksInCol);

		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				c.setLocal(i, j, f(i + firstRow, j + firstCol, a[i][j], b.getLocal(i, j)));

		return c;
	}

	template<class E2, class R>
	inline DistributedMatrix<R> zipWithIndex(const DistributedMatrix<E2>& b,
	R (*f)(int, int, E, E2)) const {
		return zipWithIndex(b, curry(f));
	}

	// requires no communication
	template<class E2, class F>
	inline void zipWithIndexInPlace(const DistributedMatrix<E2>& b, const Fct4<int, int, E, E2,E, F> f) {
		#pragma omp parallel for
		for(int i = 0; i < localRows; i++)
			for(int j = 0; j < localCols; j++)
				a[i][j] = f(i + firstRow, j + firstCol, a[i][j], b.getLocal(i, j));
	}

	template<class E2>
	inline void zipWithIndexInPlace(const DistributedMatrix<E2>& b, E (*f)(int, int, E, E2)) {
		zipWithIndexInPlace(b, curry(f));
	}

	// *************************  gather  ********************************************
	
	struct Buffer{
		int row;
		int col;
	};

	// E b[n][m]
	void gather(E** b) const {
		int i, j, k, l;

		#pragma omp parallel for
		for(i = 0; i < localRows; i++)
			for(j = 0; j < localCols; j++)
				b[i + firstRow][j + firstCol] = a[i][j];

		// prepare communication buffers and pointers to their components
		Buffer inbuf;
		Buffer outbuf;
		E* inmatrix  = new E[n * m];
		E* outmatrix = new E[n * m];
		int bufCounter;

		int power = 1;
		ProcessorNo neighbor;
		int log2noprocs = log2(MSL_numOfLocalProcs);
		int* indexStack = new int[2 * n]; // can be avoided by massive index calculations
		indexStack[0] = firstRow;
		indexStack[1] = firstCol;
		int top = 1;
		int oldtop;

		for(i = 0; i < log2noprocs; i++) {
			neighbor = MSL_myEntrance + (localPosition ^ power);
			power *= 2;
			oldtop = top;

			//  synchronous!
			for(j = 0; j <= oldtop; j += 2) {
				// fill output buffer
				outbuf.row = indexStack[j];
				outbuf.col = indexStack[j + 1];
				bufCounter = 0;

				for(k = 0; k < localRows; k++)
					for(l = 0; l < localCols; l++)
						outmatrix[bufCounter++] = b[outbuf.row + k][outbuf.col + l];

				sendReceive(neighbor, &outbuf, &inbuf, 2 * sizeof(int));
				sendReceive(neighbor, outmatrix, inmatrix, localsize * sizeof(E));
				// fetch from input buffer
				indexStack[++top] = inbuf.row;
				indexStack[++top] = inbuf.col;
				bufCounter = 0;

				for(k = 0; k < localRows; k++)
					for(l = 0; l < localCols; l++)
						b[inbuf.row + k][inbuf.col + l] = inmatrix[bufCounter++];
			}
		}

		delete[] indexStack;
		delete[] inmatrix;
		delete[] outmatrix;
	}

	// **************************  fold  *********************************************

	// result on every collaborating processor
	template<class F>
	E fold(const Fct2<E, E, E, F>& f) const {
		int i, j;
		// assumption: f is associative and commutative
		// parallel prefix-like algorithm
		// step 1: local fold
		E result = a[0][0];

		for(i = 0; i < localRows; i++)
			for(j = 0; j < localCols; j++)
				if((i != 0) || (j != 0))
					result = f(result, a[i][j]);

		// step 2: global folding
		int power = 1;
		int neighbor;
		E result2;
		int log2noprocs = log2(MSL_numOfLocalProcs);

		for(i = 0; i < log2noprocs; i++) {
			neighbor = MSL_myEntrance + (localPosition ^ power);
			dbg(std::cout << MSL_myId << ": fold: neighbor " << neigbor << std::endl);
			power *= 2;
			// important: this has to be synchronous!
			sendReceive(neighbor, &result, &result2, sizeof(E));
			result = f(result, result2);
		}

		return result;
	}

	inline E fold(E (*f)(E, E)) const {
		return fold(curry(f));
	}

	// ****************************  scan  ********************************************

	//   template<class F>
	//   void scan(const Fct2<E, E, E, F> f) const {... still  missing ...;}

	// **************************  permute  ********************************************

	template<class F1, class F2>
	inline void permutePartition(const Fct2<int, int, int,F1>& newRow,
	const Fct2<int, int, int,F2>& newCol) {
		int i, j, receiver;
		MPI_Request req;
		MPI_Status stat;
		receiver = MSL_myEntrance + newRow(localRowPosition,localColPosition) * blocksInRow +
			newCol(localRowPosition,localColPosition);

		if((receiver < MSL_myEntrance) || (receiver >= MSL_myEntrance+MSL_numOfLocalProcs))
			throws(IllegalPartitionException());

		int sender = MSL_UNDEFINED;
		
		for(i = 0; i < blocksInCol; i++) // determine sender
			for(j = 0; j < blocksInRow; j++) {
				if(MSL_myEntrance + newRow(i, j) * blocksInRow + newCol(i, j) == MSL_myId)
					if(sender == MSL_UNDEFINED)
						sender = MSL_myEntrance + i * blocksInRow + j;
					else
						throws(IllegalPermuteException());
			}
			// newRow and newCol don't produce a permutation
				
			if(sender == MSL_UNDEFINED)
				throws(IllegalPermuteException()); // newRow and newCol don't produce a permutation
			
			if(receiver != MSL_myId) {
				E* b1 = new E[localRows*localCols];
				E* b2 = new E[localRows*localCols];
				int bufCount = 0;

				for(i = 0; i < localRows; ++i)
					for(j = 0; j < localCols; ++j)
						b1[bufCount++] = a[i][j];

				// assumption: messages with same sender
				nonblockingSend(receiver, &b1[0], sizeof(E) * localRows * localCols, &req);
				// and receiver don't pass each other
				MSL_receive(sender, &b2[0], sizeof(E) * localRows * localCols, &stat);
				MPI_Wait(&req, &stat);
				bufCount = 0;
				
				for(i = 0; i < localRows; ++i)
					for(j = 0; j < localCols; ++j)
						a[i][j] = b2[bufCount++];
				
				delete[] b1;
				delete[] b2;
			}
	}

	inline void permutePartition(int (*f)(int, int), int (*g)(int, int)) {
		permutePartition(curry(f), curry(g));
	}

	template<class F>
	inline void permutePartition(int (*f)(int, int), const Fct2<int, int, int, F>& g) {
		permutePartition(curry(f), g);
	}

	template<class F>
	inline void permutePartition(const Fct2<int, int, int, F>& f, int (*g)(int, int)) {
		permutePartition(f, curry(g));
	}

	// **************************  rotate  ********************************************

	template<class F>
	inline void rotateRows(const Fct1<int, int, F>& f) {
	// Most C++ compilers need an explicit cast here :(
	// I have to check what the standard says ... (JS)
	permutePartition(curry((int (*)(int, int)) proj1_2<int, int>),
		curry((int (*)(const Fct1<int, int, F>&, int, int, int))auxRotateRows<F>) (f)(blocksInRow));
	}

	inline void rotateRows(int (*f)(int)) {
		rotateRows(curry(f));
	}

	inline void rotateRows(int cols) {
		rotateRows(curry((int (*)(int, int))proj1_2<int, int>)(cols));
	}

	template<class F>
	inline void rotateCols(const Fct1<int, int, F>& f) {
		permutePartition(curry((int (*)(const Fct1<int, int, F>&,int, int, int))auxRotateCols<F>)(f)(blocksInCol),
			curry((int (*)(int, int))proj2_2<int, int>));
	}

	inline void rotateCols(int (*f)(int)) {
		rotateCols(curry(f));
	}

	inline void rotateCols(int rows) {
		rotateCols(curry((int (*)(int, int))proj1_2<int, int>)(rows));
	}

	// **************************  broadcast  ********************************************

	void broadcastPartition(int blockRow, int blockCol) {
		if((blockRow < 0) || (blockRow >= blocksInCol) ||
		(blockCol < 0) || (blockCol >= blocksInRow))
			throws(IllegalPartitionException());

		int block = blockRow * blocksInRow + blockCol;
		unsigned int neighbor;
		unsigned int power = 1;
		unsigned int mask = 1073741822; // 2^30-2
		MPI_Status stat;
		E* buf = new E[localRows * localCols];
		int bufCount;
		int i, k, l;
		int log2noprocs = log2(MSL_numOfLocalProcs);
	 
		for(i = 0; i < log2noprocs; i++) {
			if((localPosition & mask) == (block & mask)) {
				neighbor = MSL_myEntrance + (localPosition ^ power);
					
				if((localPosition & power) == (block & power)) {
					bufCount = 0;

					for(k = 0; k < localRows; k++)
						for(l = 0; l < localCols; l++)
							buf[bufCount++] = a[k][l];

					syncSend(neighbor, buf, sizeof(E) * localsize);
				}
				else {
					//receive(neighbor, buf, sizeof(E) * localsize, &stat);
					MSL_receive(neighbor, buf, sizeof(E) * localsize, &stat);
					bufCount = 0;

					for(k = 0; k < localRows; k++)
						for(l = 0; l< localCols; l++)
							a[k][l] = buf[bufCount++];
				}
			}
			 
			power *= 2;
			mask &= ~power;
		}

		delete[] buf;
	}

	void broadcast(int row, int col) {
		int block = row / localRows * (m / localCols) + col / localCols;
		
		if((block < 0) || (block >= MSL_numOfLocalProcs))
			throws(IllegalPartitionException());

		unsigned int i, j, neighbor;
		unsigned int power = 1;
		unsigned int mask = 1073741822; // 2^30-2
		MPI_Status stat;

		if(block == localPosition)
			a[0][0] = a[row - firstRow][col - firstCol];

		int log2noprocs = log2(MSL_numOfLocalProcs);

		for(i = 0; i < log2noprocs; i++) {
			if((localPosition & mask) == (block & mask)) {
				neighbor = MSL_myEntrance + (localPosition ^ power);

				if((localPosition & power) == (block & power))
					syncSend(neighbor, &a[0][0], sizeof(E));
				else
					MSL_receive(neighbor, &a[0][0], sizeof(E), &stat);
			}
			
			power *= 2;
			mask &= ~power;
		}

		for(i = 0; i < localRows; i++)
			for(j = 0; j < localCols; j++)
				a[i][j] = a[0][0];
	}

	// possible additional skeletons: broadcastRow, broadcastCol

	// **************************  show  ********************************************

	inline void show() const { // alternatively use << (see below)
		//#if Output == 1
		int n = getRows();
		int m = getCols();
		int i, j;
		E** b = new E*[n];

		for(i = 0; i < n; i++)
			b[i] = new E[m];

		gather(b);

		if(MSL_myId == MSL_myEntrance) {
			for(i = 0; i < n; i++) {
				std::cout << "(";
				
				for(j = 0; j < m; j++)
					std::cout << b[i][j] << " ";
				
				std::cout << ")" << std::endl;
			}
			
			std::cout << std::flush;
		}

		for(i = 0; i < n; i++)
			delete[] b[i];

		delete[] b;
		//#endif
	}

};

// *********************** end of class DistributedMatrix **********************

template<class E, int n, int m>
inline std::ostream& operator<<(std::ostream& os,  const DistributedMatrix<E>& a) {
	#if Output == 1
	E** b = new E*[n];

	for(i = 0; i < n; i++)
		b[i] = new E[m];

	gather(b);
	
	if(MSL_myId == MSL_myEntrance) {
		for(int i = 0; i < n; i++) {
			os << "(";

			for(int j = 0; j < m; j++)
				os << b[i][j] << " ";

			os << ")" << std::endl;
		}
	}

	for(i = 0; i < n; i++)
		delete[] b[i];

	delete[] b;
	return os;
	#endif
}

// ***************************************************************************************************************************
// *** INITIALIZATION AND TERMINATION OF SKELETON COMPUTATIONS ***************************************************************
// ***************************************************************************************************************************

double MSL_Starttime;		// MPI_Wtime();
char *MSL_ProgrammName;		// argv[0]
int MSL_ARG1 = 0;			// argv[1]  size
int MSL_ARG2 = 0;			// argv[2]	1: MSL_RANDOM_DISTRIBUTION, 	2: MSL_CYCLIC_DISTRIBUTION
int MSL_ARG3 = 0;			// argv[3]
int MSL_ARG4 = 0;			// argv[4]

// needs to be called before a skeleton is used
// MSL_ARG2 = 1: MSL_RANDOM_DISTRIBUTION, MSL_ARG2 = 2: cyclic
void InitSkeletons(int argc, char* argv[], bool serialization = MSL_NOT_SERIALIZED) {
	// MPI initialisieren
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &MSL_numOfTotalProcs);
	MPI_Comm_rank(MPI_COMM_WORLD, &MSL_myId);

	// std::cout << "InitSkeletons: " << &argc << "  " << &argv << "  " << serialization << "  " << &MSL_numOfTotalProcs << "  " << &MSL_myId << std::endl;

	// Parameter von der Konsole einlesen und verarbeiten
	if(1 <= argc) {
		MSL_ProgrammName = argv[0];
	}

	if(2 <= argc) {
		MSL_ARG1 = (int)strtol(argv[1], NULL,10);
	}

	if(3 <= argc) {
		MSL_ARG2 = (int)strtol(argv[2], NULL,10);
	}

	if(4 <= argc) {
		MSL_ARG3 = (int)strtol(argv[3], NULL,10);
	}

	if(5 <= argc) {
		MSL_ARG4 = (int)strtol(argv[4], NULL,10);
	}

	// if(MSL_ARG2==MSL_RANDOM_DISTRIBUTION) { MSL_DISTRIBUTION_MODE = MSL_RANDOM_DISTRIBUTION; srand(time(NULL)); }
	// else if(MSL_ARG2==MSL_CYCLIC_DISTRIBUTION) MSL_DISTRIBUTION_MODE = MSL_CYCLIC_DISTRIBUTION;
	// else MSL_DISTRIBUTION_MODE = MSL_DEFAULT_DISTRIBUTION;

	// Kommunikationsmodi festlegen
	MSL_DISTRIBUTION_MODE = MSL_CYCLIC_DISTRIBUTION;
	MSL_COMMUNICATION = serialization; // Serialisierung aktivieren / deaktivieren (per default aus)

    // Sendepuffer vorbereiten
    // MSL_Buffer = malloc(MSL_BUFFERSIZE);

    MSL_myEntrance = 0;
	MSL_myExit = 0;
    MSL_numOfLocalProcs = MSL_numOfTotalProcs; // ??? hier wird globales MSL_numOfLocalProcs=1 doch direkt überschrieben? vgl. Atomic!

    MSL_Starttime = MPI_Wtime(); // Zeitmessung: vgl. MPI_Wtick(void)
    //std::cout << "MSL_Starttime = " << MSL_Starttime << std::endl;
}

// needs to be called after the skeletal computation is finished
void TerminateSkeletons() {
	MPI_Barrier(MPI_COMM_WORLD);

	if(MSL_myId == 0) {
		std::cout << MSL_ProgrammName << ", Muesli 1." << MSL_VERSION << ", Comm: ";
		
		if(MSL_DISTRIBUTION_MODE == MSL_RANDOM_DISTRIBUTION)
			std::cout << "RAND/"; else std::cout << "CYCL/";

		if(MSL_isSerialized())
			std::cout << "SERIALIZED ";
		else
			std::cout << "NOT_SERIALIZED ";

		std::cout << "Proc: " << MSL_numOfTotalProcs
			 << "  Size: " << MSL_ARG1
			 << "  Time = " << MPI_Wtime()-MSL_Starttime << std::endl << std::flush;
	}

	MPI_Finalize(); // MPI beenden
	MSL_runningProcessorNo = 0; // Anzahl arbeitender Prozessoren auf Null setzen
}

// ***************************************************************************************************************************
// *** TASK PARALLEL SKELETONS ***********************************************************************************************
// ***************************************************************************************************************************

// Oberklasse aller Taskparalleler Skelette
class Process { // abstract

private:

	// für die interne Berechnung des Empfängers der nächsten Nachricht
	int nextReceiver;		// fuer die zyklische Bestimmung des Nachfolgers

protected:

	ProcessorNo* predecessors; 	// Prozess empfaengt von mehreren Prozessoren
	ProcessorNo* successors;	// Prozess sendet an mehrere Prozessoren
	ProcessorNo* entrances;		// Skelett hat mehrere Entrances
	ProcessorNo* exits;			// Skelett hat mehrere Exits
	int numOfPredecessors;		// size of predecessors array
	int numOfSuccessors;		// size of successors array
	int numOfEntrances;
	int numOfExits;

	// für Zeitmessungen
	double processBeginTime, processEndTime, processSendTime, processRecvTime;

	// counter fuer empfangene Tags
	int receivedStops;  // = 0
	int receivedTT;		// = 0

	// Dieses Flag wird benutzt um herauszufinden, ob ein Prozessor an einem bestimmten
	// Prozess beteiligt ist oder nicht. Jedes Skelett wird auf eine bestimmte Prozessor-
	// menge abgebildet. Anhand seiner eigenen Id kann jeder Prozessor feststellen, ob er
	// Teil eines bestimmten Skeletts ist (finished=false) oder eben nicht (finished=true).
	// Anhand des Zustands dieser Variable wird der Programmablauf gesteuert und parallelisiert.
	bool finished;

public:

	// Konstruktor: numOfEntrances/numOfExits vordefiniert mit 1 (nur Farm hat i.d.R. mehrere)
	Process():
	numOfEntrances(1), numOfExits(1), nextReceiver(-1), receivedStops(0), receivedTT(0) {
	};

	virtual ~Process() {
	};

	ProcessorNo* getSuccessors() const {
		return successors;
	}

	ProcessorNo* getPredecessors() const {
		return predecessors;
	}

	ProcessorNo* getEntrances() const {
		return entrances;
	}

	ProcessorNo* getExits() const {
		return exits;
	}

	// Methoden zum Verwalten der Tags und zur Prozesssteuerung
	int getReceivedStops() {
		return receivedStops;
	}

	int getReceivedTT() {
		return receivedTT;
	}

	void addReceivedStops() {
		receivedStops++;
	}

	void addReceivedTT() {
		receivedTT++;
	}

	void resetReceivedStops() {
		receivedStops = 0;
	}

	void resetReceivedTT() {
		receivedTT = 0;
	}

	int getNumOfPredecessors() {
		return numOfPredecessors;
	}

	int getNumOfSuccessors() {
		return numOfSuccessors;
	}

	int getNumOfEntrances() {
		return numOfEntrances;
	}

	int getNumOfExits() {
		return numOfExits;
	}

	// für Zeitmessungen
	void addProcessSendTime(double t) {
		processSendTime += t;
	}

	double getProcessSendTime() {
		return processSendTime;
	}

	void addProcessRecvTime(double t) {
		processRecvTime += t;
	}

	double getProcessRecvTime() {
		return processRecvTime;
	}

	// Soll der Empfaenger einer Nachricht per Zufall ausgewaehlt werden, kann mit Hilfe dieser
	// Methode der Seed des Zufallsgenerators neu gesetzt werden. Als Seed wird die Systemzeit
	// gewaehlt.
	void newSeed() {
		srand(time(NULL));
	}

	// jeder Prozess kann einen zufaelligen Empfaenger aus seiner successors-Liste bestimmen
	// Den Seed kann jeder Prozess mit newSeed() auf Wunsch selbst neu setzten.
	inline ProcessorNo getRandomReceiver() {
		int i = rand() % numOfSuccessors;
		// std::cout << "getRandomReceiver() liefert: " << successors[i] << std::endl;
		return successors[i];
	}

	// jeder Prozess kann den Nachrichtenempfaenger zyklisch aus seiner successors-Liste bestimmen.
	inline ProcessorNo getNextReceiver() {
		if(nextReceiver == -1)
			std::cout << "INITIALIZATION ERROR: first receiver in cyclic mode was not defined" << std::endl;
		
		// Index in successors-array zyklisch weitersetzen
		nextReceiver = (nextReceiver + 1) % numOfSuccessors;
		return successors[nextReceiver];
	}

	// jeder Prozess kann den Nachrichtenempfaenger zyklisch aus seiner successors-Liste bestimmen.
	inline ProcessorNo getReceiver() {
		// RANDOM MODE
		if(MSL_DISTRIBUTION_MODE==MSL_RANDOM_DISTRIBUTION)
			return getRandomReceiver();
		// CYCLIC MODE: Index in successors-array zyklisch weitersetzen
		else
			return getNextReceiver();
	}

	// jeder Prozessor kann den Empfaenger seiner ersten Nachricht frei waehlen.
	// Dies ist in Zusammenhang mit der zyklischen Empfaengerwahl sinnvoll, um eine Gleichverteilung
	// der Nachrichten und der Prozessorlast zu erreichen. Wichtig ist dies insbesondere bei einer
	// Pipe von Farms.
	void setNextReceiver(int index) {
		// receiver 0 existiert immer
		if(index==0 || (index>0 && index<numOfSuccessors))
			nextReceiver = index;
		else {
			std::cout << "error in Process " << MSL_myId << ": index out of bounds -> " <<
				 "index = " << index << "  numOfSuccessors = " << numOfSuccessors << std::endl;
			throws(UndefinedDestinationException());
		}
	}

	// zeigt an, ob der uebergebene Prozessor in der Menge der bekannten Quellen ist, von denen
	// Daten erwartet werden. Letztlich wird mit Hilfe dieser Methode und dem predecessors-array
	// eine Prozessgruppe bzw. Kommunikator simuliert. Kommunikation kann nur innerhalb einer
	// solchen Prozessgruppe stattfinden. Werden Nachrichten von einem Prozess ausserhalb dieser
	// Prozessgruppe empfangen fuehrt das zu einer undefinedSourceException. Damit sind die Skelette
	// deutlich weniger fehleranfaellig. Auf die Verwendung der MPI-Kommunikatoren wurde aus Gruenden
	// der Portabilitaet bewusst verzichtet.
	bool isKnownSource(ProcessorNo no) {
		for(int i = 0; i < numOfPredecessors; i++)
			if(predecessors[i] == no)
				return true;

		return false;
	}

	// >> !!! Der Compiler kann moeglicherweise den Zugriff auf folgende virtuelle Methoden
	// >> optimieren, wenn der Zugriff auf diese statisch aufzuloest werden kann. Ansonsten
	// >> wird der Zugriff ueber die vtbl (virtual table) einen geringen Performanceverlust
	// >> bedeuten ==> ggf. ueberdenken, ob das "virtual" wirklich notwendig ist... !!!

	// Teilt einem Prozess mit, von welchen anderen Prozessoren Daten empfangen werden koennen.
	// Dies sind u.U. mehrere, z.B. dann, wenn eine Farm vorgelagert ist. In diesem Fall darf
	// der Prozess von jedem worker Daten entgegennehmen.
	// @param p			Array mit Prozessornummern
	// @param length	arraysize
	virtual void setPredecessors(ProcessorNo* p, int length) {
		numOfPredecessors = length;
		predecessors = new ProcessorNo[length];
		
		for(int i = 0; i < length; i++)
			predecessors[i] = p[i];
	}

	// Teilt einem Prozess mit, an welche anderen Prozessoren Daten gesendet werden koennen.
	// Dies sind u.U. mehrere, z.B. dann, wenn eine Farm nachgelagert ist. In diesem Fall darf
	// der Prozess an einen beliebigen worker Daten senden.
	// @param p			Array mit Prozessornummern
	// @param length	arraysize
	virtual void setSuccessors(ProcessorNo* p, int length) {
		numOfSuccessors = length;
		successors = new ProcessorNo[length];
		
		for(int i = 0; i < length; i++)
			successors[i] = p[i];
	}

	// rein virtuelle Methoden (abstrakte Methoden)
	virtual void start() = 0;
	virtual Process* copy() = 0;
	virtual void show() const = 0;

};

// ******************  Initial Process  ********************

// Objekte dieser Klasse benoetigen keinen Eingabestrom. Die weitergeleiteten Daten werden mit
// Hilfe der Argumentfunktion erzeugt (z.B. Einlesen einer Datei). Fuer dieses atomare Skelett
// ist nur ein Prozessor vorgesehen. Die Daten werden je nach gewaehltem Modus per Zufall oder
// zyklisch an den oder die Nachfolger weitergeleitet. Die Datenuebertragung erfolgt blockierend,
// asynchron und ggf. serialisiert.

template<class O>
class Initial: public Process {

	// speichert die im Konstruktor übergebene Argumentfunktion
    DFct1<Empty,O*> fct;

public:

	// wenn Initial das erste Skelett ist, liegt entrance auf Prozessor 0 (vgl. initSkeletons)
	Initial(O* (*f)(Empty)):
	Process(), fct(Fct1<Empty, O*, O* (*)(Empty)>(f)) {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo++;
		exits[0] = entrances[0];
		setNextReceiver(0); // Empfaenger der ersten Nachricht festlegen
	}

	// wenn Initial das erste Skelett ist, liegt entrance auf Prozessor 0 (vgl. initSkeletons)
	Initial(const DFct1<Empty, O*>& f):
	Process(), fct(f) {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo++;
		exits[0] = entrances[0];
		setNextReceiver(0); // Empfaenger der ersten Nachricht festlegen
	}

	~Initial() {
		//std::cout << MSL_myId << ": ~Initial called" << std::endl;
		delete[] entrances;
		delete[] exits;
		entrances = NULL;
		exits = NULL;
	}

	void start() {
		// alle unbeteiligten Prozessoren rausfiltern
		finished = !(MSL_myId == entrances[0]);
		
		if(finished)
			return;
		// ******************

		O* pProblem = NULL;
		// void-parameter fuer Argumentfunktion
		Empty dummy;
		bool debug = false;

    	while(!finished) {
			// Argumentfunktion anwenden
			//std::cout << MSL_myId << ": Initial ruft init() auf." << std::endl;
			pProblem = fct(dummy);
			
			// wenn die Funktion NULL liefert, stop-tag senden
			if(pProblem == NULL) {
				for(int i = 0; i < numOfSuccessors; i++) {
					//std::cout << MSL_myId << ": Initial sendet STOP an " << successors[i] << std::endl;
					if(debug)
						std::cout << MSL_myId << ": Initial sendet STOP an " << successors[i] << std::endl;
					
					MSL_SendTag(successors[i], MSLT_STOP);
				}

				finished = true;
			}
			// ansonsten leite die Daten an einen bekannten Nachfolger weiter
			else {
				int receiver = getReceiver();
				
				if(debug)
					std::cout << MSL_myId << ": Initial sendet Problem an " << receiver << std::endl;
				
				MSL_Send(receiver, pProblem);
			}
		}

		if(debug)
			std::cout << MSL_myId << ": Initial terminiert. " << std::endl;
	}

	Process* copy() {
		return new Initial(fct);
	}

	void show() const {
		if(MSL_myId == 0)
			std::cout << "Initial (PID = " << entrances[0] << ")" << std::endl << std::flush;
	}

};

// ******************  Final Process  ********************

// Annahme: Final ist niemals Teil einer Farm
// Obwohl: warum eigentlich nicht? Damit kann man zur Not auch einen Prozessor sparen...(durchdenken!)
// Die Kommunikation erfolgt nichtblockierend und asynchron.
template<class Solution>
class Final: public Process {

    DFct1<Solution*,void> fct;
    // DFct1<Solution,Empty> fct; // ???

public:

	Final(void (* f)(Solution*)):
	Process(), fct(Fct1<Solution*, void, void (*)(Solution*)>(f)) {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo++;
		exits[0] = entrances[0];
		receivedStops = 0;			// Anzahl bereits eingegangener Stop-Tags
	}

	Final(const DFct1<Solution*,void>& f):
	Process(), fct(f) {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo++;
		exits[0] = entrances[0];
		receivedStops = 0;			// Anzahl bereits eingegangener Stop-Tags
	}

	~Final() {
		// std::cout << MSL_myId << ": ~Final called" << std::endl;
		delete[] entrances;
		delete[] exits;
		entrances = NULL;
		exits = NULL;
	}

	void start() {
		dbg({std::cout << MSL_myId << " in Final::start" << std::endl << std::flush;})
		// alle unbeteiligten Prozessoren rausfiltern
		finished = !(MSL_myId == entrances[0]);
		
		if(finished) return;
		// ******************

		bool debugCommunication = false;

		MPI_Status status; 				// Attribute: MPI_SOURCE, MPI_TAG, MPI_ERROR
		Solution* pSolution;
		int predecessorIndex = 0;
		receivedStops = 0;
		int flag;

		while(!finished) {
			// faire Annahme einer Nachricht von den Vorgängern

			flag = 0;

			while(!flag) {
				MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
				predecessorIndex = (predecessorIndex + 1) % numOfPredecessors;
			}

			ProcessorNo source = status.MPI_SOURCE;
			if(debugCommunication) std::cout << MSL_myId << ": Final empfängt Nachricht von " << source << std::endl;

			// TAGS
			if(status.MPI_TAG == MSLT_STOP) {
				if(debugCommunication)
					std::cout << MSL_myId << ": Final hat STOP empfangen" << std::endl;
				
				MSL_ReceiveTag(source, MSLT_STOP);
				receivedStops++;

				if(receivedStops == numOfPredecessors) {
					if(debugCommunication)
						std::cout << MSL_myId << ": Final hat #numOfPredecessors STOPS empfangen -> Terminiere" << std::endl;
					
					receivedStops = 0;
					finished = true;
				}
			}
			// user data
			else {
				if(debugCommunication)
					std::cout << MSL_myId << ": Final empfängt Daten" << std::endl;
				
				pSolution = new Solution();
				MSL_Receive(source, pSolution, MSLT_ANY_TAG, &status);
				fct(pSolution);
				//delete pSolution; // p4_error: interrupt SIGSEGV: 11 ???
				//pSolution = NULL; // p4_error: interrupt SIGSEGV: 11 ???
			}
		}
		// std::cout << MSL_myId << ": Final terminiert." << std::endl;
	}

	Process* copy() {
		return new Final(fct);
	}

	void show() const {
		if(MSL_myId == 0)
			std::cout << "Final (PID = " << entrances[0] << ")" << std::endl << std::flush;
	}

}; // ende Final

//***********************  Atomic Process  *************************
/* nicht blockierendes, asynchrones Receive
 * blockierendes, asynchrones Send
 * 21.2.06: Verhalten bei STOP
 * Sammeln von 1 STOP von jedem Vorgaenger, dann Versenden von STOP an jeden Nachfolger
 */
template<class Problem, class Solution>
class Atomic: public Process {

	// speichert die uebergebene Argumentfunktion f; Klasse DFct1 aus curry.h
    DFct1<Problem*,Solution*> fct;

	// anzahl gewuenschter Prozessoren fuer diesen Prozess
    int noprocs; // assumption: uses contiguous block of processors

public:

	// Konstruktor
	// Fct1 in Initialisierungsliste aus curry.h
	// Die Argumentfunktion f beschreibt, wie die Inputdaten transformiert werden sollen
	Atomic(Solution* (*f)(Problem*), int n):
	Process(), fct(Fct1<Problem*, Solution*, Solution* (*)(Problem*)>(f)), noprocs(n) {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setNextReceiver(0); // Default-Receiver ist 0. Den gibt's immer
		receivedStops = 0;			// Anzahl bereits eingegangener Stop-Tags
	}

	// Konstruktor
	// Fct1 in Initialisierungsliste aus curry.h
	// Die Argumentfunktion f beschreibt, wie die Inputdaten transformiert werden sollen
	Atomic(const DFct1<Problem*, Solution*>& f, int n):
	fct(f), noprocs(n), Process() {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setNextReceiver(0); // Default-Receiver ist 0. Den gibt's immer
		receivedStops = 0;			// Anzahl bereits eingegangener Stop-Tags
	}

	~Atomic() {
		delete[] entrances;
		delete[] exits;
		entrances = NULL;
		exits = NULL;
	}

	void start() {
		// unbeteiligte Prozessoren steigen hier aus
		finished = ((MSL_myId < entrances[0]) || (MSL_myId >= entrances[0] + noprocs));

		if(finished)
			return;
		// ******************

		bool debugCommunication = false;

		if(debugCommunication)
			std::cout << MSL_myId << ": starting Atomic" << std::endl;

		MPI_Status status; 				// Attribute: MPI_SOURCE, MPI_TAG, MPI_ERROR
		Problem* pProblem;
		Solution* pSolution;
		ProcessorNo source;

		MSL_myEntrance = entrances[0];
		MSL_myExit = exits[0];
		MSL_numOfLocalProcs = noprocs;
		int flag = 0;
		int predecessorIndex = 0;
		receivedStops = 0;

		// solange Arbeit noch nicht beendet ist
		while(!finished) {
			// Masterprocess
			if(MSL_myId == MSL_myEntrance) {
				// faire Annahme einer Nachricht von den Vorgängern
				flag = 0;
				
				if(debugCommunication)
					std::cout << MSL_myId << ": Atomic wartet auf Nachricht von " << predecessors[predecessorIndex] << std::endl;
				
				while(!flag) {
					MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
					predecessorIndex = (predecessorIndex +1 ) % numOfPredecessors;
				}

				source = status.MPI_SOURCE;
				
				if(debugCommunication)
					std::cout << MSL_myId << ": Atomic empfängt Nachricht von " << source << std::endl;
				
				// STOP_TAG
				if(status.MPI_TAG == MSLT_STOP) {
					if(debugCommunication)
						std::cout << MSL_myId << ": Atomic hat STOP empfangen" << std::endl;
					
					MSL_ReceiveTag(source, MSLT_STOP);
					receivedStops++;

					if(receivedStops == numOfPredecessors) {
						if(debugCommunication)
							std::cout << MSL_myId << ": Atomic hat #numOfPredecessors STOPS empfangen -> Terminiere" << std::endl;
						
						// genug gesammelt! alle eigenen Mitarbeiter alle nach Hause schicken
						for(int i = MSL_myEntrance + 1; i < MSL_myEntrance + noprocs; i++) {
							//std::cout << MSL_myId << ": Atomic sendet STOP an datenparallel eingesetzen Prozessor " << i << std::endl;
							MSL_SendTag(i, MSLT_STOP);
						}

						// alle Nachfolger benachrichtigen
						for(int i = 0; i < numOfSuccessors; i++) {
							//std::cout << MSL_myId << ": Atomic sendet STOP an " << successors[i] << std::endl;
							MSL_SendTag(successors[i], MSLT_STOP);
						}

						receivedStops = 0;
						finished = true;
					}
				}
				// user data
				else {
					if(debugCommunication)
						std::cout << MSL_myId << ": Atomic empfängt ein Problem" << std::endl;
					
					// Masterprocess empfängt ein neues Problem
					pProblem = new Problem();
					MSL_Receive(source, pProblem, MSLT_ANY_TAG, &status);

					// Problem an Mitarbeiter weiterleiten
					for(int i = MSL_myEntrance + 1; i < MSL_myEntrance + noprocs; i++) {
						if(debugCommunication)
							std::cout << MSL_myId << ": Atomic sendet Problem an datenparallelen Mitarbeiter " << i << std::endl;
						
						MSL_Send(i, pProblem);
					}
				} // user data
			}
			else { // Datenparallel eingesetzte Prozessoren warten auf den Parameter für die Argumentfunktion vom Masterprocess
				if(debugCommunication)
					std::cout << MSL_myId << ": Atomic-Mitarbeiter wartet auf Daten von " << MSL_myEntrance << std::endl;
				
				flag = 0;

				while(!flag) { // warte auf Nachricht
					MPI_Iprobe(MSL_myEntrance, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
				}

				if(status.MPI_TAG == MSLT_STOP) {
					MSL_ReceiveTag(MSL_myEntrance, MSLT_STOP);
					finished = true;
				}
				else {
					pProblem = new Problem();
					MSL_Receive(MSL_myEntrance, pProblem, MSLT_ANY_TAG, &status);
				}
			} // alle datenparallel eingesetzten Prozessoren haben nun den Parameter für die Argumentfunktion

			// das Ergebnis berechnen alle Prozessoren gemeinsam
			if(!finished) {
				pSolution = fct(pProblem); // pSolution wird ggf. datenparallel berechnet
				//delete pProblem; // p4_error: interrupt SIGSEGV: 11 ???
				//pProblem = NULL; // p4_error: interrupt SIGSEGV: 11 ???
				
				if(debugCommunication)
					std::cout << MSL_myId << ": intermediate result " << *pSolution << std::endl << std::flush;
				
				// nur der Masterprocess sendet Lösung an Nachfolger
				if(MSL_myId == MSL_myEntrance) {
					int receiver = getReceiver();
					
					if(debugCommunication)
						std::cout << MSL_myId << ": Atomic sendet Lösung an " << receiver << std::endl;
					
					MSL_Send(receiver, pSolution);
				}

				// DO NOT delete pSolution, if MSL_Send is based on a non blocking MPI-send-operation which returns
				// before the pSolution is written to the MPI send buffer!
				//delete pSolution;
				//pSolution = NULL;
			}
		} // ende while

		if(debugCommunication)
			std::cout << MSL_myId << ": Atomic finished " << std::endl << std::flush;
	} // ende start


	Process* copy() {
		return new Atomic(fct,noprocs);
	}

	void show() const {
		if(MSL_myId == 0)
			std::cout << "Atomic (PID = " << entrances[0] << ")" << std::endl << std::flush;
	}

}; // ende atomic2

// ****************************  Filter   ******************************

// class I: Input type
// class O: Output type
template<class I, class O>
class Filter: public Process {

    // speichert die uebergebene Argumentfunktion f; Klasse DFct1 aus curry.h
	DFct1<Empty,void> fct;

	// Anzahl der in diesem Skelett beteiligten Prozessoren
    int noprocs; // assumption: uses contiguous block of processors

public:

	// Konstruktor
	// Fct1 in Initialisierungsliste aus curry.h
	// Die Argumentfunktion f beschreibt, wie die Inputdaten transformiert werden sollen
	// und stuetzt sich auf die Methoden MSL_get und MSL_put (s.u.). Die Funktion kann ueber MSL_get bel.
	// viele Daten entgegennehmen und diese in bel. viele Outputdaten transformieren.
	Filter(void (* f)(Empty), int n):
	 fct(Fct1<Empty, void, void (*)(Empty) >(f)), noprocs(n), Process() {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setNextReceiver(0); // Default-Receiver ist 0. Den gibt's immer
		receivedStops = 0;
		receivedTT = 0;
		processSendTime = 0;
		processRecvTime = 0;
	}

	// Konstruktor
	// Fct1 in Initialisierungsliste aus curry.h
	// Die Argumentfunktion f beschreibt, wie die Inputdaten transformiert werden sollen
	// und sttzt sich auf die Methoden MSL_get und MSL_put (s.u.). Die Funktion kann ber MSL_get bel.
	// viele Daten entgegennehmen und diese in bel. viele Outputdaten transformieren.
	Filter(const DFct1<Empty,void>& f, int n):
	fct(f), noprocs(n), Process() {
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setNextReceiver(0); // Default-Receiver ist 0. Den gibt's immer
		receivedStops = 0;
		receivedTT = 0;
		processSendTime = 0;
		processRecvTime = 0;
	}

	void start() {
		// unbeteiligte Prozessoren steigen hier aus
		finished = ((MSL_myId < entrances[0]) || (MSL_myId >= entrances[0] + noprocs));
		
		if(finished)
			return;

		// ******************
		// alle Prozessoren, die am Prozess/Skelett beteiligt sind, merken sich in ihren globalen
		// Variablen den Ein- und Ausgang des Skeletts und einen Zeiger auf das Skelett selbst
		// Ein- und Ausgang des Skeletts merken
		MSL_myEntrance = entrances[0];	// entrance PID
		MSL_myExit = exits[0];			// exit PID
		MSL_myProcess = this;
		MSL_numOfLocalProcs = noprocs;	// size (> 1, if data parallel)
		receivedStops = 0;				// used for termination detection

		// uebergebene Funktion ohne Parameter aufrufen (dummy steht fuer leere Param-liste)
		// In dieser Funktion werden die Methoden MSL_get und MSL_put benutzt um Daten zu empfangen
		// und Daten zu senden. Auf diese Weise koennen beliebig viele Daten empfangen und
		// in beliebig viele Outputdaten transformiert werden.
		// Diese Funktion enthaelt i.d.R. eine Endlosschleife, in welcher die Daten verarbeitet werden,
		// bis ausreichend STOPs empfangen werden.
		Empty dummy;
		fct(dummy);
	} // ende start

	Process* copy() {
		return new Filter(fct, MSL_numOfLocalProcs);
	}

	void show() const {
		if(MSL_myId == 0)
			std::cout << "Filter (PID = " << entrances[0] << ")" << std::endl << std::flush;
	}

}; // ende filter

// Template-Funktion MSL_get: Liefert einen Zeiger auf ein empfangenes Datenpaket zurück.
// Diese Funktion sollte nur im Rumpf der Argumentfunktion des Filter-Prozesses aufgerufen werden.
// Wird ein MSLT_STOP vom Chef empfangen, wird dieses korrekt an die Slaves weitergeleitet. Ein
// empfangenes MSLT_STOP setzt den Zustand des Prozessors auf finished und seine Arbeit wird mit
// dem zurcksenden von NULL beendet.
// Get liefert der user defined function einen empfangenen Wert. Es duerfen daher keine(!)
// nicht-blockierenden receive-operationen verwendet werden, da sichergestellt sein muss, dass der user
// buffer die Daten auf jeden Fall enthält, wenn MSL_get() die Kontrolle an die aufrufende user function
// zurueckgibt.
// should only be used in the body of the argument function of Filter
template<class Problem>
Problem* MSL_get() {

	// Schutz gegen unbefugten Aufruf. Aufruf nur in Verbindung mit Filter erlaubt.
	if(MSL_myProcess == NULL)
		throws(IllegalGetException());
	
	// *************************

	bool debugCommunication = false;

	Problem* pProblem;
    MPI_Status status;
	ProcessorNo source;
	int flag = 0;
	int predecessorIndex = 0;
	ProcessorNo* predecessors = MSL_myProcess->getPredecessors();
	int numOfPredecessors = MSL_myProcess->getNumOfPredecessors();
	bool finished = false;

	// Master process nimmt Daten an und leitet sie ggf. an Slaves weiter
	if(MSL_myId == MSL_myEntrance) {
		// faire Annahme einer Nachricht von den Vorgängern
		flag = 0;
		
		if(debugCommunication)
			std::cout << MSL_myId << ": Filter::MSL_get wartet auf Nachricht von " << predecessors[predecessorIndex] << std::endl;
		
		while(!flag) {
			MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
			predecessorIndex = (predecessorIndex+1)%numOfPredecessors;
		}

		source = status.MPI_SOURCE;

		if(debugCommunication)
			std::cout << MSL_myId << ": Filter::MSL_get empfängt Nachricht von " << source << std::endl;
		
		// STOP_TAG
		if(status.MPI_TAG == MSLT_STOP) {
			if(debugCommunication)
				std::cout << MSL_myId << ": Filter::MSL_get hat STOP empfangen" << std::endl;
			
			MSL_ReceiveTag(source, MSLT_STOP);
			MSL_myProcess->addReceivedStops(); // STOPs sammeln
			
			if(MSL_myProcess->getReceivedStops() == MSL_myProcess->getNumOfPredecessors()) {
				if(debugCommunication)
					std::cout << MSL_myId << ": Filter::MSL_get hat #numOfPredecessors STOPS empfangen -> Terminiere" << std::endl;
				
				// genug gesammelt! alle eigenen Mitarbeiter alle nach Hause schicken
				for(int i = MSL_myEntrance + 1; i < MSL_myEntrance + MSL_numOfLocalProcs; i++) {
					//std::cout << MSL_myId << ": Filter::MSL_get sendet STOP an datenparallel eingesetzen Prozessor " << i << std::endl;
					MSL_SendTag(i, MSLT_STOP);
				}

				// alle Nachfolger benachrichtigen
				ProcessorNo* successors = MSL_myProcess->getSuccessors();
				int numOfSuccessors = MSL_myProcess->getNumOfSuccessors();
				
				for(int i = 0; i < numOfSuccessors; i++) {
					//std::cout << MSL_myId << ": Filter::MSL_get sendet STOP an " << successors[i] << std::endl;
					MSL_SendTag(successors[i], MSLT_STOP);
				}

				MSL_myProcess->resetReceivedStops();
				finished = true;
			}
			// noch nicht genug STOPS gesammelt. Gebe naechstes Datenpaket zurueck
			else return MSL_get<Problem>();
		}
		// user data
		else {
			if(debugCommunication)
				std::cout << MSL_myId << ": Filter::MSL_get empfängt ein Problem" << std::endl;
			
			// Masterprocess empfängt ein neues Problem
			pProblem = new Problem();
			MSL_Receive(source, pProblem, MSLT_ANY_TAG, &status);
			
			// Problem an Mitarbeiter weiterleiten
			for(int i = MSL_myEntrance + 1; i < MSL_myEntrance + MSL_numOfLocalProcs; i++) {
				if(debugCommunication) std::cout << MSL_myId << ": Filter::MSL_get sendet Problem an datenparallelen Mitarbeiter " << i << std::endl;
				MSL_Send(i, pProblem);
			}
		}
		// user data
	}
	// slaves
	else {
		// Datenparallel eingesetzte Prozessoren warten auf den Parameter für die Argumentfunktion vom Masterprocess
		if(debugCommunication)
			std::cout << MSL_myId << ": Atomic-Mitarbeiter wartet auf Daten von " << MSL_myEntrance << std::endl;
		
		flag = 0;

		while(!flag) { // warte auf Nachricht
			MPI_Iprobe(MSL_myEntrance, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
		}

		if(status.MPI_TAG == MSLT_STOP) {
			MSL_ReceiveTag(MSL_myEntrance, MSLT_STOP);
			finished = true;
		}
		else {
			pProblem = new Problem();
			MSL_Receive(MSL_myEntrance, pProblem, MSLT_ANY_TAG, &status);
		}
	}
	// alle datenparallel eingesetzten Prozessoren haben nun den Parameter für die Argumentfunktion
	
	// --- Master and Slave ---
	if(finished)
		return NULL;
	else
		return pProblem;
}

// should only be used in the body of the argument function of Filter
template<class Solution>
inline void MSL_put(Solution* pSolution) {
	if(MSL_myProcess == NULL)
		throws(IllegalPutException());

	// ************************

	bool debugCommunication = false;

	// nur der Masterprocess sendet Lösung an Nachfolger
	if(MSL_myId == MSL_myEntrance) {
		int receiver = MSL_myProcess->getReceiver();
		
		if(debugCommunication)
			std::cout << MSL_myId << ": Final::MSL_put sendet Lösung an " << receiver << std::endl;

		MSL_Send(receiver, pSolution);
	}

}

// **************************  Farm  ******************************
// *
// * Eine Farm verwaltet sich dezentral.
// * Alle Worker werden in einem logischen Ring verwaltet.
// * Jeder Worker dieser Farm kann Ein- bzw. Ausgang des Skeletts sein. Der vorgelagerte Prozess
// * waehlt per Zufall (gleichverteilte Zufallsvariable) einen Arbeiter aus, dem er eine Nachricht
// * zusendet. Handelt es sich hierbei um "normale" Daten, dann verarbeitet der worker diese und leitet
// * sie an einen der nachgelagerten Empfaenger weiter (es kann mehrere geben, z.B. wiederum eine Farm).
// * Handelt es sich um ein STOP- oder TERMINATION-TEST-TAG, so wird diese Nachricht einmal durch den
// * Ring geschickt, bis diese bei dem urspruenglichen Empfaenger wieder angekommen ist ("Stille
// * Post"-Prinzip). Dann leitet er diese Nachricht an einen der nachgelagerten Empfaenger weiter.
// *
// *******************************************************************

template<class I, class O>
class Farm: public Process {

    Process ** p;	// Worker
    int i, j, k;
	int length; 	// size der Farm

    public:

		Farm(Process& worker, int l): length(l), Process() {

			ProcessorNo* entr;
			ProcessorNo* ext;

			// Arbeiter sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new Process*[length];
            p[0] = &worker;
            for(i = 1; i < length; i++)
            	p[i] = worker.copy();

			// alle Worker sind gleichzeitig Ein- und Ausgang des Skeletts
			// Die Anzahl der Ein- und Ausgaenge der Farm ergibt sich aus der Anzahl der Worker
			// und der Anzahl ihrer Ein- und Ausgaenge

			// Eingaenge berechnen
			numOfEntrances = length * (*(p[0])).getNumOfEntrances();
			entrances = new ProcessorNo[numOfEntrances];
			k = 0;
			// alle worker durchlaufen
            for(i = 0; i < length; i++) {
				entr = (*(p[i])).getEntrances();
				// alle ihre Eingaenge durchlaufen
				for(j = 0; j < (*(p[i])).getNumOfEntrances(); j++) {
					entrances[k++] = entr[j];
				}
			}
			// Ausgaenge berechnen
			numOfExits = length * (*(p[0])).getNumOfExits();
			exits = new ProcessorNo[numOfExits];
			k = 0;
			// alle worker durchlaufen
            for(i = 0; i < length; i++) {
				ext = (*(p[i])).getExits();
				// alle ihre Ausgaenge durchlaufen
				for(j = 0; j < (*(p[i])).getNumOfExits(); j++) {
					exits[k++] = ext[j];
				}
			}
			// Empfaenger der ersten Nachricht festlegen
			setNextReceiver(0);
		}

		// Teilt allen workern die Sender mit.
		// Dies sind u.U. mehrere, z.B. dann, wenn eine Farm vorgelagert ist. In diesem Fall darf
		// jeder Worker dieser Farm von jedem worker der vorgelagerten Farm Daten entgegennehmen.
		// @param src	predecessors
		// @param l		arraysize
		inline void setPredecessors(ProcessorNo* src, int l) {
			numOfPredecessors = l;
			for(int i = 0; i < length; i++)
				(*(p[i])).setPredecessors(src,l);
		}

		// Teilt allen workern die Empfaenger mit.
		// Dies sind u.U. mehrere, z.B. dann, wenn eine Farm nachgelagert ist. In diesem Fall darf
		// jeder Worker dieser Farm an jeden worker der nachgelagerten Farm Daten senden.
		// @param drn	successors
		// @param l		arraysize
		inline void setSuccessors(ProcessorNo* drn, int l) {
			numOfSuccessors = l;
			for(int i = 0; i < length; i++)
				(*(p[i])).setSuccessors(drn,l);
		}

        // startet alle worker
		inline void start() {
            for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

        Process* copy() { return new Farm(*((*(p[0])).copy()),length);}

        inline void show() const {
            if(MSL_myId == 0) {
                std::cout << MSL_myId << " Farm " << entrances[0] << std::endl << std::flush;
                for(int i = 0; i < length; i++)
                    (*(p[i])).show();
			}
		}
}; // end of Farm



// *********************   Pipe   ***********************

// TODO: Beschreibung Überarbeiten! Kommt noch aus Skeleton.h
// Diese Klasse konstruiert eine Pipeline aus den übergebenen Prozessen.
// Ein Prozess kann ein beliebiges Konstrukt aus verschachtelten taskparallelen Skeletten
// sein. Ein solches Konstrukt hat mindestens einen Eingang (entrances[0]) und Ausgang (exits[0])
// zum Empfangen bzw. zum Senden von Daten. Dies entspricht in gewissem Sinne einer Blackbox-
// Sichtweise. Wie das Konstrukt intern aus anderen Prozessen zusammengesetzt bzw. verknüpft ist,
// interessiert hier nicht weiter. Es wird lediglich eine Schnittstelle zur Kommunikation mit
// einem Prozess definiert. Die interne Verknpfung (hier als Pipeline) wird über predecessors[] und
// successors[] erreicht. Der Eingang des Pipe-Konstrukts ist der Eingang des ersten Konstrukts
// in der Pipe (entrance = p1.getEntrance()) und der Ausgang ist entsprechend der Ausgang des
// letzten Konstrukts in der Pipe (exit = p3.getExit()).
// Um die Verkettung zu erreichen muss das erste Konstrukt in der Pipe an den Eingang des zweiten
// Konstrukts in der Pipe Daten schicken, das zweite Konstrukt in der Pipe an den Eingang des
// dritten Konstruks usw. (p1.setOut(p2.getEntrance()), p2.setOut(p3.getEntrance())...). Nun muss
// den Konstrukten noch mitgeteilt werden, von welchem Ausgang eines anderen Konstrukt es Daten
// empfangen kann. Also Konstrukt 2 erwartet Daten von K1, K3 von K2 etc.
// (p2.setIn(p1.getExit()),...).

class Pipe: public Process {

	// Zeiger auf Array von Adressen der Prozesse
    Process** p;
	int i;
	// Laenge der Pipe
    int length;

  	public:
		// Konstruktor fr 2 Prozesse
		// @param p1	Adresse von Prozess1
		// @param p2	Adresse von Prozess2
    	Pipe(Process& p1, Process& p2): Process() {

			setNextReceiver(0);
			// Laenge der Pipe merken und Speicherplatz fuer die Pipe-Elemente reservieren
       		length = 2;
        	p = new Process* [length];

			// interne Verknpfung herstellen
			p1.setSuccessors(p2.getEntrances(),p2.getNumOfEntrances());	// p1 schickt an p2
        	p2.setPredecessors(p1.getExits(),p1.getNumOfExits());		// p2 empfaengt von p1

			// fuer jeden Worker der Empfaenger seiner ersten Nachricht festlegen
			if(MSL_DISTRIBUTION_MODE==MSL_CYCLIC_DISTRIBUTION) {
				// bestimme die Anzahl der Ausgaenge von p1 und die Anzahl der Eingaenge von p2
				int p1Exits = p1.getNumOfExits();
				int p2Entrances = p2.getNumOfEntrances();
				// Skelette mit einem Eingang oder Ausgang sind per Default korrekt verknuepft
				// derzeit muessen nur Farmen mit Farmen vernetzt werden
				if(p1Exits>1 && p2Entrances>1) {
					// weise nun Ausgang von p1 zyklisch einen der Eingaenge von p2 zu
					int recv;
					for(int skel=0; skel<p1Exits; skel++) {
						recv = skel % p2Entrances;
						p1.setNextReceiver(recv);
					}
				}
			}
			// Eingang und Ausgang der Pipe merken
        	entrances = p1.getEntrances();
			numOfEntrances = p1.getNumOfEntrances();
        	exits = p2.getExits();
			numOfExits = p2.getNumOfExits();

			// Adressen der beteiligten Prozesse sichern
        	p[0] = &p1;
			p[1] = &p2;

		} // ende Kunstruktor


		// Konstruktor fr 3 Prozesse
    	Pipe(Process& p1, Process& p2, Process& p3): Process() {

			setNextReceiver(0);
			// Laenge der Pipe merken und Speicherplatz fuer die Pipe-Elemente reservieren
			length = 3;
			p = new Process* [length];

			// interne Verknpfung herstellen
			p1.setSuccessors(p2.getEntrances(),p2.getNumOfEntrances());	// p1 schickt an p2
        	p2.setPredecessors(p1.getExits(),p1.getNumOfExits());		// p2 empfaengt von p1
        	p2.setSuccessors(p3.getEntrances(),p3.getNumOfEntrances());	// p2 schickt an p3
        	p3.setPredecessors(p2.getExits(),p2.getNumOfExits());		// p3 empfaengt von p2

			// erst jetzt kann fuer jeden Worker der Empfaenger seiner ersten Nachricht
			// festgelegt werden
			if(MSL_DISTRIBUTION_MODE==MSL_CYCLIC_DISTRIBUTION) {
				// bestimme die Anzahl der Ausgaenge von p1 und die Anzahl der Eingaenge von p2
				int p1Exits = p1.getNumOfExits();
				int p2Entrances = p2.getNumOfEntrances();
				// Skelette mit einem Eingang oder Ausgang sind per Default korrekt verknuepft
				// Farms und Pipes koennen mehrere Ein- und Ausgaenge haben, die vernetzt werden muessen
				if(p1Exits>1 && p2Entrances>1) {
					// weise nun Ausgang von p1 zyklisch einen der Eingaenge von p2 zu
					int recv;
					for(int skel=0; skel<p1Exits; skel++) {
						recv = skel % p2Entrances;
						p1.setNextReceiver(recv);
					}
				}
				// (zur besseren Lesbarkeit wurden fuer Prozess 2 und 3 neue Variablen definiert)
				int p2Exits = p2.getNumOfExits();
				int p3Entrances = p3.getNumOfEntrances();
				if(p2Exits>1 && p3Entrances>1) {
					// weise nun Ausgang von p2 zyklisch einen der Eingaenge von p3 zu
					int recv;
					for(int skel=0; skel<p1Exits; skel++) {
						recv = skel % p2Entrances;
						p1.setNextReceiver(recv);
					}
				}
			}

			// debug
			dbg({
				ProcessorNo* no;
				int num;
				std::cout << "Pipe::Pipe --> verknuepfe Prozesse" << std::endl;
				no = p2.getEntrances();
				num = p2.getNumOfEntrances();
				std::cout << "setze << " << num << "Empfaenger von p1 (init): ";
				for(i = 0; i < num; i++) std::cout << no[i] << "  ";
				std::cout << std::endl;
				no = p1.getExits();
				num = p1.getNumOfExits();
				std::cout << "setze << " << num << "Quellen von p2 (farm) (worker): ";
				for(i = 0; i < num; i++) std::cout << no[i] << "  ";
				std::cout << std::endl;
				no = p3.getEntrances();
				num = p3.getNumOfEntrances();
				std::cout << "setze << " << num << "Empfaenger von p2 (farm) (worker): ";
				for(i = 0; i < num; i++) std::cout << no[i] << "  ";
				std::cout << std::endl;
				no = p2.getExits();
				num = p2.getNumOfExits();
				std::cout << "setze << " << num << "Quellen von p3 (final): ";
				for(i = 0; i < num; i++) std::cout << no[i] << "  ";
				std::cout << std::endl;})

			// Eingang und Ausgang der Pipe merken
    	    entrances = p1.getEntrances();
			numOfEntrances = p1.getNumOfEntrances();
	        exits = p3.getExits();
			numOfExits = p3.getNumOfExits();

			// Adressen der uebergebenen Prozesse sichern
        	p[0] = &p1;
			p[1] = &p2;
			p[2] = &p3;
		} // ende Konstruktor


		~Pipe() {
			delete[] p;
			p = NULL;
		}


		// setzt alle Nachfolger der Pipe
		inline void setSuccessors(ProcessorNo* drn, int len) {
			numOfSuccessors = len;
			successors = new ProcessorNo[len];
			for(int i = 0; i < len; i++)
				successors[i] = drn[i];
			(*(p[length-1])).setSuccessors(drn, len);
		}

		// setzt alle Vorgaenger der Pipe
    	inline void setPredecessors(ProcessorNo* src, int len) {
			numOfPredecessors = len;
			predecessors = new ProcessorNo[len];
			for(i = 0; i < len; i++)
				predecessors[i] = src[i];
			(*(p[0])).setPredecessors(src,len);
		}

		// der Reihe nach werden alle Prozesse in der Pipe gestartet
    	void start() {
			for(int i = 0; i < length; i++) (*(p[i])).start();
		}

		// erstellt eine Kopie der Pipe und liefert ihre Adresse
    	Process* copy() {
      		return new Pipe(*((*(p[0])).copy()), *((*(p[1])).copy()));
		}

		// zeigt auf, welche Prozesse in der Pipe haengen
    	void show() const {
			if(MSL_myId == 0) {
				std::cout << std::endl;
				std::cout << "**********************************************************************" << std::endl;
				std::cout << "*                         Process-Topology                           *" << std::endl;
				std::cout << "**********************************************************************" << std::endl;
				std::cout << "Pipe (entrance at " << entrances[0] << ") with " << length << " stage(s): " << std::endl;
				for(int i = 0; i < length; i++) {
					std::cout << "  Stage " << (i+1) << ": ";
      				(*(p[i])).show();
				}
				std::cout << "**********************************************************************" << std::endl;
				std::cout << std::endl;
			}
		}
};







// *** demand driven work distribution ***

/**********************************************************************************************
 *  Lastverteilung: mit best. WS
 * Jeder Prozessor schickt an seinen linken Nachbarn eine Info-Message mit der lowerBound
 * seines besten Teilproblems im Workpool. Eine Lastverteilung erfolgt nur dann,
 * wenn der linke Nachbar ein Problem abgeben kann, das besser ist. LB-Info-Messages werden
 * immer verschickt, wenn der Solver einen nicht leeren Workpool hat. Ist sein Pool leer, wird
 * nur einmalig eine Empty-Pool-Nachricht verschickt. Dies verhindert, dass am Anfang das
 * System mit Anfragen überschwemmt werden.
 */


// ************************************************************************************************************************
// *** Workpool für Branch and Bound Skeleton
// ***
// *** - best first strategy implemented
// ***
// ************************************************************************************************************************
//
template<class I>
class Workpool {

	bool debug;

	int last;
    int size;
    I* heap;
    DFct2<I,I,bool> betterThan;


  public:

  	// was passiert beim aufruf eines Destruktors???



  	Workpool(const DFct2<I,I,bool>& less): betterThan(less), last(-1), size(8) {
  		//std::cout << MSL_myId <<" ruft Konstruktor 1 auf"<<std::endl;
  		heap = new I[size];
  		debug = false;
  	}

  	Workpool(bool (*less)(I,I)): last(-1), size(8), betterThan(Fct2<I,I,bool, bool (*)(I,I)>(less)) {
  		//std::cout << MSL_myId <<" ruft Konstruktor 2 auf"<<std::endl;
  		heap = new I[size];
  		debug = false;
  	}

  	bool isEmpty() {return last < 0;}

  	I top() {
    	if(isEmpty()) throws(EmptyHeapException());
    	return heap[0];
    }

  	I get() {
    	if(isEmpty()) throws(EmptyHeapException());
    	I result = heap[0];
    	heap[0] = heap[last--];
    	int current = 0;
    	int next = 2*current+1;
    	while(next <= last+1) {
      		if((next <= last) && (betterThan(heap[next+1],heap[next]))) next++;
      		if(betterThan(heap[next],heap[current])) {
        		I aux = heap[next]; heap[next] = heap[current];  heap[current] = aux;
        		current = next;
        		next = 2*current+1;
        	}
      		else next  = last+2;
      	} // stop while loop
    	return result;
    }

  	void insert(I val) {
    	int current = ++last;
    	if(last >= size) { // extend heap
      		if(debug) std::cout << "Workpool::insert() : extending heap" << std::endl << std::flush;
      		I* newheap = new I[2*size];
      		for(int i = 0; i < size; i++) newheap[i] = heap[i];
      		size *= 2;
      		if(debug) std::cout << "Workpool::insert() : deleting old heap" << std::endl;
      		//delete[] heap;
      		if(debug) std::cout << "Workpool::insert() : heap deleted" << std::endl;
      		heap = newheap;
      	}
    	int next;
    	heap[last] = val;
    	while(current > 0) {
      		next = (current-1)/2;
      		if(betterThan(heap[current],heap[next])) {
        		I aux = heap[next];
        		heap[next] = heap[current];
        		heap[current] = aux;
        		current = next;
        	}
      		else current = 0;
      	}
    } // stop loop

  	DFct2<I,I,bool> getBetterThanFunction() {
  		return betterThan;
  	}

	void reset() { last = -1; }

  	Workpool<I>* fresh() {
  		if(debug) std::cout << "Workpool::fresh() invoked"<<std::endl;
  		Workpool<I>* h = new Workpool<I>(betterThan);
        return h;
    }

  	void show(ProcessorNo n) {
    	std::cout << "Prozessor" << n << " hat Workpool: [";

    	for(int i = 0; i <= last; i++)
      		std::cout << heap[i] << " ";

    	std::cout << "]" << std::endl << std::flush;
    }

};

// ************ Variante 2: passive Lastverteilung mit Arbeitsanfragen, ALLE interne Nachrichten lesen **************

// Empfang von Incumbents und Problemen getrennt.

// TODO: Prüfen, was passiert, wenn keine Lsg. gefunden wird. Was wird dann an den Nachfolger geschickt?


template<class Problem>
class BBSolverOld: public Process {

private:

	bool blocked;					// state of Mastersolver
	ProcessorNo* entranceOfSolver;	// Eingänge aller Solver (inkl. this): entranceOfSolver[0] ist Master
	ProcessorNo* exitOfSolver; 		// Ausgänge aller Solver (inkl. this): exitOfSolver[0] ist Master
	ProcessorNo pred;				// Ringstruktur: Solver empfängt Token von pred
	ProcessorNo succ;				// Ringstruktur: Solver sendet Token an succ
	Problem incumbent;					// best solution found so far
	bool solutionFound;				// indicates if incumbent found
	int numOfSolvers;				// Anzahl der Solver, die durchs BnB-Skelett gruppiert werden
	// int indexID; 					// an dieser Position findet man sich selbst in den Arrays
    int noprocs; 					// assumption: uses contiguous block of processors
	bool (*isSolution)(Problem);			// Zeiger auf benutzerdefinierte Funktion
	int (*getLowerBound)(Problem);
	bool (*betterThan)(Problem,Problem);	// Zeiger auf benutzerdefinierte Funktion
	Problem (*bound)(Problem);				// Zeiger auf bound-Methode. Schätzt Subproblem ab.
	Problem* (*branch)(Problem,int*);		// Zeiger auf branch-Methode. Liefert Zeiger auf Array mit Zeigern auf erzeugte Subprobleme
    Workpool<Problem>* workpool; 			// collection.h
    double timerStart, timerStop;

public:

	/**
	 * Rules:
	 * branchingRule: liefert Array mit Subproblemen; Param: Problem, Zeiger auf Länge des erzeugten Arrays
	 * boundingRule: schätzt Problem ab; Param: Zeiger auf zu manilpulierende Datenstruktur
	 * selectionRule: basiert auf der best first strategy; workpool wird durch lth organisiert
	 * eliminationRule: ist implizit im Workpool vorhanden  (nur so halb)
	 */
/*	BBSolverOld(Problem* (*br)(Problem,int*), Problem (*bnd)(Problem), bool (*lth)(Problem,Problem), bool (*isSol)(Problem), int (*getLB)(Problem), int n):
				noprocs(n), betterThan(lth), isSolution(isSol), branch(br), bound(bnd), getLowerBound(getLB), Process() {
      	workpool = new Workpool<Problem>(lth);	// workpool anlegen
      	solutionFound = false;
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setStartReceiver(0); 			// Default-Receiver ist 0. Den gibt's immer
		processSendTime = 0;
		processRecvTime = 0;
	}*/


/*	void start() {

		//debugmessages
		bool debugCommunication = false;
		bool debugTerminationDetection = false;
		bool debugComputation = false;
		bool debugFreeMem = false;

		// für den externen Nachrichtenverkehr
		MPI_Request* externalRequest 			= new MPI_Request[numOfPredecessors];
		Problem* externalMessage 				= new Problem[numOfPredecessors];	// buffer fuer Input vom Vorgaenger
		// für den internen Nachrichtenverkehr
		MPI_Request* internalProblemRequest 	= new MPI_Request[numOfSolvers];
		MPI_Request* internalIncumbentRequest 	= new MPI_Request[numOfSolvers];
		MPI_Request* internalStopRequest 		= new MPI_Request[numOfSolvers];
		MPI_Request* internalTokenRequest 		= new MPI_Request[numOfSolvers];
		MPI_Request* internalLoadInfoRequest 	= new MPI_Request[numOfSolvers];
		Problem* internalProblemMessage 		= new Problem[numOfSolvers];	// intern verschickte Daten
		Problem* internalIncumbentMessage 		= new Problem[numOfSolvers];	// intern verschickte Daten
		// TODO: für STOPs keine Probleme verschicken, sondern irgendwas kleines, wie z.B. int !!!
		// Wird bereits gemacht: Datentyp int!
		Problem* internalStopMessage 			= new Problem[numOfSolvers];	// intern verschickte Daten
		int* internalTokenMessage 				= new int[numOfSolvers];	// intern verschickte Daten
		int* internalLoadInfoMessage 			= new int[numOfSolvers];	// intern verschickte Daten
		int* internalMessageIndex 				= new int[numOfSolvers];
		MPI_Status* internalMessageStatus 		= new MPI_Status[numOfSolvers];

		Problem problem; 							// buffer fuer aktuellen Input
		MPI_Status stat; 							// Attribute: MPI_SOURCE, MPI_TAG, MPI_ERROR
		int token = -1;								// Problem is solved, if this value is 0
		int loadInfo;
		int myBestLB;
        int whoIsReady;								// Index auf die beendete Kommunikationsoperation
		int receivedStops = 0;						// Anzahl bereits eingegangener Stop-Tags
		int receivedTT = 0;							// Anzahl bereits eingegangener TerminationTest-Tags
		bool newProblemReceived = false;
		bool emptyPoolFlag = false;
		int errorcode;


		// am Prozess beteiligte Prozessoren merken sich entrance und exit des Solvers
		finished = ((MSL_myId < entrances[0]) || (MSL_myId >= entrances[0] + noprocs));
		if(!finished) {
			MSL_myEntrance = entrances[0]; 	// Eingang des BBSolvers merken
			MSL_myExit = exits[0];			// Ausgang des BBSolvers merken
		}
		// unbeteiligte Prozessoren steigen hier aus
		if(Debug && finished) { std::cout << MSL_myId << " not concerned; leaving " << std::endl << std::flush; }
		if(finished) return;

		// Start der Zeitmessung
		//if(MSL_TIMER) { processBeginTime = MPI_Wtime(); }

		std::vector<unsigned long> v;
		int anzahlBearbeiteterProbleme = 0;
		bool analyse = false;
		bool logIDs = false;

		int messagesReceived = 0;
		int messagesSend = 0;
		int lbMessagesReceived = 0;
		int lbMessagesSend = 0;

		// alle anderen merken sich die Anzahl der am Skelett beteiligten Prozessoren und den masterSolver-Eingang
		MSL_numOfLocalProcs = noprocs;
		ProcessorNo masterSolver = entranceOfSolver[0];


		// solange Arbeit noch nicht beendet ist
		blocked = false;
		int flag = 0;

		int predecessorIndex = 0; // Index auf den Ausgang des ersten Vorgängers

		while(!finished) {

			// PROBLEMANNAHME:
			// Nur der Master-Solver kann im Zustand FREE externe Nachrichten (Probleme und TAGs) annehmen
			if(MSL_myId == masterSolver && !blocked) {
				if(debugCommunication) std::cout << "(BBSolverOld::start): Prozessor " << MSL_myId << " wartet auf neues Problem" << std::endl;
				// Verbleibe in dieser Schleife, bis das nächste Problem ankommt oder Feierabend ist
				while(!blocked && !finished) {
					// AKTIVES WARTEN: blockiere bis Nachricht vom Vorgänger eingetroffen ist (Problem oder STOP möglich)
					flag = 0;
					if(debugCommunication) std::cout << MSL_myId << ": Mastersolver wartet auf neues Problem" << std::endl;
					while(!flag) {
						MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
						predecessorIndex = (predecessorIndex+1)%numOfPredecessors;
					}
					if(debugCommunication) std::cout << MSL_myId << ": Mastersolver empfängt Daten von " << status.MPI_SOURCE << " mit Tag = " << status.MPI_TAG << std::endl;
					// TAGS verarbeiten (TODO: optimierbar, wenn man Reihenfolge ändert: 1.) Problem, 2.) TAG)
					ProcessorNo source = status.MPI_SOURCE;
					if(status.MPI_TAG == MSLT_TERMINATION_TEST) {
						// Nimm TT-Nachricht entgegen
						MSL_ReceiveTag(source, MSLT_TERMINATION_TEST);
						// Mache erstmal nichts
					}
					else if(status.MPI_TAG == MSLT_STOP) {
						// Nimm STOP-Nachricht entgegen
						MSL_ReceiveTag(source, MSLT_STOP);
						if(debugCommunication) std::cout << MSL_myId << ":(BBSolverOld::start) Mastersolver hat STOP empfangen" << std::endl;
						receivedStops++; // Tags sammeln
						// wenn genug STOPs gesammelt
						if(receivedStops==numOfPredecessors) {
							if(debugCommunication) std::cout << "(BBSolverOld::start): Mastersolver hat #numOfPredecessors STOPS empfangen -> Terminiere" << std::endl;
							// alle solverinternen Prozessoren benachrichtigen (falls Solver datenparallel arbeitet)
							for(int i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)
								MSL_SendTag(i, MSLT_STOP);
							// alle Solver (außer sich selbst) benachrichtigen
							if(numOfSolvers > 1) {
								for(int i = 0; i < numOfSolvers; i++) {
									if(entranceOfSolver[i]!=MSL_myId) {
										if(debugCommunication) std::cout << MSL_myId << ":(BBSolverOld::start) Mastersolver sendet STOP intern an "<<entranceOfSolver[i]<<std::endl;
										MSL_SendTag(entranceOfSolver[i], MSLT_STOP);
									}
								}
							}
							// alle Nachfolger benachrichtigen
							for(int i = 0; i < numOfSuccessors; i++) {
								if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) Mastersolver schickt STOP an Nachfolger "<<successors[i]<<std::endl;
								MSL_SendTag(successors[i], MSLT_STOP);
							}
							receivedStops = 0;
							finished = true; // Feierabend! BBSolverOld terminiert...
						}
					}
					// wenn's kein TAG war, war's ein neues Optimierungsproblem: -> RampUp!
					else {
						problem = externalMessage[whoIsReady];
						messagesReceived++;
						if(debugCommunication) std::cout << "(BBSolverOld::start) Mastersolver empfängt Problem" << std::endl;
						// sende Problem an alle solverinternen Prozessoren
						for(i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)
							send(i, &problem, sizeof(Problem));
						// Problem (ggf. parallel) abschätzen und in lokalen Workpool schreiben
						// ### TODO ### wird so nicht datenparallel funktionieren!!!
						problem = bound(problem);
						// nur ungelöste Probleme werden verarbeitet.
						if(!isSolution(problem)) {
							// Blockiere Solver bis Problem gelöst ist
							blocked = true; // Abbruch der inneren while-Schleife
							if(debugCommunication) std::cout << "(BBSolverOld::start) problem saved in workpool: lb="<<problem.lowerBound<< std::endl;
							(*workpool).insert(problem);
							// optional: RampUp-Algorithmus (generiere M > N Teilprobleme und verteile alle M Teilprobleme auf die N Solver)
							// initiate termination detection
							// Termination Detection nur nötig, wenn es mehr als einen Solver gibt.
							if(numOfSolvers>1) {
								newProblemReceived = true;
								if(debugCommunication) std::cout << "(BBSolverOld::start) "<<MSL_myId <<" verschickt Token "<<numOfSolvers<<" an "<<succ << std::endl;
								sendToken(numOfSolvers,succ); // Token für TerminationDetection losschicken (Token = #Solver)
							}
						}
						// ein bereits gelöstes Problem wird direkt weitergeleitet.
						else {
							if(MSL_DISTRIBUTION_MODE==MSL_RANDOM_DISTRIBUTION) MSL_send(getRandomReceiver(), &problem, sizeof(Problem));
							else if(MSL_DISTRIBUTION_MODE==MSL_CYCLIC_DISTRIBUTION) MSL_send(getNextReceiver(), &problem, sizeof(Problem));
							messagesSend++;
						}
					}
					// erwarte neue Nachricht vom letzten Sender
					nonblockingReceive(sources[whoIsReady], &externalMessage[whoIsReady], sizeof(Problem), &externalRequest[whoIsReady]);
				} // end of inner while
				// hier angekommen, wurde entweder ein neues Optimierungsproblem empfangen (Workpool nicht leer) oder es ist Feierabend
			} // ende Problemannahme


			// INTERNER NACHRICHTENVERKEHR zwischen den Solvern, falls es mehrere gibt:
			if((numOfSolvers>1) && (!finished) && (MSL_myId==MSL_myEntrance)) {

				// 1. Testen, ob neue Incumbents angekommen sind.
				int numOfCompletedReq = 0;
            	MPI_Testsome(numOfSolvers, &internalIncumbentRequest[0], &numOfCompletedReq, &internalMessageIndex[0], &internalMessageStatus[0]);
            	messagesReceived += numOfCompletedReq;
				// verarbeite alle empfangenen Incumbents
				for(int i = 0; i < numOfCompletedReq;i++) {
					problem = internalIncumbentMessage[internalMessageIndex[i]];
					if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " hat Incumbent empfangen. lb=" << problem.lowerBound  << std::endl;
					// wenn man selber noch keine Lösung gefunden hat oder empfangene Lösung besser als eigene ist
					if(!solutionFound || betterThan(problem, incumbent)) {
						// lokales Incumbent-Update
						if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " merkt sich empfangenen Incumbent. lb=" << problem.lowerBound << std::endl;
						incumbent = problem;
						solutionFound=true;
					} else { // sonst ist der empfangene Incumbent schlechter und wird hier verworfen.
						if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " verwirft empfangenen Incumbent, weil bereits ein besserer Incumbent gefunden wurde"<< std::endl;
					}
					MPI_Irecv(&internalIncumbentMessage[internalMessageIndex[i]], sizeof(Problem), MPI_BYTE, internalMessageStatus[i].MPI_SOURCE, MSLT_BBINCUMBENT_TAG, MPI_COMM_WORLD, &internalIncumbentRequest[internalMessageIndex[i]]);
				}


				// 2. Testen, ob neue Probleme angekommen sind.
				numOfCompletedReq = 0;
            	MPI_Testsome(numOfSolvers, &internalProblemRequest[0], &numOfCompletedReq, &internalMessageIndex[0], &internalMessageStatus[0]);
            	messagesReceived += numOfCompletedReq;
				// verarbeite alle empfangenen Probleme
				for(int i = 0; i < numOfCompletedReq;i++) {
					// wenn Problem empfangen (ist bereits mit bound abgeschätzt worden!)
					problem = internalProblemMessage[internalMessageIndex[i]];
					// schreibe Teilproblem in Workpool, wenn es zum Optimum führen kann
					if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " empfängt ungelöstes Problem. lb=" << problem.lowerBound  <<std::endl;
					if(!solutionFound || betterThan(problem,incumbent)) {
						if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " schreibt Problem in den workpool. lb=" << problem.lowerBound  << std::endl;
						(*workpool).insert(problem);
					}
					// sonst verwerfe Teilproblem
					else { if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " verwirft Problem (lb="<<problem.lowerBound<<"), weil Incumbent besser ist. lb=" << incumbent.lowerBound  << std::endl; }
					newProblemReceived = true;
					// warte auf die nächste Nachricht vom Sender
					MPI_Irecv(&internalProblemMessage[internalMessageIndex[i]], sizeof(Problem), MPI_BYTE, internalMessageStatus[i].MPI_SOURCE, MSLT_PROBLEM_TAG, MPI_COMM_WORLD, &internalProblemRequest[internalMessageIndex[i]]);
				} // Ende interne Problemannahme


				// 3. Termination Detection Algorithm
				MPI_Testany(numOfSolvers, &internalTokenRequest[0], &whoIsReady, &flag, &stat); // MPI_Test(IN,OUT,OUT)
				if(flag==true) {
					token = internalTokenMessage[whoIsReady];
					if(debugTerminationDetection) std::cout << "Prozessor "<< MSL_myId << " hat Token "<<token<<" angenommen" << std::endl;
					// wenn Pool leer && neuesProblem = false
					if((*workpool).isEmpty() && !newProblemReceived) {
						token--;
						if(debugTerminationDetection) std::cout << "workpool leer, token=" << token << "   newProblemReceived="<<newProblemReceived<<std::endl;
					}
					// sonst wenn Pool nicht leer || neuesProblem = true
					else {
						if(!(*workpool).isEmpty() || newProblemReceived) {
							token = numOfSolvers;
							newProblemReceived = false;
							if(debugTerminationDetection) std::cout << "workpool nicht leer oder newProblemReceived, token=" << token << "   newProblemReceived="<<newProblemReceived<<std::endl;
						}
					}
					// wenn Token == 0 (& Pool leer & neuesProblem = false)
					if(token == 0) {
						if(debugTerminationDetection) std::cout << "Prozessor "<<MSL_myId<<" beendet die Berechnung und verschickt Incumbent "<<std::endl;
						// Incumbent verschicken
						if(debugTerminationDetection)	if(!solutionFound) std::cout << "keine Lösung gefunden!!" << std::endl;
						if(MSL_DISTRIBUTION_MODE==MSL_RANDOM_DISTRIBUTION) MSL_send(getRandomReceiver(), &incumbent, sizeof(Problem));
						else if(MSL_DISTRIBUTION_MODE==MSL_CYCLIC_DISTRIBUTION) MSL_send(getNextReceiver(), &incumbent, sizeof(Problem));
						// wenn ich der Mastersolver bin
						if(MSL_myId == masterSolver) {
							blocked = false; // löse die Blockierung
							solutionFound=false; // TODO: alle anderen Solver müssen auch dieses Flag zurücksetzen!!!
							if(debugTerminationDetection) std::cout << "Mastersolver wird in den Anfangszustand versetzt (nicht blockiert, keine Lösung gefunden)" << std::endl;
						}
						// sonst bin ich's nicht und sende Token = -1 an Master (FREE-Message)
						else {
							if(debugTerminationDetection) std::cout << "sende token=" << -1 << " an "<<masterSolver<<std::endl;
							sendToken(-1, masterSolver);
						}
					}
					else {
						// wenn Token > 0 sende Token an nächsten Solver
						if(token > 0) {
							if(debugTerminationDetection) std::cout << "sende token=" << token << " an "<<succ<<std::endl;
							sendToken(token, succ);
						}
						// sonst ist Token == -1 (FREE-Message "-1" wird nur an Master geschickt und nur von diesem empfangen)
						else {
							if(debugTerminationDetection) std::cout << "Mastersolver löst Blockierung und wartet auf neue Nachricht vom Vorgänger" << std::endl;
							blocked = false;
						}
					}
					// auf nächstes Token warten
					MPI_Irecv(&internalTokenMessage[whoIsReady], sizeof(int), MPI_BYTE, exitOfSolver[whoIsReady], MSLT_TOKEN_TAG, MPI_COMM_WORLD, &internalTokenRequest[whoIsReady]);
				} // end of termination detection


				// 4. Prüfe auf STOPS (Mastersolver empfängt nie STOP als interne Nachricht)
				MPI_Testany(numOfSolvers, &internalStopRequest[0], &whoIsReady, &flag, &stat); // MPI_Test(IN,OUT,OUT)
				if(flag==true) {
					if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " hat STOP von "<<stat.MPI_SOURCE<<" angenommen"  << std::endl;
					// alle solverinternen Prozessoren benachrichtigen (falls Solver datenparallel ist)
					for(int i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)
						sendStop(i);
					// sende STOP an Nachfolger (alter Terminierungsalgorithmus)
					for(int i = 0; i < numOfSuccessors; i++)
						sendStop(drains[i]);
					receivedStops = 0;
					finished = true; // Feierabend
					MPI_Irecv(&internalStopMessage[whoIsReady], sizeof(int), MPI_BYTE, exitOfSolver[whoIsReady], MSLT_STOP, MPI_COMM_WORLD, &internalStopRequest[whoIsReady]);
					if(debugCommunication) std::cout << "Prozessor "<< MSL_myId << " terminiert"<< std::endl;
				}

				// 5. Lastverteilung
				if(numOfSolvers>1) {
					// verschicke die LowerBound des besten eigenen Subproblems an den Vorgänger (pred)
					if((*workpool).isEmpty()) {
						// bei leerem Pool wird nur eine Nachricht verschickt.
						if(!emptyPoolFlag) {
							//std::cout << MSL_myId << ": empty pool message" << std::endl;
							// TODO: schlecht! unterstellt Minimierungsproblem!!! eigene Methode betterThan zum LB-Vergleich?
							sendLBInfo(2147483647, pred); // User muss definieren, was die schlechteste LB ist!
							lbMessagesSend++;
							emptyPoolFlag = true;
						}
					}
					else { // Pool ist nicht leer
						int dice = rand()%100+1; // erzeugt Zufallszahl zwischen 1 und 100.
						if(dice<=MSL_ARG2) { // verrate LB nur mit einer bestimmten WS
							if((lbMessagesSend-lbMessagesReceived)<1000) {
								sendLBInfo(getLowerBound((*workpool).top()), pred);
								lbMessagesSend++;
							}
						}
						// wenn der Pool beim nächsten Mal leer ist, darf wieder eine "schlechteste LB"-Message verschickt werden.
						emptyPoolFlag = false;
					}
				}

			} // interner Nachrichtenverkehr



			// PROBLEMVERARBEITUNG - Wenn Workpool nicht leer (verarbeite Problem) (wenn finished=true, dann ist workpool immer leer)
			// Heapmethoden: bool isEmpty(); Problem top(); Problem get(); void insert(Problem); DFct2<Problem,Problem,bool> getLth(); Collection<Problem>* fresh();
			if(!finished && (!(*workpool).isEmpty())) {
				// nimm Problem aus workpool
				Problem nextSubproblem = (*workpool).get();
			   	if(analyse) anzahlBearbeiteterProbleme++;
				if(logIDs) v.push_back(nextSubproblem.problemID);

				if(debugComputation) std::cout << "Prozessor "<< MSL_myId << " nimmt sich ein Teilproblem aus dem Pool. lb=" << nextSubproblem.lowerBound  << std::endl;
				if(debugComputation) if((*workpool).isEmpty()) std::cout << "Prozessor "<< MSL_myId << " hat jetzt leeren Workpool, weil das eben entnommene Problem das letzte im Pool war."<< std::endl;

				// MEAN LOAD DISTRIBUTION: LB-Infomessage nur annehmen, wenn man auch was abzugeben hat.
				// EMPTY_POOL_Messages werden erst gelesen, wenn sie tatsächlich bedient werden können.
				if(numOfSolvers>1 && !(*workpool).isEmpty()) {
					MPI_Testany(numOfSolvers, &internalLoadInfoRequest[0], &whoIsReady, &flag, &stat); // MPI_Test(IN,OUT,OUT)
					if(flag==true) {
						// mindestens eine Nachricht ist angekommen.
						lbMessagesReceived++;
						loadInfo = internalLoadInfoMessage[whoIsReady];
						// auf nächste LoadInfo-Message warten
						MPI_Irecv(&internalLoadInfoMessage[whoIsReady], sizeof(int), MPI_BYTE, exitOfSolver[whoIsReady], MSLT_LB_TAG, MPI_COMM_WORLD, &internalLoadInfoRequest[whoIsReady]);
						while(flag) {
							MPI_Testany(numOfSolvers, &internalLoadInfoRequest[0], &whoIsReady, &flag, &stat); // MPI_Test(IN,OUT,OUT)
							if(flag==true) {
								lbMessagesReceived++;
								if(loadInfo < internalLoadInfoMessage[whoIsReady]) loadInfo = internalLoadInfoMessage[whoIsReady];
								MPI_Irecv(&internalLoadInfoMessage[whoIsReady], sizeof(int), MPI_BYTE, exitOfSolver[whoIsReady], MSLT_LB_TAG, MPI_COMM_WORLD, &internalLoadInfoRequest[whoIsReady]);
							}
						}
						myBestLB = getLowerBound((*workpool).top());
						// falls der Solver ein besseres Problem hat, sendet er dies an den Nachfolger
						if(myBestLB<loadInfo) { // TODO: schlecht! unterstellt Minimierungsproblem!!! eigene Methode betterThan zum LB-Vergleich?
							// nimm Problem aus workpool und sende es an den nächsten Solver
							Problem load = (*workpool).get();
							// nur verschicken, wenn das Problem ein Optimum sein sein. (evtl. ist gerade eben ein neuer Incumbent empfangen worden)
							if(solutionFound && betterThan(incumbent, load)) { (*workpool).reset(); }
							else {
								if(debugComputation) std::cout << "Prozessor "<< MSL_myId << " verteilt Last an "<<succ << "   lb=" << load.lowerBound<<std::endl;
								sendProblem(succ, &load, sizeof(Problem));
								messagesSend++;
							}
						}
					}
				}
				else {
					//std::cout << "solver " << MSL_myId << ": Workpool leer. Nehme keine LB-Nachrichten an." << std::endl;
				}


				// wenn Incumbent besser als beste Problemabschätzung im workpool (lower bound)
				if(solutionFound && betterThan(incumbent, nextSubproblem)) {
					// leere Workpool (kein Problemlösung wird besser als Incumbent sein) <-- IM PAPER ANMERKEN!!!
					if(debugComputation) std::cout << "Prozessor "<< MSL_myId << " leert seinen Workpool, weil keine Lösung besser als Incumbent wird. lb=" << incumbent.lowerBound<<std::endl;
					if(!(*workpool).isEmpty())
						(*workpool).reset();
				}
				// sonst zerlege Problem in Teilprobleme
				else {
					int numOfProblems = 0;
					if(debugComputation) std::cout << "Prozessor "<< MSL_myId << " zerlegt das Problem ";
					Problem* subproblem = branch(nextSubproblem, &numOfProblems); // !!! nextSubproblem ist ggf. modifiziert!
					if(debugComputation) std::cout <<" in "<<numOfProblems<<" neue Teilprobleme "<<std::endl;
					// Bestimme darunter alle neuen besten Lösungen (Incumbent), schätze restliche Probleme ab
					bool newIncumbentFound = false;
					for(int i = 0; i < numOfProblems; i++) {
						// falls Lösung gefunden
						if(debugComputation) std::cout << "Prozessor "<< MSL_myId << " prüft das Subproblem["<<i < <"]"<< std::endl;
						if(isSolution(subproblem[i])) {
							// prüfen, ob neuen Incumbent gefunden
							if(!solutionFound || betterThan(subproblem[i], incumbent)) {
								if(debugComputation) std::cout << "Prozessor "<< MSL_myId<<" findet neuen Incumbent. lb=" << subproblem[i].lowerBound <<std::endl;
								incumbent = subproblem[i];
								solutionFound = true;
								newIncumbentFound = true;
							}
						}
						// sonst ungelöstes Problem
						else {
							// Problem (ggf. parallel) abschätzen ### TODO ### wird so nicht parallel funktionieren!!!
							subproblem[i] = bound(subproblem[i]);
							// falls durch bounding Lösung entdeckt
							if(isSolution(subproblem[i])) {
								// prüfen, ob neuen Incumbent gefunden
								if(!solutionFound || betterThan(subproblem[i], incumbent)) {
									incumbent = subproblem[i];
									solutionFound = true;
									newIncumbentFound = true;
								}
							}
							// sonst ungelöstes Problem in Workpool schreiben
							else {
								if(debugComputation) std::cout << "Prozessor "<< MSL_myId << " identifiziert Teilproblem als ungelöstes Problem und schreibt es in den workpool. lb=" << (subproblem[i]).lowerBound << std::endl;
								// TODO: Prüfen, ob neuen Incumbent gefunden
								(*workpool).insert(subproblem[i]);
							}
						}
					}
					delete[] subproblem;
					// Neuer Incumbent wurde entdeckt! Diesen an alle Solver-Kollegen schicken, falls es welche gibt
					if((numOfSolvers>1) && newIncumbentFound) {
						if(debugComputation) std::cout << "Prozessor "<< MSL_myId << " hat neuen Incumbent gefunden und teilt dieses allen mit. lb=" << incumbent.lowerBound<< std::endl;
						// versende neuen Incumbent an alle Solverkollegen, nur nicht an sich selbst.
						for(int i = 0; i < numOfSolvers; i++)
							if(entranceOfSolver[i]!=MSL_myEntrance) {
								if(debugComputation) std::cout << "Prozessor " <<MSL_myId<<" versendet Incumbent an " << entranceOfSolver[i] << std::endl;
								sendIncumbent(entranceOfSolver[i], &incumbent, sizeof(Problem));
								messagesSend++;
							}
					}
				}
				// termination detection, falls der Solver allein und ohne Solver-Kollegen arbeitet
				if(numOfSolvers==1) {
					if((*workpool).isEmpty()) {
						if(debugTerminationDetection) std::cout << "Prozessor "<<MSL_myId<<" beendet die Berechnung und verschickt Incumbent "<<std::endl;
						// Incumbent verschicken
						if(debugTerminationDetection)	if(!solutionFound) std::cout << "keine Lösung gefunden!!" << std::endl;
						if(MSL_DISTRIBUTION_MODE==MSL_RANDOM_DISTRIBUTION) MSL_send(getRandomReceiver(), &incumbent, sizeof(Problem));
						else if(MSL_DISTRIBUTION_MODE==MSL_CYCLIC_DISTRIBUTION) MSL_send(getNextReceiver(), &incumbent, sizeof(Problem));
						// reset
						blocked = false;
						solutionFound = false;
					}
				}
			} // Ende Problemverarbeitung
		}

		// Request-objekte löschen und speicher freigeben
		if(MSL_myId==masterSolver) {
			if(debugFreeMem) std::cout << MSL_myId << " Mastersolver löscht externe Requestobjekte"<<std::endl;
			for(int i = 0; i < numOfPredecessors; i++)
				MPI_Request_free(&externalRequest[i]);
		}
		// diese Requestobjekte gibt es nur dann, wenn der Solver mindestens einen Solver-Kollegen hat.
		if(numOfSolvers>1) {
			for(int i = 0; i < numOfSolvers; i++) {
				//std::cout << MSL_myId << " löscht interne Requestobjekte"<<std::endl;
				MPI_Request_free(&internalTokenRequest[i]);
				MPI_Request_free(&internalIncumbentRequest[i]);
				MPI_Request_free(&internalStopRequest[i]);
				MPI_Request_free(&internalProblemRequest[i]);
			}
		}
		if(debugFreeMem) std::cout << MSL_myId << " löscht Arrays"<<std::endl;
		delete[] externalMessage;
		delete[] externalRequest;
		delete[] internalIncumbentMessage;
		delete[] internalIncumbentRequest;
		delete[] internalProblemMessage;
		delete[] internalProblemRequest;
		delete[] internalStopMessage;
		delete[] internalStopRequest;
		delete[] internalTokenMessage;
		delete[] internalTokenRequest;

		// Ende der Zeitmessung
		//if(MSL_TIMER) {
		//	processEndTime = MPI_Wtime();
		//	double filterTime = (double) (processEndTime - processBeginTime);
		//	std::cout << "Filter " << MSL_myId << " aktiv für: " << filterTime << "s";
		//}
		// if(analyse) std::cout << "Filter " << MSL_myId << ": Zeit für branch and bound: " << calcTime << std::endl;
		if(analyse) {
			std::cout << "Filter " << MSL_myId << ": anzahlBearbeiteterProbleme = " << anzahlBearbeiteterProbleme << std::endl;
			std::cout << "Filter " << MSL_myId << ": messagesSend = " << messagesSend << std::endl;
			std::cout << "Filter " << MSL_myId << ": messagesReceived = " << messagesReceived << std::endl;
			std::cout << "Filter " << MSL_myId << ": lbMessagesSend = " << lbMessagesSend << std::endl;
			std::cout << "Filter " << MSL_myId << ": lbMessagesReceived = " << lbMessagesReceived << std::endl;
		}
		if(logIDs) {
			for(int i = 0; i < v.size(); i++) {
				std::cout << "Filter " << MSL_myId << ": " << (i+1) << ".Problem: ID = " << v[i] << std::endl;
				//std::cout << MSL_myId << ":" << v[i] << ";" << std::endl;
			}
		}
	} */ // end of start()


	/**
	 * Berechnet die Ein- und Ausgaenge aller Solverkollegen.
	 * @param solver	Zeiger auf das komplette Solverarray (dort ist man selbst auch eingetragen)
	 * @param length	Laenge des Solverarrays
	 * @param id		Index des Zeigers im Solverarray, der auf diesen Solver (this) zeigt
	 */
/*	void setWorkmates(BBSolverOld<Problem>** solver, int length, int id) {
		// indexID = id; // merke, an welcher Position man selbst steht
		numOfSolvers = length;
		entranceOfSolver = new ProcessorNo[numOfSolvers];
		exitOfSolver = new ProcessorNo[numOfSolvers];

		// Ein- und Ausgaenge aller Solver berechnen (inkl. des eigenen Ein- und Ausgangs)
		ProcessorNo* ext;
		ProcessorNo* entr;
		for(int i = 0; i < length; i++) {
			entr = (*(solver[i])).getEntrances();
			entranceOfSolver[i] = entr[0];
			ext = (*(solver[i])).getExits();
			exitOfSolver[i] = ext[0];
			//std::cout << "setWorkmates: entr/exit of solver["<<i < <"]: "<<entranceOfSolver[i]<<"/"<<exitOfSolver[i]<<std::endl;
		}
		// Sender und Empfänger für das Token berechnen
		pred = exitOfSolver[(id-1+length)%length];
		succ = entranceOfSolver[(id+1)%length];
		//std::cout << "setWorkmates: pred/succ of solver["<<id<<"]: " << pred << "/" << succ << std::endl;
	}*/


/*    Process* copy() {
		return new BBSolverOld<Problem>(branch, bound, betterThan, isSolution, getLowerBound, noprocs);
	}*/

/*   	void show() const {
		if(MSL_myId == 0) std::cout << MSL_myId << " BBSolverOld " << entrance << std::endl << std::flush;
	}*/

}; // end of class BBSolverOld

template<class Problem>
class BranchAndBoundOld: public Process {

	// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
	BBSolverOld<Problem>** p;
	int length; // #Solver

    public:

		/**
		 * Ein dezentrales Branch&Bound-Skelett.
		 *
		 * Dieser Konstruktor erzeugt (l-1) Kopien des übergebenen BBSolvers. Insgesamt besteht dieses Skelett aus l Solvern.
		 *
		 * Externe Schnittstellen:
		 * Jeder Solver verfügt über genau einen Eingang und einen Ausgang. Das BranchAndBoundOld-Skelett hat genau einen Eingang.
		 * Dieser wird auf einen der Solver gemappt. Die Ausgänge der Solver sind die Ausgänge des BranchAndBoundOld-Skeletts.
		 *
		 * Interne schnittstellen:
		 * Jeder Solver kennt die Ein- und Ausgänge seiner Kollegen. Über pred und succ ist zudem eine logische Ringstruktur definiert,
		 * die für das versenden des Tokens im Rahmen des Termination Detection Algorithmus genutzt wird.
		 */
    	BranchAndBoundOld(BBSolverOld<Problem>& solver, int l): length(l), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new BBSolverOld<Problem>* [length];
            p[0] = &solver;
            for(int i = 1; i < length; i++)
            	p[i] = (BBSolverOld<Problem>*)solver.copy();

			// externe Schnittstellen definieren:
			// Es gibt genau einen Mastersolver, der als Eingang fungiert.
			// Der (einzige) Eingang des Mastersolvers ist (einziger) Eingang des Skeletts, alle Solverausgänge sind Ausgänge des Skeletts.
			numOfEntrances = 1;
			entrances = new ProcessorNo[numOfEntrances];
			ProcessorNo* entr = (*(p[0])).getEntrances(); // Mastersolver
			entrances[0] = entr[0];
			//std::cout << "BranchAndBoundOld: entrance of mastersolver: " << entr[0] << std::endl;

			numOfExits = length;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext;
            for(int i = 0; i < length; i++) {
            	ext = (*(p[i])).getExits(); // exit von jedem Solver besorgen
            	exits[i] = ext[0];
            	//std::cout << "BranchAndBoundOld: exit of solver["<<i < <"]: " << exits[i] << std::endl;
            }

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
			// Empfaenger der ersten Nachricht festlegen
			//setStartReceiver(0); // BranchAndBoundOld verschickt doch gar nichts, sondern die Solver
		}

		// von diesen l vorgelagerten Skeletten werden Daten empfangen
		// nur der Mastersolver kommuniziert mit den Sendern
		inline void setPredecessors(ProcessorNo* src, int l) {
			numOfPredecessors = l;
			(*(p[0])).setPredecessors(src,l);
		}

		// an diese n nachgelagerten Skelette werden Daten gesendet.
		// alle Solver können Daten verschicken
		inline void setSuccessors(ProcessorNo* succs, int n) {
			numOfSuccessors = n;
			for(int i = 0; i < length; i++)
				(*(p[i])).setSuccessors(succs,n);
		}

        // startet alle Solver
		inline void start() {
            for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

        Process* copy() { return new BranchAndBoundOld<Problem>(*(BBSolverOld<Problem>*)(*(p[0])).copy(), length);}

        inline void show() const {
            if(MSL_myId == 0) {
                std::cout << MSL_myId << " BranchAndBoundOld " << entrances[0] << std::endl << std::flush;
                for(int i = 0; i < length; i++)
                    (*(p[i])).show();
			}
		}
}; // end of BranchAndBoundOld

// *************************************************************************************


/**
 *  Workpool, der Frames verwaltet. Arbeitet im Gegensatz zum obigen Workpool mit Zeigern auf die Elemente statt mit Kopien
 *  Der Workpool ist in Form eines Heaps realisiert, dessen Priorisierung gemäß der unteren Schranken der Verwalteten Probleme
 *  erfolgt. Um die Heapbedingung herzustellen, benötigt er die betterThan Funktion des BranchAndBound Skeletts.
 */
template<class I>
class BBFrameWorkpool {

	bool debug;

	int last;
    int size;
    BBFrame<I>** heap; // Array von Pointern auf Frames
//    DFct2<I,I,bool> betterThan; // Funktion zum Vergleich zweier Elemente des Workpools
    DFct2<I*,I*,bool> betterThan; // Funktion zum Vergleich zweier Elemente des Workpools

	int statMaxSize;
	int statCumulatedSize;
	int statNumOfInserts;

  public:

  	BBFrameWorkpool(const DFct2<I*,I*,bool>& less): betterThan(less), last(-1), size(8) {
  		//std::cout << MSL_myId <<" ruft Konstruktor 1 auf"<<std::endl;
  		heap = new BBFrame<I>*[size];
  		debug = false;

		statCumulatedSize = 0;
		statMaxSize = 0;
		statNumOfInserts = 0;

  	}

// 	BBFrameWorkpool(bool (*less)(I const&,I const&)): last(-1), size(8), betterThan(Fct2<I,I,bool, bool (*)(I const&,I const&)>(less)) {
  	BBFrameWorkpool(bool (*less)(I*,I*)): last(-1), size(8), betterThan(Fct2<I*,I*,bool, bool (*)(I*,I*)>(less)) {
  		//std::cout << MSL_myId <<" ruft Konstruktor 2 auf"<<std::endl;
  		heap = new BBFrame<I>*[size];
  		debug = false;

  		statCumulatedSize = 0;
  		statMaxSize = 0;
  		statNumOfInserts = 0;
  	}

  	~BBFrameWorkpool() {
  		delete[] heap;
  	}

  	bool isEmpty() {return last < 0;}

  	BBFrame<I>* top() { // gibt einen Zeiger auf das Frame ganz oben auf dem Heap zurück
    	if(isEmpty()) throws(EmptyHeapException());
    	return heap[0];
    }

  	BBFrame<I>* get() { // liefert das oberste Element des Heaps und stellt die Heapbedingung wieder her
    	if(isEmpty()) throws(EmptyHeapException());
    	BBFrame<I>* result = heap[0]; // Ergebnis merken
    	// Heapbedingung wieder herstellen
    	heap[0] = heap[last--];
    	int current = 0;
    	int next = 2*current+1;
    	while(next <= last+1) {
      		if((next <= last) && (betterThan((heap[next+1]->getData()),(heap[next]->getData())))) next++;
      		if(betterThan((heap[next]->getData()),(heap[current]->getData()))) {
        		BBFrame<I>* aux = heap[next]; heap[next] = heap[current];  heap[current] = aux;
        		current = next;
        		next = 2*current+1;
        	}
      		else next  = last+2;
      	} // stop while loop
    	return result;
    }

  	void insert(BBFrame<I>* val) { // fügt ein Element in den Heap ein und stellt die Heapbedingung wieder her
    	int current = ++last;
    	if(last >= size) { // extend heap
      		if(debug) std::cout << "Workpool::insert() : extending heap" << std::endl << std::flush;
      		BBFrame<I>** newheap = new BBFrame<I>*[2*size];
      		for(int i = 0; i < size; i++) newheap[i] = heap[i];
      		size *= 2;
      		if(debug) std::cout << "Workpool::insert() : deleting old heap" << std::endl;
      		delete[] heap; // Speicher wieder freigeben
      		if(debug) std::cout << "Workpool::insert() : heap deleted" << std::endl;
      		heap = newheap;
      	}
    	int next;
    	heap[last] = val;
    	while(current > 0) {
      		next = (current-1)/2;

      		if(betterThan((heap[current]->getData()),(heap[next]->getData()))) {
        		BBFrame<I>* aux = heap[next];
        		heap[next] = heap[current];
        		heap[current] = aux;
        		current = next;
        	}
      		else current = 0;
      	}
    	statMaxSize = (last>statMaxSize) ? last : statMaxSize;
    	statCumulatedSize +=last;
    	statNumOfInserts++;
    } // stop loop

  	DFct2<I*,I*,bool> getBetterThanFunction() {
  		return betterThan;
  	}
	
	// Setzt den Heap auf den Ausgangszustand zurück
	void reset() {
		last = -1;
	}

  	Workpool<I>* fresh() {
  		if(debug)
			std::cout << "Workpool::fresh() invoked"<<std::endl;

  		Workpool<I>* h = new Workpool<I>(betterThan);

        return h;
    }

  	void show(ProcessorNo n) {
    	std::cout << "Prozessor" << n << " hat Workpool: [";

    	for(int i = 0; i <= last; i++)
      		std::cout << heap[i]->getID() << "," << heap[i]->getData() << " ; ";

    	std::cout << "]" << std::endl;
    }

	int getMaxLength() {
		return statMaxSize;
	}

	int getAverageLength() {
		if(statNumOfInserts>0)
			return (statCumulatedSize / statNumOfInserts);

		return 0;
	}

};

/**
 * Die Klasse BBProblemTracker verwaltet einen Baum von BBFrame-Objekten. Sie wird im Rahmen der Termination Detection
 * des Branch-and-Bound Skelettes genutzt.  Dabei wird ausgenutzt, dass die BBFrames intern Verweise auf das Vater-
 * frame speichern. So können einem Frame zugehörige Probleme als im Vaterframe gelöst markiert werden. Sind alle Kinder
 * gelöst, so ist das Vaterproblem gelöst und dessen Vater wird benachrichtigt.
 * */
template<class Problem>
class BBProblemTracker {
private:
	class ListElement {
		friend class BBProblemTracker;
		ListElement *predecessor, *successor;
		BBFrame<Problem>* problemFrame;

		ListElement(): predecessor(NULL), successor(NULL), problemFrame(NULL) {}
		ListElement(BBFrame<Problem>* prob) : predecessor(NULL), successor(NULL), problemFrame(prob) {}
	};

	ListElement *headSolved, *tailSolved;
	int sizeTracker;
	int sizeSolved;
	int numOfMaxSubProblems;

	int statMaxSize;
	int statMaxSizeSolved;
	int statCumulatedSize;
	int statCumulatedSizeSolved;
	int statNumOfInserts;
	int statNumOfInsertsSolved;
	bool debug;
public:

	// Erzeugt ein neues Tracker Objekt
	BBProblemTracker(int subproblems) : numOfMaxSubProblems(subproblems), sizeTracker(0), sizeSolved(0) {

		headSolved = new ListElement();
		tailSolved = new ListElement();


		headSolved->successor = tailSolved;
		tailSolved->predecessor = headSolved;

		statCumulatedSize = 0;
		statCumulatedSizeSolved = 0;
		statMaxSize = 0;
		statMaxSizeSolved = 0;
		statNumOfInserts = 0;
		statNumOfInsertsSolved = 0;
		debug = false;
	}

	~BBProblemTracker() {
		delete headSolved;
		delete tailSolved;
	}

	// Prüft, ob der Tracker leer ist
	bool isTrackerEmpty() { return sizeTracker==0; }

	// Fügt ein neues Problem in den Tracker ein
	void addProblem(BBFrame<Problem>* prob) {
		// Die Verweise zum Vater bestehen bereits, es ist lediglich sicherzustellen, dass kein Verweis auf die Nutzdaten existiert
		// Diese werden durch das Skelett gelöscht
		prob->setData(NULL);

		sizeTracker++;
		statMaxSize = (sizeTracker>statMaxSize) ? sizeTracker : statMaxSize;
		statNumOfInserts++;
		statCumulatedSize += sizeTracker;
	}

	// Markiert ein Problem im Tracker als gelöst
	void problemSolved(BBFrame<Problem>* prob) {
		if(prob->getID()==0) return; // Urproblem gelöst; nichts zu erledigen

		bool solved = true;
		BBFrame<Problem>* parent;
		BBFrame<Problem>* old;
		int numSolved;

		// Traversiert den Baum von der untersten bis ggf. zur obersten Ebene
		// und aktualisiert die Status aller betroffen Vaterprobleme
		while(solved) {
			if(prob->getID()==0)
				solved = false;
			else if(prob->getOriginator()==MSL_myId) { // vater benachrichtigen
				if(debug) std::cout << "Tracker: Problem " << prob->getID() << " gelöst" << std::endl;
				parent = prob->getParentProblem();
				numSolved = parent->getNumOfSolvedSubProblems();
				parent->setNumOfSolvedSubProblems(++numSolved);
				old = prob;
				if(parent->getNumOfSolvedSubProblems() == parent->getNumOfSubProblems()) { // Vater ist gelöst
					if(debug) std::cout << "Tracker: Alle Kinder des VaterProblems " << parent->getID() << " gelöst" << std::endl;
					prob = parent;
					solved = true;
					sizeTracker--;
				}
				else solved = false;

			}
			else {
				writeToSolvedQueue(prob);
				solved = false;
			}
		}
	}

	// Prüft, ob die termporäre Liste leer ist
	bool isSolvedQueueEmpty() { return sizeSolved == 0; }

	// Schreibt ein Problem in die Liste der gelösten, zu versendenden Probleme
	void writeToSolvedQueue(BBFrame<Problem>* prob) {
		ListElement *newProblem;
		newProblem = new ListElement(prob);

		// hinten anhängen!
		tailSolved->predecessor->successor = newProblem;
		newProblem->predecessor = tailSolved->predecessor;
		tailSolved->predecessor = newProblem;
		newProblem->successor = tailSolved;
		sizeSolved++;

		statMaxSizeSolved = (sizeSolved>statMaxSizeSolved) ? sizeSolved : statMaxSizeSolved;
		statCumulatedSizeSolved+=sizeSolved;
		statNumOfInsertsSolved++;
	}
	// Liest ein Problem aus der Liste der gelösten, zu versendenden Probleme aus
	BBFrame<Problem>* readFromSolvedQueue() {
		if(isSolvedQueueEmpty())
			throw "SolvedQueue ist leer";
		else
			return headSolved->successor->problemFrame;
	}
	// Löscht ein Problem aus der Liste der gelösten, zu versendenden Probleme
	void removeFromSolvedQueue() {
		if(isSolvedQueueEmpty())
			throw "SolvedQueue ist leer";
		else {
			ListElement* old = headSolved->successor;
			headSolved->successor->successor->predecessor = headSolved;
			headSolved->successor = headSolved->successor->successor;
			sizeSolved--;
			delete old->problemFrame;
			delete old;
		}
	}
	// Methoden zum Auslesen von Statistiken
	int getSolvedQueueLength() { return sizeSolved; }
	int getProblemTrackerLength() { return sizeTracker; }
	int getProblemTrackerMaxLength() { return statMaxSize; }
	int getSolvedQueueMaxLength() { return statMaxSizeSolved; }
	int getProblemTrackerAverageLength() {
		if(statNumOfInserts>0)
			return (statCumulatedSize / statNumOfInserts);
		return 0;
	}
	int getSolvedQueueAverageLength() {
		if(statNumOfInsertsSolved>0)
			return (statCumulatedSizeSolved / statNumOfInsertsSolved);
		return 0;
	}
};
// Kapselt Statistiken für die Experimente mit dem Branch-and-Bound Skelett
typedef struct {
	// Statistik Variablen
	int statNumMsgProblemSolvedSent;
	int statNumMsgProblemSolvedReceived;
	int statNumMsgWorkPoolEmptySent;
	int statNumMsgWorkPoolEmptyRejectionReceived;
	int statNumMsgBoundInfoSent;
	int statNumMsgBoundInfoReceived;
	int statNumMsgBoundRejectionSent;
	int statNumMsgBoundRejectionReceived;
	int statNumProblemsSent;
	int statNumProblemsReceived;
	int statNumProblemsSolved;
	int statNumIncumbentReceivedAccepted;
	int statNumIncumbentReceivedDiscarded;
	int statNumIncumbentSent;
	int statNumProblemsBranched;
	int statNumProblemsBounded;
	int statNumSolutionsFound;
	int statNumProblemsTrackedTotal;
	int statNumProblemsKilled;
	double statTimeProblemProcessing;
	double statTimeCommunication;
	double statTimeIncumbentHandling;
	double statTimeLoadBalancing;
	double statTimeTrackerSolvedProblemsReceived;
	double statTimeTrackerSolvedProblemsSent;
	double statTimeCleanWorkpool;
	double statTimeSubProblemSolvedInsert;
	double statTimeIdle;
	double statTimeInitialIdle;
	double statTimeTotal;
	double statTimeSinceWorkpoolClean;
	int problemTrackerMaxLength;
	int problemTrackerAverageLength;
	int solvedProblemsQueueMaxLength;
	int polvedProblemsQueueAverageLength;
	int workpoolMaxLength;
	int workpoolAverageLength;

} statistics;

/**
 * Die Klasse repräsentiert einen am Branch-and-Bound Prozess beteiligten Solver.
 * Instanzen werden durch die Klasse BranchAndBound erzeugt.
 */
template<class Problem>
class BBSolver : public Process {

private:

	// Variablen zur Steuerung des Ablaufs in start()

	// Variablen für Kommunikation
	ProcessorNo* entranceOfSolver;			// Eingänge aller Solver (inkl. this): entranceOfSolver[0] ist Master
	ProcessorNo* exitOfSolver; 				// Ausgänge aller Solver (inkl. this): exitOfSolver[0] ist Master
	ProcessorNo* entranceOfWorkmate;		// Zur Definition der Topologie; zwar kennt bereits jeder jeden (für Incumbents und STOPS)
	ProcessorNo* exitOfWorkmate;			// aber zur Lastverteiltung besteht eine bestimmte Topologie

	ProcessorNo masterSolver;				// Mastersolver; entspricht entranceOFSolver[0]
	ProcessorNo tokenPredecessor;			// Vorgänger merken zum Verschicken des Tokens für Termination Detection
	ProcessorNo tokenSuccessor;				// Nachfolger merken zum Verschicken des Tokens für Termination Detection
	int numOfSolvers;						// Anzahl der Solver, die zur Verfügung stehen
	int noprocs;
	int numOfWorkmates;						// Anzahl der Partner, mit denen die Lastverteilung durchgeführt wird

	// Variablen für Problembearbeitung
	Problem* incumbent; 						// Zeiger auf aktuell beste (globale) Lösung
	BBFrameWorkpool<Problem>* workpool; 		// lokaler Speicher für zu bearbeitende Probleme
											// Speichert Frames (Frames für Termination Detection)
	BBProblemTracker<Problem>* problemTracker;
	int numOfMaxSubProblems;					// Maximale Anzahl generierter Teilprobleme

	// Benutzerdefinierte Funktionen

    DFct2<Problem*,int*,Problem**> branch;		// branch(Fct2<Problem*,int*,Problem**, Problem** (*)(Problem*,int*)>(br)),
    DFct1<Problem*,void> bound;					// bound(Fct1<Problem*, void, void (*)(Problem*)>(bnd)),
    DFct2<Problem*,Problem*,bool> betterThan;   // betterThan(Fct2<Problem*,Problem*,bool, bool (*)(Problem*,Problem*)>(lth)),
    DFct1<Problem*,bool> isSolution;			// isSolution(Fct1<Problem*, bool, bool (*)(Problem*)>(isSol)),
    DFct1<Problem*,int> getLowerBound;			// getLowerBound(Fct1<Problem*, int, int (*)(Problem*)>(getLB)),

//	bool (*isSolution)(Problem*);			// Zeiger auf benutzerdefinierte Funktion
//	int (*getLowerBound)(Problem*);
//	bool (*betterThan)(Problem*,Problem*);	// Zeiger auf benutzerdefinierte Funktion
//	void (*bound)(Problem*);				// Zeiger auf bound-Methode. Schätzt Subproblem ab.
//	Problem** (*branch)(Problem*,int*);		// Zeiger auf branch-Methode. Liefert Array mit Zeigern auf erzeugte Subprobleme

public:

	/* Standardkonstruktor
	 *
	 */
//	BBSolver(Problem** (*br)(Problem&,int&), int numSub, void (*bnd)(Problem&), bool (*lth)(Problem*,Problem*), bool (*isSol)(Problem const&), int (*getLB)(Problem*), int n):
//	BBSolver(Problem** (*br)(Problem*,int*), void (*bnd)(Problem*), bool (*lth)(Problem*,Problem*), bool (*isSol)(Problem*), int (*getLB)(Problem*), int numSub, int n) :
//		noprocs(n), betterThan(lth), isSolution(isSol), branch(br), numOfMaxSubProblems(numSub), bound(bnd), getLowerBound(getLB), Process() {
	BBSolver(Problem** (*br)(Problem*,int*),
			 void (*bnd)(Problem*),
			 bool (*lth)(Problem*,Problem*),
			 bool (*isSol)(Problem*),
			 int (*getLB)(Problem*),
			 int numSub, int n)
			 : branch(Fct2<Problem*,int*,Problem**, Problem** (*)(Problem*,int*)>(br)),
			   bound(Fct1<Problem*, void, void (*)(Problem*)>(bnd)),
			   betterThan(Fct2<Problem*,Problem*,bool, bool (*)(Problem*,Problem*)>(lth)),
			   isSolution(Fct1<Problem*, bool, bool (*)(Problem*)>(isSol)),
			   getLowerBound(Fct1<Problem*, int, int (*)(Problem*)>(getLB)),
			   numOfMaxSubProblems(numSub),
			   noprocs(n),
			   Process() {

      	workpool = new BBFrameWorkpool<Problem>(lth);	// workpool anlegen; muss die betterThan Funktion kennen
		numOfEntrances = 1;
		numOfExits = 1;
		problemTracker = new BBProblemTracker<Problem>(numOfMaxSubProblems);
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo; // Eingang des Solvers ist der erste Prozessor
		exits[0] = entrances[0];			// ebenso der Ausgang
		MSL_runningProcessorNo += n;		// Ein Solver arbeitet auf n Prozessoren  TODO: nur bei Datenparallel nötig -> Streichen
		setNextReceiver(0); 			// Default-Receiver ist 0. Den gibt's immer
		processSendTime = 0;
		processRecvTime = 0;
		incumbent = NULL;
	}

	BBSolver(const DFct2<Problem*,int*,Problem**>& br,
			 const DFct1<Problem*,void>& bnd,
			 const DFct2<Problem*,Problem*,bool>& lth,
			 const DFct1<Problem*,bool>& isSol,
			 const DFct1<Problem*,int>& getLB,
			 int numSub, int n)
			 : branch(br), bound(bnd), betterThan(lth), isSolution(isSol), getLowerBound(getLB), numOfMaxSubProblems(numSub),
			   noprocs(n), Process() {

      	workpool = new BBFrameWorkpool<Problem>(lth);	// workpool anlegen; muss die betterThan Funktion kennen
		numOfEntrances = 1;
		numOfExits = 1;
		problemTracker = new BBProblemTracker<Problem>(numOfMaxSubProblems);
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo; // Eingang des Solvers ist der erste Prozessor
		exits[0] = entrances[0];			// ebenso der Ausgang
		MSL_runningProcessorNo += n;		// Ein Solver arbeitet auf n Prozessoren  TODO: nur bei Datenparallel nötig -> Streichen
		setNextReceiver(0); 			// Default-Receiver ist 0. Den gibt's immer
		processSendTime = 0;
		processRecvTime = 0;
		incumbent = NULL;
	}

	~BBSolver() {
		delete workpool;
		delete problemTracker;
		delete[] entrances;
		delete[] exits;
		if(entranceOfSolver!=entranceOfWorkmate) // sind bei ALL-TO-ALL gleich
			delete[] entranceOfWorkmate;
		if(exitOfSolver!=exitOfWorkmate)
			delete[] exitOfWorkmate;
		delete[] entranceOfSolver;
		delete[] exitOfSolver;
	}
	// Startet den Berechnungsprozess des Solvers
	void start() {
		finished = ((MSL_myId < entrances[0]) || (MSL_myId >= entrances[0] + noprocs));
		if(finished) return;

		MSL_numOfLocalProcs = noprocs;
		ProcessorNo masterSolver = entranceOfSolver[0];
		MSL_myEntrance = entrances[0];
		MSL_myExit = exits[0];

		// Statistikvariablen initialisieren
		statistics stat;

		stat.statNumMsgProblemSolvedSent = 0;
		stat.statNumMsgProblemSolvedReceived = 0;
		stat.statNumMsgBoundInfoSent = 0;
		stat.statNumMsgBoundInfoReceived = 0;
		stat.statNumMsgWorkPoolEmptySent = 0;
		stat.statNumMsgWorkPoolEmptyRejectionReceived = 0;
		stat.statNumMsgBoundRejectionSent = 0;
		stat.statNumMsgBoundRejectionReceived = 0;
		stat.statNumProblemsSent = 0;
		stat.statNumProblemsReceived = 0;
		stat.statNumProblemsSolved = 0;
		stat.statNumIncumbentReceivedAccepted = 0;
		stat.statNumIncumbentReceivedDiscarded = 0;
		stat.statNumIncumbentSent = 0;
		stat.statNumProblemsBranched = 0;
		stat.statNumProblemsBounded = 0;
		stat.statNumSolutionsFound = 0;
		stat.statNumProblemsTrackedTotal = 0;
		stat.statNumProblemsKilled = 0;
		stat.statTimeProblemProcessing = 0;
		stat.statTimeCommunication = 0;
		stat.statTimeIncumbentHandling = 0;
		stat.statTimeLoadBalancing = 0;
		stat.statTimeTrackerSolvedProblemsReceived = 0;
		stat.statTimeTrackerSolvedProblemsSent = 0;
		stat.statTimeCleanWorkpool = 0;
		stat.statTimeSubProblemSolvedInsert = 0;
		stat.statTimeIdle = 0;
		stat.statTimeInitialIdle = 0;
		stat.statTimeTotal = 0;
		stat.statTimeSinceWorkpoolClean = 0;
		stat.problemTrackerMaxLength = 0;
		stat.problemTrackerAverageLength = 0;
		stat.solvedProblemsQueueMaxLength = 0;
		stat.polvedProblemsQueueAverageLength = 0;
		stat.workpoolMaxLength = 0;
		stat.workpoolAverageLength = 0;

		double statStart = 0;
		double statStartTimeIdle = 0;
		double statStartTimeCommunication = 0;
		double statStartTimeProcessing = 0;

		// Variablen zur Steuerung der Debug Ausgaben
		bool debugMaster = false;
		bool debugCommunication = false;
		bool debugComputation = false;
		bool debugLoadBalancing = false;
		bool debugTermination = false;

		bool analysis = false;

		// Variablen zur Steuerung des Ablaufs
		bool masterBlocked = false; 			// falls STOP oder Problem eingeht wird Blockade aufgehoben
		bool solutionFound = false;				// Zeigt an, ob bereits eine gültige Lösung ermittelt wurde
		bool sentBoundInfo = false;				// damit ein Workrequest bei leerem workpool nur einmal rausgeht
		bool sentProblemSendRequest = false;	// Wird auf true gesetzt, wenn eine Sendeanfrage für ein Problem (für Lastverteilung) gesendet wurde
		bool sentIncumbentSendRequest = false;	// wird auf true gesetzt, wenn eine Sendeanfrage für ein Incumbent gesendet wurde
		bool newIncumbentFound = false;			// wird auf true gesetzt, wenn ein neues Incumbent gefunden wurde
		int numOfIncumbentMessagesSent(0);		// Anzahl der Incumbent Nachrichten an die Partner, die gesendet wurden
		finished = false;
		masterBlocked = false;

		// Variablen zur Problemverwaltung
		Problem* problem = NULL;  				// Zeiger auf aktuell bearbeitetes Problem
		BBFrame<Problem>* problemFrame = NULL;	// Zeiger auf auf das Frame, mit dem ein Problem verschickt wird
		BBFrame<Problem>* loadBalanceProblemFrame = NULL;
		int senderOfLoadBalance = 0;				// Array ID des Senders der Lastverteilungsanfrage
		int receiverOfBoundInfo = 0;				// Array ID des Empfängers der letzten Lastverteilungsanfrage

		// Variablen zur MPI Steuerung
		int messageWaiting = 0;					// Flag zum Testen auf MPI Nachrichteneingang
		MPI_Status messageStatus;				// Status der MPI Nachricht bei Empfang einer Nachricht

		int receivedStops = 0;					// zählt Anzahl Empfänger STOP Nachrichten von Vorgängern
		int predecessorIndex = 0;				// Zähler über das predecessors-Array

		stat.statTimeTotal = MPI_Wtime();			// Statistik
		stat.statTimeInitialIdle = MPI_Wtime();

		// Hauptschleife des Solvers. Läuft solange, bis eine STOP Nachricht empfangen wird
		while(!finished) {
			if(MSL_myId == masterSolver) { // Aufgaben des Master Solvers erledigen
				if(debugMaster) std::cout << MSL_myId << ": Mastersolver betritt die Buehne" << std::endl;
				// Masterempfang regeln (Eintritt in Problemverarbeitung)
				while(!masterBlocked && !finished) {
					// warte auf Nachricht von Vorgängern -> blockierend!
					if(debugMaster) std::cout << MSL_myId << ": Mastersolver wartet auf neues Problem" << std::endl;
					messageWaiting = 0;
					while(!messageWaiting) {
						MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
						predecessorIndex = (predecessorIndex+1) % numOfPredecessors;
					}

					// Verarbeite Nachricht (Tag oder Problem)
					// TERMINATION_TEST
					ProcessorNo source = messageStatus.MPI_SOURCE;
					if(messageStatus.MPI_TAG == MSLT_TERMINATION_TEST) {
						MSL_ReceiveTag(source, MSLT_TERMINATION_TEST);
					}
					// STOP Tag
					else if(messageStatus.MPI_TAG == MSLT_STOP) {
						// entgegennehmen
						MSL_ReceiveTag(source, MSLT_STOP);
						if(debugMaster) std::cout << MSL_myId << ": Mastersolver empfaengt STOP" << std::endl;
						// muss von allen Vorgängern kommen
						receivedStops++;
						if(receivedStops==numOfPredecessors) {
							// benachrichtige Solverkollegen
							if(numOfSolvers > 1) {
								for(int i = 0; i < numOfSolvers; i++) {
									if(entranceOfSolver[i]!=MSL_myId) { // sich selbst schicken würde Absturz bewirken
										MSL_SendTag(entranceOfSolver[i], MSLT_STOP);
										if(debugMaster) std::cout << MSL_myId << ": Mastersolver sendet STOP an Solver " << entranceOfSolver[i] << std::endl;
									}
								}
							}

							// benachrichtige Nachfolger
							for(int i = 0; i < numOfSuccessors; i++) {
								if(debugMaster) std::cout << MSL_myId << ": Mastersolver sendet STOP an Nachfolger " << successors[i] << std::endl;
								MSL_SendTag(successors[i], MSLT_STOP);
							}

							receivedStops = 0; 			// zurücksetzen
							masterBlocked = true;  		// nichts mehr annehmen
							finished = true;
						}
					}

					// jetzt kann höchstens noch ein Problem angekommen sein
					else {

						problem = new Problem(); 			// Speicher reservieren
						MSL_Receive(source, problem, MSLT_MYTAG, &messageStatus);
						if(debugMaster) std::cout << MSL_myId << ": Mastersolver hat ein neues Problem empfangen" << std::endl;

						stat.statNumProblemsReceived++;
						stat.statTimeInitialIdle = MPI_Wtime() - stat.statTimeInitialIdle;

						bound(problem); 			// Problem abschätzen

						if(!isSolution(problem)) {			// Prüfe, ob Problem direkt lösbar ist
							masterBlocked = true;			// nichts neues mehr annehmen
							problemFrame = new BBFrame<Problem>(0, NULL, masterSolver, -1, problem);
							workpool->insert(problemFrame);	// ab in den Workpool
							if(debugMaster) std::cout << MSL_myId << ": Mastersolver hat ein neues Problem in den Workpool gelegt" << std::endl;
						}
						else {								// schon fertig -> sende an Nachfolger
							MSL_Send(getReceiver(), problem);
							delete problem;
							problem = NULL;
						}
					}
				}  // Ende der Empfangsschleife des Masters
			}


			// Solver Kommunikation
			if((numOfSolvers > 1) && !finished && (MSL_myId == MSL_myEntrance)) {
				if(debugCommunication) std::cout << MSL_myId << ": Solver beginnt Kommunikationsphase" << std::endl;
				statStartTimeCommunication = MPI_Wtime();
				// 1. Neues Incumbent zu versenden?
				statStart = MPI_Wtime();
				if(newIncumbentFound && !sentIncumbentSendRequest) {
					if(debugCommunication) std::cout << MSL_myId << ": Solver beginnt mit Versand des neuen Incumbent" << std::endl;
					numOfIncumbentMessagesSent = 0;
					for(int i = 0; i < numOfSolvers; i++) {
						if(MSL_myId == entranceOfSolver[i]) continue;
						else if(MSL_myId < entranceOfSolver[i]) {
							MSL_Send(entranceOfSolver[i], incumbent, MSLT_BB_INCUMBENT);
							numOfIncumbentMessagesSent++;
							if(debugCommunication) std::cout << MSL_myId << ": Incumbent an Prozessor " << entranceOfSolver[i] <<"versendet" << std::endl;
						}
						else {
							MSL_SendTag(entranceOfSolver[i], MSLT_BB_INCUMBENT_SENDREQUEST);
							if(debugCommunication) std::cout << MSL_myId << ": Incumbent SendRequest an Prozessor " << entranceOfSolver[i] <<"versendet" << std::endl;
							sentIncumbentSendRequest = true;
						}
					}
					newIncumbentFound = false; // jetzt zurücknehmen, weiteres über sentIncumbentSendRequest
					stat.statNumIncumbentSent++;
				} // nun sind alle versandt oder SendRequests wurden verschickt
				if(sentIncumbentSendRequest) {  // Prüfen, ob SendRequest beantwortet wurde
					if(debugCommunication) std::cout << MSL_myId << ": Solver prüft, ob IncumbentSendRequest beantwortet wurde" << std::endl;
					for(int i = 0; i < numOfSolvers; i++) {
						MPI_Iprobe(exitOfSolver[i], MSLT_BB_INCUMBENT_READYSIGNAL, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
						if(messageWaiting) {
							if(debugCommunication) std::cout << MSL_myId << ": IncumbentReadySignal von Prozessor " << exitOfSolver[i] <<"empfangen" << std::endl;
							MSL_ReceiveTag(exitOfSolver[i], MSLT_BB_INCUMBENT_READYSIGNAL);
							MSL_Send(entranceOfSolver[i], incumbent, MSLT_BB_INCUMBENT);
							if(debugCommunication) std::cout << MSL_myId << ": Incumbent an Prozessor " << entranceOfSolver[i] <<"versendet" << std::endl;
							numOfIncumbentMessagesSent++;
						}
					}
					if(numOfIncumbentMessagesSent>=numOfSolvers-1) { // nicht an sich selbst geschickt -> 1 weniger
						sentIncumbentSendRequest = false;
					}
				} // 1. Ende: Neues Incumbent versenden

				// 2. Neue Incumbents angekommen?
				if(debugCommunication) std::cout << MSL_myId << ": Solver prüft, ob neues Incumbent eingegangen ist" << std::endl;
				for(int i = 0; i < numOfSolvers; i++) {
					bool incumbentReceived = false;
					// Solver mit kleinerer ProzessorID dürfen direkt senden -> prüfen, ob eingegangen
					MPI_Iprobe(exitOfSolver[i], MSLT_BB_INCUMBENT, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					if(messageWaiting) {	// Nachricht eingegangen
						problem = new Problem();
						MSL_Receive(exitOfSolver[i], problem, MSLT_BB_INCUMBENT, &messageStatus);
						if(debugCommunication) std::cout << MSL_myId << ": Neues Incumbent von Prozessor " << exitOfSolver[i] <<"empfangen. LB: "<< getLowerBound(problem) << std::endl;
						incumbentReceived = true;
					}
					// Solver mit größerer ID senden erst Sendeanfrage!
					MPI_Iprobe(exitOfSolver[i], MSLT_BB_INCUMBENT_SENDREQUEST, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					if(messageWaiting) {
						if(debugCommunication) std::cout << MSL_myId << ": IncumbentSendrequest von Prozessor " << exitOfSolver[i] <<"empfangen" << std::endl;
						problem = new Problem();
						MSL_ReceiveTag(exitOfSolver[i], MSLT_BB_INCUMBENT_SENDREQUEST);							// Tag holen
						if(debugCommunication) std::cout << MSL_myId << ": IncumbentSendrequest von Prozessor " << exitOfSolver[i] <<" abgeholt" << std::endl;
						MSL_SendTag(entranceOfSolver[i], MSLT_BB_INCUMBENT_READYSIGNAL);  						// Gegenüber darf jetzt senden
						if(debugCommunication) std::cout << MSL_myId << ": IncumbentReadySignal an " << exitOfSolver[i] <<" gesendet " << std::endl;
						MSL_Receive(exitOfSolver[i], problem, MSLT_BB_INCUMBENT, &messageStatus);
						if(debugCommunication) std::cout << MSL_myId << ": Neues Incumbent von Prozessor " << exitOfSolver[i] <<"empfangen. LB: "<< getLowerBound(problem) << std::endl;
						incumbentReceived = true;
					}
					// Empfangenes Incumbent verarbeiten
					if(incumbentReceived) {
						if(!solutionFound || betterThan(problem, incumbent)) {// Empfangene Lösung besser als bisherige?
							if(incumbent!=NULL) delete incumbent;
							incumbent = problem; // pool beschneiden später
							stat.statNumIncumbentReceivedAccepted++;
							solutionFound = true;
							if(debugCommunication) std::cout << MSL_myId << ": Neues Incumbent gesetzt" << std::endl;
						}
						else {
							if(debugCommunication) std::cout << MSL_myId << ": Empfangenes Incumbent von Prozessor " << exitOfSolver[i] <<"schlechter als aktuelles mit LB: "<< getLowerBound(incumbent) << std::endl;
							delete problem;
							stat.statNumIncumbentReceivedDiscarded++;
						}
						incumbentReceived = false;
					}
				} // Ende 2. Incumbent eingegangen
				stat.statTimeIncumbentHandling += MPI_Wtime() - statStart;

				statStart = MPI_Wtime();
				// 3. Nachrichten über gelöste Probleme eingegangen?
				// muss nicht über Handshake erfolgen, da nur kleine Daten versandt werden (24 Byte)
				for(int i = 0; i < numOfSolvers; i++) {
					if(debugTermination) std::cout << MSL_myId << ": Pruefe, ob Problemloesungsnachricht eingegangen ist" << std::endl;
					MPI_Iprobe(exitOfSolver[i], MSLT_BB_PROBLEM_SOLVED, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					if(messageWaiting) {
						problemFrame = new BBFrame<Problem>();
						MSL_Receive(exitOfSolver[i], problemFrame, MSLT_BB_PROBLEM_SOLVED, &messageStatus);
						if(debugTermination) std::cout << MSL_myId << ": Geloestes Problem (ID: " << problemFrame->getID() << ") von Prozessor " << exitOfSolver[i] <<" empfangen" << std::endl;
						double startTime = MPI_Wtime();
						problemTracker->problemSolved(problemFrame);

						stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
						stat.statNumMsgProblemSolvedReceived++;
						stat.statNumProblemsSolved++;
					}
				} // Ende 3. Nachrichten über gelöste Probleme eingegangen
				stat.statTimeTrackerSolvedProblemsReceived += MPI_Wtime() - statStart;

				// 4. Gelöste Probleme verschicken
				statStart = MPI_Wtime();
				while(!problemTracker->isSolvedQueueEmpty()) {
					problemFrame = problemTracker->readFromSolvedQueue();
					MSL_Send(problemFrame->getOriginator(), problemFrame, MSLT_BB_PROBLEM_SOLVED);
					if(debugTermination) std::cout << MSL_myId << ": Gelöstes Problem (ID: " << problemFrame->getID() << ") an Prozessor " <<  problemFrame->getOriginator() <<" gesendet" << std::endl;
					stat.statNumMsgProblemSolvedSent++;
					problemTracker->removeFromSolvedQueue();
				} // 4. Ende Gelöste Probleme verschicken
				stat.statTimeTrackerSolvedProblemsSent += MPI_Wtime() - statStart;

				// 5. Lastverteilungsanfragen senden
				statStart = MPI_Wtime();
				if(!sentBoundInfo)	{		// immer nur eine Anfrage auf einmal raus, um das Netz nicht zu überfluten, diese wird entweder beantwortet oder abgelehnt
					if(!sentIncumbentSendRequest) { // erst Incumbent raus
						if(debugLoadBalancing) std::cout << MSL_myId << ": BoundInfo senden beginnen" << std::endl;
						if(workpool->isEmpty())	{  	// leer, sende, falls noch keine Anfrage raus ist

							int bound = 2147483647;

							receiverOfBoundInfo = rand()%numOfWorkmates;
							while(entranceOfWorkmate[receiverOfBoundInfo]==MSL_myId)
								receiverOfBoundInfo = rand()%numOfWorkmates;

							if(debugLoadBalancing) std::cout << MSL_myId << ": Workpool leer, sende kleinsmögliche BoundInfo an " << entranceOfWorkmate[receiverOfBoundInfo] << std::endl;
							MSL_Send(entranceOfWorkmate[receiverOfBoundInfo], &bound, MSLT_BB_LOADBALANCE);
							sentBoundInfo = true;		// vorerst keine weitere Anfrage
							statStartTimeIdle = MPI_Wtime();
							stat.statNumMsgWorkPoolEmptySent++;
						}
						else {								// Pool nicht (mehr) leer -> sende trotzdem mit gewisser WS eine Bound; dies führt ggf. zu einem work request
							int random = rand()%100+1; 		// erzeugt Zufallszahl zwischen 1 und 100.
							if(random <= MSL_ARG2) { 		// verrate LB nur mit einer bestimmten WS, diese per Kommandozeile übergeben

								int bound = getLowerBound(workpool->top()->getData());

								receiverOfBoundInfo = rand()%numOfWorkmates;
								while(entranceOfWorkmate[receiverOfBoundInfo]==MSL_myId)
									receiverOfBoundInfo = rand()%numOfWorkmates;

								MSL_Send(entranceOfWorkmate[receiverOfBoundInfo], &bound, MSLT_BB_LOADBALANCE); 	// wrkpool->top() ist bestes Teilproblem
								stat.statNumMsgBoundInfoSent++;
								sentBoundInfo = true;
								if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance, sende lowerbound " << bound << " an Prozessor " << entranceOfWorkmate[receiverOfBoundInfo]  << std::endl;
							}
						}
					}
				}
				else {	// Anfrage ist schon raus, prüfe ob Antwort vorliegt
					bool loadReceived = false;
					if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: Prüfe, ob Antwort von Prozessor " << exitOfWorkmate[receiverOfBoundInfo] << " vorliegt"  << std::endl;
					MPI_Iprobe(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_LOADBALANCE_REJECTION, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					if(messageWaiting) { // wurde nicht angenommen, weil a) Pool leer b) bessere untere Schranke
						sentBoundInfo = false;
						MSL_ReceiveTag(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_LOADBALANCE_REJECTION);

						if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: Absage von Prozessor " << exitOfWorkmate[receiverOfBoundInfo] << " empfangen"  << std::endl;
						if(workpool->isEmpty()) {
							stat.statTimeIdle += MPI_Wtime() - statStartTimeIdle;
							stat.statNumMsgWorkPoolEmptyRejectionReceived++;
						} else
							stat.statNumMsgBoundRejectionReceived++;
					}
					// Solver mit kleinerer ID senden direkt
					MPI_Iprobe(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_PROBLEM, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					if(messageWaiting) {
						problemFrame = new BBFrame<Problem>();
						MSL_Receive(exitOfWorkmate[receiverOfBoundInfo], problemFrame, MSLT_BB_PROBLEM, &messageStatus);
						if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: Problem " << problemFrame->getID() << " von Prozessor " << exitOfWorkmate[receiverOfBoundInfo] << " empfangen"  << std::endl;
						loadReceived = true;
					}
					// Solver mit größerer ID senden erst eine Sendeanfrage
					MPI_Iprobe(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_PROBLEM_SENDREQUEST, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					if(messageWaiting) {
						problemFrame = new BBFrame<Problem>();
						if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: ProblemSendRequest von Prozessor " << exitOfWorkmate[receiverOfBoundInfo] << " empfangen"  << std::endl;
						MSL_ReceiveTag(exitOfWorkmate[receiverOfBoundInfo], MSLT_BB_PROBLEM_SENDREQUEST);
						MSL_SendTag(entranceOfWorkmate[receiverOfBoundInfo], MSLT_BB_PROBLEM_READYSIGNAL);
						if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: ProblemReadySignal an " << entranceOfWorkmate[receiverOfBoundInfo] << " geschickt"  << std::endl;
						MSL_Receive(exitOfWorkmate[receiverOfBoundInfo], problemFrame, MSLT_BB_PROBLEM, &messageStatus);
						if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: Problem " << problemFrame->getID() << " von Prozessor " << exitOfWorkmate[receiverOfBoundInfo] << " empfangen"  << std::endl;
						loadReceived = true;
					}

					// Wenn Nachricht empfangen, diese Verarbeiten
					if(loadReceived) {
						if(stat.statNumProblemsReceived==0)
							stat.statTimeInitialIdle = MPI_Wtime() - stat.statTimeInitialIdle;
						else
							if(workpool->isEmpty())
								stat.statTimeIdle += MPI_Wtime() - statStartTimeIdle;
						// nur akzeptieren, wenn theoretisch besser als Incumbent --> (falls zwischendurch eines empfangen wurde)
						problem = problemFrame->getData();
						if(!solutionFound || betterThan(problem, incumbent)) {
							workpool->insert(problemFrame);
							if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: Problem in Workpool eingefügt" << std::endl;
						}
						else { // Problem verwerfen -> also gelöst!
							double startTime = MPI_Wtime();
							problemTracker->problemSolved(problemFrame); // Tracker kümmert sich darum, dass es in die SendQueue kommt
							stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
							if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance: Problem verworfen. Absender " << exitOfWorkmate[receiverOfBoundInfo] << " benachrichtigt"  << std::endl;
							delete problem; // Tracker löscht Frame
						}
						sentBoundInfo = false;
						loadReceived = false;
						stat.statNumProblemsReceived++;
					}
				} // Ende 5. Lastverteilungsanfragen senden

				// 6. Lastverteilungsanfragen empfangen Teil 1: Anwort bei leerem Workpool; wird sonst unten übersprungen!
				if(workpool->isEmpty()) {
					if(debugLoadBalancing) std::cout << MSL_myId << ": Workpool leer; prüft, ob LoadBalanceanfrage vorliegen, die abgesagt werden müssen" << std::endl;
					for(int i = 0; i < numOfWorkmates; i++) {
						MPI_Iprobe(exitOfWorkmate[i], MSLT_BB_LOADBALANCE, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
						if(debugLoadBalancing) std::cout << MSL_myId << ": Prüfung, ob Loadbalanceanfrage von Prozessor " << entranceOfSolver[i] << " vorliegt durchgeführt: " << messageWaiting << std::endl;
						if(messageWaiting) {
							int loadInfo;
							if(debugLoadBalancing) std::cout << MSL_myId << ": Loadbalance Anfrage von Prozessor" << exitOfWorkmate[i] << " liegt vor"  << std::endl;
							MSL_Receive(exitOfWorkmate[i], &loadInfo, MSLT_BB_LOADBALANCE, &messageStatus);
							stat.statNumMsgBoundInfoReceived++;
							if(debugLoadBalancing) std::cout << MSL_myId << ": LoadInfo von Prozessor " << exitOfWorkmate[i] << " angenommen: loadInfo " << loadInfo << std::endl;
							MSL_SendTag(entranceOfWorkmate[i], MSLT_BB_LOADBALANCE_REJECTION);
							stat.statNumMsgBoundRejectionSent++;
							if(debugLoadBalancing) std::cout << MSL_myId << ": Workpool leer: LoadBalanceRejection an " << entranceOfWorkmate[i] << " schicken" << std::endl;
						}
					}
				}
				if(sentProblemSendRequest) { // muss auf jeden Fall verschickt werden, damit Termination detection funktioniert
					if(debugLoadBalancing) std::cout << MSL_myId << ": prüft, ob readysignal vorliegt" << std::endl;
					MPI_Iprobe(entranceOfWorkmate[senderOfLoadBalance], MSLT_BB_PROBLEM_READYSIGNAL, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					if(messageWaiting) {
						MSL_ReceiveTag(entranceOfWorkmate[senderOfLoadBalance], MSLT_BB_PROBLEM_READYSIGNAL);
						if(debugLoadBalancing) std::cout << MSL_myId << ": LoadInfo ProblemReadySignal angenommen: Problem verschickt an " << entranceOfSolver[senderOfLoadBalance] << std::endl;
						MSL_Send(entranceOfWorkmate[senderOfLoadBalance], loadBalanceProblemFrame, MSLT_BB_PROBLEM);
						stat.statNumProblemsSent++;

						sentProblemSendRequest = false;

						delete loadBalanceProblemFrame->getData();
						delete loadBalanceProblemFrame;
					}
				}  // Ende 6. Lastverteilungsanfragen empfangen
				stat.statTimeLoadBalancing += MPI_Wtime() - statStart;

				// 6. STOP Nachrichten empfangen
				MPI_Iprobe(masterSolver, MSLT_STOP, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
				if(messageWaiting) {
					MSL_ReceiveTag(masterSolver, MSLT_STOP);
					finished = true;
					if(debugCommunication) std::cout << MSL_myId << ": STOP: empfangen " << std::endl;
				} // Ende 6. STOP Nachrichten empfangen
				stat.statTimeCommunication += MPI_Wtime() - statStartTimeCommunication;
			}  // Ende Solverinterne Kommunikation


			// Beginn Problemverarbeitung / Lastverteilungsanfragenbearbeitung
			if(!finished && !workpool->isEmpty()) {
				if(debugComputation) std::cout << MSL_myId << ": beginnt Verarbeitungsphase" << std::endl;
				BBFrame<Problem>* workingProblemFrame = workpool->get();
				Problem* workingProblem = workingProblemFrame->getData();
				if(debugComputation) std::cout << MSL_myId << ": Problem aus dem Workpool entnommen" << std::endl;
				// eingehende Workrequests bearbeiten
				// wenn andere Solver vorhanden und der Pool nicht leer ist, dann kann Lastverteilung durchgeführt werden
				// und auch erst, wenn kein Incumbent unterwegs ist
				statStart = MPI_Wtime();
				statStartTimeCommunication = MPI_Wtime();
				if(numOfSolvers > 1 && !sentProblemSendRequest && !sentIncumbentSendRequest) {
					if(debugLoadBalancing) std::cout << MSL_myId << ": geht in Loadbalanceantwortphase" << std::endl;
					if(debugLoadBalancing) std::cout << MSL_myId << ": prüft, ob LoadBalanceanfrage vorliegt" << std::endl;

					int loadInfo;
					messageWaiting = 0;
					int check = senderOfLoadBalance; // maximal ein kompletter Durchlauf, sonst dauert es zu lang, falls die Austausch-WS zu gering ist
					do {
						senderOfLoadBalance = (senderOfLoadBalance + 1) % numOfWorkmates; // senderOfLoadBalance wurde zuletzt geprüft, fange also mit dem nächsten an!
						MPI_Iprobe(exitOfWorkmate[senderOfLoadBalance], MSLT_BB_LOADBALANCE, MPI_COMM_WORLD, &messageWaiting, &messageStatus);
					} while(!messageWaiting && senderOfLoadBalance!=check);

					if(messageWaiting) {
						MSL_Receive(exitOfWorkmate[senderOfLoadBalance], &loadInfo, MSLT_BB_LOADBALANCE, &messageStatus);
						stat.statNumMsgBoundInfoReceived++;
						if(debugLoadBalancing) std::cout << MSL_myId << ": LoadInfo von Prozessor" << entranceOfWorkmate[senderOfLoadBalance] << "ist " << loadInfo << std::endl;
						if(workpool->isEmpty()) {  // nichts zu verschicken, sende Rejection Message
							MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance], MSLT_BB_LOADBALANCE_REJECTION);
							stat.statNumMsgBoundRejectionSent++;
							if(debugLoadBalancing) std::cout << MSL_myId << ": Workpool leer: LoadBalanceRejection an " << exitOfWorkmate[senderOfLoadBalance] << " schicken" << std::endl;
						}
						else { // Mit Problem antworten
							// Dann vergleichen mit aktuell zweitbester
							int currentBestBound = getLowerBound(workpool->top()->getData());
							if(currentBestBound < loadInfo) { // dann zweitbestes Problem verschicken -> unterstützt schnelles finden guter Lösungen
								// alternativ: lösung in einen eigenen Puffer wegschreiben, der in der Kommunikationsphase geleert wird !
								loadBalanceProblemFrame = workpool->get(); // muss nun auf JEDEN FALL verschickt oder "gelöst" werden
								Problem* load = loadBalanceProblemFrame->getData();
								// nur dann überhaupt verschicken, wenn möglicherweise besser als das Incumbent
								if(!solutionFound || betterThan(load, incumbent)) {
									if(debugLoadBalancing) std::cout << MSL_myId << ": Load wird an Prozessor" << entranceOfWorkmate[senderOfLoadBalance] << "gesendet " << std::endl;
									if(MSL_myId < entranceOfWorkmate[senderOfLoadBalance]) { // Wenn ID kleiner als die des Empfängers, dann direkt verschicken
										if(debugLoadBalancing) std::cout << MSL_myId << ": Load direkt an Prozessor " << entranceOfWorkmate[senderOfLoadBalance] << " senden" << std::endl;
										MSL_Send(entranceOfWorkmate[senderOfLoadBalance], loadBalanceProblemFrame, MSLT_BB_PROBLEM);
										stat.statNumProblemsSent++;

										if(debugLoadBalancing) std::cout << MSL_myId << ": LoadInfo angenommen: Problem verschickt an " << entranceOfWorkmate[senderOfLoadBalance] << std::endl;

										delete load;
										delete loadBalanceProblemFrame; loadBalanceProblemFrame = NULL;
									}
									else {
										MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance], MSLT_BB_PROBLEM_SENDREQUEST);
										if(debugLoadBalancing) std::cout << MSL_myId << ": LoadInfo angenommen: ProblemSendRequest verschickt an " << entranceOfWorkmate[senderOfLoadBalance] << std::endl;
										sentProblemSendRequest = true;
									}
								}
								else { // Absage verschicken, weil problem gar nicht verschickt werden muss, da schlechter als Incumbent (nur wenn zwischenzeitlich incumbent eingegangen und pool nicht bereinigt)
									if(debugLoadBalancing) std::cout << MSL_myId << ": Load schlechter als Incumbent!: LoadBalanceRejection an " << entranceOfWorkmate[senderOfLoadBalance] << " schicken" << std::endl;
									MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance], MSLT_BB_LOADBALANCE_REJECTION);
									stat.statNumMsgBoundRejectionSent++;
									delete load; // Tracker löscht Frame
									double startTime = MPI_Wtime();
									problemTracker->problemSolved(loadBalanceProblemFrame);
									stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
								}
							}
							else {
								if(debugLoadBalancing) std::cout << MSL_myId << ": LoadInfo schlechter als zweitbestes Problem im Workpool! LoadBalanceRejection an " << entranceOfSolver[senderOfLoadBalance] << " schicken" << std::endl;
								MSL_SendTag(entranceOfWorkmate[senderOfLoadBalance], MSLT_BB_LOADBALANCE_REJECTION);
								stat.statNumMsgBoundRejectionSent++;
							}
						}
					}
				} // Ende Lastverteilung


				stat.statTimeLoadBalancing += MPI_Wtime() - statStart;
				stat.statTimeCommunication += MPI_Wtime() - statStartTimeCommunication;

				if(debugComputation) std::cout << MSL_myId << ": Beginne aktuelles Problem zu prüfen" << std::endl;
				// test if the best local problem may lead to a new incumbent.
				if(solutionFound && betterThan(incumbent, workingProblem)) {
					// No local problem can lead to a new incumbent -> discard all local problems from the work pool
					statStart = MPI_Wtime();
					if(debugTermination) std::cout << MSL_myId << ": Incumbent besser als alle Subprobleme!"<< std::endl;
					double startTime;
					if(!workpool->isEmpty()) {
						if(debugTermination) std::cout << MSL_myId << ": Pool leeren, da Incumbent besser als alle Subprobleme!"<< std::endl;
						// alle Probleme im Workpool werden gelöscht und sind damit "gelöst"
						// daher müssen für diese Probleme die entsprechenden Vaterprobleme benachrichtigt werden
						// möglich, da zu diesem Zeitpunkt alle Probleme im Workpool ungelöste Subprobleme sind
						while(!workpool->isEmpty()) {
							problemFrame = workpool->get();
							delete problemFrame->getData(); // Tracker löscht Frame
							startTime = MPI_Wtime();
							problemTracker->problemSolved(problemFrame);
							stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
							stat.statNumProblemsSolved++;
						}
						workpool->reset();
					}
					// aktuelles Problem ist ebenfalls gelöst!
					startTime = MPI_Wtime();
					problemTracker->problemSolved(workingProblemFrame);
					stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
					delete workingProblem;

					stat.statTimeCleanWorkpool += MPI_Wtime() - statStart;
					stat.statTimeSinceWorkpoolClean = MPI_Wtime();
					stat.statNumProblemsSolved++;
				}
				// Yes, the workingProblem may lead to a new incumbent.
				else {  // sonst zerlegen
					statStartTimeProcessing = MPI_Wtime();
					// Vaterproblem ID und Ursprung merken und anschließend zerlegen
					long parentID = workingProblemFrame->getID();
					ProcessorNo originator = workingProblemFrame->getOriginator();
					if(debugComputation) std::cout << MSL_myId << ": Problem ID: " << parentID << " originator: " << originator << " ist zu zerlegen" << std::endl;

					// branch the problem
					int numOfGeneratedSubproblems = 0;
/* ? */				Problem** subProblems = branch(workingProblem, &numOfGeneratedSubproblems);
					stat.statNumProblemsBranched++;


					if(debugComputation) std::cout << MSL_myId << ": Problem (ID " << parentID << ") in" << numOfGeneratedSubproblems << " zerlegt!" << std::endl;
					workingProblemFrame->setNumOfSubProblems(numOfGeneratedSubproblems);
					workingProblemFrame->setNumOfSolvedSubProblems(0); // sicherheitshalber

					if(numOfGeneratedSubproblems>0) {
						problemTracker->addProblem(workingProblemFrame);
						if(debugComputation) std::cout << MSL_myId << ": Problem ID: " << parentID << " originator: " << originator << " in den Tracker eingefügt" << std::endl;
					}
					else {// illegale Lösung -> als gelöst behandeln
						if(debugTermination) std::cout << MSL_myId << ": Problem ID: " << parentID << " originator: " << originator << " hat keine Teilprobleme erzeugt: als gelöst markieren" << std::endl;
						delete workingProblem;
						double startTime = MPI_Wtime();
						problemTracker->problemSolved(workingProblemFrame);
						stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
						stat.statNumProblemsKilled++;
					}
					// höchste KindID
					long subProblemID = parentID * numOfMaxSubProblems + numOfMaxSubProblems;

					// analyze subproblems
					for(int i = 0; i < numOfGeneratedSubproblems; i++) {
						// create a problem Frame
						problemFrame = new BBFrame<Problem>(subProblemID--, workingProblemFrame, MSL_myId, 0, subProblems[i]);
						if(debugComputation) std::cout << MSL_myId << ": Betrachte Teilproblem: " << i << std::endl;

						// if a solution is found
						if(isSolution(subProblems[i])) {
							if(debugTermination) std::cout << MSL_myId << ": Problem (ID " << subProblemID << ") gelöst!" << std::endl;
							// test if it is a new incumbent
							if(!solutionFound || betterThan(subProblems[i], incumbent)) {
								if(debugComputation) std::cout << MSL_myId << ": Problem (ID " << subProblemID << ") ist neues Incumbent!" << std::endl;
								// yes! New incumbent is found
								newIncumbentFound = true;
								// free memory for old incumbent
								if(incumbent!=NULL) delete incumbent;
								// store new incumbent
								incumbent = subProblems[i];
								solutionFound = true;
							}
							// free memory for worse solutions
							else delete subProblems[i];
							double startTime = MPI_Wtime();

							problemTracker->problemSolved(problemFrame);
							stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
							stat.statNumProblemsSolved++;
							stat.statNumSolutionsFound++;
						}
						// subproblem is not solved yet
						else  {
							if(debugComputation) std::cout << MSL_myId << ": Problem (ID " << subProblemID << ") ist keine Lösung, zunächst abschätzen" << std::endl;
							// estimate subproblem
							bound(subProblems[i]);
							stat.statNumProblemsBounded++;
							// check if the problem has been solved by applying bound
							if(isSolution(subProblems[i])) {		// könnte Incumbent sein, falls es jetzt eine Lösung ist
								if(debugTermination) std::cout << MSL_myId << ": Problem (ID " << subProblemID << ") gelöst!" << std::endl;
								// test if it is a new incumbent
								if(!solutionFound || betterThan(subProblems[i], incumbent)) {
									if(debugComputation) std::cout << MSL_myId << ": Problem (ID " << subProblemID << ") ist neues Incumbent!" << std::endl;
									// yes! New incumbent is found
									newIncumbentFound = true;
									// free memory for old incumbent
									if(incumbent!=NULL) delete incumbent;
									// store new incumbent
									incumbent = subProblems[i];
									solutionFound = true;
								}
								// free memory for worse solutions
								else delete subProblems[i];
								if(debugTermination) std::cout << MSL_myId << ": Problem (ID " << subProblemID << ") im Tracker als gelöst markieren" << std::endl;
								double startTime = MPI_Wtime();
								problemTracker->problemSolved(problemFrame);
								stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
								stat.statNumProblemsSolved++;
								stat.statNumSolutionsFound++;
							}
							// subproblem is not solved yet
							else { // ist noch ein Problem -> wenn es noch besser werden kann als das Incumbent -> in den Workpool
								if(debugComputation) std::cout << MSL_myId << ": Problem (ID " << subProblemID << ") in den Workpool verschoben! LowerBound: " << getLowerBound(subProblems[i]) << std::endl;
								// if the estimation is better than the best solution found so far, solving the subproblem may lead to a new incumbent
								if(!solutionFound || betterThan(subProblems[i], incumbent))
									workpool->insert(problemFrame);
								// oherwise discard this problem
								else {
									double startTime = MPI_Wtime();
									problemTracker->problemSolved(problemFrame);
									stat.statTimeSubProblemSolvedInsert += MPI_Wtime() - startTime;
									// free memory
									delete subProblems[i];
									stat.statNumProblemsSolved++;
								}
							}
						}
					}
					// free memory for array of subproblems returned by branch
					delete[] subProblems; // ist nur das Array von Zeigern
					stat.statTimeProblemProcessing += MPI_Wtime() - statStartTimeProcessing;
				}

			}  // Ende Problemverarbeitung
			// Termination Detection, Tracker ist leer, falls alle Solver terminiert sind
			if(debugTermination) std::cout << MSL_myId << ": Teste, ob Tracker leer" << std::endl;
			if(MSL_myId==masterSolver && problemTracker->isTrackerEmpty()) {
				if(debugTermination) std::cout << MSL_myId << ": Optimale Lösung gefunden!" << std::endl;
				MSL_Send(getReceiver(), incumbent);
				masterBlocked = false;
				solutionFound = false;
			}
		} // Ende while(!finished)

		// Statistiken berechnen, am MasterSolver sammeln und ausgeben
		stat.statTimeTotal = MPI_Wtime() - stat.statTimeTotal;
		stat.statTimeSinceWorkpoolClean = MPI_Wtime() - stat.statTimeSinceWorkpoolClean;

		stat.problemTrackerMaxLength = problemTracker->getProblemTrackerMaxLength();
		stat.problemTrackerAverageLength = problemTracker->getProblemTrackerAverageLength();
		stat.solvedProblemsQueueMaxLength = problemTracker->getSolvedQueueMaxLength();
		stat.polvedProblemsQueueAverageLength = problemTracker->getSolvedQueueAverageLength();
		stat.workpoolMaxLength = workpool->getMaxLength();
		stat.workpoolAverageLength = workpool->getAverageLength();

		//std::cout << "Analyse ; " << MSL_myId << "; ; " << << std::endl;
		if(analysis) {
			if(MSL_myId==masterSolver) {
				std::cout << MSL_ARG1 << "; total; runtime; " << MPI_Wtime()-MSL_Starttime << std::endl << std::flush;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumMsgProblemSolvedSent; " << stat.statNumMsgProblemSolvedSent << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumMsgProblemSolvedReceived; " << stat.statNumMsgProblemSolvedReceived << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumMsgBoundInfoSent; " << stat.statNumMsgBoundInfoSent << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumMsgBoundInfoReceived; " << stat.statNumMsgBoundInfoReceived << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumMsgWorkPoolEmptySent; " << stat.statNumMsgWorkPoolEmptySent << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumMsgBoundRejectionSent; " << stat.statNumMsgBoundRejectionSent << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumMsgBoundRejectionReceived; " << stat.statNumMsgBoundRejectionReceived << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumProblemsSent; " << stat.statNumProblemsSent << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumProblemsReceived; " << stat.statNumProblemsReceived << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumProblemsSolved; " << stat.statNumProblemsSolved << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumIncumbentReceivedAccepted; " << stat.statNumIncumbentReceivedAccepted << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumIncumbentReceivedDiscarded; " << stat.statNumIncumbentReceivedDiscarded << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumIncumbentSent; " << stat.statNumIncumbentSent << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumProblemsBranched; " << stat.statNumProblemsBranched << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumProblemsBounded; " << stat.statNumProblemsBounded << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumSolutionsFound; " << stat.statNumSolutionsFound << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumProblemsTrackedTotal; " << stat.statNumProblemsTrackedTotal << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statNumProblemsKilled; " << stat.statNumProblemsKilled << std::endl;

				std::cout << MSL_ARG1 << "; " << MSL_myId << "; ProblemTrackerMaxLength ; " << stat.problemTrackerMaxLength << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; ProblemTrackerAverageLength; " << stat.problemTrackerAverageLength << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; SolvedProblemsQueueMaxLength; " << stat.solvedProblemsQueueMaxLength << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; SolvedProblemsQueueAverageLength; " << stat.polvedProblemsQueueAverageLength << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; WorkpoolMaxLength; " << stat.workpoolMaxLength << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; WorkpoolAverageLength; " << stat.workpoolAverageLength << std::endl;

				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeTotal; " << stat.statTimeTotal << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeProblemProcessing; " << stat.statTimeProblemProcessing << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeCommunication; " <<  stat.statTimeCommunication << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeIncumbentHandling; " << stat.statTimeIncumbentHandling << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeLoadBalancing; " << stat.statTimeLoadBalancing << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeTrackerSolvedProblemsReceived; " << stat.statTimeTrackerSolvedProblemsReceived << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeTrackerSolvedProblemsSent; " << stat.statTimeTrackerSolvedProblemsSent << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeCleanWorkpool; " << stat.statTimeCleanWorkpool << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeSubProblemSolvedInsert; " << stat.statTimeSubProblemSolvedInsert << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeIdle; " << stat.statTimeIdle << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; statTimeInitialIdle; " << stat.statTimeInitialIdle << std::endl;
				std::cout << MSL_ARG1 << "; " << MSL_myId << "; timeSinceWorkpoolClean; " << stat.statTimeSinceWorkpoolClean<< std::endl;

				for(int i = 0; i < numOfSolvers; i++) {
					if(exitOfSolver[i]==MSL_myId) continue;
					statistics* statRemote = new statistics();
					MSL_Receive(exitOfSolver[i], statRemote, MSLT_BB_STATISTICS, &messageStatus);
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumMsgProblemSolvedSent; " << statRemote->statNumMsgProblemSolvedSent << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumMsgProblemSolvedReceived; " << statRemote->statNumMsgProblemSolvedReceived << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumMsgBoundInfoSent; " << statRemote->statNumMsgBoundInfoSent << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumMsgBoundInfoReceived; " << statRemote->statNumMsgBoundInfoReceived << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumMsgWorkPoolEmptySent; " << statRemote->statNumMsgWorkPoolEmptySent << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumMsgBoundRejectionSent; " << statRemote->statNumMsgBoundRejectionSent << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumMsgBoundRejectionReceived; " << statRemote->statNumMsgBoundRejectionReceived << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumProblemsSent; " << statRemote->statNumProblemsSent << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumProblemsReceived; " << statRemote->statNumProblemsReceived << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumProblemsSolved; " << statRemote->statNumProblemsSolved << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumIncumbentReceivedAccepted; " << statRemote->statNumIncumbentReceivedAccepted << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumIncumbentReceivedDiscarded; " << statRemote->statNumIncumbentReceivedDiscarded << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumIncumbentSent; " << statRemote->statNumIncumbentSent << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumProblemsBranched; " << statRemote->statNumProblemsBranched << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumProblemsBounded; " << statRemote->statNumProblemsBounded << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumSolutionsFound; " << statRemote->statNumSolutionsFound << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumProblemsTrackedTotal; " << statRemote->statNumProblemsTrackedTotal << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statNumProblemsKilled; " << statRemote->statNumProblemsKilled << std::endl;

					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; ProblemTrackerMaxLength ; " << statRemote->problemTrackerMaxLength << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; ProblemTrackerAverageLength; " << statRemote->problemTrackerAverageLength << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; SolvedProblemsQueueMaxLength; " << statRemote->solvedProblemsQueueMaxLength << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; SolvedProblemsQueueAverageLength; " << statRemote->polvedProblemsQueueAverageLength << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; WorkpoolMaxLength; " << statRemote->workpoolMaxLength << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; WorkpoolAverageLength; " << statRemote->workpoolAverageLength << std::endl;

					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeTotal; " << statRemote->statTimeTotal << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeProblemProcessing; " << statRemote->statTimeProblemProcessing << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeCommunication; " <<  statRemote->statTimeCommunication << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeIncumbentHandling; " << statRemote->statTimeIncumbentHandling << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeLoadBalancing; " << statRemote->statTimeLoadBalancing << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeTrackerSolvedProblemsReceived; " << statRemote->statTimeTrackerSolvedProblemsReceived << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeTrackerSolvedProblemsSent; " << statRemote->statTimeTrackerSolvedProblemsSent << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeCleanWorkpool; " << statRemote->statTimeCleanWorkpool << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeSubProblemSolvedInsert; " << statRemote->statTimeSubProblemSolvedInsert << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeIdle; " << statRemote->statTimeIdle << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; statTimeInitialIdle; " << statRemote->statTimeInitialIdle << std::endl;
					std::cout << MSL_ARG1 << "; " << exitOfSolver[i] << "; timeSinceWorkpoolClean; " << statRemote->statTimeSinceWorkpoolClean<< std::endl;
					delete statRemote;
				}
			} else {
				MSL_Send(masterSolver, &stat, MSLT_BB_STATISTICS);
			}
		}
	}


	void setWorkmates(BBSolver<Problem>** solver, int length, int id, int topology = MSL_BB_TOPOLOGY_ALLTOALL) {
		// indexID = id; // merke, an welcher Position man selbst steht
		numOfSolvers = length;
		entranceOfSolver = new ProcessorNo[numOfSolvers];
		exitOfSolver = new ProcessorNo[numOfSolvers];

		// Ein- und Ausgaenge aller Solver berechnen (inkl. des eigenen Ein- und Ausgangs)
		ProcessorNo* ext;
		ProcessorNo* entr;
		for(int i = 0; i < length; i++) {
			entr = (*(solver[i])).getEntrances();
			entranceOfSolver[i] = entr[0];
			ext = (*(solver[i])).getExits();
			exitOfSolver[i] = ext[0];
			//std::cout << "setWorkmates: entr/exit of solver["<<i < <"]: "<<entranceOfSolver[i]<<"/"<<exitOfSolver[i]<<std::endl;
		}

		// erzeuge gewünschte Topologie
		switch (topology) {
		case MSL_BB_TOPOLOGY_ALLTOALL:
			// einfach ein- und ausgänge von oben nutzen
			entranceOfWorkmate = entranceOfSolver;
			exitOfWorkmate = exitOfSolver;
			numOfWorkmates = numOfSolvers;
			break;
		case MSL_BB_TOPOLOGY_HYPERCUBE: {
				// generiere Hyperwürfel
				// Bestimme Dimension -> 2er logarithmus der Länge (muss dazu 2er Potenz sein!)
				int dim = 0;
				int tmp = length;
				while(tmp!=1) {
					dim++; tmp>>=1;
				}
				entranceOfWorkmate = new ProcessorNo[dim];
				exitOfWorkmate = new ProcessorNo[dim];
				int conNr = 1;
				for(int i = 0; i < dim; i++) {
					int mate = id ^ conNr;
					entranceOfWorkmate[i] = entranceOfSolver[mate];
					exitOfWorkmate[i] = exitOfSolver[mate];
					conNr<<=1;
				}
				numOfWorkmates = dim;
			}
			break;
		case MSL_BB_TOPOLOGY_RING: {
			// bidirektionaler ring
			// nachfolger <-> id <-> vorgänger
			// zur Umsetzung eines unidirektionalen Rings muss die Lastverteilung geändert werden
			// denn dort werden nur die Ausgänge von Workmates auf Anfragen geprüft
			entranceOfWorkmate = new ProcessorNo[2];
			exitOfWorkmate = new ProcessorNo[2];
			// vorgänger
			entranceOfWorkmate[0] = entranceOfSolver[(id-1+length)%length];
			exitOfWorkmate[0] = exitOfSolver[(id-1+length)%length];

			// nachfolger
			entranceOfWorkmate[1] = entranceOfSolver[(id+1)%length];
			exitOfWorkmate[1] = exitOfSolver[(id+1)%length];
			numOfWorkmates = 2;
			}
			break;
		}


	}

    Process* copy() { return new BBSolver<Problem>(branch, bound, betterThan, isSolution, getLowerBound, numOfMaxSubProblems, noprocs);}

   	void show() const {
   		if(MSL_myId == 0) std::cout << MSL_myId << " BBSolver " << entranceOfSolver[0] << std::endl << std::flush;
	}

};

/**
 * Zentrale Ausgangsklasse des Branch-and-Bound Skeletts. Der Nutzer bindet diese Klasse in seine Anwendung ein,
 * um das BnB Skelett zu nutzen.
 */
template<class Problem>
class BranchAndBound: public Process {

	// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
	BBSolver<Problem>** p;
	int length; // #Solver

    public:

		/**
		 * Ein dezentrales Branch&Bound-Skelett.
		 *
		 * Dieser Konstruktor erzeugt (l-1) Kopien des übergebenen BBSolvers. Insgesamt besteht dieses Skelett aus l Solvern.
		 *
		 * Externe Schnittstellen:
		 * Jeder Solver verfügt über genau einen Eingang und einen Ausgang. Das BranchAndBound-Skelett hat genau einen Eingang.
		 * Dieser wird auf einen der Solver gemappt. Die Ausgänge der Solver sind die Ausgänge des BranchAndBound-Skeletts.
		 *
		 * Interne schnittstellen:
		 * Jeder Solver kennt die Ein- und Ausgänge seiner Kollegen. Über pred und succ ist zudem eine logische Ringstruktur definiert,
		 * die für das versenden des Tokens im Rahmen des Termination Detection Algorithmus genutzt wird.
		 */
    	BranchAndBound(BBSolver<Problem>& solver, int l, int topology = MSL_BB_TOPOLOGY_ALLTOALL): length(l), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new BBSolver<Problem>* [length];
            p[0] = &solver;
            for(int i = 1; i < length; i++)
            	p[i] = (BBSolver<Problem>*)solver.copy();

			// externe Schnittstellen definieren:
			// Es gibt genau einen Mastersolver, der als Eingang fungiert.
			// Der (einzige) Eingang des Mastersolvers ist (einziger) Eingang des Skeletts, alle Solverausgänge sind Ausgänge des Skeletts.
			numOfEntrances = 1;
			entrances = new ProcessorNo[numOfEntrances];
			ProcessorNo* entr = (*(p[0])).getEntrances(); // Mastersolver
			entrances[0] = entr[0];

			numOfExits = 1;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext = (*(p[0])).getExits();
			exits[0] = ext[0];

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i, topology);
			// Empfaenger der ersten Nachricht festlegen
			//setStartReceiver(0); // BranchAndBound verschickt doch gar nichts, sondern die Solver
		}

//    	BranchAndBound(Problem** (*branch)(Problem&,int&), int numMaxSub, void (*bound)(Problem&), bool (*betterThan)(Problem const&,Problem const&), bool (*isSolution)(Problem const&), int (*getLowerBound)(Problem const&), int numSolver, int topology = MSL_BB_TOPOLOGY_ALLTOALL): length(numSolver), Process() {
       	BranchAndBound(Problem** (*branch)(Problem*,int*),
						void (*bound)(Problem*),
						bool (*betterThan)(Problem*,Problem*),
						bool (*isSolution)(Problem*),
						int (*getLowerBound)(Problem*),
						int numMaxSub,
						int numSolver,
						int topology = MSL_BB_TOPOLOGY_ALLTOALL)
						: length(numSolver), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new BBSolver<Problem>* [length];
            for(int i = 0; i < length; i++)
            	p[i] = new BBSolver<Problem>(branch, bound, betterThan, isSolution, getLowerBound, numMaxSub, 1);

			// externe Schnittstellen definieren:
			// Es gibt genau einen Mastersolver, der als Eingang fungiert.
			// Der (einzige) Eingang des Mastersolvers ist (einziger) Eingang des Skeletts, alle Solverausgänge sind Ausgänge des Skeletts.
			numOfEntrances = 1;
			entrances = new ProcessorNo[numOfEntrances];
			ProcessorNo* entr = (*(p[0])).getEntrances(); // Mastersolver
			entrances[0] = entr[0];

			numOfExits = 1;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext = (*(p[0])).getExits();
			exits[0] = ext[0];

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i, topology);
		}

    	~BranchAndBound() {
    		delete[] p; // Zeiger Array löschen
    		delete[] entrances;
    		delete[] exits;
    	}

		// von diesen l vorgelagerten Skeletten werden Daten empfangen
		// nur der Mastersolver kommuniziert mit den Sendern
		inline void setPredecessors(ProcessorNo* src, int l) {
			numOfPredecessors = l;
			(*(p[0])).setPredecessors(src,l);
		}

		// an diese n nachgelagerten Skelette werden Daten gesendet.
		// alle Solver können Daten verschicken
		inline void setSuccessors(ProcessorNo* succs, int n) {
			numOfSuccessors = n;
			for(int i = 0; i < length; i++)
				(*(p[i])).setSuccessors(succs,n);
		}

        // startet alle Solver
		inline void start() {
            for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

        Process* copy() { return new BranchAndBound<Problem>(*(BBSolver<Problem>*)(*(p[0])).copy(), length);}

        inline void show() const {
            if(MSL_myId == 0) {
                std::cout << MSL_myId << " BranchAndBound " << entrances[0] << std::endl << std::flush;
                for(int i = 0; i < length; i++)
                    (*(p[i])).show();
			}
		}
}; // end of BranchAndBound




//*######################################################################################################################

// Divide and conquer:

// Die Probleme werden in einer verketteten Liste verwaltet.
template<class Data>
class WorkpoolManager {

private:

	// soviele Probleme müssen mindestens im Pool sein, damit Last abgegeben wird
	// ! Wenn Threshold = 1, dann ist es möglich, dass das Originalproblem aufrund einer Arbeitsanfrage unmittelbar an einen anderen
	// Prozessor abgegeben wird und die Terminierung probleme bereitet.
	static const int THRESHOLD = 2;

	class ListElem {
		friend class WorkpoolManager<Data>;
		Frame<Data>* data;
		ListElem *pred, *succ;
		ListElem(Frame<Data>* pDat): data(pDat), pred(NULL), succ(NULL) {}
	};

	ListElem *head, *tail;
	int size;

public:

	// erzeugt einen neuen WorkpoolManager
	WorkpoolManager() {
		size = 0;
		head = new ListElem(NULL);
		tail = new ListElem(NULL);
		head->succ = tail;
		tail->pred = head;
		//current = head;
	}

	// gibt Speicher für die sortierte Liste wieder frei
	~WorkpoolManager() {
		// std::cout << "~WorkpoolManager: Speicher freigeben" << std::endl;
		if(!isEmpty()) {
			ListElem* current = head->succ;
			while(current!=tail) {
				current = current->succ;
				delete current->pred;
			}
		}
		delete head;
		head = NULL;
		delete tail;
		tail = NULL;
	}

	// zeigt an, ob der Workpoolpool leer ist bzw. voll genug, um Last abzugeben
	inline bool isEmpty() { return size == 0; }
	inline bool hasLoad() { return size >= THRESHOLD; }
	inline int getSize() { return size; }

	void insert(Frame<Data>* f) {
		ListElem* e = new ListElem(f);
		e->succ = head->succ;
		e->pred = head;
		e->succ->pred = e;
		head->succ = e;
		size++;
	}

	Frame<Data>* get() {
		if(isEmpty()) {
			std::cout << MSL_myId << ": EMPTY WORKPOOL EXCEPTION (get)" << std::endl;
			throw "empty workpool exception";
		}
		else {
			ListElem* firstElement = head->succ;
			Frame<Data>* pProblem = firstElement->data;
			head->succ = firstElement->succ;
			head->succ->pred = head;
			delete firstElement; // Speicher für das ListElem freigeben. Probleme?
			firstElement = NULL;
			size--;
			return pProblem;
		}
	}

	Frame<Data>* getLoad() {
		if(!hasLoad()) {
			std::cout << MSL_myId << ": NO LOAD EXCEPTION (getLoad)" << std::endl;
			throw "no load exception";
		}
		else {
			ListElem* lastElement = tail->pred;
			Frame<Data>* pLoad = lastElement->data;
			tail->pred = lastElement->pred;
			tail->pred->succ = tail;
			delete lastElement;
			lastElement = NULL;
			size--;
			return pLoad;
		}
	}

	void show() {
		if(isEmpty()) std::cout << MSL_myId << ": Workpool: []" << std::endl;
		else {
			std::cout << MSL_myId << ": Workpool: [";
			ListElem* currentElem = head->succ;
			while(currentElem->succ != tail) {
				std::cout << currentElem->data->getID() << ", ";
				currentElem = currentElem->succ;
			}
			std::cout << currentElem->data->getID() << "]" << std::endl;
		}
	}
}; // WorkpoolManager



// A solutionpool is a kind of a sorted stack
// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
template<class Solution>
class SolutionpoolManager {

private:

	// SolutionStack
	Frame<Solution>** stack;		// array of pointers to Frame-objects
	int size;						// number of elements that can be stored in the stack
	int lastElement;				// index of the top element stored in the stack

	Frame<Solution>** sendQueue;	// sendqueue stores Solutionframes to send back to a solver
	int sizeOfSendQueue;
	int lastElementOfSendQueue;

	Solution* (*comb)(Solution**); 	// combine-function
	int D;							// number of child nodes within the computation tree


public:

	// erzeugt einen neuen SolutionpoolManager
	// Atomic(O (* f)(I), int n): Process(), fct(Fct1<I,O,O (*)(I)>(f) ), noprocs(n) {
	SolutionpoolManager(Solution* (*combineFct)(Solution**), int d): comb(combineFct), D(d) {
		size = 8;			 // default value
		sizeOfSendQueue = 8; // default value
		stack = new Frame<Solution>*[size]; // Default: Array mit 8 Zeigern auf Frames
		sendQueue = new Frame<Solution>*[sizeOfSendQueue];
		lastElement = -1;
		lastElementOfSendQueue = -1;
	}

	// gibt Speicher für die sortierte Liste wieder frei
	~SolutionpoolManager() {
		delete[] stack;
		stack = NULL;
		delete[] sendQueue;
		sendQueue = NULL;
	}

	// *** SENDQUEUE ************************************************************************************
	inline bool sendQueueIsEmpty() { return lastElementOfSendQueue == -1; }

	inline bool sendQueueIsFull() { return lastElementOfSendQueue == sizeOfSendQueue-1; }

	inline void removeElementFromSendQueue() {
		if(sendQueueIsEmpty()) {
			std::cout << MSL_myId << ": EMPTY SENDQUEUE EXCEPTION (remove)" << std::endl;
			throw "empty SENDQUEUE exception";
		}
		else {
			// DRAFT: erstes Element löschen, alles andere umkopieren.
			// TODO verkettete Liste daraus bauen! TODO
			delete sendQueue[0];
			numSF--;
			for(int i = 0; i < lastElementOfSendQueue; i++)
				sendQueue[i] = sendQueue[i+1];
			sendQueue[lastElementOfSendQueue--] = NULL;
		}
	}

	inline Frame<Solution>* readElementFromSendQueue() {
		if(sendQueueIsEmpty()) {
			std::cout << MSL_myId << ": EMPTY SENDQUEUE EXCEPTION (read)" << std::endl;
			throw "empty SENDQUEUE exception";
		}
		else return sendQueue[0];
	}

	void writeElementToSendQueue(Frame<Solution>* pFrame) {
		//std::cout << MSL_myId << ": schreibe Problem " << pFrame->getID() << " in SendQueue" << std::endl;
		// sendQueue is full -> extend sendQueue
		if(sendQueueIsFull()) {
			Frame<Solution>** s = new Frame<Solution>*[sizeOfSendQueue*2];
			for(int i = 0; i < sizeOfSendQueue; i++) s[i] = sendQueue[i]; // copy pointers to new sendqueue
			sizeOfSendQueue *= 2;
			delete[] sendQueue; // free memory
			sendQueue = s;
		}
		sendQueue[++lastElementOfSendQueue] = pFrame;
	}

	void showSendQueue() {
		if(lastElementOfSendQueue==-1) std::cout << MSL_myId << ": SendQueue: []" << std::endl;
		else {
			std::cout << MSL_myId << ": SendQueue: [";
			for(int i = 0; i < lastElementOfSendQueue; i++)
				std::cout << sendQueue[i]->getID() << ", ";
			std::cout << sendQueue[lastElementOfSendQueue]->getID() << "]" << std::endl;
		}
	}
	// **************************************************************************************************


	// zeigt an, ob der Solutionpool leer/voll ist
	inline bool isEmpty() { return lastElement == -1; }
	inline bool isFull() { return lastElement == size-1; }

	inline Frame<Solution>* top() {
		if(isEmpty()) {
			std::cout << MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (top)" << std::endl;
			throw "empty solutionpool exception";
		}
		else return stack[lastElement];
	}

	inline void pop() {
		if(isEmpty()) {
			std::cout << MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (pop)" << std::endl;
			throw "empty solutionpool exception";
		}
		else {
			delete stack[lastElement]->getData();
			numS--;
			delete stack[lastElement];
			numSF--;
			stack[lastElement] = NULL;
			lastElement--;
		}
	}

	// Legt ein Frame auf den Stack. Der Stack wird entsprechend der Frame-IDs sortiert gehalten.
	// Wenn ein Problem in N Subprobleme aufgeteilt wird, können ggf. nach dem Versinken die oberen N Probleme auf dem Stack
	// zu einer neuen Lösung kombiniert werden.
	void insert(Frame<Solution>* pFrame) {
		// stack is full -> extend stack
		if(isFull()) {
			// std::cout << "extending solutionpool" << std::endl;
			Frame<Solution>** s = new Frame<Solution>*[size*2];
			for(int i = 0; i < size; i++) s[i] = stack[i]; // copy pointers to new stack
			size *= 2;
			delete[] stack; // free memory
			stack = s;
		}
		int index = lastElement++; // can be -1 !
		stack[lastElement] = pFrame;
		// sink
		while(index >= 0 && stack[index]->getID() > stack[index+1]->getID()) {
			Frame<Solution>* pHelp = stack[index+1];
			stack[index+1] = stack[index];
			stack[index] = pHelp;
			index--;
		}
		// std::cout << "Frame sinkt bis Indexposition " << ++index << std::endl;

		// TODO: Hier muss ggf. kombinert werden!

	}

	inline bool isLeftSon(int id) { return (id%D) == 1; }
	inline bool isRightSon(int id) { return (id%D) == 0; }

	// Wenn ein Frame mit der ID eines rechten Sohns auf dem Stack liegt, ist ein combine denkbar. Es muss jedoch
	// nicht explizit geprüft werden, ob alle Bruderknoten auf dem Stack liegen. Da der Stack sortiert ist,
	// reicht es zu prüfen, ob an der Stelle, an der der linkeste Bruders stehen sollte, tatsächlich auch der
	// linkeste Bruder steht. In diesem Fall müssen alle anderen Bruderknoten ebenfalls auf dem Stack liegen.
	inline bool combineIsPossible() {
		// Wenn rechter Sohn
		if(!isEmpty() && stack[lastElement]->getID()%D==0) { // Solutionpool ist bei Aufruf per Definition nicht leer
			int leftSonIndex = lastElement-D+1;
			if((leftSonIndex >= 0) && (stack[leftSonIndex]->getID()+D-1 == stack[lastElement]->getID())) return true;
		}
		return false;
	}

	// deepCombine scannt den Stack nach kombinierbaren Teillösungen, die durch empfangene Teillösungen tief im Stack vorliegen können.
	// Sind solche Lösungen vorhanden, wird pro Aufruf genau ein combine durchgeführt.
	// Ein erfolgreiches Kombinieren wird durch den Rückgabewert true angezeigt.
	bool deepCombine() {
		bool combined = false;
		if(!isEmpty()) {
			int currentID;
			int end = lastElement-D+1;
			for(int currentFrameIndex=0; currentFrameIndex<=end; currentFrameIndex++) {
				currentID = stack[currentFrameIndex]->getID();
				// suche einen linken Sohn
				if(isLeftSon(currentID)) {
					// sind seine Brüder auch da?
					if(stack[currentFrameIndex+D-1]->getID() == currentID+D-1) {
						// kombiniere Elemente i bis i+(D-1)
						Solution** partialSolution = new Solution*[D];
						// alle Bruderknoten aus den Frames entpacken
						for(int j = 0; j < D; j++) partialSolution[j] = stack[currentFrameIndex+j]->getData();
						// fasse Teillösungen zusammen, berechne neue ID und baue daraus einen Frame
						Solution* solution = comb(partialSolution); // Muscle-Function call
						numS++;
						// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
						long parentID = (stack[currentFrameIndex]->getID()-1)/D;
						long rootNodeID = stack[currentFrameIndex]->getRootNodeID();
						long originator = stack[currentFrameIndex]->getOriginator();
						long poolID = stack[currentFrameIndex]->getPoolID();
						Frame<Solution>* solutionFrame = new Frame<Solution>(parentID, rootNodeID, originator, poolID, solution);
						numSF++;
						// Speicher freigeben
						delete partialSolution; // TODO Damit können keine Teillösungen aus partialSolution für die TODO
						partialSolution = NULL; // TODO kombinierte Teillösung wiederverwendet werden! Ggf. ändern! TODO
						for(int j = 0; j < D; j++) {
							delete stack[currentFrameIndex+j]->getData();
							numS--;
							delete stack[currentFrameIndex+j];
							numSF--;
						}
						// umkopieren
						while(currentFrameIndex+D <= lastElement) {
							stack[currentFrameIndex] = stack[currentFrameIndex+D];
							currentFrameIndex++;
						}
						while(currentFrameIndex <= lastElement) stack[currentFrameIndex++] = NULL;
						lastElement -= D;
						numSF++;
						if(rootNodeID==parentID) {
							// Lösung in SendQueue schreiben
							writeElementToSendQueue(solutionFrame);
							//insert(solutionFrame);
							solutionFrame = NULL;
						}
						// schreibe neu berechneten Frame in den Pool
						else { insert(solutionFrame); }
						combined = true;
					}
				}
			}
		}
		return combined;
	}

	void combine() {
		Solution** partialSolution = new Solution*[D];
		int currentFrameIndex = lastElement - D + 1;
		// alle Bruderknoten aus den Frames entpacken
		for(int i = 0; i < D; i++) partialSolution[i] = stack[currentFrameIndex++]->getData();
		// fasse Teillösungen zusammen, berechne neue ID und baue daraus einen Frame
		Solution* solution = comb(partialSolution); // Muscle-Function call
		numS++;
		delete partialSolution; // TODO Damit können keine Teillösungen aus partialSolution für die TODO
		partialSolution = NULL; // TODO kombinierte Teillösung wiederverwendet werden! Ggf. ändern! TODO
		// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
		long parentID = (stack[lastElement-D+1]->getID()-1)/D;
		long rootNodeID = stack[lastElement-D+1]->getRootNodeID();
		long originator = stack[lastElement-D+1]->getOriginator();
		long poolID = stack[lastElement-D+1]->getPoolID();
		Frame<Solution>* solutionFrame = new Frame<Solution>(parentID, rootNodeID, originator, poolID, solution);
		numSF++;
		// entferne Teillösungen aus dem Solutionpool, die soeben zusammengefasst wurden
		for(int i = 0; i < D; i++)	pop(); // Speicher wird freigegeben
		if(rootNodeID==parentID) {
			// Lösung in SendQueue schreiben
			writeElementToSendQueue(solutionFrame);
			//insert(solutionFrame);
			solutionFrame = NULL;
		}
		// schreibe neu berechneten Frame in den Pool
		else { insert(solutionFrame); }
		// wiederhole dies rekursiv, bis nichts mehr zusammengefasst werden kann
		if(combineIsPossible()) combine();
	}


	// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
	inline bool hasSolution() {
		if(!isEmpty()) return (stack[lastElement]->getID() == 0);
		else return false;
	}


	void show() {
		if(lastElement==-1) std::cout << MSL_myId << ": Solutionpool: []" << std::endl;
		else {
			std::cout << MSL_myId << ": Solutionpool: [";
			for(int i = 0; i < lastElement; i++)
				std::cout << stack[i]->getID() << ", ";
			std::cout << stack[lastElement]->getID() << "]" << std::endl;
		}
	}
}; // SolutionpoolManager



// Dezentrales Divide and Conquer Skelett, angelehnt an BranchAndBoundOld
template<class Problem, class Solution>
class DCSolver: public Process {

private:

	bool blocked;							// state of Mastersolver
	bool solutionFound;						// indicates if incumbent found
	ProcessorNo* entranceOfSolver;			// Eingänge aller Solver (inkl. this): entranceOfSolver[0] ist Master
	ProcessorNo* exitOfSolver; 				// Ausgänge aller Solver (inkl. this): exitOfSolver[0] ist Master
	ProcessorNo left;						// Ringstruktur: Solver empfängt Token von left
	ProcessorNo right;						// Ringstruktur: Solver sendet Token an right
	int numOfSolvers;						// Anzahl der Solver, die durchs BnB-Skelett gruppiert werden
	// int indexID; 							// an dieser Position findet man sich selbst in den Arrays
    int noprocs; 							// assumption: uses contiguous block of processors
    int D;
	bool (*isSimple)(Problem*);				// Zeiger auf benutzerdefinierte Funktion
	Solution* (*solve)(Problem*);			// Zeiger auf benutzerdefinierte Funktion
	// DFct1<Problem*,Solution> fct;
	Problem** (*divide)(Problem*);			// Zeiger auf bound-Methode. Schätzt Subproblem ab.
	Solution* (*combine)(Solution**);		// Zeiger auf branch-Methode. Liefert Zeiger auf Array mit Zeigern auf erzeugte Subprobleme
    WorkpoolManager<Problem>* workpool;
    SolutionpoolManager<Solution>* solutionpool;

public:

	DCSolver(Problem** (*div)(Problem*), Solution* (*comb)(Solution**), Solution* (*solv)(Problem*), bool (*smpl)(Problem*), int d, int n):
				D(d), noprocs(n), isSimple(smpl), solve(solv), divide(div), combine(comb), Process() {
		// Workpool und Solutionpool erzeugen
		workpool = new WorkpoolManager<Problem>();
		solutionpool = new SolutionpoolManager<Solution>(comb,d);
      	solutionFound = false;
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setNextReceiver(0); 			// Default-Receiver ist 0. Den gibt's immer
		srand(time(NULL)); 				// zufällig gestellte Workrequests sicherstellen
		//srand(12345);
	}

	~DCSolver() {
		// std::cout << MSL_myId << ": ~DCSolver called" << std::endl;
		delete[] entrances; 	 entrances = NULL;
		delete[] exits; 		 exits = NULL;
		delete workpool; 		 workpool = NULL;
		delete solutionpool; 	 solutionpool = NULL;
		delete entranceOfSolver; entranceOfSolver = NULL;
		delete exitOfSolver;	 exitOfSolver = NULL;
	}


	void start() {
		// am Prozess beteiligte Prozessoren merken sich entrance und exit des Solvers
		finished = ((MSL_myId < entrances[0]) || (MSL_myId >= entrances[0] + noprocs));
		if(finished) return;
		// ******************

		bool deepCombineEnabled = true;
		//   flatCombineEnabled = false;
		bool analyse = false;

		//debugmessages
		bool debugCommunication = false;
		bool debugLoadBalancing = false;
		bool debugTerminationDetection = false;
		bool debugComputation = false;
		bool debugFreeMem = false;
		bool logIDs = false;

		// für die Statistik
		std::vector<unsigned long> v;
		int numOfProblemsProcessed = 0;
		int numOfSolutionsSent = 0;
		int numOfSolutionsReceived = 0;
		int numOfSubproblemsSent = 0;
		int numOfSubproblemsReceived = 0;
		int numOfWorkRequestsSent = 0;
		int numOfWorkRequestsReceived = 0;
		int numOfRejectionsSent = 0;
		int numOfRejectionsReceived = 0;
		int numOfSimpleProblemsSolved = 0;
		double time_solve = 0;
		double time_divide = 0;
		double time_combine = 0;
		double time_start = 0;
		double time_new = 0;
		double time_workpool = 0;
		double time_solutionpool = 0;
		double time_solver = 0;


		// define internal tolology
		int ALL_TO_ALL = 1;
		int topology = ALL_TO_ALL;

		// alle anderen merken sich die Anzahl der am Skelett beteiligten Prozessoren und den masterSolver-Eingang
		MSL_numOfLocalProcs = noprocs;
		ProcessorNo masterSolver = entranceOfSolver[0];
		MSL_myEntrance = entrances[0]; 	// Eingang des DCSolvers merken
		MSL_myExit = exits[0];			// Ausgang des DCSolvers merken


		Problem* problem = NULL;
		Solution* solution = NULL;
		Frame<Problem>* problemFrame = NULL;
		Frame<Problem>* subproblemFrame = NULL;
		Frame<Solution>* solutionFrame = NULL;



		MPI_Status status;
		int receivedStops = 0;						// Anzahl bereits eingegangener Stop-Tags
		int receivedTT = 0;							// Anzahl bereits eingegangener TerminationTest-Tags
		int dummy;									// (*) speichert TAGs
		bool workRequestSent = false;
		bool sendRequestSent = false;
		bool deepCombineIsNecessary = false;
		int flag = 0;
		long originator = -1;
		int receiverOfWorkRequest;				// nicht überschreiben, sonst geht die Lastverteilung kaputt!!!
		int predecessorIndex = 0;
		void* p1;
		void* p2;


		// solange Arbeit noch nicht beendet ist
		blocked = false;
		while(!finished) {

			// PROBLEMANNAHME
			// Nur der Master-Solver kann im Zustand FREE externe Nachrichten (Probleme und TAGs) annehmen
			if(MSL_myId == masterSolver && !blocked) {
				// Verbleibe in dieser Schleife, bis das nächste komplexe Problem ankommt oder Feierabend ist
				while(!blocked && !finished) {
					// blockiere bis Nachricht vom Vorgänger eingetroffen ist (Problem oder STOP möglich)
					flag = 0;
					if(debugCommunication) std::cout << MSL_myId << ": Mastersolver wartet auf neues Problem" << std::endl;
					while(!flag) {
						MPI_Iprobe(predecessors[predecessorIndex], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
						predecessorIndex = (predecessorIndex+1)%numOfPredecessors;
					}
					if(debugCommunication) std::cout << MSL_myId << ": Mastersolver empfängt Daten von " << status.MPI_SOURCE << " mit Tag = " << status.MPI_TAG << std::endl;
					// TAGS verarbeiten (optimierbar, wenn man Reihenfolge ändert: 1.) Problem, 2.) TAG)
					ProcessorNo source = status.MPI_SOURCE;
					if(status.MPI_TAG == MSLT_TERMINATION_TEST) {
						// Nimm TT-Nachricht entgegen
						MSL_ReceiveTag(source, MSLT_TERMINATION_TEST);
					}
					else if(status.MPI_TAG == MSLT_STOP) {
						// Nimm STOP-Nachricht entgegen
						MSL_ReceiveTag(source, MSLT_STOP);
						if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) Mastersolver hat STOP empfangen" << std::endl;
						receivedStops++; // Tags sammeln
						// wenn genug STOPs gesammelt
						if(receivedStops==numOfPredecessors) {
							if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) Mastersolver hat #numOfPredecessors STOPS empfangen -> Terminiere" << std::endl;
							// alle solverinternen Prozessoren benachrichtigen (falls Solver datenparallel arbeitet)
							for(int i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)
								MSL_SendTag(i, MSLT_STOP);
							// alle Solver (außer sich selbst) benachrichtigen
							if(numOfSolvers > 1) {
								for(int i = 0; i < numOfSolvers; i++) {
									if(entranceOfSolver[i]!=MSL_myId) {
										if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) Mastersolver sendet STOP intern an "<<entranceOfSolver[i]<<std::endl;
										MSL_SendTag(entranceOfSolver[i], MSLT_STOP);
									}
								}
							}
							// alle Nachfolger benachrichtigen
							for(int i = 0; i < numOfSuccessors; i++) {
								if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) Mastersolver schickt STOP an Nachfolger "<<successors[i]<<std::endl;
								MSL_SendTag(successors[i], MSLT_STOP);
							}
							receivedStops = 0;
							finished = true; // Feierabend! DCSolver terminiert...
							if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) MasterSolver " << MSL_myId << " terminiert"<< std::endl;
						}
					}
					// wenn's kein TAG war, war's ein neues DC-Problem
					else {
						if(debugCommunication) std::cout << MSL_myId << ": Mastersolver bereitet Empfang eines DC-Problems von " << source << " mit Tag = " << MSLT_MYTAG << " vor" << std::endl;
						problem = new Problem();
						numP++;
						MSL_Receive(source, problem, MSLT_MYTAG, &status);
						numOfSubproblemsReceived++; // statistics
						if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) Mastersolver empfängt DC-Problem" << std::endl;
						// DATAPARALLEL PART: sende Problem an alle solverinternen Prozessoren
						// for(i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)	send(i, &problem, sizeof(Problem));

						// wenn das Problem einfach genug ist, löse es direkt, und verschicke das Ergebnis an den Nachfolger
						if(isSimple(problem)) {
							if(debugComputation) std::cout << MSL_myId << ":(DCSolver::start) Problem wird direkt gelöst"<< std::endl;
							solution = solve(problem);
							numS++;
							p1 = (void*)problem;
							p2 = (void*)solution;
							if(p1 != p2) {
								delete problem;
								problem = NULL;
								numP--;
							}
							if(debugComputation) std::cout << MSL_myId << ":(DCSolver::start) Problem gelöst"<< std::endl;
							MSL_Send(getReceiver(), solution);
							if(debugCommunication) std::cout << MSL_myId << ":(DCSolver::start) Lösung verschickt"<< std::endl;
							delete solution;
							solution = NULL;
							numS--;
							numOfSolutionsSent++; // statistics
						}
						// sonst initiiere die parallele Lösung des Problems
						else {
							// Blockiere Solver bis Problem gelöst ist
							blocked = true; // Abbruch der inneren while-Schleife
							if(debugComputation) std::cout << MSL_myId << ":(DCSolver::start) Problem ist zu komplex und wird parallel gelöst"<< std::endl;
							// Root-Initialization: 	 KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
							time_start = MPI_Wtime();
							problemFrame = new Frame<Problem>(0,-1,-1,masterSolver,problem); // Pair: (id, Problem*)
							time_new += MPI_Wtime() - time_start;
							numPF++;
							// Frame in den Workpool schreiben
							time_start = MPI_Wtime();
							workpool->insert(problemFrame);
							time_workpool += MPI_Wtime() - time_start;
							if(debugComputation) { workpool->show(); solutionpool->show(); }
						}
					}

				} // end of inner while
				// hier angekommen, wurde entweder ein komplexes DC-Problem empfangen (Workpool nicht leer) oder es ist Feierabend

			} // ende Problemannahme


			// INTERNER NACHRICHTENVERKEHR zwischen den Solvern, falls es mehrere gibt:
			// Kommunikation und Problemverarbeitung muss verschachtelt werden, damit eine schnelle Reaktion auf Arbeitsanfragen
			// garantiert werden kann.
			if((numOfSolvers>1) && (!finished) && (MSL_myId==MSL_myEntrance)) {

				// 1. verarbeite alle empfangenen Teillösungen
				// Ankommende, gelöste Teilprobleme werden einfach in den SolutionPool geschrieben und dort genau so weiterverarbeitet, als wäre
				// die Lösung lokal berechnet worden. Wenn sich im Rahmen des Combine eine neue VaterID ergibt, zu der es in der RootList
				// ein Paar (VaterID, Sender) gibt, so ist das Teilproblem gelöst und kann an den Sender zurückgeschickt werden.
				// Das Paar (VaterID, Sender) wird aus der RootList entfernt.
				for(int i = 0; i < numOfSolvers; i++) {
					// alle Solver mit kleinerer ProzessID dürfen immer senden
					MPI_Iprobe(exitOfSolver[i], MSLT_SOLUTION, MPI_COMM_WORLD, &flag, &status);
					if(flag) {
						solutionFrame = new Frame<Solution>();
						numSF++;
						MSL_Receive(exitOfSolver[i], solutionFrame, MSLT_SOLUTION, &status);
						numOfSolutionsReceived++; // statistics
						//if(debugCommunication) std::cout << MSL_myId<< ": empfange Teillösung (" << solutionFrame->getID()<<","<<solutionFrame->getRootNodeID()<<","<<solutionFrame->getOriginator()<<")" << std::endl;
						if(debugLoadBalancing) { workpool->show(); solutionpool->show(); std::cout << MSL_myId<< ": empfange Teillösung " << solutionFrame->getID()<< std::endl; }
						solutionpool->insert(solutionFrame);
						time_start = MPI_Wtime();
						if(solutionpool->combineIsPossible()) solutionpool->combine();
						deepCombineIsNecessary = true;	// deepCombine anstoßen
						time_combine += MPI_Wtime() - time_start;
						if(debugLoadBalancing) { solutionpool->show(); }
					}
					// Solver mit größerer ID schicken Sendeanfragen. Bei Eingang einer Sendeanfrage geht der Solver direkt auf Empfang
					MPI_Iprobe(exitOfSolver[i], MSLT_SENDREQUEST, MPI_COMM_WORLD, &flag, &status);
					if(flag) {
						if(debugLoadBalancing) std::cout << MSL_myId << ": SENDREQUEST von " << exitOfSolver[i] << " eingegangen. Sende READYSIGNAL und gehe auf Empfang" << std::endl;
						MSL_ReceiveTag(exitOfSolver[i], MSLT_SENDREQUEST);
						MSL_SendTag(entranceOfSolver[i], MSLT_READYSIGNAL);
						solutionFrame = new Frame<Solution>();
						numSF++;
						MSL_Receive(exitOfSolver[i], solutionFrame, MSLT_SOLUTION, &status);
						numOfSolutionsReceived++; // statistics
						if(debugLoadBalancing) { workpool->show(); solutionpool->show(); std::cout << MSL_myId<< ": empfange Teillösung " << solutionFrame->getID() << std::endl; }
						solutionpool->insert(solutionFrame);
						time_start = MPI_Wtime();
						if(solutionpool->combineIsPossible()) solutionpool->combine();
						time_combine += MPI_Wtime() - time_start;
						if(debugLoadBalancing) { solutionpool->show(); }
					}
				}
				// 2. Nach dem Empfang einer Teillösung liegen ggf. kombinierbare Teillösungen tief im Stack.
				// Solange versunkene kombinierbare Teillösungen vorhanden sind, wird ein deepCombine durchgeführt.
				// Der Methodenaufruf wird umgangen, wenn keine kombinierbaren Teilprobleme im Stack liegen.
				if(deepCombineEnabled && deepCombineIsNecessary) deepCombineIsNecessary = solutionpool->deepCombine();

				// 3. kontrolliertes Verschicken von Nachrichten in der SendQueue
				// nur eine Verschicken, um Deadlocks vorzubeugen???
				// Idee: Das Verschicken einer Lösung wird übersprungen, solange noch potentiell große Teilprobleme oder Lösungen vom Empänger
				// entgegen genommen werden müssen. Auf diese Weise werden Deadlocks hoffentlich vermieden.
				// Eine Lösung darf ich nur verschicken, wenn ein ggf. aktiver Lastverteilungsprozess abgeschlossen ist
				if(!solutionpool->sendQueueIsEmpty() && !workRequestSent) { //***
					// if(debugLoadBalancing) solutionpool->showSendQueue();
					solutionFrame = solutionpool->readElementFromSendQueue();
					originator = solutionFrame->getOriginator();
					// Senden ist erlaubt, wenn SenderID < EmpfängerID und vorher kein WorkRequest verschickt wurde.
					// Dies könnte sonst zu einem zyklischen Deadlock führen:
					// Situation: A: WorkReq an C, danach Lösung an B; B schickt Lösung an C; C antwortet A mit Problem.
					// -> Deadlock, wenn alle Nachrichten Handshake erfordern! A wartet auf B, B wartet auf C und C wartet auf A
					if(MSL_myId < originator) { // ***
						// direktes Verschicken von Lösungen ist verboten, wenn vorher ein Workrequest an den Empfänger gesendet wurde,
						// denn dann kann ggf. als Antwort darauf ein Teilproblem unterwegs sein, das ebenfalls ein Handshake erfordert.
						// Das Ergebnis wäre ein Deadlock, da Sender und Empfänger gegenseitig auf die Abnahme der Nachricht warten.
						//if(!(workRequestSent && originator==receiverOfWorkRequest)) { // zyklischer Deadlock möglich?
							if(debugLoadBalancing) { std::cout << MSL_myId << ": sende Teillösung " << solutionFrame->getID() << " direkt an " << originator << std::endl; }
							MSL_Send(originator, solutionFrame, MSLT_SOLUTION); // kann blockieren!
							solutionpool->removeElementFromSendQueue();
							solutionFrame = NULL;
						//}
					}
					// sonst muss zunächst ein SendRequest an den Empfänger der Nachricht geschickt werden
					else if(!sendRequestSent) {
						if(debugLoadBalancing) {
							solutionpool->showSendQueue();
							std::cout << MSL_myId << ": versuche Frame aus Sendqueue zu senden: (" << solutionFrame->getID() << "," << solutionFrame->getRootNodeID() << "," << originator << ")" << std::endl;
							std::cout << MSL_myId << ": sende SENDREQUEST an " << originator << std::endl;
							}
						MSL_SendTag(originator, MSLT_SENDREQUEST);
						sendRequestSent = true;
					}
					// ist das bereits geschehen, darf die Nachricht erst nach Eingang des ReadySignals verschickt werden
					else {
						// warte auf readySignal
						MPI_Iprobe(originator, MSLT_READYSIGNAL, MPI_COMM_WORLD, &flag, &status);
						if(flag) {
							// Empfänger wartet auf den Eingang des SolutionFrames
							if(debugLoadBalancing) { std::cout << MSL_myId << ": READYSIGNAL eingegangen" << std::endl; }
							MSL_ReceiveTag(originator, MSLT_READYSIGNAL);
							if(debugLoadBalancing) { std::cout << MSL_myId << ": sende Teillösung " << solutionFrame->getID() << " an " << originator << std::endl; }
							MSL_Send(originator, solutionFrame, MSLT_SOLUTION);
							sendRequestSent = false; // Antwort erhalten
							solutionpool->removeElementFromSendQueue();
							solutionFrame = NULL;
							if(debugLoadBalancing) { solutionpool->showSendQueue(); }
						}
					}
				}



				//* 4. eingehende WorkRequests verarbeiten
				// eingehende Workrequests von P werden erst dann verarbeitet, wenn die Übertragung von Teillösungen an P abgeschlossen ist.
				// Andernfalls erwartet P nach einem Readysignal eine Solution, aufgrund des Workrequests wird aber ein Subproblem geschickt.
				// Dies würde zum Deadlock führen, wenn beide Nachrichten per Handshake verschickt werden müssen.
				originator = -1;
				if(!solutionpool->sendQueueIsEmpty())
					originator = solutionpool->readElementFromSendQueue()->getOriginator();
				for(int i = 0; i < numOfSolvers; i++) {
					MPI_Iprobe(exitOfSolver[i], MSLT_WORKREQUEST, MPI_COMM_WORLD, &flag, &status);
//					if(flag && !(sendRequestSent && exitOfSolver[i]==originator)) { // zyklischer Deadlock möglich?
					// workrequests erst verarbeiten, wenn Senden aus Solutionpool abgeschlossen ist
					if(flag && !sendRequestSent) {
						// receive message
						MSL_ReceiveTag(exitOfSolver[i], MSLT_WORKREQUEST);
						numOfWorkRequestsReceived++; // statistics
						if(!workpool->hasLoad()) { // inform sender about an empty workpool
							//if(debugLoadBalancing) std::cout << MSL_myId << " kann keine Arbeit abgeben! " << std::endl;
							MSL_SendTag(entranceOfSolver[i], MSLT_REJECTION);
							if(debugLoadBalancing) std::cout << MSL_myId<< ": REJECTION an " << entranceOfSolver[i] << std::endl;
							numOfRejectionsSent++; // statistics
						}
						else { // distribute load (min 2 subproblems are stored in the workpool)
							if(debugCommunication) { workpool->show(); solutionpool->show(); }
							problemFrame = workpool->getLoad(); // get the lastElement stored in the workpools' linked list
							// if(debugLoadBalancing) std::cout << MSL_myId << ": Verschicke Problem (" << problemFrame->getID()<<","<<problemFrame->getRootNodeID()<<","<<problemFrame->getOriginator()<<") an " << entranceOfSolver[i] << std::endl;
							if(debugCommunication) std::cout << MSL_myId << ": Abgabe von Problem " << problemFrame->getID()<< " an " << entranceOfSolver[i] << std::endl;
							// TODO Deadlockgefahr!? Große Probleme werden ohne Sendrequest verschickt! TODO
							MSL_Send(entranceOfSolver[i], problemFrame, MSLT_SUBPROBLEM);
							delete problemFrame->getData();
							delete problemFrame;
							numP--;
							numPF--;
							problemFrame = NULL;
							numOfSubproblemsSent++; // statistics
							//if(debugLoadBalancing) std::cout << MSL_myId << ": Arbeit verschickt." << std::endl;
						}
					}
				}

				//* 5. Lastverteilung
				if(workpool->isEmpty() && !sendRequestSent) { // ***
					// Wenn noch kein Work Request verschickt wurde
					if(!workRequestSent) {
						// sende Work Request und merke den Index des Empfänger
						do { receiverOfWorkRequest = entranceOfSolver[rand()%numOfSolvers]; }
						while(receiverOfWorkRequest == MSL_myId);
						// optimieren: nicht wieder an den gleichen schicken

						 // TODO: Umbenennen: numOfSolvers -> numOfPartners
						MSL_SendTag(receiverOfWorkRequest, MSLT_WORKREQUEST);
						if(debugLoadBalancing) { std::cout << MSL_myId << ": WORKREQUEST an " << receiverOfWorkRequest << std::endl; solutionpool->show(); }
						numOfWorkRequestsSent++; // für die Statistik
						workRequestSent = true;	// avoid flooding the network with work requests
					}
					// sonst warte auf Antwort
					else {
						// Wenn eine Absage angekommt
						MPI_Iprobe(receiverOfWorkRequest, MSLT_REJECTION, MPI_COMM_WORLD, &flag, &status);
						if(flag) {
							// Absage entgegennehmen und nächsten Work Request verschicken
							MSL_ReceiveTag(receiverOfWorkRequest, MSLT_REJECTION);
							if(debugLoadBalancing) std::cout << MSL_myId<< ": REJECTION von " << receiverOfWorkRequest << std::endl;
							numOfRejectionsReceived++; // statistics
							workRequestSent = false; // Antwort erhalten
						}
						// sonst ist vielleicht Arbeit angekommen
						else {
							// TODO:
							// Nach einer Arbeitsanfrage wird ein neues Teilproblem empfangen. Die Lösung dieses Problems muss an den Sender
							// zurückgeschickt werden. Dazu erstelle ein Paar (ID, Sender) und schreibe es in die RootList des SolutionPoolManagers.
							// Das empfange Problem wird im Workpool abgespeichert. Es ist darauf zu achten, dass dieses Problem im Rahmen
							// der Lastverteilung nicht direkt wieder abgegeben wird (sonst wäre der Workpool wieder leer). Dies ist implizit
							// sichergestellt, da die Problemverarbeitungsphase vor der nächsten Lastverteilungsphase durchlaufen wird.
							MPI_Iprobe(receiverOfWorkRequest, MSLT_SUBPROBLEM, MPI_COMM_WORLD, &flag, &status);
							if(flag) {
								problemFrame = new Frame<Problem>(); // Konstruktor?
								numPF++;
								MSL_Receive(receiverOfWorkRequest, problemFrame, MSLT_SUBPROBLEM, &status);
								numOfSubproblemsReceived++; // statistics
								// Den Absender und die RootID in den Frame schreiben
								problemFrame->setOriginator(receiverOfWorkRequest); // an diese Adr. wird die Lösung geschickt
								problemFrame->setRootNodeID(problemFrame->getID());
								// Problem einfach in den Workpool schreiben, der sich um alles Weitere kümmert
								// if(debugLoadBalancing) std::cout << MSL_myId<< ": empfange Problem (" << problemFrame->getID()<<","<<problemFrame->getRootNodeID()<<","<<problemFrame->getOriginator()<<")" << std::endl;
								if(debugCommunication) std::cout << MSL_myId<< ": empfange Problem " << problemFrame->getID() << " von " << receiverOfWorkRequest << std::endl;
								workpool->insert(problemFrame);
								workRequestSent = false; // Antwort erhalten
							}
						}
					}
				}

				// 6. Prüfe auf STOPS (Mastersolver empfängt nie STOP als interne Nachricht)
				// Anmerkung 05.03.08: Warum prüft sich der MS denn selbst, wenn er nie interne STOPs bekommen kann!?
				//                     Vermutlich ein Copy-Paste-Fehler, weil aus BnB übernommen!?
				// STOPs werden nur vom Master versendet, da dieser die Termination Detection übernimmt.
				MPI_Iprobe(masterSolver, MSLT_STOP, MPI_COMM_WORLD, &flag, &status);
				if(flag==true) {
					flag = false;
					MSL_ReceiveTag(masterSolver, MSLT_STOP);
					if(debugCommunication) std::cout << "(DCSolver::start): DCSolver " << MSL_myId << " hat STOP empfangen" << std::endl;
					// alle solverinternen Prozessoren benachrichtigen (falls Solver datenparallel ist)
					for(int i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)
						MSL_SendTag(i, MSLT_STOP);
					// sende STOP an Nachfolger (alter Terminierungsalgorithmus)

					// 05.03.08:
					// HARMLOSER BUG: Diese Nachrichten wird von niemandem empfangen, oder!?
					// ist successors[i] überhaupt für DCSolver definiert???
					for(int i = 0; i < numOfSuccessors; i++) // ### raus!?
						MSL_SendTag(successors[i], MSLT_STOP); // ### raus!?
					receivedStops = 0;
					finished = true; // Feierabend
					if(debugCommunication) std::cout << "DCSolver " << MSL_myId << " terminiert"<< std::endl;
				}
			} // interner Nachrichtenverkehr



			//* 7. PROBLEMVERARBEITUNG: (fertig)
			// Workpool speichert auch einfache Probleme
			// - simple Probleme können als Last abgegeben werden, obwohl lokale Bearbeitung schneller wäre
			// - einfache Probleme werden nicht direkt gelöst, sondern vorher als Frame wieder in den Workpool geschrieben (Overhead)
			// - erhöhter Speicherplatzbedarf, da einfache Probleme ebenfalls im Workpool liegen
			// - pro gelöstes Problem ist 1 combinationIsPossible-Prüfung notwendig (Overhead)
			// + geringer Rechnenaufwand pro Iteration -> schnelles Erreichen und häufiges Durchlaufen der Kommunikationsphase
			if(!finished && (!workpool->isEmpty())) {
				time_start = MPI_Wtime();
				problemFrame = workpool->get();
				time_workpool += MPI_Wtime() - time_start;
				problem = problemFrame->getData();
				numOfProblemsProcessed++;
				if(debugComputation) { std::cout << MSL_myId << ": get Problem" << std::endl; workpool->show(); solutionpool->show(); }
				long currentID = problemFrame->getID();
				long rootNodeID = problemFrame->getRootNodeID();
				originator = problemFrame->getOriginator();
				long poolID = problemFrame->getPoolID();
				delete problemFrame;
				problemFrame = NULL;
				numPF--;
				// Wenn das Problem zu komplex ist, zerlegen und in Workpool schreiben
				if(!isSimple(problem)) {
					time_start = MPI_Wtime();
					Problem** subproblem = divide(problem); // ### USER DEFINED DIVIDE() ###
					time_divide += MPI_Wtime() - time_start;
					numP += D;
					bool toDelete = true;
					for(int i = 0; i < D; i++) {
						// falls das an divide übergebene Problem vom user wiederverwendet wurde und Teil des subproblem-Arrays ist,
						// darf es nicht gelöscht werden!
						if(subproblem[i] == problem) toDelete = false;
						break;
					}
					if(toDelete) {	delete problem;	problem = NULL;	numP--;}
					// KindID = VaterID*D+1; 	VaterID = (KindID-1)/D; 	WurzelID = 0
					long subproblemID = currentID * D + 1 + (D-1); // ID des letzten Kindknotens
					// erzeuge pro Subproblem einen neuen Frame (ID, Problem*) und speichere ihn im Workpool
					for(int i=D-1; i>=0; i--) {
						// Original-ProblemID, Owner und Absender werden lokal nie geändert, sondern einfach kopiert
						time_start = MPI_Wtime();
						problemFrame = new Frame<Problem>(subproblemID--, rootNodeID, originator, poolID, subproblem[i]); // ###
						time_new += MPI_Wtime() - time_start;
						numPF++;
						time_start = MPI_Wtime();
						workpool->insert(problemFrame);
						time_workpool += MPI_Wtime() - time_start;
						if(debugComputation) { std::cout << MSL_myId << ": insert Problem" << std::endl; workpool->show(); solutionpool->show(); }
					}
					delete[] subproblem; // Hinweis in der Dokumentation, dass Speicher intern freigegeben wird!
				}
				// Wenn das Problem einfach genug ist, lösen, unter gleicher ID im Solutionpool ablegen und ggf. kombinieren
				else {
					time_start = MPI_Wtime();
					solution = solve(problem); // ### USER DEFINED SOLVE() ###
					time_solve += MPI_Wtime() - time_start;
					numS++;
					numOfSimpleProblemsSolved++;
					p1 = (void*)problem;
					p2 = (void*)solution;
					if(p1 != p2) { delete problem; problem = NULL; numP--;}
					// Original-ProblemID und Absender werden lokal nie geändert, sondern einfach kopiert
					time_start = MPI_Wtime();
					solutionFrame = new Frame<Solution>(currentID, rootNodeID, originator, poolID, solution);
					time_new += MPI_Wtime() - time_start;
					numSF++;
					if(currentID == rootNodeID) {
						// Lösung in SendQueue schreiben
						time_start = MPI_Wtime();
						solutionpool->writeElementToSendQueue(solutionFrame);
						time_solutionpool += MPI_Wtime() - time_start;
						// solutionpool->insert(solutionFrame);
						solutionFrame = NULL;
					}
					else {
						if(debugComputation) std::cout << MSL_myId << ": Problem (" << currentID << "," << rootNodeID << "," << originator << ") gelöst. Schreibe in Solutionpool." << std::endl;
						time_start = MPI_Wtime();
						solutionpool->insert(solutionFrame);
						time_solutionpool += MPI_Wtime() - time_start;
						time_start = MPI_Wtime();
						if(solutionpool->combineIsPossible()) solutionpool->combine();
						time_combine += MPI_Wtime() - time_start;
						if(debugComputation) { std::cout << MSL_myId << ": combine/insert solution" << std::endl; workpool->show(); solutionpool->show(); }
					}
				}
			} // Ende Problemverarbeitung
			//*/

			// weitere Variante von 1: hasLoad() = true, wenn !isSimple(potentiellAbzugebendesProblem)
			// dann kann der Alg. 1 so bleiben und hat nicht die Nachteile von Alg. 2

			/* PROBLEMVERARBEITUNG: (fertig)
			// Workpool speichert nur komplexe Probleme
			// + einfache Probleme werden stets lokal verarbeitet und nie verschickt
			// + im Rahmen der Lastverteilung werden nur komplexe Probleme abgegeben
			// + weniger Speicherplatzbedarf, da einfache Probleme direkt gelöst und nicht im Workpool abgelegt werden
			// + Probleme werden schneller kombiniert, da simple Probleme direkt gelöst werden und keine combinationIsPossible-Prüfung notwendig ist
			// + minimaler combine-Aufwand (pro 1 divide() max. 1 combine())
			// - hoher Rechenaufwand pro Iteration -> Kommunikationsphase wird weniger oft durchlaufen: Auswirkung auf IDLE-Zeiten?
			if(!finished && (!(*workpool).isEmpty())) {
				problemFrame = workpool->get();
				problem = problemFrame->getData();
				long currentID = problemFrame->getID();
				// Da der Workpool nur komplexe Probleme speichert, ist ein divide() immer möglich
				Problem** subproblem = divide(problem);
				long subproblemID = currentID * D + 1; // ID des ersten Kindknotens
				for(int i = 0; i < D; i++) {
					if(isSimple(subproblem[i])) {
						solution = solve(subproblem[i]);
						solutionFrame = new Frame<Solution>(subproblemID++, solution);
						solutionpool->insert(solutionFrame);
					}
					else {
						Frame<Problem>* subproblemFrame = new Frame<Problem>(subproblemID++, subproblem[i]); // Pair: (id, Problem*)
						workpool->insert(subproblemFrame);
					}
				}
				if(solutionpool->combineIsPossible()) solutionpool->combine();
				delete[] subproblem; // Hinweis in der Dokumentation, dass Speicher intern freigegeben wird!
			} // Ende Problemverarbeitung
			//*/


			// TERMINATION DETECTION (durch Master)
			if(MSL_myId == masterSolver && (!finished)) {
				// fertig, wenn einzige Lösung im Solutionpool die ID "0" trägt
				if(solutionpool->hasSolution()) {
					solution = solutionpool->top()->getData();
					int receiver = getReceiver();
					if(debugCommunication) std::cout << MSL_myId << ": DCSolver sendet Lösung an " << receiver << std::endl;
					MSL_Send(receiver, solution);
					solutionpool->pop();
					solution = NULL;
					numOfSolutionsSent++; // statistics
					blocked = false; // löse Blockade
				}
			}
		} // ende while
		time_solver = MPI_Wtime() - time_solver;


		if(analyse) {
			std::cout << MSL_myId << "start" << std::endl;
			std::cout << MSL_myId << ": processed subproblems: " << numOfProblemsProcessed << std::endl;
			std::cout << MSL_myId << ": simple subproblems: " << numOfSimpleProblemsSolved << std::endl;
			// std::cout << MSL_myId << ": numOfSolutionsSent: " << numOfSolutionsSent << std::endl;
			// std::cout << MSL_myId << ": numOfSolutionsReceived: " << numOfSolutionsReceived << std::endl;
			std::cout << MSL_myId << ": shared subproblems: " << numOfSubproblemsSent << std::endl;
			std::cout << MSL_myId << ": received subproblems: " << numOfSubproblemsReceived << std::endl;
			std::cout << MSL_myId << ": work requests: " << numOfWorkRequestsSent << std::endl;
			//std::cout << MSL_myId << ": numOfWorkRequestsReceived: " << numOfWorkRequestsReceived << std::endl;
			//std::cout << MSL_myId << ": numOfRejectionsSent: " << numOfRejectionsSent << std::endl;
			// std::cout << MSL_myId << ": numOfRejectionsReceived: " << numOfRejectionsReceived << std::endl;
			std::cout << MSL_myId << ": time_solve: " << time_solve << std::endl;
			std::cout << MSL_myId << ": time_combine: " << time_combine << std::endl;
			std::cout << MSL_myId << ": time_divide: " << time_divide << std::endl;
			//std::cout << MSL_myId << ": time_new: " << time_new << std::endl;
			//std::cout << MSL_myId << ": time_workpool: " << time_workpool << std::endl;
			//std::cout << MSL_myId << ": time_solutionpool: " << time_solutionpool << std::endl;
			//std::cout << MSL_myId << ": time_solver: " << time_solver << std::endl;
			//std::cout << MSL_myId << ": numP: " << numP << std::endl;
			//std::cout << MSL_myId << ": numS: " << numS << std::endl;
			//std::cout << MSL_myId << ": numPF: " << numPF << std::endl;
			//std::cout << MSL_myId << ": numSF: " << numSF << std::endl;
			std::cout << MSL_myId << "end" << std::endl;
		}
		if(debugCommunication) std::cout << MSL_myId << ": DCSolver terminiert." << std::endl;
	} // end of start()


	//
	// Berechnet die Ein- und Ausgaenge aller Solverkollegen.
	// @param solver	Zeiger auf das komplette Solverarray (dort ist man selbst auch eingetragen)
	// @param length	Laenge des Solverarrays
	// @param id		Index des Zeigers im Solverarray, der auf diesen Solver (this) zeigt
	//
	void setWorkmates(DCSolver<Problem, Solution>** solver, int length, int id) {
		// indexID = id; // merke, an welcher Position man selbst steht
		numOfSolvers = length;
		entranceOfSolver = new ProcessorNo[numOfSolvers];
		exitOfSolver = new ProcessorNo[numOfSolvers];

		// Ein- und Ausgaenge aller Solver berechnen (inkl. des eigenen Ein- und Ausgangs)
		ProcessorNo* ext;
		ProcessorNo* entr;
		for(int i = 0; i < length; i++) {
			entr = (*(solver[i])).getEntrances();
			entranceOfSolver[i] = entr[0];
			ext = (*(solver[i])).getExits();
			exitOfSolver[i] = ext[0];
			//std::cout << "setWorkmates: entr/exit of solver["<<i < <"]: "<<entranceOfSolver[i]<<"/"<<exitOfSolver[i]<<std::endl;
		}
		// Sender und Empfänger für das Token berechnen
		left = exitOfSolver[(id-1+length)%length];
		right = entranceOfSolver[(id+1)%length];
		//std::cout << "setWorkmates: left/right of solver["<<id<<"]: " << left << "/" << right << std::endl;
	}


    Process* copy() { return new DCSolver<Problem, Solution>(divide, combine, solve, isSimple, D, noprocs);}

   	void show() const {
		if(MSL_myId == 0) std::cout << "           DCSolver (PID = " << entrances[0] << ")" << std::endl << std::flush;
	}

}; // end of class DcSolver




template<class Problem, class Solution>
class DistributedDC: public Process {

	// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
	DCSolver<Problem,Solution>** p;
	int length; // #Solver

    public:

		//
		// Ein dezentrales Divide and Conquer-Skelett.
		//
		// Dieser Konstruktor erzeugt (l-1) Kopien des übergebenen DCSolvers. Insgesamt besteht dieses Skelett aus l Solvern.
		//
		// Externe Schnittstellen:
		// Jeder Solver verfügt über genau einen Eingang und einen Ausgang. Das DistributedDC-Skelett hat genau einen Eingang.
		// Dieser wird auf einen der Solver gemappt. Die Ausgänge der Solver sind die Ausgänge des BranchAndBoundOld-Skeletts.
		//
		// Interne schnittstellen:
		// Jeder Solver kennt die Ein- und Ausgänge seiner Kollegen. Über left und right ist zudem eine logische Ringstruktur definiert,
		// die für das versenden des Tokens im Rahmen des Termination Detection Algorithmus genutzt wird.
		//
    	DistributedDC(DCSolver<Problem,Solution>& solver, int l): length(l), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new DCSolver<Problem,Solution>* [length];
            p[0] = &solver;
            for(int i = 1; i < length; i++)
            	p[i] = (DCSolver<Problem,Solution>*)solver.copy();

			// externe Schnittstellen definieren:
			// Es gibt genau einen Mastersolver, der als Ein- und Ausgang fungiert.
			numOfEntrances = 1;
			entrances = new ProcessorNo[numOfEntrances];
			ProcessorNo* entr = (*(p[0])).getEntrances(); // Mastersolver
			entrances[0] = entr[0];

			numOfExits = 1;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext = (*(p[0])).getExits();
			exits[0] = ext[0];

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
			// Empfaenger der ersten Nachricht festlegen
			//setNextReceiver(0); // DistributedBB verschickt doch gar nichts, sondern die Solver
		}


    	DistributedDC(Problem** (*divide)(Problem*), Solution* (*combine)(Solution**), Solution* (*solve)(Problem*), bool (*isSimple)(Problem*), int d, int l): length(l), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new DCSolver<Problem,Solution>* [length];
            for(int i = 0; i < length; i++)
            	p[i] = new DCSolver<Problem, Solution>(divide, combine, solve, isSimple, d, 1);

			// externe Schnittstellen definieren:
			// Es gibt genau einen Mastersolver, der als Eingang fungiert.
			// Der (einzige) Eingang des Mastersolvers ist (einziger) Eingang des Skeletts, alle Solverausgänge sind Ausgänge des Skeletts.
			numOfEntrances = 1;
			entrances = new ProcessorNo[numOfEntrances];
			ProcessorNo* entr = (*(p[0])).getEntrances(); // Mastersolver
			entrances[0] = entr[0];
			numOfExits = 1;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext = (*(p[0])).getExits();
			exits[0] = ext[0];

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
		}


		~DistributedDC() {
			// std::cout << MSL_myId << ": ~DistributedDC called" << std::endl;
			delete[] p;
			delete[] entrances;
			delete[] exits;
			p = NULL;
			entrances = NULL;
			exits = NULL;
		}


		// von diesen l vorgelagerten Skeletten werden Daten empfangen
		// nur der Mastersolver kommuniziert mit den Sendern
		inline void setPredecessors(ProcessorNo* src, int l) {
			numOfPredecessors = l;
			(*(p[0])).setPredecessors(src,l);
		}

		// an diese l nachgelagerten Skelette werden Daten gesendet.
		// alle Solver können Daten verschicken
		inline void setSuccessors(ProcessorNo* drn, int l) {
			numOfSuccessors = l;
			for(int i = 0; i < length; i++)
				(*(p[i])).setSuccessors(drn,l);
		}

        // startet alle Solver
		inline void start() {
            for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

        Process* copy() { return new DistributedDC<Problem, Solution>(*(DCSolver<Problem,Solution>*)(*(p[0])).copy(), length);}

        inline void show() const {
            if(MSL_myId == 0) {
                std::cout << "DistributedDC (entrance at " << entrances[0] << ") with " << length << " Solver(s) " << std::endl << std::flush;
                for(int i = 0; i < length; i++)
                    (*(p[i])).show();
			}
		}
}; // end of DistributedDC
















//*######################################################################################################################


//*######################################################################################################################


// Die Probleme werden in einer verketteten Liste verwaltet.
template<class Data>
class WorkpoolManager2 {

private:

	// soviele Probleme müssen mindestens im jedem Pool sein, damit Last abgegeben wird
	// ! Wenn Threshold = 1, dann ist es möglich, dass das Originalproblem aufrund einer Arbeitsanfrage unmittelbar an einen anderen
	// Prozessor abgegeben wird und die Terminierung Probleme bereitet.
	static const int THRESHOLD = 3;

	class ListElem {
		friend class WorkpoolManager2<Data>;
		Frame<Data>* data;
		ListElem *pred, *succ;
		ListElem(Frame<Data>* pDat): data(pDat), pred(NULL), succ(NULL) {}
	};

	int numOfPools;		// zeigt an, wie viele DCP gleichzeitig im System vorhanden sein können
	int	primaryPoolID;

	ListElem** head;
	ListElem** tail;
	int* sizeOfPool;

public:

	// erzeugt einen neuen WorkpoolManager2
	WorkpoolManager2(int nop): numOfPools(nop) {
		primaryPoolID = -1;
		sizeOfPool = new int[nop];
		head = new ListElem*[nop];
		tail = new ListElem*[nop];
		// initialisiere die benötigten Workpools
		for(int i = 0; i < numOfPools; i++) {
			sizeOfPool[i] = 0;
			head[i] = new ListElem(NULL);
			tail[i] = new ListElem(NULL);
			head[i]->succ = tail[i];
			tail[i]->pred = head[i];
		}
	}

	// gibt Speicher für die sortierte Liste wieder frei
	~WorkpoolManager2() {
		// std::cout << "~WorkpoolManager2: Speicher freigeben" << std::endl;
		ListElem* current = NULL;
		for(int i = 0; i < numOfPools; i++) {
			// Speicher für alle noch im Pool liegenden Elemente freigeben (sollte nie durchlaufen werden)
			if(sizeOfPool[i] > 0) {
				// pool i ist nicht leer
				current = head[i]->succ;
				while(current!=tail[i]) {
					current = current->succ;
					delete current->pred;
				}
			}
			// Sentinels löschen
			delete head[i];
			delete tail[i];
			head[i] = NULL;
			tail[i] = NULL;
		}
		// Arrays löschen
		delete[] head;
		delete[] tail;
		delete[] sizeOfPool;
		tail = NULL;
		head = NULL;
		sizeOfPool = NULL;
	}

	// zeigt an, ob der Workpoolpool leer ist
	inline bool isEmpty() {
		for(int i = 0; i < numOfPools; i++)
			if(sizeOfPool[i] > 0) return false;
		return true;
	}

	// zeigt an, ob der Workpoolpool voll genug ist, um Last abzugeben
	inline bool hasLoad() {
		for(int i = 0; i < numOfPools; i++)
			if(sizeOfPool[i] >= THRESHOLD) return true;
		return false;
	}


	inline int getSize(int p) { return sizeOfPool[p]; }

	void insert(Frame<Data>* f) {
		int poolID = f->getPoolID();
		ListElem* e = new ListElem(f);
		e->succ = head[poolID]->succ;
		e->pred = head[poolID];
		e->succ->pred = e;
		head[poolID]->succ = e;
		sizeOfPool[poolID]++;
	}

	Frame<Data>* get() {
		if(isEmpty()) {
			std::cout << MSL_myId << ": EMPTY WORKPOOL EXCEPTION (get)" << std::endl;
			throw "empty workpool exception";
		}
		// sonst liegt im Workpool mindestens ein Problem
		else {
			// bestimme einen nicht leeren Pool. Hierbei sind verschiedene Strategien denkbar.
			// 1) Nimm den ersten nicht leeren Pool: Vorteil: Speicherplatzsparend, weil schnell gemeinsam ein
			// bestimmtes Problem gelöst wird
			// 2) bearbeite erst alle fremden Teilprobleme
			// 3) wähle Pool mit den wenigsten/kleinsten Problemen aus.
			ListElem* firstElement = NULL;
			int poolID = 0;

			/*// Strategie 1: Nimm das erste Problem, dass gefunden wird
			for(poolID=0; poolID<numOfPools; poolID++) {
				if(sizeOfPool[poolID] > 0) {
					firstElement = head[poolID]->succ;
					break;
				}
			} //*/

			/// Strategie 2: Nimm erst ein Fremdproblem
			bool probFound = false;
			// Es gilt: primaryPoolID := -1 für alle Nicht-MS. Nicht-MS finden also immer Fremdprobleme
			for(poolID=0; poolID<numOfPools; poolID++) {
				// Wenn ich ein Fremdproblem habe, wird das bevorzugt
				if(poolID != primaryPoolID && sizeOfPool[poolID]>0) {
					firstElement = head[poolID]->succ;
					probFound = true;
					break;
				}
			}
			// wenn keine Fremdprobleme im Pool liegen, nimm ein Problem vom primaryPool
			if(!probFound) {
				firstElement = head[primaryPoolID]->succ;
				poolID = primaryPoolID;
			}
			//*/

			Frame<Data>* pProblem = firstElement->data;
			head[poolID]->succ = firstElement->succ;
			head[poolID]->succ->pred = head[poolID];
			delete firstElement; // Speicher für das ListElem freigeben. Probleme?
			firstElement = NULL;
			sizeOfPool[poolID]--;
			return pProblem;
		}
	}

	Frame<Data>* getLoad() {
		if(!hasLoad()) {
			std::cout << MSL_myId << ": NO LOAD EXCEPTION (getLoad)" << std::endl;
			throw "no load exception";
		}
		else {
			// bestimme einen nicht leeren Pool. Hierbei sind verschiedene Strategien denkbar.
			// 1) Nimm den ersten nicht leeren Pool: Vorteil: Speicherplatzsparend, weil schnell gemeinsam ein
			// bestimmtes Problem gelöst wird
			// 2) wähle Pool mit den wenigsten/kleinsten Problemen aus.
			ListElem* lastElement = NULL;
			int poolID = 0;

			/*// Strategie 1: Gebe das erste Problem ab, dass gefunden wird.
			for(poolID=0; poolID<numOfPools; poolID++) {
				if(sizeOfPool[poolID] >= THRESHOLD) {
					lastElement = tail[poolID]->pred;
					break;
				}
			} //*/

			/*// Strategie 2: Gebe vorzugsweise ein primaryDCP ab
			if(primaryPoolID != -1 && sizeOfPool[primaryPoolID] >= THRESHOLD) {
				lastElement = tail[primaryPoolID]->pred;
				poolID = primaryPoolID;
			}
			else {
				// im Pool muss ein Fremdproblem liegen
				for(poolID=0; poolID<numOfPools; poolID++) {
					if(poolID != primaryPoolID && sizeOfPool[poolID] >= THRESHOLD) {
						lastElement = tail[poolID]->pred;
						break;
					}
				}
			} //*/

			/// Strategie 3: Gebe vorzugsweise ein Fremdproblem ab
			// im Pool muss ein Fremdproblem liegen
			bool probFound = false;
			for(poolID=0; poolID<numOfPools; poolID++) {
				if(poolID!=primaryPoolID && sizeOfPool[poolID] >= THRESHOLD) {
					lastElement = tail[poolID]->pred;
					probFound = true;
					break;
				}
			}
			// wenn keins gefunden, nimm eins vom primaryPool
			if(!probFound) {
				lastElement = tail[primaryPoolID]->pred;
				poolID = primaryPoolID;
			}
			if(lastElement == NULL) std::cout << "ERROR in Workpool2::getLoad(): *lastElement == NULL" << std::endl;
			//*/

			Frame<Data>* pLoad = lastElement->data;
			tail[poolID]->pred = lastElement->pred;
			tail[poolID]->pred->succ = tail[poolID];
			delete lastElement;
			lastElement = NULL;
			sizeOfPool[poolID]--;
			return pLoad;
		}
	}

	inline void setPrimaryPoolID(int ppid) { primaryPoolID = ppid; }

	void show() {
		ListElem* currentElem = NULL;
		for(int poolID=0; poolID<numOfPools; poolID++) {
			if(sizeOfPool[poolID]==0) std::cout << MSL_myId << ": Workpool " << poolID << ":[]" << std::endl;
			else {
				std::cout << MSL_myId << ": Workpool " << poolID << ": [";
				currentElem = head[poolID]->succ;
				while(currentElem->succ != tail[poolID]) {
					std::cout << currentElem->data->getID() << ", ";
					currentElem = currentElem->succ;
				}
				std::cout << currentElem->data->getID() << "]" << std::endl;
			}
		}
	}

}; // WorkpoolManager2





// A solutionpool is a kind of a sorted stack
// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
template<class Solution>
class SolutionpoolManager2 {

private:

	int numOfStacks;				// zeigt an, wie viele Probleme gleichzeitig verwaltet werden müssen -> determiniert #stacks

	// SolutionStack
	Frame<Solution>*** stack;		// array of pointers to array of pointers to Frame-objects (I hate C++ :-)
	int* sizeOfStack;				// number of elements that can be stored in the stack[i]
	int* lastElementOfStack;		// index of the top element stored in the stack[i]

	Frame<Solution>** sendQueue;	// sendqueue stores Solutionframes to send back to a solver
	int sizeOfSendQueue;
	int lastElementOfSendQueue;

//	Solution* (*comb)(Solution**); 	// combine-function
	DFct1<Solution**,Solution*> comb;

	int D;							// number of child nodes within the computation tree


public:


	// erzeugt einen neuen SolutionpoolManager2
	// Atomic(O (* f)(I), int n): Process(), fct(Fct1<I,O,O (*)(I)>(f) ), noprocs(n) {
// 	SolutionpoolManager2(int nos, Solution* (*combineFct)(Solution**), int d): numOfStacks(nos), comb(combineFct), D(d) {
	SolutionpoolManager2(int nos, DFct1<Solution**,Solution*>& combineFct, int d): numOfStacks(nos), comb(combineFct), D(d) {
		// für jedes theoretisch koexistente initiale DC-Problem einen eigenen Solutionstack anlegen
		sizeOfStack = new int[numOfStacks];
		lastElementOfStack = new int[numOfStacks];
		stack = new Frame<Solution>**[numOfStacks];
		for(int i = 0; i < numOfStacks; i++) {
			sizeOfStack[i] = 8;					// default value
			lastElementOfStack[i] = -1;
			stack[i] = new Frame<Solution>*[8];	// default: Array mit 8 Zeigern auf Frames
		}
		sizeOfSendQueue = 8; // default value
		sendQueue = new Frame<Solution>*[sizeOfSendQueue];
		lastElementOfSendQueue = -1;
	}


	// gibt Speicher für die sortierten Listen wieder frei
	~SolutionpoolManager2() {
		for(int i = 0; i < numOfStacks; i++) {
			delete[] stack[i];
			stack[i] = NULL;
		}
		delete[] stack;
		stack = NULL;
		delete[] sizeOfStack;
		sizeOfStack = NULL;
		delete[] lastElementOfStack;
		lastElementOfStack = NULL;
		delete[] sendQueue;
		sendQueue = NULL;
	}

	// *** SENDQUEUE ************************************************************************************
	inline bool sendQueueIsEmpty() { return lastElementOfSendQueue == -1; }

	inline bool sendQueueIsFull() { return lastElementOfSendQueue == sizeOfSendQueue-1; }

	inline void removeElementFromSendQueue() {
		if(sendQueueIsEmpty()) {
			std::cout << MSL_myId << ": EMPTY SENDQUEUE EXCEPTION (remove)" << std::endl;
			throw "empty SENDQUEUE exception";
		}
		else {
			// DRAFT: erstes Element löschen, alles andere umkopieren.
			// TODO verkettete Liste daraus bauen! TODO
			delete sendQueue[0];
			numSF--;
			for(int i = 0; i < lastElementOfSendQueue; i++)
				sendQueue[i] = sendQueue[i+1];
			sendQueue[lastElementOfSendQueue--] = NULL;
		}
	}

	inline Frame<Solution>* readElementFromSendQueue() {
		if(sendQueueIsEmpty()) {
			std::cout << MSL_myId << ": EMPTY SENDQUEUE EXCEPTION (read)" << std::endl;
			throw "empty SENDQUEUE exception";
		}
		else return sendQueue[0];
	}

	void writeElementToSendQueue(Frame<Solution>* pFrame) {
		//std::cout << MSL_myId << ": schreibe Problem " << pFrame->getID() << " in SendQueue" << std::endl;
		// sendQueue is full -> extend sendQueue
		if(sendQueueIsFull()) {
			Frame<Solution>** s = new Frame<Solution>*[sizeOfSendQueue*2];
			for(int i = 0; i < sizeOfSendQueue; i++) s[i] = sendQueue[i]; // copy pointers to new sendqueue
			sizeOfSendQueue *= 2;
			delete[] sendQueue; // free memory
			sendQueue = s;
		}
		sendQueue[++lastElementOfSendQueue] = pFrame;
	}

	void showSendQueue() {
		if(lastElementOfSendQueue==-1) std::cout << MSL_myId << ": SendQueue: []" << std::endl;
		else {
			std::cout << MSL_myId << ": SendQueue: [";
			for(int i = 0; i < lastElementOfSendQueue; i++)
				std::cout << sendQueue[i]->getID() << ", ";
			std::cout << sendQueue[lastElementOfSendQueue]->getID() << "]" << std::endl;
		}
	}
	// **************************************************************************************************


	// zeigt an, ob der Solutionpool i leer/voll ist
	inline bool isEmpty(int i) { return lastElementOfStack[i] == -1; }
	inline bool isFull(int i) { return lastElementOfStack[i] == sizeOfStack[i]-1; }

	inline Frame<Solution>* top(int stackID) {
		if(isEmpty(stackID)) {
			std::cout << MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (top)" << std::endl;
			throw "empty solutionpool exception";
		}
		else return stack[stackID][lastElementOfStack[stackID]];
	}

	inline void pop(int stackID) {
		if(isEmpty(stackID)) {
			std::cout << MSL_myId << ": EMPTY SOLUTIONPOOL EXCEPTION (pop)" << std::endl;
			throw "empty solutionpool exception";
		}
		else {
			delete stack[stackID][lastElementOfStack[stackID]]->getData();
			numS--;
			delete stack[stackID][lastElementOfStack[stackID]];
			numSF--;
			stack[stackID][lastElementOfStack[stackID]] = NULL;
			lastElementOfStack[stackID]--;
		}
	}

	// Legt ein Frame auf den Stack. Der Stack wird entsprechend der Frame-IDs sortiert gehalten.
	// Wenn ein Problem in N Subprobleme aufgeteilt wird, können ggf. nach dem Versinken die oberen N Probleme auf dem Stack
	// zu einer neuen Lösung kombiniert werden.
	void insert(Frame<Solution>* pFrame) {
		// bestimme den Stack, in den der Frame geschrieben werden muss
		int stackID = pFrame->getPoolID();

		// stack is full -> extend stack
		if(isFull(stackID)) {
			// std::cout << "extending solutionpool" << std::endl;
			Frame<Solution>** s = new Frame<Solution>*[sizeOfStack[stackID]*2];
			for(int i = 0; i < sizeOfStack[stackID]; i++) s[i] = stack[stackID][i]; // copy pointers to new stack
			sizeOfStack[stackID] *= 2;
			delete[] stack[stackID]; // free memory
			stack[stackID] = s;
		}
		int index = lastElementOfStack[stackID]++; // can be -1 !
		stack[stackID][lastElementOfStack[stackID]] = pFrame;
		// sink
		while(index >= 0 && stack[stackID][index]->getID() > stack[stackID][index+1]->getID()) {
			Frame<Solution>* pHelp = stack[stackID][index+1];
			stack[stackID][index+1] = stack[stackID][index];
			stack[stackID][index] = pHelp;
			index--;
		}
		// std::cout << "Frame sinkt bis Indexposition " << ++index << std::endl;

		// TODO: Hier muss ggf. kombinert werden!

	}

	inline bool isLeftSon(int id) { return (id%D) == 1; }
	inline bool isRightSon(int id) { return (id%D) == 0; }


	// deepCombine scannt alle Stacks nach kombinierbaren Teillösungen, die durch empfangene Teillösungen tief im Stack vorliegen können.
	// Sind solche Lösungen vorhanden, wird pro Aufruf genau ein combine durchgeführt.
	// Ein erfolgreiches Kombinieren wird durch den Rückgabewert true angezeigt.
	bool deepCombine() {
		bool combined = false;
		for(int i = 0; i < numOfStacks; i++) {
			if(!isEmpty(i)) {
				int currentID;
				int end = lastElementOfStack[i]-D+1;
				// Durchlaufe den Stack von tiefsten bis zum obersten Element
				for(int currentFrameIndex=0; currentFrameIndex<=end; currentFrameIndex++) {
					currentID = stack[i][currentFrameIndex]->getID();
					// suche einen linken Sohn
					if(isLeftSon(currentID)) {
						// sind seine Brüder auch da? Wenn ja, dann kann kombiniert werden!
						if(stack[i][currentFrameIndex+D-1]->getID() == currentID+D-1) {
							// kombiniere Elemente i bis i+(D-1)
							Solution** partialSolution = new Solution*[D];
							// alle Bruderknoten aus den Frames entpacken
							for(int j = 0; j < D; j++) partialSolution[j] = stack[i][currentFrameIndex+j]->getData();
							// fasse Teillösungen zusammen, berechne neue ID und baue daraus einen Frame
							Solution* solution = comb(partialSolution); // Muscle-Function call
							numS++;
							// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
							long parentID = (stack[i][currentFrameIndex]->getID()-1)/D;
							long rootNodeID = stack[i][currentFrameIndex]->getRootNodeID();
							long originator = stack[i][currentFrameIndex]->getOriginator();
							long poolID = stack[i][currentFrameIndex]->getPoolID();
							Frame<Solution>* solutionFrame = new Frame<Solution>(parentID, rootNodeID, originator, poolID, solution);
							numSF++;
							// Speicher freigeben
							delete partialSolution; // TODO Damit können keine Teillösungen aus partialSolution für die TODO
							partialSolution = NULL; // TODO kombinierte Teillösung wiederverwendet werden! Ggf. ändern! TODO
							for(int j = 0; j < D; j++) {
								delete stack[i][currentFrameIndex+j]->getData();
								numS--;
								delete stack[i][currentFrameIndex+j];
								numSF--;
							}
							// umkopieren: Mitten im Stack ist ein Loch entstanden. Alle elemente fallen herunter
							while(currentFrameIndex+D <= lastElementOfStack[i]) {
								stack[i][currentFrameIndex] = stack[i][currentFrameIndex+D];
								currentFrameIndex++;
							}
							while(currentFrameIndex <= lastElementOfStack[i]) stack[i][currentFrameIndex++] = NULL;
							lastElementOfStack[i] -= D;
							numSF++;
							if(rootNodeID==parentID) {
								// Lösung in SendQueue schreiben
								writeElementToSendQueue(solutionFrame);
								//insert(solutionFrame);
								solutionFrame = NULL;
							}
							// schreibe neu berechneten Frame in den Pool
							else { insert(solutionFrame); }
							combined = true; // an dieser Stelle ist (currentFrameIndex == end) und die for-schleife bricht ab
						} // if
					} // if
				} // for
			} // if
			if(combined) break;
		} // for
		return combined;
	}

	// kombiniert, sofern möglich, alle top D Elemente auf den Stacks, die kombiniert werden können.
	void combine() {
		// bestimme den Stack, der kombinierbare Lösungen enthält (ist bei Aufruf (noch) gegeben)
		int stackID = -1;
		for(int pool=0; pool<numOfStacks; pool++) {
			// Wenn ein rechter Sohn auf dem betrachteten Stack liegt
			if(!isEmpty(pool) && stack[pool][lastElementOfStack[pool]]->getID()%D==0) {
				// und alle Bruderknoten ebenfalls vorhanden sind
				int leftSonIndex = lastElementOfStack[pool]-D+1;
				if((leftSonIndex >= 0) && (stack[pool][leftSonIndex]->getID()+D-1 == stack[pool][lastElementOfStack[pool]]->getID())) {
					// beende die Suche
					stackID = pool;
					break;
				}
			}
		}
		// wenn es nichts zu kombinieren gibt, ist man hier schon fertig
		if(stackID == -1) return;
		// ansonsten verweist stackID auf den Stack, der kombinierbare Teillösungen enthält
		else {
			// Array mit Teillösungen vorbereiten
			Solution** partialSolution = new Solution*[D];
			int currentFrameIndex = lastElementOfStack[stackID] - D + 1;
			// alle Bruderknoten aus den Frames entpacken und ins Array schreiben
			for(int i = 0; i < D; i++) partialSolution[i] = stack[stackID][currentFrameIndex++]->getData();
			// kombiniere Teillösungen, berechne neue ID und baue daraus einen Frame
			Solution* solution = comb(partialSolution); // Muscle-Function call
			numS++;
			delete partialSolution; // TODO Damit können keine Teillösungen aus partialSolution für die TODO
			partialSolution = NULL; // TODO kombinierte Teillösung wiederverwendet werden! Ggf. ändern! TODO
			// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
			long parentID = (stack[stackID][lastElementOfStack[stackID]-D+1]->getID()-1)/D;
			long rootNodeID = stack[stackID][lastElementOfStack[stackID]-D+1]->getRootNodeID();
			long originator = stack[stackID][lastElementOfStack[stackID]-D+1]->getOriginator();
			long poolID = stack[stackID][lastElementOfStack[stackID]-D+1]->getPoolID();
			Frame<Solution>* solutionFrame = new Frame<Solution>(parentID, rootNodeID, originator, poolID, solution);
			numSF++;
			// entferne Teillösungen aus dem Solutionpool, die soeben kombiniert wurden
			for(int i = 0; i < D; i++)	pop(stackID); // Speicher wird freigegeben
			if(rootNodeID==parentID) {
				// Lösung in SendQueue schreiben
				writeElementToSendQueue(solutionFrame);
				//insert(solutionFrame);
				solutionFrame = NULL;
			}
			// schreibe neu erzeugten Frame in den Pool
			else { insert(solutionFrame); }
			// wiederhole dies rekursiv, bis nichts mehr zusammengefasst werden kann
			combine();
		}
	}


	// KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
	inline bool hasSolution(int stackID) {
		if(!isEmpty(stackID)) return (stack[stackID][lastElementOfStack[stackID]]->getID() == 0);
		else return false;
	}


	void show() {
		for(int stackID=0; stackID<numOfStacks; stackID++) {
			if(lastElementOfStack[stackID]==-1) std::cout << MSL_myId << ": Solutionpool " << stackID << ": []" << std::endl;
			else {
				std::cout << MSL_myId << ": Solutionpool " << stackID << ": [";
				for(int i = 0; i < lastElementOfStack[stackID]; i++)
					std::cout << stack[stackID][i]->getID() << ", ";
				std::cout << stack[stackID][lastElementOfStack[stackID]]->getID() << "]" << std::endl;
			}
		}
	}
}; // SolutionpoolManager2













// Dezentrales Divide and Conquer Skelett, angelehnt an BranchAndBoundOld
template<class Problem, class Solution>
class DCStreamSolver: public Process {

private:

	bool isMasterSolver;
	bool blocked;							// state of Mastersolver
	bool solutionFound;						// indicates if incumbent found
	ProcessorNo* entranceOfSolver;			// Eingänge aller Solver (inkl. this): entranceOfSolver[0] ist Master
	ProcessorNo* exitOfSolver; 				// Ausgänge aller Solver (inkl. this): exitOfSolver[0] ist Master
	ProcessorNo left;						// Ringstruktur: Solver empfängt Token von left
	ProcessorNo right;						// Ringstruktur: Solver sendet Token an right
	int numOfSolvers;						// Anzahl der Solver, die durchs BnB-Skelett gruppiert werden
	int numOfMasterSolvers;
	// int indexID; 							// an dieser Position findet man sich selbst in den Arrays
    int noprocs; 							// assumption: uses contiguous block of processors
    int D;

//	Problem** (*divide)(Problem*);			// Zeiger auf bound-Methode. Schätzt Subproblem ab.
//	Solution* (*combine)(Solution**);		// Zeiger auf branch-Methode. Liefert Zeiger auf Array mit Zeigern auf erzeugte Subprobleme
//	Solution* (*solve)(Problem*);			// Zeiger auf benutzerdefinierte Funktion
//	bool (*isSimple)(Problem*);				// Zeiger auf benutzerdefinierte Funktion

    DFct1<Problem*,Problem**> divide;			// divide(Fct1<Problem*, Problem**, Problem** (*)(Problem*)>(div)),
    DFct1<Solution**,Solution*> combine;		// combine(Fct1<Solution**, Solution*, Solution* (*)(Solution**)>(comb)),
    DFct1<Problem*,Solution*> solve;			// solve(Fct1<Problem*, Solution*, Solution* (*)(Problem*)>(solv)),
    DFct1<Problem*,bool> isSimple;				// isSimple(Fct1<Problem*, bool, bool (*)(Problem*)>(smpl)),

	WorkpoolManager<Problem>* workpool;
    SolutionpoolManager2<Solution>* solutionpool;

public:

//	DCStreamSolver(Problem** (*div)(Problem*), Solution* (*comb)(Solution**), Solution* (*solv)(Problem*), bool (*smpl)(Problem*), int d, int noms, int n)
//			: D(d), numOfMasterSolvers(noms), noprocs(n), isSimple(smpl), solve(solv), divide(div), combine(comb), Process() {

	DCStreamSolver(Problem** (*div)(Problem*), Solution* (*comb)(Solution**), Solution* (*solv)(Problem*), bool (*smpl)(Problem*), int d, int noms, int n)
			: divide(Fct1<Problem*, Problem**, Problem** (*)(Problem*)>(div)),
			  combine(Fct1<Solution**, Solution*, Solution* (*)(Solution**)>(comb)),
			  solve(Fct1<Problem*, Solution*, Solution* (*)(Problem*)>(solv)),
			  isSimple(Fct1<Problem*, bool, bool (*)(Problem*)>(smpl)),
			  D(d), numOfMasterSolvers(noms), noprocs(n), Process() {
		// Workpool und Solutionpool erzeugen
		// workpool = new WorkpoolManager2<Problem>(numOfMasterSolvers);
		workpool = new WorkpoolManager<Problem>();
		solutionpool = new SolutionpoolManager2<Solution>(numOfMasterSolvers,combine,d);
      	isMasterSolver = false; // ob dieser DCStreamSolver als Mastersolver eingesetzt wird, entscheidet das StreamDC-Skelett
		// DCStreamSolver arbeiten NICHT datenparallel
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setNextReceiver(0); 			// Default-Receiver ist 0. Den gibt's immer
		srand(time(NULL)); 				// zufällig gestellte Workrequests sicherstellen
	}

    DCStreamSolver(DFct1<Problem*,Problem**>& div, DFct1<Solution**,Solution*>& comb, DFct1<Problem*,Solution*>& solv, DFct1<Problem*,bool>& smpl, int d, int noms, int n)
			: divide(div), combine(comb), solve(solv), isSimple(smpl), D(d), numOfMasterSolvers(noms), noprocs(n), Process() {
		// Workpool und Solutionpool erzeugen
		// workpool = new WorkpoolManager2<Problem>(numOfMasterSolvers);
		workpool = new WorkpoolManager<Problem>();
		solutionpool = new SolutionpoolManager2<Solution>(numOfMasterSolvers,comb,d);
      	isMasterSolver = false; // ob dieser DCStreamSolver als Mastersolver eingesetzt wird, entscheidet das StreamDC-Skelett
		// DCStreamSolver arbeiten NICHT datenparallel
		numOfEntrances = 1;
		numOfExits = 1;
		entrances = new ProcessorNo[numOfEntrances];
		exits = new ProcessorNo[numOfExits];
		entrances[0] = MSL_runningProcessorNo;
		exits[0] = entrances[0];
		MSL_runningProcessorNo += n;
		setNextReceiver(0); 			// Default-Receiver ist 0. Den gibt's immer
		srand(time(NULL)); 				// zufällig gestellte Workrequests sicherstellen
	}

	// Speicher freigeben
	~DCStreamSolver() {
		// std::cout << MSL_myId << ": ~DCStreamSolver called" << std::endl;
		delete[] entrances; 	 entrances = NULL;
		delete[] exits; 		 exits = NULL;
		delete[] workpool; 		 workpool = NULL;
		delete[] solutionpool; 	 solutionpool = NULL;
		delete entranceOfSolver; entranceOfSolver = NULL;
		delete exitOfSolver;	 exitOfSolver = NULL;
	}


	void start() {
		// am Prozess beteiligte Prozessoren merken sich entrance und exit des Solvers
		finished = ((MSL_myId < entrances[0]) || (MSL_myId >= entrances[0] + noprocs));
		if(finished) return;
		// ******************

		bool deepCombineEnabled = true;
		//   flatCombineEnabled = false;
		bool analyse = false;

		// ****************************************************************************************
		// DEBUG

		bool debugCommunication = false;
		bool debugLoadBalancing = false;
		bool debugComputation = false;
		bool debugFreeMem = false;
		bool logIDs = false;

		// ****************************************************************************************

		// für die Statistik
		std::vector<unsigned long> v;
		int numOfProblemsProcessed = 0;
		int numOfSolutionsSent = 0;
		int numOfSolutionsReceived = 0;
		int numOfSubproblemsSent = 0;
		int numOfSubproblemsReceived = 0;
		int numOfWorkRequestsSent = 0;
		int numOfWorkRequestsReceived = 0;
		int numOfRejectionsSent = 0;
		int numOfRejectionsReceived = 0;
		int numOfSimpleProblemsSolved = 0;
		double time_solve = 0;
		double time_divide = 0;
		double time_combine = 0;
		double time_start = 0;
		double time_new = 0;
		double time_workpool = 0;
		double time_solutionpool = 0;
		double time_solver = 0;


		// define internal tolology
		int ALL_TO_ALL = 1;
		int topology = ALL_TO_ALL;	// die Topologie ist z.Zt. hardcodiert

		// alle anderen merken sich die Anzahl der am Skelett beteiligten Prozessoren und den masterSolver-Eingang
		MSL_numOfLocalProcs = noprocs;
/*!!!*/ ProcessorNo masterSolver = entranceOfSolver[0];
		MSL_myEntrance = entrances[0]; 	// Eingang des DCStreamSolvers merken
		MSL_myExit = exits[0];			// Ausgang des DCStreamSolvers merken


		Problem* problem = NULL;
		Solution* solution = NULL;
		Frame<Problem>* problemFrame = NULL;
		Frame<Problem>* subproblemFrame = NULL;
		Frame<Solution>* solutionFrame = NULL;



		MPI_Status status;
		int receivedStops = 0;						// Anzahl bereits eingegangener Stop-Tags
		int internallyReceivedStops = 0;
		int receivedTT = 0;							// Anzahl bereits eingegangener TerminationTest-Tags
		int dummy;									// (*) speichert TAGs
		bool workRequestSent = false;
		bool sendRequestSent = false;
		bool deepCombineIsNecessary = false;
		int flag = 0;
		long originator = -1;
		int receiverOfWorkRequest;				// nicht überschreiben, sonst geht die Lastverteilung kaputt!!!
		int predecessorIndex = 0;
		int primaryPoolID = MSL_myId - entranceOfSolver[0];
		// if(isMasterSolver) workpool->setPrimaryPoolID(primaryPoolID);

		void* p1; // wird benötigt, um zu erkennen, ob der Benutzer Speicherplatz wiederverwendet hat
		void* p2; // wird benötigt, um zu erkennen, ob der Benutzer Speicherplatz wiederverwendet hat


		// solange Arbeit noch nicht beendet ist
		blocked = false;
		while(!finished) {


			// PROBLEMANNAHME
			// Nur der Master-Solver kann im Zustand FREE externe Nachrichten (Probleme und TAGs) annehmen
			// Der Mastersolver darf nur dann neue Nachrichten entgegen nehmen, wenn sichergestellt ist, dass keine Probleme im Pool
			// vergraben werden!

			// if(debugCommunication) { if(isMasterSolver && !blocked && !workpool->isEmpty()) std::cout << "WARTE MIT PROBLEMANNAHME" << std::endl; }
			if(isMasterSolver && !blocked && workpool->isEmpty() && !workRequestSent ) {

				// schaue nach, ob irgendein Vorgänger eine Nachricht senden will oder gesendet hat
				// TODO: Klären: Klappt das bei großen Nachrichten, die erst noch per Handshake übertragen werden müssen?
				flag = 0;
				for(int i = 0; i < numOfPredecessors; i++) {
					MPI_Iprobe(predecessors[i], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
				}
				// wenn eingehende Nachricht vorhanden
				if(flag) {
					// Ermittle Sender und nimm Nachricht entgegen
					if(debugCommunication) std::cout << MSL_myId << ": Mastersolver empfängt Daten von " << status.MPI_SOURCE << " mit Tag = " << status.MPI_TAG << std::endl;
					// TAGS verarbeiten (optimierbar, wenn man Reihenfolge ändert: 1.) Problem, 2.) TAG)
					ProcessorNo source = status.MPI_SOURCE;
					if(status.MPI_TAG == MSLT_TERMINATION_TEST) {
						// Nimm TT-Nachricht entgegen
						MSL_ReceiveTag(source, MSLT_TERMINATION_TEST);
					}

					// STOPS vom Vorgänger
					else if(status.MPI_TAG == MSLT_STOP) {
						// Nimm STOP-Nachricht entgegen
						MSL_ReceiveTag(source, MSLT_STOP);
						if(debugCommunication) std::cout << MSL_myId << ":(DCStreamSolver::start) Mastersolver hat STOP empfangen" << std::endl;
						receivedStops++; // Tags sammeln
						// wenn genug STOPs gesammelt
						if(receivedStops==numOfPredecessors) {
							// An dieser Stelle weiß der MasterSolver, dass keine weiteren DCP von den Vorgängern zu erwarten sind, da sich diese
							// bereits abgeschaltet haben. Der MasterSolver kann bereits jetzt seinen Nachfolgern ein STOP zusenden, da er sein
							// letztes DCP bereits gelöst und versendet hat. Weitere Nachrichten nach außen sind von ihm also nicht zu erwarten.
							// Der Mastersolver darf sich an dieser Stelle jedoch noch nicht abschalten, da er ggf. seine Kollegen beim Lösen von
							// weiteren DCP helfen muss, die sich noch im System befinden können. Daher gibt er seinen MasterSolver-Status auf
							// und setzt isMasterSolver auf FALSE. Er arbeitet nun als ganz normaler Worker weiter. Über ein intern verschicktes
							// STOP werden alle Kollegen hierüber informiert.
							if(debugCommunication) std::cout << MSL_myId << ":(DCStreamSolver::start) Mastersolver hat #numOfPredecessors STOPS empfangen" << std::endl;
							// alle Solver (außer sich selbst) über Statuswechsel benachrichtigen
							if(numOfSolvers > 1) {
								for(int i = 0; i < numOfSolvers; i++) {
									if(entranceOfSolver[i]!=MSL_myId) {
										if(debugCommunication) std::cout << MSL_myId << ":(DCStreamSolver::start) Mastersolver sendet STOP intern an "<<entranceOfSolver[i]<<std::endl;
										MSL_SendTag(entranceOfSolver[i], MSLT_STOP);
									}
								}
							}
							// anstatt sich selbst eine Nachricht zu senden (MPI-Absturz!), wird der Counter direkt erhöht
							internallyReceivedStops++;
							// alle Nachfolger benachrichtigen
							for(int i = 0; i < numOfSuccessors; i++) {
								if(debugCommunication) std::cout << MSL_myId << ":(DCStreamSolver::start) Mastersolver schickt STOP an Nachfolger "<<successors[i]<<std::endl;
								MSL_SendTag(successors[i], MSLT_STOP);
							}
							receivedStops = 0;
							blocked = true; // Abbruch der inneren While-Schleife, damit nicht wieder auf eine Nachricht vom Vorgänger gewartet wird
							isMasterSolver = false; // Aufgabe des MS-Status
							if(debugCommunication) std::cout << MSL_myId << ":(DCStreamSolver::start) MasterSolver " << MSL_myId << " gibt seinen MS-Status auf und wechselt in Worker-Modus."<< std::endl;
						}
					}

					// wenn's kein TAG war, war's ein neues DC-Problem
					else {
						if(debugCommunication) std::cout << MSL_myId << ": Mastersolver bereitet Empfang eines DC-Problems von " << source << " mit Tag = " << MSLT_MYTAG << " vor" << std::endl;
						problem = new Problem();
						numP++;
						MSL_Receive(source, problem, MSLT_MYTAG, &status);
						numOfSubproblemsReceived++; // statistics
						if(debugCommunication) std::cout << MSL_myId << ":(DCStreamSolver::start) Mastersolver empfängt DC-Problem" << std::endl;

						// DATAPARALLEL PART: sende Problem an alle solverinternen Prozessoren
						// for(i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)	send(i, &problem, sizeof(Problem));

						// wenn das Problem einfach genug ist, löse es direkt, und verschicke das Ergebnis an den Nachfolger
						if(isSimple(problem)) {
							if(debugComputation) std::cout << MSL_myId << ":(DCStreamSolver::start) Problem wird direkt gelöst"<< std::endl;
							solution = solve(problem);
							numS++;
							// Prüfe, ob der Nutzer die internen Datenstrukturen wiederverwendet hat
							p1 = (void*)problem;
							p2 = (void*)solution;
							if(p1 != p2) {
								delete problem;
								problem = NULL;
								numP--;
							}
							if(debugComputation) std::cout << MSL_myId << ":(DCStreamSolver::start) Problem gelöst"<< std::endl;
							MSL_Send(getReceiver(), solution);
							if(debugCommunication) std::cout << MSL_myId << ":(DCStreamSolver::start) Lösung verschickt"<< std::endl;
							delete solution;
							solution = NULL;
							numS--;
							numOfSolutionsSent++; // statistics
						}
						// sonst initiiere die parallele Lösung des Problems
						else {
							// Blockiere Solver bis Problem gelöst ist
							blocked = true; // Abbruch der inneren while-Schleife
							if(debugComputation) std::cout << MSL_myId << ":(DCStreamSolver::start) Problem ist zu komplex und wird parallel gelöst"<< std::endl;
							// Root-Initialization: 	 KindID = VaterID*D+1		VaterID = (KindID-1)/D		WurzelID = 0
							time_start = MPI_Wtime();
							problemFrame = new Frame<Problem>(0, -1, -1, primaryPoolID, problem); // (id, rootNodeID, originator, poolID, Problem*)
							if(debugComputation) std::cout << MSL_myId << ": erzeuge Problem (0,-1,-1," << primaryPoolID << ")" << std::endl;
							time_new += MPI_Wtime() - time_start;
							numPF++;
							// Frame in den Workpool schreiben
							time_start = MPI_Wtime();
							// PRECONDITION: PIDs werden aufsteigend vergeben; Mastersolver haben aufeinander folgende PIDs; keine Datenparallelität
							// nur dann bildet (MSL_myId - entrance[0]) auf einen Pool eines Mastersolvers ab.
							workpool->insert(problemFrame);
							time_workpool += MPI_Wtime() - time_start;
							if(debugComputation) { workpool->show(); solutionpool->show(); }
						}
					}
				} // if(flag)
				else {
					// keine eingehende Nachricht liegt vor
				}

				// hier angekommen, wurde entweder ein komplexes DC-Problem empfangen (Workpool nicht leer) oder es ist Feierabend
			} // ende Problemannahme


			// INTERNER NACHRICHTENVERKEHR zwischen den Solvern, falls es mehrere gibt:
			// Kommunikation und Problemverarbeitung muss verschachtelt werden, damit eine schnelle Reaktion auf Arbeitsanfragen
			// garantiert werden kann.
			if((numOfSolvers>1) && (!finished) && (MSL_myId==MSL_myEntrance)) {

				// 1. verarbeite alle empfangenen Teillösungen
				// Ankommende, gelöste Teilprobleme werden einfach in den SolutionPool geschrieben und dort genau so weiterverarbeitet, als wäre
				// die Lösung lokal berechnet worden. Wenn sich im Rahmen des Combine eine neue VaterID ergibt, zu der es in der RootList
				// ein Paar (VaterID, Sender) gibt, so ist das Teilproblem gelöst und kann an den Sender zurückgeschickt werden.
				// Das Paar (VaterID, Sender) wird aus der RootList entfernt.
				for(int i = 0; i < numOfSolvers; i++) {
					// alle Solver mit kleinerer ProzessID dürfen immer senden
					MPI_Iprobe(exitOfSolver[i], MSLT_SOLUTION, MPI_COMM_WORLD, &flag, &status);
					if(flag) {
						solutionFrame = new Frame<Solution>();
						numSF++;
						MSL_Receive(exitOfSolver[i], solutionFrame, MSLT_SOLUTION, &status);
						numOfSolutionsReceived++; // statistics
						//if(debugCommunication) std::cout << MSL_myId<< ": empfange Teillösung (" << solutionFrame->getID()<<","<<solutionFrame->getRootNodeID()<<","<<solutionFrame->getOriginator()<<")" << std::endl;
						if(debugLoadBalancing) { workpool->show(); solutionpool->show(); std::cout << MSL_myId<< ": empfange Teillösung (" << solutionFrame->getID() <<"," <<  solutionFrame->getRootNodeID() << "," << solutionFrame->getOriginator() << "," << solutionFrame->getPoolID() << ")" << std::endl; }
						solutionpool->insert(solutionFrame);
						time_start = MPI_Wtime();
						solutionpool->combine();
						deepCombineIsNecessary = true;	// deepCombine anstoßen
						time_combine += MPI_Wtime() - time_start;
						if(debugLoadBalancing) { solutionpool->show(); }
					}
					// Solver mit größerer PID schicken Sendeanfragen. Bei Eingang einer Sendeanfrage geht der Solver direkt auf Empfang,
					// nimmt die Nachricht unmittelbar entgegen und schreibt sie in den Solutionpool
					MPI_Iprobe(exitOfSolver[i], MSLT_SENDREQUEST, MPI_COMM_WORLD, &flag, &status);
					if(flag) {
						if(debugLoadBalancing) std::cout << MSL_myId << ": SENDREQUEST von " << exitOfSolver[i] << " eingegangen. Sende READYSIGNAL und gehe auf Empfang" << std::endl;
						MSL_ReceiveTag(exitOfSolver[i], MSLT_SENDREQUEST);
						MSL_SendTag(entranceOfSolver[i], MSLT_READYSIGNAL);
						solutionFrame = new Frame<Solution>();
						numSF++;
						MSL_Receive(exitOfSolver[i], solutionFrame, MSLT_SOLUTION, &status);
						numOfSolutionsReceived++; // statistics
						if(debugLoadBalancing) { workpool->show(); solutionpool->show(); std::cout << MSL_myId<< ": empfange Teillösung (" << solutionFrame->getID() <<"," <<  solutionFrame->getRootNodeID() << "," << solutionFrame->getOriginator() << "," << solutionFrame->getPoolID() << ")" << std::endl; }
						solutionpool->insert(solutionFrame);
						time_start = MPI_Wtime();
						solutionpool->combine();
						time_combine += MPI_Wtime() - time_start;
						if(debugLoadBalancing) { solutionpool->show(); }
					}
				}
				// 2. Nach dem Empfang einer Teillösung liegen ggf. kombinierbare Teillösungen tief im Stack.
				// Solange versunkene kombinierbare Teillösungen vorhanden sind, wird ein deepCombine durchgeführt.
				// Der Methodenaufruf wird umgangen, wenn keine kombinierbaren Teilprobleme im Stack liegen.
				if(deepCombineEnabled && deepCombineIsNecessary) deepCombineIsNecessary = solutionpool->deepCombine();

				// 3. kontrolliertes Verschicken von Nachrichten in der SendQueue
				// nur eine Verschicken, um Deadlocks vorzubeugen???
				// Idee: Das Verschicken einer Lösung wird übersprungen, solange noch potentiell große Teilprobleme oder Lösungen vom Empänger
				// entgegen genommen werden müssen. Auf diese Weise werden Deadlocks hoffentlich vermieden.
				// Eine Lösung darf ich nur verschicken, wenn ein ggf. aktiver Lastverteilungsprozess abgeschlossen ist
				if(!solutionpool->sendQueueIsEmpty() && !workRequestSent) { //***
					// if(debugLoadBalancing) solutionpool->showSendQueue();
					solutionFrame = solutionpool->readElementFromSendQueue();
					originator = solutionFrame->getOriginator();
					// Senden ist erlaubt, wenn SenderID < EmpfängerID und vorher kein WorkRequest verschickt wurde.
					// Dies könnte sonst zu einem zyklischen Deadlock führen:
					// Situation: A: WorkReq an C, danach Lösung an B; B schickt Lösung an C; C antwortet A mit Problem.
					// -> Deadlock, wenn alle Nachrichten Handshake erfordern! A wartet auf B, B wartet auf C und C wartet auf A
					if(MSL_myId < originator) { // ***
						// direktes Verschicken von Lösungen ist verboten, wenn vorher ein Workrequest an den Empfänger gesendet wurde,
						// denn dann kann ggf. als Antwort darauf ein Teilproblem unterwegs sein, das ebenfalls ein Handshake erfordert.
						// Das Ergebnis wäre ein Deadlock, da Sender und Empfänger gegenseitig auf die Abnahme der Nachricht warten.
						//if(!(workRequestSent && originator==receiverOfWorkRequest)) { // zyklischer Deadlock möglich?
							if(debugLoadBalancing) { std::cout << MSL_myId << ": sende Teillösung (" << solutionFrame->getID() <<"," <<  solutionFrame->getRootNodeID() << "," << solutionFrame->getOriginator() << "," << solutionFrame->getPoolID() << ") direkt an " << originator << std::endl; }
							MSL_Send(originator, solutionFrame, MSLT_SOLUTION); // kann blockieren!
							solutionpool->removeElementFromSendQueue();
							solutionFrame = NULL;
						//}
					}
					// sonst muss zunächst ein SendRequest an den Empfänger der Nachricht geschickt werden
					else if(!sendRequestSent) {
						if(debugLoadBalancing) {
							solutionpool->showSendQueue();
							std::cout << MSL_myId << ": versuche Frame aus Sendqueue zu senden: (" << solutionFrame->getID() << "," << solutionFrame->getRootNodeID() << "," << originator << "," << solutionFrame->getPoolID() << ")" << std::endl;
							std::cout << MSL_myId << ": sende SENDREQUEST an " << originator << std::endl;
							}
						MSL_SendTag(originator, MSLT_SENDREQUEST);
						sendRequestSent = true;
					}
					// ist das bereits geschehen, darf die Nachricht erst nach Eingang des ReadySignals verschickt werden
					else {
						// warte auf readySignal
						MPI_Iprobe(originator, MSLT_READYSIGNAL, MPI_COMM_WORLD, &flag, &status);
						if(flag) {
							// Empfänger wartet auf den Eingang des SolutionFrames
							if(debugLoadBalancing) { std::cout << MSL_myId << ": READYSIGNAL eingegangen" << std::endl; }
							MSL_ReceiveTag(originator, MSLT_READYSIGNAL);
							if(debugLoadBalancing) { std::cout << MSL_myId << ": sende Teillösung (" << solutionFrame->getID() <<"," <<  solutionFrame->getRootNodeID() << "," << solutionFrame->getOriginator() << "," << solutionFrame->getPoolID() << ") an " << originator << std::endl; }
							MSL_Send(originator, solutionFrame, MSLT_SOLUTION);
							sendRequestSent = false; // Antwort erhalten
							solutionpool->removeElementFromSendQueue();
							solutionFrame = NULL;
							if(debugLoadBalancing) { solutionpool->showSendQueue(); }
						}
					}
				}



				//* 4. eingehende WorkRequests verarbeiten
				// eingehende Workrequests von P werden erst dann verarbeitet, wenn die Übertragung von Teillösungen an P abgeschlossen ist.
				// Andernfalls erwartet P nach einem Readysignal eine Solution, aufgrund des Workrequests wird aber ein Subproblem geschickt.
				// Dies würde zum Deadlock führen, wenn beide Nachrichten per Handshake verschickt werden müssen.
				originator = -1;
				if(!solutionpool->sendQueueIsEmpty())
					originator = solutionpool->readElementFromSendQueue()->getOriginator();
				for(int i = 0; i < numOfSolvers; i++) {
					MPI_Iprobe(exitOfSolver[i], MSLT_WORKREQUEST, MPI_COMM_WORLD, &flag, &status);
//					if(flag && !(sendRequestSent && exitOfSolver[i]==originator)) { // zyklischer Deadlock möglich?
					// workrequests erst verarbeiten, wenn Senden aus Solutionpool abgeschlossen ist
					if(flag && !sendRequestSent) {
						// receive message
						MSL_ReceiveTag(exitOfSolver[i], MSLT_WORKREQUEST);
						numOfWorkRequestsReceived++; // statistics
						if(!workpool->hasLoad()) { // inform sender about an empty workpool
							//if(debugLoadBalancing) std::cout << MSL_myId << " kann keine Arbeit abgeben! " << std::endl;
							MSL_SendTag(entranceOfSolver[i], MSLT_REJECTION);
							if(debugLoadBalancing) std::cout << MSL_myId<< ": REJECTION an " << entranceOfSolver[i] << std::endl;
							numOfRejectionsSent++; // statistics
						}
						else { // distribute load (min 2 subproblems are stored in the workpool)
							if(debugCommunication) { workpool->show(); solutionpool->show(); }
							problemFrame = workpool->getLoad(); // get the lastElement stored in the workpools' linked list
							// if(debugLoadBalancing) std::cout << MSL_myId << ": Verschicke Problem (" << problemFrame->getID()<<","<<problemFrame->getRootNodeID()<<","<<problemFrame->getOriginator()<<") an " << entranceOfSolver[i] << std::endl;
							if(debugCommunication) std::cout << MSL_myId << ": Abgabe von Problem (" << problemFrame->getID() <<"," <<  problemFrame->getRootNodeID() << "," << problemFrame->getOriginator() << "," << problemFrame->getPoolID() << ") an " << entranceOfSolver[i] << std::endl;
							// TODO Deadlockgefahr!? Große Probleme werden ohne Sendrequest verschickt! TODO
							MSL_Send(entranceOfSolver[i], problemFrame, MSLT_SUBPROBLEM);
							delete problemFrame->getData();
							delete problemFrame;
							numP--;
							numPF--;
							problemFrame = NULL;
							numOfSubproblemsSent++; // statistics
							//if(debugLoadBalancing) std::cout << MSL_myId << ": Arbeit verschickt." << std::endl;
						}
					}
				}

				//* 5. Lastverteilung
				if(workpool->isEmpty() && !sendRequestSent) { // ***
					// Wenn noch kein Work Request verschickt wurde
					if(!workRequestSent) {
						// sende Work Request und merke den Index des Empfänger
						do { receiverOfWorkRequest = entranceOfSolver[rand()%numOfSolvers]; }
						while(receiverOfWorkRequest == MSL_myId);
						// optimieren: nicht wieder an den gleichen schicken

						 // TODO: Umbenennen: numOfSolvers -> numOfPartners
						MSL_SendTag(receiverOfWorkRequest, MSLT_WORKREQUEST);
						if(debugLoadBalancing) { std::cout << MSL_myId << ": WORKREQUEST an " << receiverOfWorkRequest << std::endl; solutionpool->show(); }
						numOfWorkRequestsSent++; // für die Statistik
						workRequestSent = true;	// avoid flooding the network with work requests
					}
					// sonst warte auf Antwort
					else {
						// Wenn eine Absage angekommt
						MPI_Iprobe(receiverOfWorkRequest, MSLT_REJECTION, MPI_COMM_WORLD, &flag, &status);
						if(flag) {
							// Absage entgegennehmen und nächsten Work Request verschicken
							MSL_ReceiveTag(receiverOfWorkRequest, MSLT_REJECTION);
							if(debugLoadBalancing) std::cout << MSL_myId<< ": REJECTION von " << receiverOfWorkRequest << std::endl;
							numOfRejectionsReceived++; // statistics
							workRequestSent = false; // Antwort erhalten
						}
						// sonst ist vielleicht Arbeit angekommen
						else {
							// TODO:
							// Nach einer Arbeitsanfrage wird ein neues Teilproblem empfangen. Die Lösung dieses Problems muss an den Sender
							// zurückgeschickt werden. Dazu erstelle ein Paar (ID, Sender) und schreibe es in die RootList des SolutionPoolManagers.
							// Das empfange Problem wird im Workpool abgespeichert. Es ist darauf zu achten, dass dieses Problem im Rahmen
							// der Lastverteilung nicht direkt wieder abgegeben wird (sonst wäre der Workpool wieder leer). Dies ist implizit
							// sichergestellt, da die Problemverarbeitungsphase vor der nächsten Lastverteilungsphase durchlaufen wird.
							MPI_Iprobe(receiverOfWorkRequest, MSLT_SUBPROBLEM, MPI_COMM_WORLD, &flag, &status);
							if(flag) {
								problemFrame = new Frame<Problem>(); // Konstruktor?
								numPF++;
								MSL_Receive(receiverOfWorkRequest, problemFrame, MSLT_SUBPROBLEM, &status);
								numOfSubproblemsReceived++; // statistics
								// Den Absender und die RootID in den Frame schreiben
								problemFrame->setOriginator(receiverOfWorkRequest); // an diese Adr. wird die Lösung geschickt
								problemFrame->setRootNodeID(problemFrame->getID());
								// Problem einfach in den Workpool schreiben, der sich um alles Weitere kümmert
								// if(debugLoadBalancing) std::cout << MSL_myId<< ": empfange Problem (" << problemFrame->getID()<<","<<problemFrame->getRootNodeID()<<","<<problemFrame->getOriginator()<<")" << std::endl;
								if(debugCommunication) std::cout << MSL_myId<< ": empfange Problem " << problemFrame->getID() << " von " << receiverOfWorkRequest << std::endl;
								workpool->insert(problemFrame);
								workRequestSent = false; // Antwort erhalten
							}
						}
					}
				}

				// 6. Prüfe auf intern versendete STOPS
				// STOPs werden nur von den MS versendet, die daraufhin ihren Status als MS verlieren und einfache Worker sind.
				// Dass noch immer auf STOPs von ggf. ehemaligen MS geprüft wird, macht nichts.
				for(int currentMS=0; currentMS<numOfMasterSolvers; currentMS++) {
					MPI_Iprobe(exitOfSolver[currentMS], MSLT_STOP, MPI_COMM_WORLD, &flag, &status);
					if(flag==true) {
						flag = false;
						MSL_ReceiveTag(exitOfSolver[currentMS], MSLT_STOP);
						if(debugCommunication) std::cout << "(DCStreamSolver::start): DCStreamSolver " << MSL_myId << " hat STOP von " << exitOfSolver[currentMS] << " empfangen" << std::endl;
						internallyReceivedStops++;
					}
				}
			} // interner Nachrichtenverkehr




			//* 7. PROBLEMVERARBEITUNG: (fertig)
			// Workpool speichert auch einfache Probleme
			// - simple Probleme können als Last abgegeben werden, obwohl lokale Bearbeitung schneller wäre
			// - einfache Probleme werden nicht direkt gelöst, sondern vorher als Frame wieder in den Workpool geschrieben (Overhead)
			// - erhöhter Speicherplatzbedarf, da einfache Probleme ebenfalls im Workpool liegen
			// - pro gelöstes Problem ist 1 combinationIsPossible-Prüfung notwendig (Overhead)
			// + geringer Rechnenaufwand pro Iteration -> schnelles Erreichen und häufiges Durchlaufen der Kommunikationsphase
			if(!finished && (!workpool->isEmpty())) {
				time_start = MPI_Wtime();
				problemFrame = workpool->get();
				time_workpool += MPI_Wtime() - time_start;
				problem = problemFrame->getData();
				numOfProblemsProcessed++;
				if(debugComputation) { std::cout << MSL_myId << ": get Problem" << std::endl; workpool->show(); solutionpool->show(); }
				long currentID = problemFrame->getID();
				long rootNodeID = problemFrame->getRootNodeID();
				originator = problemFrame->getOriginator();
				long poolID = problemFrame->getPoolID();
				delete problemFrame;
				problemFrame = NULL;
				numPF--;
				// Wenn das Problem zu komplex ist, zerlegen und in Workpool schreiben
				if(!isSimple(problem)) {
					time_start = MPI_Wtime();
					Problem** subproblem = divide(problem); // ### USER DEFINED DIVIDE() ###
					time_divide += MPI_Wtime() - time_start;
					numP += D;
					bool toDelete = true;
					for(int i = 0; i < D; i++) {
						// falls das an divide übergebene Problem vom user wiederverwendet wurde und Teil des subproblem-Arrays ist,
						// darf es nicht gelöscht werden!
						if(subproblem[i] == problem) toDelete = false;
						break;
					}
					if(toDelete) {	delete problem;	problem = NULL;	numP--;}
					// KindID = VaterID*D+1; 	VaterID = (KindID-1)/D; 	WurzelID = 0
					long subproblemID = currentID * D + 1 + (D-1); // ID des letzten Kindknotens
					// erzeuge pro Subproblem einen neuen Frame (ID, Problem*) und speichere ihn im Workpool
					for(int i=D-1; i>=0; i--) {
						// Original-ProblemID, Owner und Absender werden lokal nie geändert, sondern einfach kopiert
						time_start = MPI_Wtime();
						problemFrame = new Frame<Problem>(subproblemID--, rootNodeID, originator, poolID, subproblem[i]); // ###
						time_new += MPI_Wtime() - time_start;
						numPF++;
						time_start = MPI_Wtime();
						workpool->insert(problemFrame);
						time_workpool += MPI_Wtime() - time_start;
						if(debugComputation) { std::cout << MSL_myId << ": insert Problem" << std::endl; workpool->show(); solutionpool->show(); }
					}
					delete[] subproblem; // Hinweis in der Dokumentation, dass Speicher intern freigegeben wird!
				}
				// Wenn das Problem einfach genug ist, lösen, unter gleicher ID im Solutionpool ablegen und ggf. kombinieren
				else {
					time_start = MPI_Wtime();
					solution = solve(problem); // ### USER DEFINED SOLVE() ###
					time_solve += MPI_Wtime() - time_start;
					numS++;
					numOfSimpleProblemsSolved++;
					p1 = (void*)problem;
					p2 = (void*)solution;
					if(p1 != p2) { delete problem; problem = NULL; numP--;}
					// Original-ProblemID und Absender werden lokal nie geändert, sondern einfach kopiert
					time_start = MPI_Wtime();
					solutionFrame = new Frame<Solution>(currentID, rootNodeID, originator, poolID, solution);
					time_new += MPI_Wtime() - time_start;
					numSF++;
					if(currentID == rootNodeID) {
						// Lösung in SendQueue schreiben
						time_start = MPI_Wtime();
						solutionpool->writeElementToSendQueue(solutionFrame);
						time_solutionpool += MPI_Wtime() - time_start;
						// solutionpool->insert(solutionFrame);
						solutionFrame = NULL;
					}
					else {
						if(debugComputation) std::cout << MSL_myId << ": Problem (" << currentID << "," << rootNodeID << "," << originator << "," << poolID << ") gelöst. Schreibe in Solutionpool." << std::endl;
						time_start = MPI_Wtime();
						solutionpool->insert(solutionFrame);
						time_solutionpool += MPI_Wtime() - time_start;
						time_start = MPI_Wtime();
						solutionpool->combine();
						time_combine += MPI_Wtime() - time_start;
						if(debugComputation) { std::cout << MSL_myId << ": combine/insert solution" << std::endl; workpool->show(); solutionpool->show(); }
					}
				}
			} // Ende Problemverarbeitung
			//*/

			// TERMINATION DETECTION (durch Master)
			if(isMasterSolver && (!finished)) {
				// fertig, wenn einzige Lösung im primären Solutionpool die ID "0" trägt
				if(solutionpool->hasSolution(primaryPoolID)) {
					solution = solutionpool->top(primaryPoolID)->getData();
					int receiver = getReceiver();
					if(debugCommunication) std::cout << MSL_myId << ": DCStreamSolver sendet Lösung an " << receiver << std::endl;
					MSL_Send(receiver, solution);
					solutionpool->pop(primaryPoolID);
					solution = NULL;
					numOfSolutionsSent++; // statistics
					blocked = false; // löse Blockade
				}
			}
			// Jeder MS kann nach Verlust seines MS-Status in diesen Bereich eintreten und terminieren.
			if(!isMasterSolver && internallyReceivedStops==numOfMasterSolvers) {
				// alle solverinternen Prozessoren benachrichtigen (falls Solver datenparallel ist)
				for(int i=MSL_myEntrance+1; i < MSL_myEntrance+noprocs; i++)
					MSL_SendTag(i, MSLT_STOP);
				receivedStops = 0;
				internallyReceivedStops = 0;
				finished = true; // Feierabend
				if(debugCommunication) std::cout << "DCStreamSolver " << MSL_myId << " terminiert"<< std::endl;
			}

		} // ende: while(!finished)
		time_solver = MPI_Wtime() - time_solver;


		if(analyse) {
			std::cout << MSL_myId << "start" << std::endl;
			std::cout << MSL_myId << ": processed subproblems: " << numOfProblemsProcessed << std::endl;
			std::cout << MSL_myId << ": simple subproblems: " << numOfSimpleProblemsSolved << std::endl;
			// std::cout << MSL_myId << ": numOfSolutionsSent: " << numOfSolutionsSent << std::endl;
			// std::cout << MSL_myId << ": numOfSolutionsReceived: " << numOfSolutionsReceived << std::endl;
			std::cout << MSL_myId << ": shared subproblems: " << numOfSubproblemsSent << std::endl;
			std::cout << MSL_myId << ": received subproblems: " << numOfSubproblemsReceived << std::endl;
			std::cout << MSL_myId << ": work requests: " << numOfWorkRequestsSent << std::endl;
			//std::cout << MSL_myId << ": numOfWorkRequestsReceived: " << numOfWorkRequestsReceived << std::endl;
			//std::cout << MSL_myId << ": numOfRejectionsSent: " << numOfRejectionsSent << std::endl;
			// std::cout << MSL_myId << ": numOfRejectionsReceived: " << numOfRejectionsReceived << std::endl;
			std::cout << MSL_myId << ": time_solve: " << time_solve << std::endl;
			std::cout << MSL_myId << ": time_combine: " << time_combine << std::endl;
			std::cout << MSL_myId << ": time_divide: " << time_divide << std::endl;
			//std::cout << MSL_myId << ": time_new: " << time_new << std::endl;
			//std::cout << MSL_myId << ": time_workpool: " << time_workpool << std::endl;
			//std::cout << MSL_myId << ": time_solutionpool: " << time_solutionpool << std::endl;
			//std::cout << MSL_myId << ": time_solver: " << time_solver << std::endl;
			//std::cout << MSL_myId << ": numP: " << numP << std::endl;
			//std::cout << MSL_myId << ": numS: " << numS << std::endl;
			//std::cout << MSL_myId << ": numPF: " << numPF << std::endl;
			//std::cout << MSL_myId << ": numSF: " << numSF << std::endl;
			std::cout << MSL_myId << "end" << std::endl;
		}
		if(debugCommunication) std::cout << MSL_myId << ": DCStreamSolver terminiert." << std::endl;
	} // end of start()


	//
	// Berechnet die Ein- und Ausgaenge aller Solverkollegen.
	// @param solver	Zeiger auf das komplette Solverarray (dort ist man selbst auch eingetragen)
	// @param length	Laenge des Solverarrays
	// @param id		Index des Zeigers im Solverarray, der auf diesen Solver (this) zeigt
	//
	void setWorkmates(DCStreamSolver<Problem, Solution>** solver, int length, int id) {
		// indexID = id; // merke, an welcher Position man selbst steht
		numOfSolvers = length;
		entranceOfSolver = new ProcessorNo[numOfSolvers];
		exitOfSolver = new ProcessorNo[numOfSolvers];

		// Ein- und Ausgaenge aller Solver berechnen (inkl. des eigenen Ein- und Ausgangs)
		ProcessorNo* ext;
		ProcessorNo* entr;
		for(int i = 0; i < length; i++) {
			entr = (*(solver[i])).getEntrances();
			entranceOfSolver[i] = entr[0];
			ext = (*(solver[i])).getExits();
			exitOfSolver[i] = ext[0];
			//std::cout << "setWorkmates: entr/exit of solver["<<i < <"]: "<<entranceOfSolver[i]<<"/"<<exitOfSolver[i]<<std::endl;
		}
		// Sender und Empfänger für das Token berechnen
		left = exitOfSolver[(id-1+length)%length];
		right = entranceOfSolver[(id+1)%length];
		//std::cout << "setWorkmates: left/right of solver["<<id<<"]: " << left << "/" << right << std::endl;
	}

	void setMastersolver() { isMasterSolver = true; }


    Process* copy() { return new DCStreamSolver<Problem, Solution>(divide, combine, solve, isSimple, D, numOfMasterSolvers, noprocs);}

   	void show() const {
		if(MSL_myId == 0) std::cout << "           DCStreamSolver (PID = " << entrances[0] << ")";
		if(isMasterSolver == true) {
			std::cout << " Mastersolver - pred: ";
			for(int i = 0; i < numOfPredecessors; i++)
				std::cout << predecessors[i] << " ";
			std::cout << "  succ: ";
			for(int i = 0; i < numOfSuccessors; i++)
				std::cout << successors[i] << " ";
		}
		std::cout << std::endl << std::flush;
	}

}; // end of class DCStreamSolver




/*
 * StreamDC ist ein hinsichtlich streambasierter Berechnungen optimiertes dezentrales Divide and Conquer Skelett.
 * Die Optimierung besteht im Wesentlichen darin, zeitgleich mehrere DC-Probleme durch das Skelett bearbeiten zu lassen.
 * Auf diese Weise überlappen sich die Startup- und Endphasen einzelner Lösungsprozesse, was zu einer Reduktion der
 * Idle-Zeiten führen kann. Zudem kann die Generierung von Teilproblemen grobgranularer erfolgen, d.h. der Rekursionsabbruch
 * kann eher erfolgen (oder anders: der DC-Baum jedes DC-Problems kann in seiner Höhe beschnitten werden).
 * Dies spart in vielen Anwendungen Speicherplatz und Rechenzeit, da die Anzahl der divide()- und combine()-Aufrufe
 * sowie die Ausführungszeit von solve() reduziert wird (es gilt: time(solve(N)) <= D*time(solve(N/D)), mit D = Grad des DC-Baums).
 * Des Weiteren kann die Anzahl der zeitgleich zu lösenden Probleme an den insgesamt verfügbaren verteilten Speicher angepasst werden.
 * Bei N Solvern und N zeitgleich zu lösenden Problemen wird sich das Verhalten dieses Skeletts an eine Farm von sequentiell arbeitenden
 * DCSolvern anpassen. Vermutlich wird jedoch eine Farm von sequentiell arbeitenden DC-Solvern etwas schneller sein (was zu zeigen wäre).
 */
 template<class Problem, class Solution>
class StreamDC: public Process {

	// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
	DCStreamSolver<Problem,Solution>** p;
	int length; // #Solver
	int numOfMastersolvers;

    public:

		//
		// Ein dezentrales Divide and Conquer-Skelett.
		//
		// Dieser Konstruktor erzeugt (l-1) Kopien des übergebenen DCStreamSolvers. Insgesamt besteht dieses Skelett aus l Solvern.
		//
		// Externe Schnittstellen:
		// Jeder Solver verfügt über genau einen Eingang und einen Ausgang. Das StreamDC-Skelett hat genau einen Eingang.
		// Dieser wird auf einen der Solver gemappt. Die Ausgänge der Solver sind die Ausgänge des BranchAndBoundOld-Skeletts.
		//
		// Interne schnittstellen:
		// Jeder Solver kennt die Ein- und Ausgänge seiner Kollegen. Über left und right ist zudem eine logische Ringstruktur definiert,
		// die für das versenden des Tokens im Rahmen des Termination Detection Algorithmus genutzt wird.
		//
    	StreamDC(DCStreamSolver<Problem,Solution>& solver, int l, int e): length(l), numOfMastersolvers(e), Process() {
			if(e < 1) {
    			std::cout << "#MasterSolvers < 1 in StreamDC! Setting #MasterSolvers = 1 for this run" << std::endl;
    			e = l;
			}
    		if(e > l) {
    			std::cout << "#MasterSolvers > #Processors in StreamDC! Setting #MasterSolvers = #Processors for this run" << std::endl;
    			e = l;
    		}
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new DCStreamSolver<Problem,Solution>* [length];
            p[0] = &solver;
            for(int i = 1; i < length; i++)
            	p[i] = (DCStreamSolver<Problem,Solution>*)solver.copy();

            // bestimme die Mastersolver
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setMastersolver();

			// externe Schnittstellen des StreamDC-Skeletts definieren:
			// Frage dazu die MasterSolver nach ihren Ein- und Ausgängen. Jeder Mastersolver hat genau einen Ein- und Ausgang.
			numOfEntrances = numOfMastersolvers;
			entrances = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* entr;
            for(int i = 0; i < numOfMastersolvers; i++) {
            	// Mastersolver i hat nur einen Eingang
            	entr = (*(p[i])).getEntrances();
            	entrances[i] = entr[0];
            }
			numOfExits = numOfMastersolvers;
			exits = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* ext;
            for(int i = 0; i < numOfMastersolvers; i++) {
            	// Mastersolver i hat nur einen Ausgang
            	ext = (*(p[i])).getExits();
            	exits[i] = ext[0];
            }

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
			// Empfaenger der ersten Nachricht festlegen
			//setNextReceiver(0); // DistributedBB verschickt doch gar nichts, sondern die Solver
		}


    	StreamDC(Problem** (*divide)(Problem*), Solution* (*combine)(Solution**), Solution* (*solve)(Problem*), bool (*isSimple)(Problem*), int d, int l, int e): length(l), numOfMastersolvers(e), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new DCStreamSolver<Problem,Solution>* [length];
            for(int i = 0; i < length; i++)
            	p[i] = new DCStreamSolver<Problem, Solution>(divide, combine, solve, isSimple, d, numOfMastersolvers, 1);

            // bestimme die Mastersolver
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setMastersolver();

			// externe Schnittstellen des StreamDC-Skeletts definieren:
			// Frage dazu die MasterSolver nach ihren Ein- und Ausgängen. Jeder Mastersolver hat genau einen Ein- und Ausgang.
			numOfEntrances = numOfMastersolvers;
			entrances = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* entr;
            for(int i = 0; i < numOfMastersolvers; i++) {
            	// Mastersolver i hat nur einen Eingang
            	entr = (*(p[i])).getEntrances();
            	entrances[i] = entr[0];
            }
			numOfExits = numOfMastersolvers;
			exits = new ProcessorNo[numOfMastersolvers];
			ProcessorNo* ext;
            for(int i = 0; i < numOfMastersolvers; i++) {
            	// Mastersolver i hat nur einen Ausgang
            	ext = (*(p[i])).getExits();
            	exits[i] = ext[0];
            }

			// interne Schnittstellen definieren: jeder kennt jeden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
		}


		~StreamDC() {
			// std::cout << MSL_myId << ": ~StreamDC called" << std::endl;
			delete[] p;
			delete[] entrances;
			delete[] exits;
			p = NULL;
			entrances = NULL;
			exits = NULL;
		}


		// von diesen L vorgelagerten Skeletten werden Daten empfangen
		inline void setPredecessors(ProcessorNo* src, int L) {
			numOfPredecessors = L;
			// allen Mastersolvern ihre vorgelagerten Skelette mitteilen
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setPredecessors(src,L);
		}

		// an diese L nachgelagerten Skelette werden Daten gesendet.
		inline void setSuccessors(ProcessorNo* drn, int L) {
			numOfSuccessors = L;
			// allen Mastersolvern ihre nachgelagerten Skelette mitteilen
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setSuccessors(drn,L);
		}

        // startet alle Solver
		inline void start() {
            for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

        Process* copy() { return new StreamDC<Problem, Solution>(*(DCStreamSolver<Problem,Solution>*)(*(p[0])).copy(), length, numOfMastersolvers);}

        inline void show() const {
            if(MSL_myId == 0) {
                std::cout << "StreamDC (entrances at PID ";
                for(int i = 0; i < numOfEntrances-1; i++)
                	std::cout << entrances[i] << ",";
                std::cout << entrances[numOfEntrances-1] << ") with " << length << " Solver(s) " << std::endl << std::flush;
                for(int i = 0; i < length; i++)
                    (*(p[i])).show();
			}
		}
}; // end of StreamDC







/*


template<class Problem, class Solution>
class StreamDC: public Process {

	// Solver; solver[0] ist Mastersolver (Eingang des Skeletts)
	DCStreamSolver<Problem,Solution>** p;
	int length; // #Solver
	int numOfMastersolvers;

    public:

    	StreamDC(DCStreamSolver<Problem,Solution>& solver, int l, int e): length(l), numOfMastersolvers(e), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new DCStreamSolver<Problem,Solution>* [length];
            p[0] = &solver;
            for(int i = 1; i < length; i++)
            	p[i] = (DCStreamSolver<Problem,Solution>*)solver.copy();

			// externe Schnittstellen definieren: Es gibt e Mastersolver, die als Ein- und Ausgänge des skeletts dienen.
			numOfEntrances = numOfMastersolvers;
			entrances = new ProcessorNo[numOfEntrances];
			ProcessorNo* entr;
            for(int i = 0; i < numOfEntrances; i++) {
            	// Mastersolver i hat nur einen Eingang
            	entr = (*(p[i])).getEntrances();
            	exits[i] = entr[0];
            }
			numOfExits = numOfMastersolvers;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext;
            for(int i = 0; i < numOfExits; i++) {
            	// Mastersolver i hat nur einen Ausgang
            	ext = (*(p[i])).getExits();
            	exits[i] = ext[0];
            }

			// interne Schnittstellen definieren: jeder kennt jeden.
			// Hier kann auch eine andere Topologie hinterlegt werden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
		}


		//
		// Ein dezentrales Divide and Conquer-Skelett.
		//
		// Externe Schnittstellen:
		// Jeder DCStreamSolver verfügt über genau einen Eingang und einen Ausgang. Das StreamDC-Skelett hat e MasterSolver und damit e Ein- und Ausgänge.
		// Es werden maximal e DC-Probleme zeitgleich bearbeitet.
		//
		// Interne schnittstellen:
		// Jeder Solver kennt die Ein- und Ausgänge seiner Kollegen. Über left und right ist zudem eine logische Ringstruktur definiert,
		// die zur Zeit nicht genutzt wird.
		//
    	StreamDC(Problem** (*divide)(Problem*), Solution* (*combine)(Solution**), Solution* (*solve)(Problem*), bool (*isSimple)(Problem*), int d, int l, int e): length(l), numOfMastersolvers(e), Process() {
			// Solver sind alle vom gleichen Typ und werden in der gewuenschten Anzahl generiert
            p = new DCStreamSolver<Problem,Solution>* [length];
            for(int i = 0; i < length; i++)
            	p[i] = new DCStreamSolver<Problem, Solution>(divide, combine, solve, isSimple, d, 1);

			// externe Schnittstellen definieren: Es gibt e Mastersolver, die als Ein- und Ausgänge des skeletts dienen.
			numOfEntrances = numOfMastersolvers;
			entrances = new ProcessorNo[numOfEntrances];
			ProcessorNo* entr;
            for(int i = 0; i < numOfEntrances; i++) {
            	// Mastersolver i hat nur einen Eingang
            	entr = (*(p[i])).getEntrances();
            	exits[i] = entr[0];
            }
			numOfExits = numOfMastersolvers;
			exits = new ProcessorNo[numOfExits];
			ProcessorNo* ext;
            for(int i = 0; i < numOfExits; i++) {
            	// Mastersolver i hat nur einen Ausgang
            	ext = (*(p[i])).getExits();
            	exits[i] = ext[0];
            }

			// interne Schnittstellen definieren: jeder kennt jeden.
			// Hier kann auch eine andere Topologie hinterlegt werden.
			for(int i = 0; i < length; i++)
				(*(p[i])).setWorkmates(p,length,i);
		}


		~StreamDC() {
			// std::cout << MSL_myId << ": ~StreamDC called" << std::endl;
			delete[] p;
			delete[] entrances;
			delete[] exits;
			p = NULL;
			entrances = NULL;
			exits = NULL;
		}


		// von diesen len vorgelagerten Skeletten werden Daten empfangen
		// alle Mastersolver kommunizieren mit den Sendern
		inline void setPredecessors(ProcessorNo* src, int len) {
			numOfPredecessors = len;
			// teile jedem Mastersolver seine vorgelagerten Skelette mit
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setPredecessors(src,len);
			// was ist mit den anderen DCStreamSolvern???
		}

		// an diese len nachgelagerten Skelette werden Daten gesendet.
		// alle Mastersolver können Daten verschicken
		inline void setSuccessors(ProcessorNo* drn, int len) {
			numOfSuccessors = len;
			for(int i = 0; i < numOfMastersolvers; i++)
				(*(p[i])).setSuccessors(drn,len);
		}

        // startet alle Solver
		inline void start() {
            for(int i = 0; i < length; i++)
				(*(p[i])).start();
		}

        Process* copy() { return new StreamDC<Problem, Solution>(*(DCStreamSolver<Problem,Solution>*)(*(p[0])).copy(), length, numOfMastersolvers);}

        inline void show() const {
            if(MSL_myId == 0) {
                std::cout << "StreamDC (Mastersolver at ";
                for(int i = 0; i < numOfEntrances; i++)
                	std::cout << entrances[i] << ",";
                std::cout << ") with " << length << " Solver(s) " << std::endl << std::flush;
                for(int i = 0; i < length; i++)
                    (*(p[i])).show();
			}
		}
}; // end of StreamDC

*/

#endif
