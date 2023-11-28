/* Hash.h
 *
 *	ハッシュテンプレート
 *
 *		programmed 1997. 
 */
#ifndef _HASH_H_
#define _HASH_H_

template< class keyT, class recT, unsigned int numT, class funcT >
class Hash {
public:

	Hash() : head_( new ListHead[ numT ] ) {}
	~Hash();

	void AddKey( const keyT&, const recT& );
	void DeleteKey( const keyT& );
	const recT& FindKey( const keyT& );

private:

	class List;

	class ListHead {
	public:
		List*	next_;

		ListHead() : next_( 0 ) {}

		List* NewNext()
		{
			if( !next_ ) next_ = new List;
			return next_;
		}
	};

	class List : public ListHead {
	public:
		keyT	key_;
		recT	rec_;
	};


	ListHead*	head_;
	funcT		func_;

	unsigned int hash_func( const keyT& key ) { return func_( key ) % numT; }

};

template< class keyT, class recT, unsigned int numT, class funcT >
Hash< keyT, recT, numT, funcT >::~Hash()
{
	for( int i = 0; i < numT; i++ ){
		List* list = head_[ i ].next_;
		while( list ){
			List* next = list->next_;
			delete list;
			list = next;
		}
	}
	delete [] head_;
}

template< class keyT, class recT, unsigned int numT, class funcT >
void Hash< keyT, recT, numT, funcT >::AddKey( const keyT& key, const recT& rec )
{
	unsigned int i = hash_func( key );
	List* list = head_[ i ].next_;

	List* old = (List*)( &(head_[ i ]) );
	while( list ){
		if( list->key_ == key ){
			list->rec_ = rec;
			return;
		}

		old = list;
		list = list->next_;
	}

	list = old->NewNext();
	list->key_ = key;
	list->rec_ = rec;

	return;
}

template< class keyT, class recT, unsigned int numT, class funcT >
void Hash< keyT, recT, numT, funcT >::DeleteKey( const keyT& key )
{
	unsigned int i = hash_func( key );
	List* list = head_[ i ].next_;

	List* old = (List*)(head_[ i ]);
	while( list ){
		if( list->key_ == key ){
			old->next_ = list->next_;
			delete list;
			return;
		}

		old = list;
		list = list->next_;
	}

	return;
}

template< class keyT, class recT, unsigned int numT, class funcT >
const recT& Hash< keyT, recT, numT, funcT >::FindKey( const keyT& key )
{
	unsigned int i = hash_func( key );
	List* list = head_[ i ].next_;

	while( list ){
		if( list->key_ == key ){
			return list->rec_;
		}

		list = list->next_;
	}

	return (recT)0;
}

#endif
/*--- end of Hash.h ---*/
