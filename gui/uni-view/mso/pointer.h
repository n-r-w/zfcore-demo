#ifndef  _POINTER_H
#define  _POINTER_H

//
// A pair of smart pointer template classes. Provides basic conversion
// operator to T*, as well as dereferencing (*), and 0-checking (!).
// These classes assume that they alone are responsible for deleting the
// object or array unless Relinquish() is called.
//

//
// class DPtrBase
// ~~~~~ ~~~~~~~~~~~~
template<class T> class DPtrBase
{
public:
	T& operator *() {return *P;}
	const T& operator *() const {return *P;}

	operator T*() {return P;}
	operator const T*() const {return P;}

	T* GetPtr(void){return P;}
	const T* GetPtr(void) const {return P;}

	T** GetPtrPtr(void){return &P;}
	T*& GetPtrRef(void){return P;}

	int operator !() const {return P==0;}
	T* Relinquish(T* newp=0) {T* p = P; P = newp; return p;}

protected:
	DPtrBase(T* pointer) : P(pointer) {}
	DPtrBase() : P(0) {}

    T* P = nullptr;

#ifndef MPATROL_VERSION // *nix bounds checker
private:
	void* operator new(size_t) {return 0;} // prohibit use of new
	#ifdef __GNUC__
	// in gcc operator delete seems to be needed for destructors in derived classes,
	// so it can't be private
protected:
	#endif // __GNUC__
	void operator delete(void* p) {((DPtrBase<T>*)p)->P=0;}
	#endif // MPATROL_VERSION
private:
	DPtrBase(const DPtrBase<T> &);
	void operator = (const DPtrBase<T> &);
};

//
// class DPtr
// ~~~~~ ~~~~~~~~
// Pointer to a single object. Provides member access operator ->
//
template<class T> class DPtr : public DPtrBase<T>
{
public:
	DPtr() : DPtrBase<T>() {}
	DPtr(T* pointer) : DPtrBase<T>(pointer) {}
	~DPtr() {DELETE(P);}
	DPtr<T>& operator =(T* src) {DELETE(P); P = src; return *this;}
	T* operator -> () {return P;}	// Could throw exception if P==0
	const T* operator -> () const {return P;}	// Could throw exception if P==0

	// Boost shared_ptr<> compatibility methods
	T* get() { return P;}
	void reset() { DELETE(P); }

private:
	DPtr(const DPtr<T> &);
	void operator = (const DPtr<T> &);
};

//
// class DAPtr
// ~~~~~ ~~~~~~~~~
// Pointer to an array of type T. Provides an array subscript operator and uses
// array delete[]
//
template<class T> class DAPtr : public DPtrBase<T>
{
public:
	DAPtr() : DPtrBase<T>() {}
	DAPtr(T array[]) : DPtrBase<T>(array) {}
	~DAPtr() {FREE_(P);}
	DAPtr<T>& operator =(T src[]) {FREE_(P); P = src; return *this;}
	T& operator[](int i) {return P[i];}	// Could throw exception if P==0
	const T& operator[](int i) const {return P[i];}	// Could throw exception if P==0
private:
	DAPtr(const DAPtr<T> &);
	void operator = (const DAPtr<T> &);
};

typedef DAPtr<char>    StringPtr;
typedef DAPtr<wchar_t> StringPtrW;
// typedef DPtr<KeyList>  KeyListPtr;

//******************************************************************************
//
// class DRefBase
// ~~~~~ ~~~~~~~~~~~~
enum AddRefMode0 { NoAddRef = 0 };
enum AddRefMode1 { WithAddRef = 1 };

template<class T> class DRefBase
{
public:
	T& operator * () {return *P;}
	const T& operator * () const {return *P;}

	operator T* (){return P;}
	operator const T* () const {return P;}

	int operator !() const {return P==0;}

	T* Relinquish(T* newp=0) {T* p = P; P = newp; return p;}
	T* GetPtr(void) {return P;}
	const T* GetPtr(void) const {return P;}
	T** GetPtrPtr(void) {return &P;}
	void Clear(void){ if(P){ P->Release(); P=0; } }

protected:
	DRefBase(void) : P(0) {}
	DRefBase(T* p, AddRefMode0) : P(p) {}
	DRefBase(T* p, AddRefMode1) : P(p) { addref(); }

	void addref(void){ if(P) P->AddRef(); }
	void release(void){ if(P) P->Release(); }

    T* P = nullptr;

#ifndef MPATROL_VERSION // *nix bounds checker
private:
	void* operator new(size_t) {return 0;}	// prohibit use of new
	#ifdef __GNUC__
	// in gcc operator delete seems to be needed for destructors in derived classes,
	// so it can't be private
protected:
	#endif // __GNUC__
	void operator delete(void* p) {((DRefBase<T>*)p)->P=0;}
	#endif // MPATROL_VERSION
private:
	DRefBase(const DRefBase<T> &);
	void operator = (const DRefBase<T> &);
};

//
// class DRef
// ~~~~~ ~~~~~~~~
// Pointer to a single object. Provides member access operator ->
//
template<class T> class DRef : public DRefBase<T>
{
public:
	DRef(void) : DRefBase<T>() {}
	DRef(T* p, AddRefMode0) : DRefBase<T>(p, NoAddRef) {}
	DRef(T* p, AddRefMode1) : DRefBase<T>(p, WithAddRef) {}
	~DRef() {release();}
	void operator = (T* p) { if(p) p->AddRef(); if(P) P->Release(); P = p; }
	//<removed> 18.11.09 strange behaviour in Borland in expression:
	// DRef<T> var1, var2;
	// var1 = var2;
	// void operator = (const DRef<T> &o){ if(o.P) o.P->AddRef(); if(P) P->Release(); P = o.P; }
	//</removed>
	T* operator -> () {return P;}	// Could throw exception if P==0
	const T* operator -> () const {return P;}	// Could throw exception if P==0

private:
	DRef(const DRef<T> &);
	void operator = (const DRef<T> &o);//{ if(o.P) o.P->AddRef(); if(P) P->Release(); P = o.P; }
};


//
// class ComRef
// ~~~~~ ~~~~~~~
// Encapsulation of OLE interface pointers
//
template<class T> class DComRefBase {
 public:
	        operator T*()         {return I;}
	virtual operator T**()        {Clear(); return &I;}
	int     operator !() const    {return I==0;}
	#ifndef MPATROL_VERSION
	void operator delete(void* p) {((DComRefBase<T>*)p)->I=0;}
	#endif // MPATROL_VERSION
protected:
	DComRefBase(const DComRefBase<T>& i) : I(i.I) {if (I) I->AddRef();}
	DComRefBase(T* i) : I(i) {}
	DComRefBase()     : I(0) {}
	virtual ~DComRefBase() {Clear();}

	void Clear(void) {if (I) {I->Release(); I = 0;}}
    T* I = nullptr;

private:

	#ifndef MPATROL_VERSION
	// Prohibit use of operator new
	//
	void* operator new(size_t) {return 0;}
	#endif // MPATROL_VERSION
private:
	//DComRefBase(const DComRefBase<T> &);
	void operator = (const DComRefBase<T> &);
};

template<class T> class DComRef : public DComRefBase<T>
{
public:
	DComRef() : DComRefBase<T>() {}
	DComRef(T* iface) : DComRefBase<T>(iface) {}
	DComRef(const DComRef<T>& i) : DComRefBase<T>(i) {}

	DComRef<T>& operator =(T* iface) {Clear(); I = iface; return *this;}
	DComRef<T>& operator =(const DComRef<T>& i) {Clear(); if (i.I) {I = i.I; I->AddRef();} return *this;}

	T* operator->() {return I;}			// Could throw exception if I==0
	const T* operator->() const {return I;}			// Could throw exception if I==0
private:
	//DComRef(const DComRef<T> &);
	//void operator = (const DComRef<T> &);
};

#endif// _POINTER_H

