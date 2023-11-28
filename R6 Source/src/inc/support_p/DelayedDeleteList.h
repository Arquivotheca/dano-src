/*
 * This is not designed to be accessed from multiple threads.  It is designed for short
 * term storage of Dynamically allocated items of the same type.  The advantage of 
 * using this class is that it will allow you to free up BAtoms and such outside of code
 * areas where a lock is being held.  This can remove possible contentions between threads.
 * It is also designed for speed and not space efficiency.  It has as few comparisons, function
 * calls and other stuff like that as possible. 
 */

#ifndef _MYRON_DELAYED_DELETE_LIST_H
#define _MYRON_DELAYED_DELETE_LIST_H

template <class DELETETYPE>

class DelayedDeleteList
{
	private:
		struct del_node 
		{
			DELETETYPE *item;
			del_node *next;
		};
	
	public:
		DelayedDeleteList()
		{
			fHead = NULL;
		};
		
		~DelayedDeleteList()
		{
			while(fHead != NULL)
			{
				del_node *nextdel = fHead;
				fHead = nextdel->next;
				delete nextdel->item;
				delete nextdel;
			}
		};

		void Delete(DELETETYPE *item)
		{
			if(item != NULL)
			{
				del_node *newnode = new del_node;
				newnode->item = item;
				newnode->next = fHead;
				fHead = newnode;
			}
		};
	
	private:
		del_node *fHead;	
};

#endif
