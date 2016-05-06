#include<stdio.h>
#include<stdlib.h>

#define W 0.1



struct no {
	float x;
	float y;
};

typedef struct no no;

float S(int *matrix[1][500], no no[500], int linha){
    int i, j;
    float soma = 0;

    for(i = 0; i < 500; i++){
        if(matrix[linha][i] == 1 && no[linha].x > 0){
            soma += W;
        }
    }
    return soma;
}


void main(){
    int matriz[500][500];
    int i, j;
    float vet_S[500];
    no neuronios[500];

    for(i = 0; i < 500; i++){
        //vet_S[i] = S(matriz)
    }
}
