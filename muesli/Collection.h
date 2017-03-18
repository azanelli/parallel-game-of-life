#ifndef COLLECTION_H
#define COLLECTION_H

#include "Curry.h"
#include "Exception.h"
#include "Muesli.h"

// ********************** Collection (abstract) ******************

template<class I> class Collection {

public:

	Collection() {
	};

	virtual ~Collection() {
	};

	virtual bool isEmpty() = 0;
	virtual I top() = 0;
	virtual I get() = 0;
	virtual void insert(I val) = 0;
	virtual Collection<I>* fresh() = 0;
	virtual void show() = 0;

};
 
// **************************** Heap *****************************

template<class I> class Heap: public Collection<I> {

	int last;
	int size;
	I* heap;
	DFct2<I, I, bool> lth;

public:
	
	Heap(const DFct2<I, I, bool>& less): lth(less), last(-1), size(8) {
		heap = new I[size];
	}

	Heap(bool (*less)(I, I)): last(-1), size(8), lth(Fct2<I, I, bool, bool (*)(I, I)>(less)) {
		heap = new I[size];
	}

	bool isEmpty() {
		return last < 0;
	}

	I top() {
		if(isEmpty())
			throws(EmptyHeapException());

		return heap[0];
	}

	I get() {
		if(isEmpty())
			throws(EmptyHeapException());

		I result = heap[0];
		heap[0] = heap[last--];
		int current = 0;
		int next = 2 * current + 1;

		while(next <= last + 1) {
			if((next <= last) && (lth(heap[next + 1], heap[next])))
				next++;

			if(lth(heap[next], heap[current])) {
				I aux = heap[next];
				heap[next] = heap[current];
				heap[current] = aux;
				current = next;
				next = 2 * current + 1;
			}
			else
				next = last + 2;
		}
		// stop while loop

		return result;
	}

	void insert(I val) {
		int current = ++last;

		// extend heap
		if(last >= size) {
			I* newheap = new I[2 * size];

			for(int i = 0; i < size; i++)
				newheap[i] = heap[i];

			size *= 2;
			delete[] heap;
			heap = newheap;
		}

		int next;
		heap[last] = val;
		
		while(current > 0) {
			next = (current - 1) / 2;
			
			if(lth(heap[current], heap[next])) {
				I aux = heap[next];
				heap[next] = heap[current];
				heap[current] = aux;
				current = next;
			}
			else {
				current = 0;
			}
		}
	}
	// stop loop

	DFct2<I, I, bool> getLth() {
		return lth;
	}

	Collection<I>* fresh() {
		Heap<I>* h = new Heap<I>(lth);

		return h;
	}

	void show(){
		std::cout << ": heap: [";

		for(int i = 0; i <= last; i++) 
			std::cout << heap[i] << " ";

		std::cout << "]" << std::endl << std::flush;
	}

};

// ************************** Stack **************************

template<class I> class Stack: public Collection<I> {

	int last;
	int size;
	I* stack; 

public:
	 
	Stack(): last(-1), size(8) {
		stack = new I[size];
	}

	bool isEmpty() {
		return last < 0;
	}
 
	I top() {
		if(isEmpty())
			throws(EmptyStackException());

		return stack[last];
	}

	I get() {
		if(isEmpty())
			throws(EmptyStackException());

		I result = stack[last--];

		return result;
	}

	void insert(I val) {
		// extend stack
		if(++last >= size) {
			I* newstack = new I[2 * size];

			for(int i = 0; i < size; i++)
				newstack[i] = stack[i];

			size *= 2;
			delete[] stack;
			stack = newstack;
		}

		stack[last] = val;
	}

	Collection<I>* fresh() {
		Stack<I>* st = new Stack<I>();

		return st;
	}

	void show(){
		std::cout << ": stack: [";

		for(int i = 0; i <= last; i++)
			std::cout << stack[i] << " ";

		std::cout << "]" << std::endl << std::flush;
	}

};

// ************************** Queue **************************

template<class I> class Queue: public Collection<I> {

    int first;  // index of first element to take out
    int count;  // number of elements in queue
    int size;
    I* queue; 

public:
	
	Queue(): first(0), count(0), size(8) {
		queue = new I[size];
	}

	bool isEmpty() {
		return count == 0;
	}
 
	I top() {
		if(isEmpty())
			throws(EmptyQueueException());
		
		return queue[first];
	}

	I get(){
		if(isEmpty())
			throws(EmptyQueueException());

		I result = queue[first];
		first = (first + 1) % size;
		count--;

		return result;
	}

	void insert(I val) {
		// extend queue
		if(count == size) {
			I* newqueue = new I[2 * size];
			
			for(int i = 0; i < size; i++)
				newqueue[i] = queue[(first + i) % size];

			size *= 2;
			first = 0;
			delete[] queue;
			queue = newqueue;
		}

		queue[(first + (count++)) % size] = val;
	}

	Collection<I>* fresh() {
		Queue<I>* q = new Queue<I>();

		return q;
	}

	void show(){
		std::cout << ": queue: [";

		for(int i = 0; i < count; i++)
			std::cout << queue[(first + i) % size] << " ";

		std::cout << "]" << std::endl << std::flush;
	}

};

#endif
