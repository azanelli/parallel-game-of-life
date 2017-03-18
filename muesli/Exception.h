// **********************************************************
// Exceptions 
// **********************************************************

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <sstream>

//abstract
class Exception {

public:
	
	virtual ~Exception() {
	};

	virtual char* toString() const = 0;

};

// ***************** Exception for Skeletons ****************

class UndefinedSourceException: public Exception {

public:
	
	char* toString() const {
		return "UndefinedSourceException";
	}

};

class UndefinedDestinationException: public Exception {

public:
	
	char* toString() const {
		return "UndefinedDestinationException";
	}

};

class NonLocalAccessException: public Exception {

public:

	char* toString() const {
		return "NonLocalAccessException";
	}

};

class MissingInitializationException: public Exception {

public:

	char* toString() const {
		return "MissingInitializationException";
	}

};

class IllegalGetException: public Exception {

public:

	char* toString() const {
		return "IllegalGetException";
	}

};

class IllegalPutException: public Exception {

public:

	char* toString() const {
		return "IllegalPutException";
	}

};

class IllegalPartitionException: public Exception {

public:

	char* toString() const {
		return "IllegalPartitionException";
	}

};

class PartitioningImpossibleException: public Exception {

public:

   char* toString() const {
	   return "PartitioningImpossibleException";
   }

};

class IllegalPermuteException: public Exception {

public:

	char* toString() const {
		return "IllegalPermuteException";
	}

};

class IllegalAllToAllException: public Exception {

public:

	char* toString() const {
		return "IllegalAllToAllException";
	}

};

class NoSolutionException: public Exception {

public:

	char* toString() const {
		return "NoSolutionException";
	}

};

class InternalErrorException: public Exception {

public:
	
	char* toString() const {
		return "InternalErrorException";
	}

};

// ***************** Exceptions for Collections *************

class EmptyHeapException: public Exception {

public:

	char* toString() const {
		return "EmptyHeapException";
	}

};

class EmptyStackException: public Exception {

public:
	
	char* toString() const {
		return "EmptyStackException";
	}

};

class EmptyQueueException: public Exception {

public:
	
	char* toString() const {
		return "EmptyQueueException";
	}

};

// ***************** Various *************

inline std::ostream& operator<<(std::ostream& os, const Exception& e) {
	os << e.toString() << std::endl;

	return os;
}

// ***************** DistributedSparseMatrix Exceptions *************

class IndexOutOfBoundsException: public Exception {

public:
	
	char* toString() const {
		return "IndexOutOfBoundsException";
	}

};

#endif
