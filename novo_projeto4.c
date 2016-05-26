#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>

#define F 0
#define V 1
#define MIN(a, b) a<b?a:b

// estados
#define EXECUTANDO 0
#define PRONTO     1
#define BLOQUEADO  2

// probabilidades assumidas
#define PROB_IO_DISPONIVEL 0.9
#define PROB_PREEMP 0.3
#define PSIZE_MAX 10
#define TAM_MEMORIA 100
#define FIRST_FIT   10
#define NEXT_FIT    20
#define BEST_FIT    30

int imprime_header = F;  // usada apenas na funcao imprime_processo
int total_tempo_cpu = 0; // marcador de tempo discreto, acumula tempos de execucao dos processos.

struct processo {
	unsigned short pid;          // identificador unico do processo
	unsigned int   prio;         // prioridade do processo
	int            quantum;      // fatia de tempo de execucao dadao ao processo para ser executado na cpu
	int            ttotal_exec;  // tempo total de execucao do processo
	unsigned short estado;       // estado do processo
	float          cpu_bound;    // probabilidade de o processo fazer uso apenas de cpu
	float          io_bound;     // probabilidade de o processo fazer IO durante seu usa da cpu

    int            proc_size;
	int            texec;           // recebe um tempo aleatorio de execucao na cpu (texec <= quantum)
	int            total_preemp;    // acumula qtd preempcoes sofridas
	int            total_uso_cpu;   // acumula qtd uso da cpu por todo quntum
	int            total_io;        // acumula qtd numero de interrupcoes por IO
	int            total_tempo_ret; // marca tempo de retorno do processo no momento da execucao do processo: tempo corrente da cpu acrescido do tempo de ingresso na fila
	int            tingresso;       // marca tempo que ingressou na fila
};
typedef struct processo processo_t;

struct no {
	processo_t proc;
	char status;
	int init;
	int tamanho;

	struct no *next_free;
	struct no *prox;
	struct no *ant;
};
typedef struct no no_m;

struct fila_memoria{
	no_m *inicio;
	no_m *fim;
	int tamanho;
	int count;
};
typedef struct fila_memoria fila_memoria;

void imprime_fila_memo(fila_memoria *f);

no_m *cria_no_memoria(processo_t proc) {
	no_m *no = (no_m *)calloc(1, sizeof(no_m));
	if (!no) return NULL;

	no->proc = proc;

	no->status = 'P';
	no->init = -1;
	no->tamanho = proc.proc_size;

	no->next_free = NULL;
	no->prox = NULL;
	no->ant = NULL;
	return no;
}

no_m *cria_no_memoria_vazio(int init, int tamanho) {
	no_m *no = (no_m *)calloc(1, sizeof(no_m));
	if (!no) return NULL;

	no->status = 'H';
	no->init = init;
	no->tamanho = tamanho;

	no->next_free = NULL;
	no->prox = NULL;
	no->ant = NULL;
	return no;
}

// Talvez mudar para somente inicializar Fila. inicio = fim = NULL
int cria_fila_memoria(fila_memoria *f) {
	f->inicio = NULL;
	f->fim = NULL;
	f->count = 0;
	f->tamanho = TAM_MEMORIA;
}

int fila_vazia(fila_memoria *f) {
	if (f->inicio == NULL) return V;
	return F;
}

int insere_fila_memo(fila_memoria *f, no_m *no, int metodo){
    char *met;
    met = calloc(15, sizeof(char));
    char fi[15] = "FIRST FIT";
    char best[15] = "BEST FIT";
    char next[15] = "NEXT FIT";
    switch(metodo){
        case 10:
            met = fi;
            break;
        case 20:
            met = next;
            break;
        case 30:
            met = best;
            break;
    }
    printf("\nInserindo proc %d de tamanho %d pelo metodo %s", no->proc.pid, no->tamanho, met);
	if (f->inicio == NULL) {    //fila vazia
            no->init = 0;
            no->ant = NULL;
            f->inicio = no;
            no_m *proximo;
            proximo = cria_no_memoria_vazio(no->tamanho, (f->tamanho - no->tamanho) );
            proximo->ant = no;
            no->prox = proximo;
            f->fim = proximo;
            f->fim->prox = NULL;
            f->count++;

            printf("\n");
            imprime_fila_memo(f);
            return V;
	}

	no_m *atual;
	atual = f->inicio;

    if(metodo == 10){
            //FIRST FIT implementado
        while(atual != NULL ){
            if(atual->status == 'P'){
                atual = atual->prox;
                continue;
            }
            else if(atual->tamanho < no->tamanho){
                atual = atual->prox;
            }
            else break;
        }
    }
    else if(metodo == 20){
        //implementar NEXT FIT
    }
    else if(metodo == 30){
        int sobra, sobra_ant = 10000000;
        no_m *aux = NULL;
        //implementar BEST FIT
        while(atual != NULL ){
            if(atual->status == 'P'){
                atual = atual->prox;
                continue;
            }
            else if(atual->tamanho < no->tamanho){
                atual = atual->prox;
            }
            else {
                sobra = atual->tamanho - no->tamanho;
                if(sobra < sobra_ant){
                        aux = atual;
                        sobra_ant = sobra;
                }
                atual = atual->prox;
            }
        }
        if(aux != NULL){
            atual = aux;
            sobra_ant = 100000;
        }
    }

	if(atual == NULL){
        // MATAR PROCESSO
        retira_processo_aleatorio(f);
        insere_fila_memo(f, no, metodo);
        return F;
    }
    if(atual->status == 'H' && atual->tamanho >= no->tamanho){  // Se processo cabe nesse slot, insira
        // inserir no aqui
        if(atual->tamanho == no->tamanho){
                if(atual->ant != NULL) atual->ant->prox = no;
                no->ant = atual->ant;
                no->init = atual->init;
                no->prox = atual->prox;
                if(atual->prox != NULL) atual->prox->ant = no;
                atual = no;
                f->count += 1;

                printf("\n");
                imprime_fila_memo(f);
                return V;
        }
        no->prox = atual;
        no->ant = atual->ant;
        no->init = atual->init;
        atual->init += no->tamanho;
        atual->tamanho -= no->tamanho;
        if(atual->ant != NULL) atual->ant->prox = no;
        atual->ant = no;
        f->count += 1;

        printf("\n");
        imprime_fila_memo(f);
        return V;
    }
    else{
        // MATAR PROCESSO
        retira_processo_aleatorio(f);
        insere_fila_memo(f, no, metodo);
        return F;
    }
    return F;
}

void imprime_fila_memo(fila_memoria *f){
    int no_id = 0;
    no_m *atual;
    atual = f->inicio;
    printf("\n\n");
    if(atual == NULL){
        printf("Fila Vazia\n");
        return ;
    }
    while(atual != NULL){
        printf("[%c->%d || init=%d || tam=%d] ", atual->status,atual->proc.pid, atual->init, atual->tamanho);
        if(atual->prox != NULL)
            printf("-->\n");
        atual = atual->prox;
    }


}

int remove_no(no_m *no){
    if(no == NULL) return F;
    if(no->ant == NULL){
//        if(no->prox == NULL){
//            no = cria_no_memoria_vazio(no->init, no->tamanho);
//        }
        if(no->prox->status == 'P'){
            no->status = 'H';
        }
        else if(no->prox->status == 'H'){
            no->tamanho += no->prox->tamanho;
            no->prox = no->prox->prox;
            if(no->prox->prox != NULL) no->prox->prox->ant = no;
            no->status = 'H';

            free(no->prox);
        }
    }
    else if(no->prox == NULL){
        if(no->ant == NULL){
            no = cria_no_memoria_vazio(no->init, no->tamanho);
        }
        else if(no->ant->status == 'P')
            no->status = 'H';
        else if(no->ant->status == 'H'){
            no->ant->tamanho += no->tamanho;
            no->ant->prox = NULL;

            free(no);
        }
    }
    else{
        if(no->prox->status == 'H'){
            if(no->ant->status == 'H'){
                no->ant->tamanho += no->tamanho + no->prox->tamanho;
                no->ant->prox = no->prox->prox;
                if(no->prox->prox != NULL) no->prox->prox->ant = no->ant;

                free(no);
                free(no->prox);
            }
            else if(no->ant->status == 'P'){
                no_m *aux;
                no->tamanho += no->prox->tamanho;
                aux = no->prox;
                no->prox = no->prox->prox;
                if(no->prox != NULL) no->prox->prox->ant = no;
                no->status = 'H';
                free(aux);
            }
        }
        else
        if(no->ant->status == 'H'){
            if(no->prox->status == 'P'){
                no->ant->tamanho += no->tamanho;
                no->ant->prox = no->prox;
                no->prox->ant = no->ant;

                free(no);
            }
        }
        else{
            no->status = 'H';
        }
    }
    return V;
}

int retira_processo_aleatorio(fila_memoria *f){
    int p;
    int j;
    int i = 1;
    no_m *atual;
    while(1){
        j = (rand()%100) + 1;
        do{
            p = (rand()%(f->count) + 1);
        }while(p > f->count);
        atual = f->inicio;
        while(i < p && atual != NULL ){
            atual = atual->prox;
            i++;
        }
        if(atual->status == 'H' || i < p) return F;
        printf("\nProcesso %d sendo encerrado", atual->proc.pid);
        if(remove_no(atual) == V) f->count -= 1;

        printf("\n");
        imprime_fila_memo(f);
        if(j < 20) break;
    }
}
// roleta ... para gerar um evento, dada uma probabilidade x.
int prob(float x){
	float p;
	p = ((float)(rand()%101))/100.0;
	if (p <= x) return V;
	return F;
}

// obtem tempo aleatorio <= x
int pega_tempo (int x) {


	return(rand()%(x+1));
}

int pega_tamanho(){

	return((rand()%(PSIZE_MAX-1))+1);
}

// subtrai a-b, menor valor 0
int sub(int a, int b) {
	int r = a - b;
	if (r < 0) return 0;
	return r;
}

float get_quantum(unsigned int prio) {
	int q;
	switch(prio) {
		case 4:
			q = 100;
			break;
		case 3:
			q = 75;
			break;
		case 2:
			q = 50;
			break;
		case 1:
			q = 25;
			break;
		default:
			q = 10;
	}
	return q;
}

processo_t cria_processo(unsigned short pid){
	processo_t proc;
	proc.pid = pid;
	proc.prio = rand()%5;
    proc.quantum = get_quantum(proc.prio);
	proc.ttotal_exec = rand()%10000;
    proc.estado = PRONTO;
    proc.cpu_bound = ((float)(rand()%101))/100.0;
	proc.io_bound = 1 - proc.cpu_bound;
	proc.texec = 0; // recebe tempo de execucao na cpu
    proc.total_preemp = 0; // conta o total de preempcoes sofridas
    proc.total_uso_cpu = 0;
    proc.total_io = 0;
    proc.total_tempo_ret = 0;
	proc.tingresso = total_tempo_cpu;
	proc.proc_size = pega_tamanho();
	return proc;
}

void cria_todos_processos(fila_memoria *f, int np, int metodo) {
	int i;
	processo_t proc;
	no_m *no;
	for (i=0; i<np; i++) {
		proc = cria_processo(i);
		no = cria_no_memoria(proc);
		insere_fila_memo(f, no, metodo);
	}
}


void main(){
    int i;
    int np = 25;
    srand(time(NULL));
	fila_memoria f;
	cria_fila_memoria(&f);
	cria_todos_processos(&f, np, BEST_FIT);
	imprime_fila_memo(&f);
    printf("\n\n");
}
