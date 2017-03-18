/*!
  \file Vector.h
  \brief Implementazione della classe gameoflife::Vector
  \author Andrea Zanelli
  \date 09-03-2010
*/

#ifndef _VECTOR_H
#define _VECTOR_H 1

#include <iostream>
#include "Muesli.h"


namespace gameoflife {

/*!
  \class Vector
  \brief Vettore interno ad un blocco.

  Rappresenta un vettore interno ad un blocco. Implementa l'interfaccia
  MSL_Serializable poiche' oggetti di questo tipo devono essere scambiati tra
  i workers durante la fase di sincronizzazione.
*/
class Vector : public MSL_Serializable {

  // PRIVATE MEMBERS
  public:
    unsigned int _size; // dimensione del vettore
    bool* _vector;      // _vector[_size]

  // PUBLIC FUNCTIONS
  public:
  
    /**
     * Costruttore di default: costruisce un vettore vuoto di dimensione "size"
     */
    Vector(unsigned int size = 0) : _size(size) {
      _vector = new bool[_size];
    } // end of default constructor
    
    /**
     * Costruttore di copia
     */
    Vector(const Vector& vector) {
      _size = vector.getVectorSize();
      _vector = new bool[_size];
      for(int i=0; i < _size; ++i)
        _vector[i] = vector.get(i);
    } // end of copy constructor

    /**
     * Distruttore
     */
    virtual ~Vector() {
      delete[] _vector;
    } // end of distructor
    
    /**
     * Operatore di assegnamento
     */
    Vector& operator=(const Vector& vector) {
      _size = vector.getVectorSize();
      delete[] _vector;
      _vector = new bool[_size];
      for(int i=0; i < _size; ++i)
        _vector[i] = vector.get(i);
      return (*this);
    } // end of operator=
    
    /**
     * Restituisce l'i-esimo elemento
     */
    bool& get(unsigned int i) {
      return _vector[i];
    } // end of method get

    /**
     * Restituisce l'i-esimo elemento
     */
    const bool& get(unsigned int i) const {
      return _vector[i];
    } // end of method get
    
    /**
     * Imposta l'i-esimo elemento
     */
    void set(unsigned int i, bool value) {
      _vector[i] = value;
      return;
    } // end of method set
    
    /**
     * Restituisce la dimensione del vettore.
     */
    unsigned int getVectorSize() const {
      return _size;
    } // end of method getVectorSize()

    /** Override */
    inline int getSize() {
      return sizeof(unsigned int) +  // _size
          sizeof(bool) * _size;      // _vector
    } // end of method getSize

    /** Override */
    void reduce(void* pBuffer, int bufferSize) {
      typedef unsigned int uint;
      uint* adr = (uint*) memcpy(pBuffer, &_size, sizeof(uint));
      adr++;
      memcpy(adr, _vector, _size*sizeof(bool));
      return;
    } // end of method reduce

    /** Override */
    void expand(void* pBuffer, int bufferSize) {
      int* adr1 = (int*) pBuffer;
      _size = *(adr1++);
      bool* adr2 = (bool*) adr1;
      delete[] _vector;
      _vector = new bool[_size];
      for(int i=0; i < _size; ++i)
        _vector[i] = *(adr2++);
      return;
    } // end of method expand

}; // end of class Vector

/**
 * Operatore di output <<: stampa il vettore rappresentando con "[ ]" le celle
 * morte e con "[*]" le celle vive.
 */
std::ostream& operator<<(std::ostream& out, const Vector& v) {
  for(int i=0; i < v.getVectorSize(); ++i)
    out <<(v.get(i) ? "[*]" : "[ ]");
  return out;
} // end of function operator<<

} // end of namespace gameoflife


#endif // _VECTOR_H

