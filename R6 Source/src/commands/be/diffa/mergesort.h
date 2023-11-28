//        FileName: mergesort.h
//          Author: Myron W. Walker
//            Date: 9/04/98
//         Version: 1.1
/*	   Description: This is a recursive template function which will sort a link list.  
	   It requires the link list to use pointer named 'next' to point to the next node
	   in the list.  It also requires that the link list store the data object as an 
	   element named 'data'.  The < and > comparison operators must be overloaded for
	   the data type being stored in the list.
*/

template <class LIST> 
int MergeSort(LIST *& head, int size)
{
	int leftsize, rightsize, counter, compares;
	LIST * headleft, * headright, * last, * curleft, * curright;

	if(size>1)
	{
		// split list into two lists
		leftsize = (size-(size % 2)) / 2;	
		rightsize = (size - leftsize);

		headleft = head;
		headright = headleft;

		counter = 0;

		while (counter < leftsize)
		{
			last = headright;
			headright = headright->next;
			counter++;	
		}
		
		last->next = NULL;
		
		// sort left list
		compares = MergeSort(headleft, leftsize);
		
		// sort right list
		compares = compares + MergeSort(headright, rightsize);
		
		// start list merge
			curleft = headleft;
			curright = headright;
			
			// choose head of list
			if(*curleft < *curright)
			{
				head = curleft;		
				curleft = curleft->next;
			}
			else
			{
				head = curright;
				curright = curright->next;
			}
			
			compares++;
			last = head;
			
			// merge till a null is reached
			while ((curleft != NULL) && (curright != NULL))
			{
				if(*curleft < *curright)
				{
					last->next = curleft;
					last = curleft;
					curleft = curleft->next;
				}
				else
				{
					last->next = curright;
					last = curright;
					curright = curright->next;
				}
				
				compares++;
			}
				
			// add the remainder of the nodes to the list
			if (curleft == NULL)
			{
				last->next = curright;
			}
			else
			{
				last->next = curleft;
			}
			
			compares++;
		// end list merge

	}// end if(size>1)
	else
	{
		compares = 1;
	}
	return(compares);
}