/*!
  \file Matrix.h
  \brief Implementazione della classe gameoflife::Matrix
  \author Andrea Zanelli
  \date 09-03-2010
*/

#ifndef _MATRIX_H
#define _MATRIX_H 1

#include <iostream>
#include <cstring>
#include <cmath>
#include "Block.h"


namespace gameoflife {

/*!
  \class Matrix
  \brief Matrice che rappresenta il Gioco della Vita.

  La classe rappresenta la matrice del gioco della vita attraverso un
  array bidimensionale di bool. La classe viene costruita passandogli le
  dimensioni della matrice (numero di righe e di colonne) e la densita', un 
  valore tra 0 e 1 che rappresenta la proporzione tra celle vive e celle morte 
  all'interno della matrice; le celle vengono quindi inizializzate in modo
  casuale tenendo conto della densità, assegnando il valore true alle celle 
  vive e false alle celle morte.
*/
class Matrix {

  // PRIVATE MEMBERS
  private:
    
    unsigned int _rows;
    unsigned int _cols;
    
    bool** _matrix;  // _matrix[_rows][_cols]

  // PUBLIC METHODS
  public:
    
    /**
     * Costruisce una matrice che rappresenta il gioco della vita di dimensione
     * rows*cols, senza inizializzare gli elementi.
     */
    Matrix(unsigned int rows, unsigned int cols) : _rows(rows), _cols(cols) { 
      _matrix = new bool*[_rows];
      for(int i=0; i < _rows; ++i)
        _matrix[i] = new bool[_cols];
      return;
    } // end of default constructor
    
    /**
     * Costruisce una matrice che rappresenta il gioco della vita di dimensione
     * rows*cols, popolandola in modo casuale con una densita' di popolazione
     * passata come parametro "density" (valore da 0 a 1).
     */
    Matrix(unsigned int rows, unsigned int cols, float density) :
        _rows(rows), _cols(cols) {
      _matrix = new bool*[_rows];
      if(density > 1) density = 1;
      srand(time(NULL));
      for(int i = 0; i < _rows; ++i) {
        _matrix[i] = new bool[_cols];
        for(int j = 0; j < _cols; ++j)
          _matrix[i][j] = (rand()/(float(RAND_MAX)+1)) < density;
      } // end for i
      return;
    } // end of constructor
    
    /**
     * Distruttore
     */
    ~Matrix() {
      for(int i=0; i < _rows; ++i)
        delete[] _matrix[i];
      delete[] _matrix;
      return;
    } // end of destructor
    
    /**
     * Restituisce il numero di righe della matrice.
     */
    unsigned int getRows() const {
      return _rows;
    } // end of method getRows();

    /**
     * Restituisce il numero di colonne della matrice.
     */
    unsigned int getColumns() const {
      return _cols;
    } // end of method getColumns();
    
    /**
     * Restituisce l'elemento nella riga i - colonna j.
     */
    bool get(unsigned int i, unsigned int j) {
      return &_matrix[i][j];
    } // end of method get
    
    /**
     * Restituisce l'elemento nella riga i - colonna j.
     */
    bool get(unsigned int i, unsigned int j) const {
      return _matrix[i][j];
    } // end of method get
    
    /**
     * Dato il parametro nBlocks (numero di blocchi in cui suddividere la 
     * matrice) restituisce l'i-esimo blocco della suddivisione (i da 0 a 
     * nBlocks - 1).
     */
    Block* getBlock(unsigned int nBlocks, unsigned int i) const {
      if(nBlocks > _cols) { nBlocks = _cols; }
      if(i > nBlocks - 1) { i = nBlocks - 1; }
      // Calcolo della dimensione del blocco
      int div = floor(_cols/nBlocks);
      int rest = _cols % nBlocks;
      int dim = ( i < rest ? div+1 : div );
      int pos = ( i < rest ? (div+1)*i : div*i + rest );
      // Costruisce e restituisce il blocco
      return new Block(i, dim, pos, _matrix, _rows, _cols);
    } // end of method getBlock
    
    /**
     * Inserisce gli elementi del blocco nella matrice, ricavando la posizione
     * del blocco nella matrice dal blocco stesso (con la funzione getPosition).
     */
    void setBlock(const Block* const block) {
      int pos = block->getPosition();
      for(int i=0; i < block->getRows(); ++i) {
        for(int j=0; j < block->getColumns(); ++j)
          _matrix[i][pos+j] = block->get(i,j);
      }
      return;
    } // end of method setBlock
    
}; // end of class Matrix

/**
 * Operatore di output <<: stampa la matrice rappresentando con "[ ]" le celle
 * morte e con "[*]" le celle vive.
 */
std::ostream& operator<<(std::ostream& out, const Matrix& m) {
  for(int i = 0; i < m.getRows(); ++i) {
    for(int j = 0; j < m.getColumns(); ++j)
      out <<(m.get(i,j) ? "[*]" : "[ ]");
    out <<std::endl;
  } // end for i
  return out;
} // end of function operator<<

} // end of namespace gameoflife


#endif // _MATRIX_H

