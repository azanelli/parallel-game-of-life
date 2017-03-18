// ************************************************************************
// *                                                                      *
// *           Part of the Draft Proposal of Skeleton library             *
// *                                                                      *
// *                   (c) Nov 2001 J. Striegnitz                         *
// *                                                                      *
// ************************************************************************

#ifndef CURRY_H
#define CURRY_H

#include<iostream>

template<typename T>
struct typeTraits {
	typedef T        id_t;
	typedef T        plain_t;
	typedef T*       pointer_t;
	typedef T&       reference_t;
	typedef const T  const_t;
	typedef const T* const_pointer_t;
	typedef const T& const_reference_t;
	typedef const T* const const_pointer_to_const_t;

	typedef const T& mkArg_t;
};

template<typename T>
struct typeTraits<T*> {
	typedef T*       id_t;
	typedef T        plain_t;
	typedef T*       pointer_t;
	typedef T&       reference_t;
	typedef const T  const_t;
	typedef const T* const_pointer_t;
	typedef const T& const_reference_t;
	typedef const T* const const_pointer_to_const_t;

	typedef T* mkArg_t;
};

template<typename T>
struct typeTraits<T&> {
	typedef T&       id_t;
	typedef T        plain_t;
	typedef T*       pointer_t;
	typedef T&       reference_t;
	typedef const T  const_t;
	typedef const T* const_pointer_t;
	typedef const T& const_reference_t;
	typedef const T* const const_pointer_to_const_t;

	typedef const T& mkArg_t;
};

template<typename T>
struct typeTraits<const T> {
	typedef const T  id_t;
	typedef T        plain_t;
	typedef T*       pointer_t;
	typedef T&       reference_t;
	typedef const T  const_t;
	typedef const T* const_pointer_t;
	typedef const T& const_reference_t;
	typedef const T* const const_pointer_to_const_t;

	typedef const T& mkArg_t;
};

template<typename T>
struct typeTraits<const T&> {
	typedef const T& id_t;
	typedef T        plain_t;
	typedef T*       pointer_t;
	typedef T&       reference_t;
	typedef const T  const_t;
	typedef const T* const_pointer_t;
	typedef const T& const_reference_t;
	typedef const T* const const_pointer_to_const_t;

	typedef const T& mkArg_t;
};

template<typename T>
struct typeTraits<const T*> {
	typedef const T*  id_t;
	typedef T         plain_t;
	typedef T*        pointer_t;
	typedef T&        reference_t;
	typedef const T   const_t;
	typedef const T*  const_pointer_t;
	typedef const T&  const_reference_t;
	typedef const T*  const const_pointer_to_const_t;

	typedef const T*  mkArg_t;
};

template<typename T>
struct curryArgMode {

	typedef typename typeTraits<T>::mkArg_t Type_t;	
	// If you don't want to enforce constant references, you may use
	// the following definition instead:
	// typedef T Type_t;

};

template<typename R, typename F>
class Fct0 {

	F f;

public:

	// Constructor, copy constructor and destructor
	Fct0(const F& _f): f(_f) {
	}

	Fct0(const Fct0& rhs): f(rhs.f) {
	}

	~Fct0() {
	}

	inline operator R() const {
		return f();
	}
	// Full application -> delegate to instance of F

	inline R operator()() const {
		return f();
	}
	// Support for partial application

};

// curry functions
template<typename R>
inline Fct0<R, R(*)()> curry(R(*f)()) {
	return Fct0<R, R(*)()>(f);
}

// stream inserters
template<typename R, typename F>
inline std::ostream& operator<<(std::ostream& os, const Fct0<R, F>&) {
	os << "0-ary function";
	return os;
}

template<typename A0, typename R, typename F>
class Fct1 {

	F f;

public:

	// Constructor, copy constructor and destructor
	Fct1(const F& _f): f(_f) {
	}

	Fct1(const Fct1& rhs): f(rhs.f) {
	}

	~Fct1() {
	}

	// Closures
	struct closureT {

		F op;
		A0 pa0;

		closureT(const F& _op, typename curryArgMode<A0>::Type_t _pa0):
		op(_op), pa0(_pa0) {
		}

		closureT(const closureT& rhs):
		op(rhs.op), pa0(rhs.pa0) {
		}

		inline R operator()() const {
			return op(pa0);
		}

	};

	typedef Fct0<R, closureT> closure_t;

	inline closure_t closure(typename curryArgMode<A0>::Type_t a0) const {
		return closure_t(closureT(f, a0));
	}
	// Full application -> delegate to instance of F

	inline R operator()(typename curryArgMode<A0>::Type_t a0) const {
		return f(a0);
	}
	// Support for partial application

};

// curry functions
template<typename A0, typename R>
inline Fct1<A0, R, R(*)(A0)>curry(R(*f)(A0)) {
	return Fct1<A0, R, R(*)(A0)>(f);
}

// stream inserters
template<typename A0, typename R, typename F>
inline std::ostream& operator<<(std::ostream& os, const Fct1<A0, R, F>&) {
	os << "1-ary function";
	return os;
}
template<typename A0, typename A1, typename R, typename F>
class Fct2 {

	F f;

public:	

	// Constructor, copy constructor and destructor
	Fct2(const F& _f): f(_f) {
	}

	Fct2(const Fct2& rhs): f(rhs.f) {
	}

	~Fct2() {
	}

	// Closures
	struct closureT {

		F op;
		A0 pa0;
		A1 pa1;	

		closureT(const F& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		op(_op), pa0(_pa0), pa1(_pa1) {
		}

		closureT(const closureT& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()() const {
			return op(pa0, pa1);
		}

	};

	typedef Fct0<R, closureT> closure_t;

	inline closure_t closure(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return closure_t(closureT(f, a0, a1));
	}

	// Full application -> delegate to instance of F
	inline R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return f(a0, a1);
	}

	// Support for partial application
	// F applied to 1 arguments
	struct PartialAppl1 {

		F f;
		A0 pa0;

		PartialAppl1(const F& _f, typename curryArgMode<A0>::Type_t _pa0):
		f(_f), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		f(rhs.f), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1) const {
			return f(pa0, a1);
		}

	};

	typedef Fct1<A1, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(f, a0));
	}
};

// curry functions
template<typename A0, typename A1, typename R>
inline Fct2<A0, A1, R, R(*)(A0, A1)> curry(R(*f)(A0, A1)) {
	return Fct2<A0, A1, R, R(*)(A0, A1)>(f);
}

// stream inserters
template<typename A0, typename A1, typename R, typename F>
inline std::ostream& operator<<(std::ostream& os, const Fct2<A0, A1, R, F> &) {
	os << "2-ary function";
	return os;
}

template<typename A0, typename A1, typename A2, typename R, typename F>
class Fct3 {

	F f;

public:

	// Constructor, copy constructor and destructor
	Fct3(const F& _f): f(_f) {
	}

	Fct3(const Fct3& rhs): f(rhs.f) {
	}

	~Fct3() {
	}

	// Closures
	struct closureT {

		F op;
		A0 pa0;
		A1 pa1;
		A2 pa2;	

		closureT(const F& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2) {
		}

		closureT(const closureT& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2) {
		}

		inline R operator()() const {
			return op(pa0, pa1, pa2);
		}

	};

	typedef Fct0<R, closureT> closure_t;

	inline closure_t closure(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return closure_t(closureT(f, a0, a1, a2));
	}

	// Full application -> delegate to instance of F
	inline R operator()(typename curryArgMode<A0>::Type_t a0, typename curryArgMode<A1>::Type_t a1,
		typename curryArgMode<A2>::Type_t a2) const {
			return f(a0, a1, a2);
	}

	// Support for partial application
	// F applied to 1 arguments
	struct PartialAppl1 {

		F f;
		A0 pa0;

		PartialAppl1(const F& _f, typename curryArgMode<A0>::Type_t _pa0):
		f(_f), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		f(rhs.f), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1,
			typename curryArgMode<A2>::Type_t a2) const {
			return f(pa0, a1, a2);
		}

	};

	typedef Fct2<A1, A2, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(f, a0));
	}

	// F applied to 2 arguments
	struct PartialAppl2 {

		F f;
		A0 pa0;
		A1 pa1;

		PartialAppl2(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		f(_f), pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2) const {
			return f(pa0, pa1, a2);
		}

	};

	typedef Fct1<A2, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return ffunc2_t(PartialAppl2(f, a0, a1));
	}

};

// curry functions
template<typename A0, typename A1, typename A2, typename R>
inline Fct3<A0, A1, A2, R, R(*)(A0, A1, A2)> curry(R(*f)(A0, A1, A2)) {
	return Fct3<A0, A1, A2, R, R(*)(A0, A1, A2)>(f);
}

// stream inserters
template<typename A0, typename A1, typename A2, typename R, typename F>
inline std::ostream& operator<<(std::ostream& os, const Fct3<A0, A1, A2, R, F> &) {
	os << "3-ary function";
	return os;
}

template<typename A0, typename A1, typename A2, typename A3, typename R, typename F>
class Fct4 {

	F f;

public:	

	// Constructor, copy constructor and destructor
	Fct4(const F& _f): f(_f) {
	}

	Fct4(const Fct4& rhs): f(rhs.f) {
	}

	~Fct4() {
	}

	// Closures
	struct closureT {

		F op;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;

		closureT(const F& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2,
			typename curryArgMode<A3>::Type_t _pa3):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3) {
		}

		closureT(const closureT& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3) {
		}

		inline R operator()() const {
			return op(pa0, pa1, pa2, pa3);
		}
	};

	typedef Fct0<R, closureT> closure_t;

	inline closure_t closure(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const {
		return closure_t(closureT(f, a0, a1, a2, a3));
	}

	// Full application -> delegate to instance of F
	inline R operator()(typename curryArgMode<A0>::Type_t a0, typename curryArgMode<A1>::Type_t a1,
		typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3) const {
		return f(a0, a1, a2, a3);
	}

	// Support for partial application
	// F applied to 1 arguments
	struct PartialAppl1 {

		F f;
		A0 pa0;

		PartialAppl1(const F& _f, typename curryArgMode<A0>::Type_t _pa0):
		f(_f), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		f(rhs.f), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1,
			typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3) const {
			return f(pa0, a1, a2, a3);
		}
	};

	typedef Fct3<A1, A2, A3, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(f, a0));
	}

	// F applied to 2 arguments
	struct PartialAppl2 {

		F f;
		A0 pa0;
		A1 pa1;

		PartialAppl2(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		f(_f), pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2,
			typename curryArgMode<A3>::Type_t a3) const {
			return f(pa0, pa1, a2, a3);
		}
	};

	typedef Fct2<A2, A3, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return ffunc2_t(PartialAppl2(f, a0, a1));
	}

	// F applied to 3 arguments
	struct PartialAppl3 {

		F f;
		A0 pa0;
		A1 pa1;
		A2 pa2;

		PartialAppl3(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2):
		f(_f), pa0(_pa0), pa1(_pa1), pa2(_pa2) {
		}

		PartialAppl3(const PartialAppl3& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2) {
		}

		inline R operator()(typename curryArgMode<A3>::Type_t a3) const {
			return f(pa0, pa1, pa2, a3);
		}
	};

	typedef Fct1<A3, R, PartialAppl3> ffunc3_t;

	inline ffunc3_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return ffunc3_t(PartialAppl3(f, a0, a1, a2));
	}
};

// curry functions
template<typename A0, typename A1, typename A2, typename A3, typename R>
inline Fct4<A0, A1, A2, A3, R, R(*)(A0, A1, A2, A3)> curry(R(*f)(A0, A1, A2, A3)) {
	return Fct4<A0, A1, A2, A3, R, R(*)(A0, A1, A2, A3)>(f);
}

// stream inserters
template<typename A0, typename A1, typename A2, typename A3, typename R, typename F>
inline std::ostream& operator<<(std::ostream& os, const Fct4<A0, A1, A2, A3, R, F> &) {
	os << "4-ary function";
	return os;
}

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename R, typename F>
class Fct5 {

	F f;

public:

	// Constructor, copy constructor and destructor
	Fct5(const F& _f): f(_f) {
	}

	Fct5(const Fct5& rhs): f(rhs.f) {
	}

	~Fct5() {
	}

	// Closures
	struct closureT {

		F op;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;
		A4 pa4;

		closureT(const F& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2,
			typename curryArgMode<A3>::Type_t _pa3, typename curryArgMode<A4>::Type_t _pa4):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3), pa4(_pa4) {
		}

		closureT(const closureT& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3), pa4(rhs.pa4) {
		}

		inline R operator()() const {
			return op(pa0, pa1, pa2, pa3, pa4);
		}
	};

	typedef Fct0<R, closureT> closure_t;

	inline closure_t closure(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const {
		return closure_t(closureT(f, a0, a1, a2, a3, a4));
	}

	// Full application -> delegate to instance of F
	inline R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const {
			return f(a0, a1, a2, a3, a4);
	}

	// Support for partial application
	// F applied to 1 arguments
	struct PartialAppl1 {

		F f;
		A0 pa0;

		PartialAppl1(const F& _f, typename curryArgMode<A0>::Type_t _pa0):
		f(_f), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		f(rhs.f), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1,
			typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4) const {
			return f(pa0, a1, a2, a3, a4);
		}
	};

	typedef Fct4<A1, A2, A3, A4, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(f, a0));
	}

	// F applied to 2 arguments
	struct PartialAppl2 {

		F f;
		A0 pa0;
		A1 pa1;

		PartialAppl2(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		f(_f), pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2,
			typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const {
			return f(pa0, pa1, a2, a3, a4);
		}
	};

	typedef Fct3<A2, A3, A4, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return ffunc2_t(PartialAppl2(f, a0, a1));
	}

	// F applied to 3 arguments
	struct PartialAppl3 {

		F f;
		A0 pa0;
		A1 pa1;
		A2 pa2;

		PartialAppl3(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2):
		f(_f), pa0(_pa0), pa1(_pa1), pa2(_pa2) {
		}

		PartialAppl3(const PartialAppl3& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2) {
		}

		inline R operator()(typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4) const {
			return f(pa0, pa1, pa2, a3, a4);
		}
	};

	typedef Fct2<A3, A4, R, PartialAppl3> ffunc3_t;

	inline ffunc3_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
			return ffunc3_t(PartialAppl3(f, a0, a1, a2));
	}

	// F applied to 4 arguments
	struct PartialAppl4 {

		F f;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;

		PartialAppl4(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2,
			typename curryArgMode<A3>::Type_t _pa3):
		f(_f), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3) {
		}

		PartialAppl4(const PartialAppl4& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3) {
		}

		inline R operator()(typename curryArgMode<A4>::Type_t a4) const {
			return f(pa0, pa1, pa2, pa3, a4);
		}
	};

	typedef Fct1<A4, R, PartialAppl4> ffunc4_t;

	inline ffunc4_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const {
		return ffunc4_t(PartialAppl4(f, a0, a1, a2, a3));
	}
};

// curry functions
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename R>
inline Fct5<A0, A1, A2, A3, A4, R, R(*)(A0, A1, A2, A3, A4)> curry(R(*f)(A0, A1, A2, A3, A4)) {
	return Fct5<A0, A1, A2, A3, A4, R, R(*)(A0, A1, A2, A3, A4)>(f);
}

// stream inserters
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename R, typename F>
inline std::ostream& operator<<(std::ostream& os, const Fct5<A0, A1, A2, A3, A4, R, F> &) {
	os << "5-ary function";
	return os;
}

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R, typename F>
class Fct6 {

	F f;

public:

	// Constructor, copy constructor and destructor
	Fct6(const F& _f): f(_f) {
	}

	Fct6(const Fct6& rhs): f(rhs.f) {
	}

	~Fct6() {
	}

	// Closures
	struct closureT {

		F op;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;
		A4 pa4;
		A5 pa5;

		closureT(const F& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2,
			typename curryArgMode<A3>::Type_t _pa3, typename curryArgMode<A4>::Type_t _pa4,
			typename curryArgMode<A5>::Type_t _pa5):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3), pa4(_pa4), pa5(_pa5) {
		}

		closureT(const closureT& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3), pa4(rhs.pa4), pa5(rhs.pa5) {
		}

		inline R operator()() const {
			return op(pa0, pa1, pa2, pa3, pa4, pa5);
		}
	};

	typedef Fct0<R, closureT> closure_t;

	inline closure_t closure(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4,
		typename curryArgMode<A5>::Type_t a5) const {
		return closure_t(closureT(f, a0, a1, a2, a3, a4, a5));
	}

	// Full application -> delegate to instance of F
	inline R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4,
		typename curryArgMode<A5>::Type_t a5) const {
		return f(a0, a1, a2, a3, a4, a5);
	}

	// Support for partial application
	// F applied to 1 arguments
	struct PartialAppl1 {

		F f;
		A0 pa0;

		PartialAppl1(const F& _f, typename curryArgMode<A0>::Type_t _pa0):
		f(_f), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		f(rhs.f), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1,
			typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4, typename curryArgMode<A5>::Type_t a5) const {
			return f(pa0, a1, a2, a3, a4, a5);
		}
	};

	typedef Fct5<A1, A2, A3, A4, A5, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(f, a0));
	}

	// F applied to 2 arguments
	struct PartialAppl2 {

		F f;
		A0 pa0;
		A1 pa1;

		PartialAppl2(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		f(_f) ,pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2,
			typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4,
			typename curryArgMode<A5>::Type_t a5) const {
			return f(pa0, pa1, a2, a3, a4, a5);
		}
	};

	typedef Fct4<A2, A3, A4, A5, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
			return ffunc2_t(PartialAppl2(f, a0, a1));
	}

	// F applied to 3 arguments
	struct PartialAppl3 {

		F f;
		A0 pa0;
		A1 pa1;
		A2 pa2;

		PartialAppl3(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2):
		f(_f), pa0(_pa0), pa1(_pa1), pa2(_pa2) {
		}

		PartialAppl3(const PartialAppl3& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2) {
		}

		inline R operator()(typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4, typename curryArgMode<A5>::Type_t a5) const {
			return f(pa0, pa1, pa2, a3, a4, a5);
		}
	};

	typedef Fct3<A3, A4, A5, R, PartialAppl3> ffunc3_t;

	inline ffunc3_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return ffunc3_t(PartialAppl3(f, a0, a1, a2));
	}

	// F applied to 4 arguments
	struct PartialAppl4 {

		F f;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;

		PartialAppl4(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2,
			typename curryArgMode<A3>::Type_t _pa3):
		f(_f), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3) {
		}

		PartialAppl4(const PartialAppl4& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3) {
		}

		inline R operator()(typename curryArgMode<A4>::Type_t a4,
			typename curryArgMode<A5>::Type_t a5) const {
			return f(pa0, pa1, pa2, pa3, a4, a5);
		}
	};

	typedef Fct2<A4, A5, R, PartialAppl4> ffunc4_t;

	inline ffunc4_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const {
		return ffunc4_t(PartialAppl4(f, a0, a1, a2, a3));
	}

	// F applied to 5 arguments
	struct PartialAppl5 {

		F f;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;
		A4 pa4;

		PartialAppl5(const F& _f, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2,
			typename curryArgMode<A3>::Type_t _pa3, typename curryArgMode<A4>::Type_t _pa4):
		f(_f), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3), pa4(_pa4) {
		}

		PartialAppl5(const PartialAppl5& rhs):
		f(rhs.f), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3), pa4(rhs.pa4) {
		}

		inline R operator()(typename curryArgMode<A5>::Type_t a5) const {
			return f(pa0, pa1, pa2, pa3, pa4, a5);
		}
	};

	typedef Fct1<A5, R, PartialAppl5> ffunc5_t;

	inline ffunc5_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const {
		return ffunc5_t(PartialAppl5(f, a0, a1, a2, a3, a4));
	}
};

// curry functions
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R>
inline Fct6<A0, A1, A2, A3, A4, A5, R, R(*)(A0, A1, A2, A3, A4, A5)> curry(R(*f)(A0, A1, A2, A3, A4, A5)) {
	return Fct6<A0, A1, A2, A3, A4, A5, R, R(*)(A0, A1, A2, A3, A4, A5)>(f);
}

// stream inserters
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R, typename F>
inline std::ostream& operator<<(std::ostream& os, const Fct6<A0, A1, A2, A3, A4, A5, R, F> &) {
	os << "6-ary function";
	return os;
}

template<typename R>
struct anyOp0 {

	anyOp0() {
	}

	anyOp0(const anyOp0& rhs) {
	}

	inline virtual R operator()() const = 0;

};

template<typename R, typename F>
struct anyFunc0: public anyOp0<R> {

	typedef Fct0<R, F> ffunc_t;
	ffunc_t f;
	
	anyFunc0(const ffunc_t& _f): f(_f) {
	}

	anyFunc0(const anyFunc0& rhs): f(rhs.f) {
	}

	inline virtual R operator()() const {
		return f();
	}

};

template<typename R>
struct DFct0 {

	int* refCount;
	anyOp0<R>* op;

	template<typename F>
	DFct0(const Fct0<R, F>& f) {
		op = new anyFunc0<R, F>(f);
		refCount = new int(1);
	}

	DFct0(const DFct0& rhs):
	refCount(rhs.refCount), op(rhs.op) {
		(*refCount)++;
	}

	~DFct0() {
		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}
	}

	DFct0<R> operator=(const DFct0<R>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = rhs.op;
		refCount = rhs.refCount;
		(*refCount)++;
		return *this;
	}

	template<typename F>
	DFct0<R> operator=(const Fct0<R, F>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = new anyFunc0<R, F>(rhs);	// error: f was not declared; f = rhs?
		refCount = new int(1);
		(*refCount)++;
		return *this;
	}

	inline R operator()() const {
		return(*op)();
	}

};

template<typename A0, typename R>
struct anyOp1 {

	anyOp1() {
	}

	anyOp1(const anyOp1& rhs) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0) const = 0;

};

template<typename A0, typename R, typename F>
struct anyFunc1: public anyOp1<A0, R> {

	typedef Fct1<A0, R, F> ffunc_t;
	ffunc_t f;
	
	anyFunc1(const ffunc_t& _f): f(_f) {
	}

	anyFunc1(const anyFunc1& rhs): f(rhs.f) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0) const {
		return f(a0);
	}

};

template<typename A0, typename R>
struct DFct1 {

	int* refCount;
	anyOp1<A0, R>* op;

	template<typename F>
	DFct1(const Fct1<A0, R, F>& f) {
		op = new anyFunc1<A0, R, F>(f);
		refCount = new int(1);
	}

	DFct1(const DFct1& rhs):
	refCount(rhs.refCount), op(rhs.op) {
		(*refCount)++;
	}

	~DFct1() {
		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}
	}

	DFct1<A0, R> operator=(const DFct1<A0, R>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = rhs.op;
		refCount = rhs.refCount;
		(*refCount)++;
		return *this;
	}

	template<typename F>
	DFct1<A0, R> operator=(const Fct1<A0, R, F>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = new anyFunc1<A0, R, F>(rhs);	// f = rhs?
		refCount = new int(1);
		(*refCount)++;
		return *this;
	}

	inline R operator()(typename curryArgMode<A0>::Type_t a0) const {
		return(*op)(a0);
	}


};
template<typename A0, typename A1, typename R>
struct anyOp2 {

	anyOp2() {
	}

	anyOp2(const anyOp2& rhs) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const = 0;

};

template<typename A0, typename A1, typename R, typename F>
struct anyFunc2: public anyOp2<A0, A1, R> {

	typedef Fct2<A0, A1, R, F> ffunc_t;
	ffunc_t f;

	anyFunc2(const ffunc_t& _f): f(_f) {
	}

	anyFunc2(const anyFunc2& rhs): f(rhs.f) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return f(a0, a1);
	}

};

template<typename A0, typename A1, typename R>
struct DFct2 {

	int* refCount;
	anyOp2<A0, A1, R>* op;

	template<typename F>
	DFct2(const Fct2<A0, A1, R, F>& f) {
		op = new anyFunc2<A0, A1, R, F>(f);
		refCount = new int(1);
	}

	DFct2(const DFct2& rhs):
	refCount(rhs.refCount), op(rhs.op) {
		(*refCount)++;
	}

	~DFct2() {
		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}
	}

	DFct2<A0, A1, R> operator=(const DFct2<A0, A1, R>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = rhs.op;
		refCount = rhs.refCount;
		(*refCount)++;
		return *this;
	}

	template<typename F>
	DFct2<A0, A1, R> operator=(const Fct2<A0, A1, R, F>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = new anyFunc2<A0, A1, R, F>(rhs);	// f = rhs?
		refCount = new int(1);
		(*refCount)++;
		return *this;
	}

	inline R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return(*op)(a0, a1);
	}

	struct PartialAppl1 { // F applied to 1 arguments
		
		anyOp2<A0, A1, R> op;
		A0 pa0;
		
		// _op = op?
		PartialAppl1(const anyOp2<A0, A1, R>& _op, typename curryArgMode<A0>::Type_t _pa0):
		op(_op), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		op(rhs.op), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1) const {
			return(*op)(pa0, a1);
		}

	};

	typedef Fct1<A1, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(*op, a0));
	}

};

template<typename A0, typename A1, typename A2, typename R>
struct anyOp3 {

	anyOp3() {
	}

	anyOp3(const anyOp3& rhs) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const = 0;

};

template<typename A0, typename A1, typename A2, typename R, typename F>
struct anyFunc3: public anyOp3<A0, A1, A2, R> {

	typedef Fct3<A0, A1, A2, R, F> ffunc_t;
	ffunc_t f;

	anyFunc3(const ffunc_t& _f): f(_f) {
	}

	anyFunc3(const anyFunc3& rhs): f(rhs.f) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return f(a0, a1, a2);
	}

};

template<typename A0, typename A1, typename A2, typename R>
struct DFct3 {

	int* refCount;
	anyOp3<A0, A1, A2, R>* op;

	template<typename F>
	DFct3(const Fct3<A0, A1, A2, R, F>& f) {
		op = new anyFunc3<A0, A1, A2, R, F>(f);
		refCount = new int(1);
	}

	DFct3(const DFct3& rhs):
	refCount(rhs.refCount), op(rhs.op) {
		(*refCount)++;
	}

	~DFct3() {
		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}
	}

	DFct3<A0, A1, A2, R> operator=(const DFct3<A0, A1, A2, R>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = rhs.op;
		refCount = rhs.refCount;
		(*refCount)++;
		return *this;
	}

	template<typename F>
	DFct3<A0, A1, A2, R> operator=(const Fct3<A0, A1, A2, R, F>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = new anyFunc3<A0, A1, A2, R, F>(rhs);	// f = rhs?
		refCount = new int(1);
		(*refCount)++;
		return *this;
	}

	inline R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return(*op)(a0, a1, a2);
	}

	// F applied to 1 arguments
	struct PartialAppl1 {

		anyOp3<A0, A1, A2, R> op;
		A0 pa0;

		// op = _op?
		PartialAppl1(const anyOp3<A0, A1, A2, R>& _op, typename curryArgMode<A0>::Type_t _pa0):
		op(_op), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		op(rhs.op), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1,
			typename curryArgMode<A2>::Type_t a2) const {
			return(*op)(pa0, a1, a2);
		}

	};

	typedef Fct2<A1, A2, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(*op, a0));
	}

	// F applied to 2 arguments
	struct PartialAppl2 {

		anyOp3<A0, A1, A2, R> op;
		A0 pa0;
		A1 pa1;

		// op = _op?
		PartialAppl2(const anyOp3<A0, A1, A2, R>& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		op(_op), pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2) const {
			return(*op)(pa0, pa1, a2);
		}			
	}; 
	
	typedef Fct1<A2, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return ffunc2_t(PartialAppl2(*op, a0, a1));
	}

};

template<typename A0, typename A1, typename A2, typename A3, typename R>
struct anyOp4 {

	anyOp4() {
	}

	anyOp4(const anyOp4& rhs) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const = 0;

};

template<typename A0, typename A1, typename A2, typename A3, typename R, typename F>
struct anyFunc4: public anyOp4<A0, A1, A2, A3, R> {

	typedef Fct4<A0, A1, A2, A3, R, F> ffunc_t;
	ffunc_t f;

	anyFunc4(const ffunc_t& _f): f(_f) {
	}

	anyFunc4(const anyFunc4& rhs): f(rhs.f) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const {
		return f(a0, a1, a2, a3);
	}

};

template<typename A0, typename A1, typename A2, typename A3, typename R>
struct DFct4 {

	int* refCount;
	anyOp4<A0, A1, A2, A3, R>* op;

	template<typename F>
	DFct4(const Fct4<A0, A1, A2, A3, R, F>& f) {
		op = new anyFunc4<A0, A1, A2, A3, R, F>(f);
		refCount = new int(1);
	}

	DFct4(const DFct4& rhs):
	refCount(rhs.refCount), op(rhs.op) {
		(*refCount)++;
	}

	~DFct4() {
		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}
	}

	DFct4<A0, A1, A2, A3, R> operator=(const DFct4<A0, A1, A2, A3, R>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = rhs.op;
		refCount = rhs.refCount;
		(*refCount)++;
		return *this;
	}

	template<typename F>
	DFct4<A0, A1, A2, A3, R> operator=(const Fct4<A0, A1, A2, A3, R, F>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = new anyFunc4<A0, A1, A2, A3, R, F>(rhs);	// f = rhs?
		refCount = new int(1);
		(*refCount)++;
		return *this;
	}

	inline R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const {
		return(*op)(a0, a1, a2, a3);
	}

	// F applied to 1 arguments
	struct PartialAppl1 {

		anyOp4<A0, A1, A2, A3, R> op;
		A0 pa0;

		// op = _op?
		PartialAppl1(const anyOp4<A0, A1, A2, A3, R>& _op, typename curryArgMode<A0>::Type_t _pa0):
		op(_op), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		op(rhs.op), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
			typename curryArgMode<A3>::Type_t a3) const {
			return(*op)(pa0, a1, a2, a3);
		}

	};

	typedef Fct3<A1, A2, A3, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(*op, a0));
	}

	struct PartialAppl2 { // F applied to 2 arguments

		anyOp4<A0, A1, A2, A3, R> op;
		A0 pa0;
		A1 pa1;

		// op = _op?
		PartialAppl2(const anyOp4<A0, A1, A2, A3, R>& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		op(_op), pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2,
			typename curryArgMode<A3>::Type_t a3) const {
			return(*op)(pa0, pa1, a2, a3);
		}

	};

	typedef Fct2<A2, A3, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return ffunc2_t(PartialAppl2(*op, a0, a1));
	}

	// F applied to 3 arguments
	struct PartialAppl3 {

		anyOp4<A0, A1, A2, A3, R> op;
		A0 pa0;
		A1 pa1;
		A2 pa2;

		// op = _op?
		PartialAppl3(const anyOp4<A0, A1, A2, A3, R>& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2) {
		}

		PartialAppl3(const PartialAppl3& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2) {
		}

		inline R operator()(typename curryArgMode<A3>::Type_t a3) const {
			return(*op)(pa0, pa1, pa2, a3);
		}

	};

	typedef Fct1<A3, R, PartialAppl3> ffunc3_t;
	
	inline ffunc3_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return ffunc3_t(PartialAppl3(*op, a0, a1, a2));
	}

};

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename R>
struct anyOp5 {

	anyOp5() {
	}

	anyOp5(const anyOp5& rhs) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const = 0;

};

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename R, typename F>
struct anyFunc5: public anyOp5<A0, A1, A2, A3, A4, R> {

	typedef Fct5<A0, A1, A2, A3, A4, R, F> ffunc_t;
	ffunc_t f;

	anyFunc5(const ffunc_t& _f): f(_f) {
	}

	anyFunc5(const anyFunc5& rhs): f(rhs.f) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const {
		return f(a0, a1, a2, a3, a4);
	}

};

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename R>
struct DFct5 {

	int* refCount;
	anyOp5<A0, A1, A2, A3, A4, R>* op;

	template<typename F>
	DFct5(const Fct5<A0, A1, A2, A3, A4, R, F>& f) {
		op = new anyFunc5<A0, A1, A2, A3, A4, R, F>(f);
		refCount = new int(1);
	}

	DFct5(const DFct5& rhs):
	refCount(rhs.refCount), op(rhs.op) {
		(*refCount)++;
	}

	~DFct5() {
		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}
	}
	DFct5<A0, A1, A2, A3, A4, R> operator=(const DFct5<A0, A1, A2, A3, A4, R>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = rhs.op;
		refCount = rhs.refCount;
		(*refCount)++;
		return *this;
	}

	template<typename F>
	DFct5<A0, A1, A2, A3, A4, R> operator=(const Fct5<A0, A1, A2, A3, A4, R, F>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = new anyFunc5<A0, A1, A2, A3, A4, R, F>(rhs);	// f = rhs?
		refCount = new int(1);
		(*refCount)++;
		return *this;
	}

	inline R operator()(typename curryArgMode<A0>::Type_t a0, typename curryArgMode<A1>::Type_t a1,
		typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3,
		typename curryArgMode<A4>::Type_t a4) const {
		return(*op)(a0, a1, a2, a3, a4);
	}

	// F applied to 1 arguments
	struct PartialAppl1 {

		anyOp5<A0, A1, A2, A3, A4, R> op;
		A0 pa0;

		// op = _op?
		PartialAppl1(const anyOp5<A0, A1, A2, A3, A4, R>& _op,
			typename curryArgMode<A0>::Type_t _pa0):
		op(_op), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		op(rhs.op), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
			typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const {
			return(*op)(pa0, a1, a2, a3, a4);
		}

	};

	typedef Fct4<A1, A2, A3, A4, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(*op, a0));
	}

	// F applied to 2 arguments
	struct PartialAppl2 {

		anyOp5<A0, A1, A2, A3, A4, R> op;
		A0 pa0;
		A1 pa1;

		// op = _op?
		PartialAppl2(const anyOp5<A0, A1, A2, A3, A4, R>& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1):
		op(_op), pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4) const {
			return(*op)(pa0, pa1, a2, a3, a4);
		}

	};

	typedef Fct3<A2, A3, A4, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return ffunc2_t(PartialAppl2(*op, a0, a1));
	}

	// F applied to 3 arguments
	struct PartialAppl3 {

		anyOp5<A0, A1, A2, A3, A4, R> op;
		A0 pa0;
		A1 pa1;
		A2 pa2;

		// op = _op?
		PartialAppl3(const anyOp5<A0, A1, A2, A3, A4, R>& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2) {
		}

		PartialAppl3(const PartialAppl3& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2) {
		}

		inline R operator()(typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4) const {
			return(*op)(pa0, pa1, pa2, a3, a4);
		}

	};

	typedef Fct2<A3, A4, R, PartialAppl3> ffunc3_t;

	inline ffunc3_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return ffunc3_t(PartialAppl3(*op, a0, a1, a2));
	}

	// F applied to 4 arguments
	struct PartialAppl4 {

		anyOp5<A0, A1, A2, A3, A4, R> op;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;

		// op = _op?
		PartialAppl4(const anyOp5<A0, A1, A2, A3, A4, R>& _op, typename curryArgMode<A0>::Type_t _pa0,
			typename curryArgMode<A1>::Type_t _pa1, typename curryArgMode<A2>::Type_t _pa2,
			typename curryArgMode<A3>::Type_t _pa3):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3) {
		}

		PartialAppl4(const PartialAppl4& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3) {
		}

		inline R operator()(typename curryArgMode<A4>::Type_t a4) const {
			return(*op)(pa0, pa1, pa2, pa3, a4);
		}
	};

	typedef Fct1<A4, R, PartialAppl4> ffunc4_t;

	inline ffunc4_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const {
		return ffunc4_t(PartialAppl4(*op, a0, a1, a2, a3));
	}

};

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R>
struct anyOp6 {

	anyOp6() {
	}

	anyOp6(const anyOp6& rhs) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4,
		typename curryArgMode<A5>::Type_t a5) const = 0;

};

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R, typename F>
struct anyFunc6: public anyOp6<A0, A1, A2, A3, A4, A5, R> {

	typedef Fct6<A0, A1, A2, A3, A4, A5, R, F> ffunc_t;
	ffunc_t f;

	anyFunc6(const ffunc_t& _f): f(_f) {
	}

	anyFunc6(const anyFunc6& rhs): f(rhs.f) {
	}

	inline virtual R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4,
		typename curryArgMode<A5>::Type_t a5) const {
		return f(a0, a1, a2, a3, a4, a5);
	}

};

template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R>
struct DFct6 {

	int* refCount;
	anyOp6<A0, A1, A2, A3, A4, A5, R>* op;

	template<typename F>
	DFct6(const Fct6<A0, A1, A2, A3, A4, A5, R, F>& f) {
		op = new anyFunc6<A0, A1, A2, A3, A4, A5, R, F>(f);
		refCount = new int(1);
	}

	DFct6(const DFct6& rhs):
	refCount(rhs.refCount), op(rhs.op) {
		(*refCount)++;
	}

	~DFct6() {
		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}
	}

	DFct6<A0, A1, A2, A3, A4, A5, R> operator=(const DFct6<A0, A1, A2, A3, A4, A5, R>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = rhs.op;
		refCount = rhs.refCount;
		(*refCount)++;
		return *this;
	}

	template<typename F>
	DFct6<A0, A1, A2, A3, A4, A5, R> operator=(const Fct6<A0, A1, A2, A3, A4, A5, R, F>& rhs) {
		if(rhs.op == op)
			return *this;

		if(--(*refCount) == 0) {
			delete refCount;
			delete op;
		}

		op = new anyFunc6<A0, A1, A2, A3, A4, A5, R, F>(rhs);	// f = rhs?
		refCount = new int(1);
		(*refCount)++;
		return *this;
	}

	inline R operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4,
		typename curryArgMode<A5>::Type_t a5) const {
		return(*op)(a0, a1, a2, a3, a4, a5);
	}

	// F applied to 1 arguments
	struct PartialAppl1 {

		anyOp6<A0, A1, A2, A3, A4, A5, R> op;
		A0 pa0;

		// op = _op?
		PartialAppl1(const anyOp6<A0, A1, A2, A3, A4, A5, R>& _op,
			typename curryArgMode<A0>::Type_t _pa0):
		op(_op), pa0(_pa0) {
		}

		PartialAppl1(const PartialAppl1& rhs):
		op(rhs.op), pa0(rhs.pa0) {
		}

		inline R operator()(typename curryArgMode<A1>::Type_t a1,
			typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4, typename curryArgMode<A5>::Type_t a5) const {
			return(*op)(pa0, a1, a2, a3, a4, a5);
		}

	};

	typedef Fct5<A1, A2, A3, A4, A5, R, PartialAppl1> ffunc1_t;

	inline ffunc1_t operator()(typename curryArgMode<A0>::Type_t a0) const {
		return ffunc1_t(PartialAppl1(*op, a0));
	}

	// F applied to 2 arguments
	struct PartialAppl2 {

		anyOp6<A0, A1, A2, A3, A4, A5, R> op;
		A0 pa0;
		A1 pa1;

		// op = _op?
		PartialAppl2(const anyOp6<A0, A1, A2, A3, A4, A5, R>& _op,
			typename curryArgMode<A0>::Type_t _pa0, typename curryArgMode<A1>::Type_t _pa1):
		op(_op), pa0(_pa0), pa1(_pa1) {
		}

		PartialAppl2(const PartialAppl2& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1) {
		}

		inline R operator()(typename curryArgMode<A2>::Type_t a2, typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4, typename curryArgMode<A5>::Type_t a5) const {
			return(*op)(pa0, pa1, a2, a3, a4, a5);
		}

	};

	typedef Fct4<A2, A3, A4, A5, R, PartialAppl2> ffunc2_t;

	inline ffunc2_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1) const {
		return ffunc2_t(PartialAppl2(*op, a0, a1));
	}

	// F applied to 3 arguments
	struct PartialAppl3 {

		anyOp6<A0, A1, A2, A3, A4, A5, R> op;
		A0 pa0;
		A1 pa1;
		A2 pa2;

		// op = _op?
		PartialAppl3(const anyOp6<A0, A1, A2, A3, A4, A5, R>& _op,
			typename curryArgMode<A0>::Type_t _pa0, typename curryArgMode<A1>::Type_t _pa1,
			typename curryArgMode<A2>::Type_t _pa2):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2) {
		}

		PartialAppl3(const PartialAppl3& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2) {
		}

		inline R operator()(typename curryArgMode<A3>::Type_t a3,
			typename curryArgMode<A4>::Type_t a4, typename curryArgMode<A5>::Type_t a5) const {
			return(*op)(pa0, pa1, pa2, a3, a4, a5);
		}

	};

	typedef Fct3<A3, A4, A5, R, PartialAppl3> ffunc3_t;

	inline ffunc3_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2) const {
		return ffunc3_t(PartialAppl3(*op, a0, a1, a2));
	}

	// F applied to 4 arguments
	struct PartialAppl4 {

		anyOp6<A0, A1, A2, A3, A4, A5, R> op;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;

		// op = _op?
		PartialAppl4(const anyOp6<A0, A1, A2, A3, A4, A5, R>& _op,
			typename curryArgMode<A0>::Type_t _pa0, typename curryArgMode<A1>::Type_t _pa1,
			typename curryArgMode<A2>::Type_t _pa2, typename curryArgMode<A3>::Type_t _pa3):
		op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3) {
		}

		PartialAppl4(const PartialAppl4& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3) {
		}

		inline R operator()(typename curryArgMode<A4>::Type_t a4,
			typename curryArgMode<A5>::Type_t a5) const {
			return(*op)(pa0, pa1, pa2, pa3, a4, a5);
		}

	};

	typedef Fct2<A4, A5, R, PartialAppl4> ffunc4_t;

	inline ffunc4_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3) const {
		return ffunc4_t(PartialAppl4(*op, a0, a1, a2, a3));
	}

	// F applied to 5 arguments
	struct PartialAppl5 {

		anyOp6<A0, A1, A2, A3, A4, A5, R> op;
		A0 pa0;
		A1 pa1;
		A2 pa2;
		A3 pa3;
		A4 pa4;

		// op = _op?
		PartialAppl5(const anyOp6<A0, A1, A2, A3, A4, A5, R>& _op,
			typename curryArgMode<A0>::Type_t _pa0, typename curryArgMode<A1>::Type_t _pa1,
			typename curryArgMode<A2>::Type_t _pa2, typename curryArgMode<A3>::Type_t _pa3,
			typename curryArgMode<A4>::Type_t _pa4):
			op(_op), pa0(_pa0), pa1(_pa1), pa2(_pa2), pa3(_pa3), pa4(_pa4) {
		}

		PartialAppl5(const PartialAppl5& rhs):
		op(rhs.op), pa0(rhs.pa0), pa1(rhs.pa1), pa2(rhs.pa2), pa3(rhs.pa3), pa4(rhs.pa4) {
		}

		inline R operator()(typename curryArgMode<A5>::Type_t a5) const {
			return(*op)(pa0, pa1, pa2, pa3, pa4, a5);
		}
	};

	typedef Fct1<A5, R, PartialAppl5> ffunc5_t;

	inline ffunc5_t operator()(typename curryArgMode<A0>::Type_t a0,
		typename curryArgMode<A1>::Type_t a1, typename curryArgMode<A2>::Type_t a2,
		typename curryArgMode<A3>::Type_t a3, typename curryArgMode<A4>::Type_t a4) const {
		return ffunc5_t(PartialAppl5(*op, a0, a1, a2, a3, a4));
	}

};

#endif
