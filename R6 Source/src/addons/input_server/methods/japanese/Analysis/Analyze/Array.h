/* Array.h
 *
 *	配列テンプレート
 *
 *		programmed 1997.
 */
#ifndef _ARRAY_H_
#define _ARRAY_H_

template< class classT, unsigned int numT /* = 10 */ >
class Array {
public:
	Array() : dat_( 0 ), dat_size_( 0 ), all_size_( 0 ) {}
	~Array() { delete [] dat_; }

	classT& GetAt( unsigned int n ) { return dat_[ n ]; }
	classT& operator[]( unsigned int n ) { return dat_[ n ]; }

	void Add( classT& );
	void Clear() { dat_size_ = 0; }
	unsigned int GetSize() { return dat_size_; }

protected:
	classT*			dat_;
	unsigned int	dat_size_;
	unsigned int	all_size_;

	Array(const Array&);
	Array& operator=(const Array&);
};

template< class classT, unsigned int numT >
void Array< classT, numT >::Add( classT& other )
{
	if( dat_size_ >= all_size_ ){
		classT* new_t = new classT[ all_size_ + numT ];
		for( unsigned int i = 0; i < all_size_; i++ ) new_t[ i ] = dat_[ i ];
		delete [] dat_;
		dat_ = new_t;
		all_size_ += numT;
	}

	dat_[ dat_size_ ] = other;
	++dat_size_;
}

#endif
/*--- end of Array.h ---*/
