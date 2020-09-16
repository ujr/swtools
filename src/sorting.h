#pragma once
#ifndef SORTING_H
#define SORTING_H

void bubblesort(int v[], int n);
void shellsort(int v[], int n);
void quicksort(int v[], int n, int (*cmp)(int,int,void*), void*);
void reheap(int heap[], int n, int (*cmp)(int,int,void*), void*);
void shuffle(int v[], int n, int (*rnd)(int));

#endif
