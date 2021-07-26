
#include <stdio.h>
#include <stdlib.h>
#include "math.h" 

// Size of vector of pairs 
int size; 

// Global vector of pairs to store 
// address ranges available in free list 
int memory[1025]; //this to define the blockes
int flag, lasts, last_index, numhist, check;
int waiting[50];
int history[100];

 void initialize() 
{ 	
	flag = 0;
	//intially we have 4-256-bytes blocks

	for (int i = 0; i < 256; i++)
	{
		memory[i] = 0;
	}

	for (int i = 0; i < 50; i++)
	{
		history[i] = 0;
	}
} 

void allocate(int sz) 
{ 
		//max is 256 ==> 2^8	
		// Calculate index in free list 
		// to search for block if available 
		//int n = ceil(log(sz) / log(2)); 
		//int diff = 8 - n;
	if(history[0] == 0) //if it's the first process
	{
		for (int i = 0; i < sz; i++)
		{
			memory[i] = 1;
		}
		last_index = sz;
		history[flag] = sz;
		flag++;
		return;
	}

	
	  /***************************************************/
	for (int i = 0; i < 100; i++)
	{
		if(history[i] == 1 && history[i+1] == 0)
		{
			numhist = i;
			check = numhist;
			break;
		}
	}

	if (sz <= history[check])
	{
		for (int i = last_index + 1; i < sz+last_index+1; i++)
		  {
			memory[i] = 1;
		  }
		last_index = sz+last_index+1;
		history[flag] = sz;
		flag++;
		check--;
		return;
	}
	else
	{
		if (sz <= history[check])
		{
			for (int i = last_index + 1; i < sz+last_index+1; i++)
		    {
				memory[i] = 1;
		    }
			last_index = sz+last_index+1;
			history[flag] = sz;
			flag++;
			check--;
			return;
		}
		else
			allocate(sz); //recuirsion
	}
}


// Driver code 
int main() 
{ 

	initialize(128); 
	allocate(32); 
	allocate(7); 
	allocate(64); 
	allocate(56); 

	return 0; 
} 
