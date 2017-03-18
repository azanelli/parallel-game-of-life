/*!
  \file Block.h
  \brief Implementazione della classe gameoflife::Block
  \author Andrea Zanelli
  \date 09-03-2010
*/

#ifndef _BLOCK_H
#define _BLOCK_H 1

#include <iostream>
#include <cstring>
#include "Muesli.h"
#include "Vector.h"


namespace gameoflife {

/*!
  \class Block
  \brief Un blocco in cui suddividere la matrice del gioco della vita.

  Rappresenta un blocco della matrice del gioco della vita, utilizzato per
  l'esecuzione in parallelo. Implementa l'interfaccia MSL_Serializable per 
  poter essere utilizzato come input e output negli skeleton della libreria 
  Muesli.
*/
class Block : public MSL_Serializable {
    
  // PRIVATE MEMBERS
  private:
    
    unsigned int _n;    // numero del blocco
    unsigned int _pos;  // posizione del blocco nella matrice
    unsigned int _rows; // numero di righe
    unsigned int _cols; // numero di colone
    
    bool** _slice;   // _slice[_rows][_cols]
    
    Vector* _leftv;  // _leftv[_rows]
    Vector* _rightv; // _rightv[_rows]
    
  // PRIVATE METHODS
  private:
    
    /* 
      Elimina la matrice _slice
    */
    void deleteSlice() {
      for(int i=0; i < _rows; ++i)
        delete[] _slice[i];
      delete[] _slice;
      return;
    } // end of method deleteBlock

    /* 
      Restituisce il numero di vicini vivi della cella con indice i,j
    */
    int getNeighborsCount(unsigned int i, unsigned int j) const {
      unsigned int count = 0;
      unsigned int row;
      
      // Vicini nella riga sopra (row := i-1)
      row = (i == 0 ? _rows-1 : i-1);
      if(j == 0) { if(_leftv->get(row)) count++; }
      else { if(_slice[row][j-1]) count++; }
      if(_slice[row][j]) count++;
      if(j == _cols-1) { if(_rightv->get(row)) count++; }
      else { if(_slice[row][j+1]) count++; }
      
      // Vicini sulla stessa riga (row := i)
      if(j == 0) { if(_leftv->get(i)) count++; }
      else { if(_slice[i][j-1]) count++; }
      if(j == _cols-1) { if(_rightv->get(i)) count++; }
      else { if(_slice[i][j+1]) count++; }

      // Vicini nella riga sotto (row := i+1)
      row = (i == _rows-1 ? 0 : i+1);
      if(j == 0) { if(_leftv->get(row)) count++; }
      else { if(_slice[row][j-1]) count++; }
      if(_slice[row][j]) count++;
      if(j == _cols-1) { if(_rightv->get(row)) count++; }
      else { if(_slice[row][j+1]) count++; }
      
      return count;
    } // end of method getNeighborsCount
    
    /* 
     Restituisce il valore della generazione successiva della cella i,j.
    */
    bool getNextValue(unsigned int i, unsigned int j) const {
      int neighbors = getNeighborsCount(i,j);
      return _slice[i][j] ? 
          (neighbors == 2 || neighbors == 3) : 
          (neighbors == 3) ;
    } // end of method getNextValue

  // PUBLIC METHODS
  public:
    
    /**
     * Costruttore di default: costruisce un blocco vuoto.
     */
    Block() : _n(0), _pos(0), _rows(0), _cols(0) { }
    
    /**
     * Costruisce un blocco a partire dalla matrice "matrix". Il blocco
     * costruito ha come numero "n", dimensione "dim" (numero di colonne) e
     * viene costruito a partire dalla colonna "pos" della matrice.
     */
    Block(unsigned int n, unsigned int dim, unsigned int pos, 
        bool** matrix, unsigned int rows, unsigned int cols) : 
        _n(n), _pos(pos), _rows(rows), _cols(dim) {
      _slice = new bool*[_rows];
      _leftv = new Vector(_rows);
      _rightv = new Vector(_rows);
      unsigned int jleft = (pos == 0 ? cols-1 : pos-1);
      unsigned int jright = (pos+dim == cols ? 0 : pos+dim);
      for(unsigned int i=0; i < _rows; ++i) {
        _leftv->set(i,matrix[i][jleft]);
        _rightv->set(i,matrix[i][jright]);
        _slice[i] = new bool[dim];
        for(unsigned int j=0; j < dim; ++j)
          _slice[i][j] = matrix[i][pos+j];
      } // end for i
      return;
    } // end of constructor
    
    /**
     * Distruttore
     */
    virtual ~Block() {
      deleteSlice();
      delete _leftv;
      delete _rightv;
    } // end of distructor
    
    /**
     * Restituisce il numero del blocco (n).
     */
    unsigned int getN() const {
      return _n;
    } // end of method get
    
    /**
     * Restituisce la posizione del blocco nella matrice.
     */
    unsigned int getPosition() const {
      return _pos;
    } // end of method get
    
    /**
     * Restituisce il numero di righe del blocco.
     */
    unsigned int getRows() const {
      return _rows;
    } // end of method getRows();

    /**
     * Restituisce il numero di colonne del blocco.
     */
    unsigned int getColumns() const {
      return _cols;
    } // end of method getColumns();
    
    /**
     * Restituisce l'elemento nella riga i - colonna j.
     */
    bool& get(unsigned int i, unsigned int j) {
      return _slice[i][j];
    } // end of method get
    
    /**
     * Restituisce l'elemento nella riga i - colonna j.
     */
    const bool& get(unsigned int i, unsigned int j) const {
      return _slice[i][j];
    } // end of method get
    
    /**
     * Restituisce un puntatore ad un nuovo oggetto di tipo Vector che contiene
     * una copia degli elementi della prima colonna del blocco (il suo bordo 
     * sinistro).
     */
    Vector* getLeftBoundary() const {
      Vector* leftb = new Vector(_rows);
      for(int i=0; i < _rows; ++i)
        leftb->set(i, _slice[i][0]);
      return leftb;
    } // end of method getLeftBoundary
    
    /**
     * Restituisce un puntatore ad un nuovo oggetto di tipo Vector che contiene
     * una copia degli elementi dell'ultima colonna del blocco (il suo bordo 
     * destro).
     */
    Vector* getRightBoundary() const {
      Vector* rightb = new Vector(_rows);
      for(int i=0; i < _rows; ++i)
        rightb->set(i, _slice[i][_cols-1]);
      return rightb;
    } // end of method getRightBoundary
    
    /**
     * Restituisce il vettore sinistro.
     */
    const Vector& getLeftVector() const {
      return (*_leftv);
    } // end of method getLeftVector
    
    /**
     * Restituisce il vettore destro.
     */
    const Vector& getRightVector() const {
      return (*_rightv);
    } // end of method getRightVector
    
    /**
     * Cambia il vettore sinistro con quello passato come parametro.
     */
    void setLeftVector(Vector* leftv) {
      delete _leftv;
      _leftv = leftv;
    } // end of method setLeftVector
    
    /**
     * Cambia il vettore destro con quello passato come parametro.
     */
    void setRightVector(Vector* rightv) {
      delete _rightv;
      _rightv = rightv;
    } // end of method setRightVector
    
    /**
     * Esegue un'iterazione del gioco della vita sugli elementi del blocco.
     */
    void compute() {
      bool** newslice = new bool*[_rows];
      for(int i=0; i < _rows; ++i) {
        newslice[i] = new bool[_cols];
        for(int j=0; j < _cols; ++j)
          newslice[i][j] = getNextValue(i,j);
      } // end for i
      deleteSlice();
      _slice = newslice;
      return;
    } // end of method compute

    /** Override */
    virtual inline int getSize() {
      return sizeof(unsigned int) +  // _n
          sizeof(unsigned int) +     // _pos
          sizeof(unsigned int) +     // _rows
          sizeof(unsigned int) +     // _cols
          sizeof(bool)*_rows*_cols + // _slice
          _leftv->getSize() +        // _leftv
          _rightv->getSize();        // _rightv          
    } // end of method getSize

    /** Override */
    virtual void reduce(void* pBuffer, int bufferSize) {
      typedef unsigned int uint;
      uint* adr1 = (uint*) memcpy(pBuffer, &(_n), sizeof(uint));
      adr1++;
      memcpy(adr1, &(_pos), sizeof(uint));
      adr1++;
      memcpy(adr1, &(_rows), sizeof(uint));
      adr1++;
      memcpy(adr1, &(_cols), sizeof(uint));
      adr1++;
      bool* adr2 = (bool*) adr1;
      for(int i = 0; i < _rows; ++i) {
        memcpy(adr2, _slice[i], sizeof(bool)*_cols);
        adr2 += _cols;
      }
      _leftv->reduce((void*)adr2, bufferSize);
      unsigned char* adr3 = ((unsigned char*)adr2 + _leftv->getSize());
      _rightv->reduce((void*)adr3, bufferSize);
      return;
    } // end of method reduce
    
    /** Override */
    virtual void expand(void* pBuffer, int bufferSize) {
      unsigned int* adr1 = (unsigned int*) pBuffer;
      _n = *(adr1++);
      _pos = *(adr1++);
      _rows = *(adr1++);
      _cols = *(adr1++);
      bool* adr2 = (bool*) adr1;
      _slice = new bool*[_rows];
      for(int i=0; i < _rows; ++i) {
        _slice[i] = new bool[_cols];
        for(int j=0; j < _cols; ++j)
          _slice[i][j] = *(adr2++);
      }
      _leftv = new Vector();
      _leftv->expand((void*)adr2, bufferSize);
      unsigned char* adr3 = ((unsigned char*)adr2 + _leftv->getSize());
      _rightv = new Vector();
      _rightv->expand((void*)adr3, bufferSize);
      return;
    } // end of method expand

}; // end of class Block

/**
 * Operatore di output <<: stampa il blocco rappresentando con "[ ]" le celle
 * morte e con "[*]" le celle vive, separando i vettori con uno spazio rispetto
 * alle celle del blocco.
 */
std::ostream& operator<<(std::ostream& out, const Block& b) {
  for(unsigned int i = 0; i < b.getRows(); ++i) {
    out <<(b.getLeftVector().get(i) ? "[*] " : "[ ] ");
    for(unsigned int j = 0; j < b.getColumns(); ++j)
      out <<(b.get(i,j) ? "[*]" : "[ ]");
    out <<(b.getRightVector().get(i) ? " [*]" : " [ ]") <<std::endl;
  }
  return out;
} // end of function operator<<

} // end of namespace gameoflife


#endif // _BLOCK_H

