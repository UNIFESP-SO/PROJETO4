/*
Instituicao: ICT/UNIFESP, Sao Jose dos Campos - SP
Curso:       BCT-N
Disciplina:  Sistemas Operacionais
Prof.:       Bruno Kimura (bruno.kimura@unifesp.br)
Descricao:   Este codigo foi desenvolvido pelo professor durante aulas da referida disciplina. Seu objetivo eh
             de possibilitar aos alunos melhor entendimento sobre o tema 'escalonamento de processos'. A aplicacao
             deste codigo eh unica e exclusivamente voltada a referida disciplina como material de apoio didatico.
Obs.:        ISTO NAO EH UM ESCALONADOR, apenas um artefato de software utilizado na disciplina para verificar
             as operacoes realizadas por um mecanismo de escalonamento.
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

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

int imprime_header = F;  // usada apenas na funcao imprime_processo
int total_tempo_cpu = 0; // marcador de tempo discreto, acumula tempos de execucao dos processos.

struct processo_t {
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
typedef struct processo_t processo_t;

struct no_t {
	processo_t proc;
	struct no_t *prox;
	struct no_t *ant;
};
typedef struct no_t no_t;

// Slot de memoria
struct no_m {
	processo_t proc;
	char status;
	int init;
	int tamanho;

	struct no_m *next_free;
	struct no_m *prox;
	struct no_m *ant;
};
typedef struct no_m no_m;

struct fila_t{
	no_t *inicio;
	no_t *fim;
	int qtd_proc;
};
typedef struct fila_t fila_t;

struct fila_memoria{
	no_m *inicio;
	no_m *fim;
	int tamanho;
	int count;
};
typedef struct fila_memoria fila_memoria;

no_m *cria_no_memoria(processo_t proc);
no_m *cria_no_memoria_vazio(int init, int tamanho);
// criacao fila
void cria_fila(fila_t *f) {
	f->inicio = NULL;
	f->fim = NULL;
	f->qtd_proc = 0;
}

int cria_fila_memoria(fila_memoria *f) {
	no_m *no = (no_m *)calloc(1, sizeof(no_m));
	if (!no) return F;
	no = cria_no_memoria_vazio(0, TAM_MEMORIA);

	f->inicio = no;
	f->fim = no;
	f->count = 0;
	f->tamanho = TAM_MEMORIA;
}

// cria no
no_t *cria_no(processo_t proc) {
	no_t *no = (no_t *)calloc(1, sizeof(no_t));
	if (!no) return NULL;
	no->proc = proc;
	no->prox = NULL;
	no->ant = NULL;
	return no;
}

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

// insercao ordenada
int insere_fila_prio(fila_t *f, processo_t proc){
	no_t *no = cria_no(proc);
	if (!no) return F;
	if (f->inicio == NULL) { // fila vazia
		f->inicio = no;
		f->fim = no;
		return V;
	}
	if (no->proc.prio > f->inicio->proc.prio) {
		no->prox = f->inicio;
		no->ant = NULL;
		f->inicio = no;
		return V;
	}
	if (no->proc.prio <= f->fim->proc.prio) {
		f->fim->prox = no;
		no->ant = f->fim;
		f->fim = no;
		return V;
	}

	no_t *atual, *antx;
	atual = f->inicio;
	antx = NULL;
	while(no->proc.prio <= atual->proc.prio) {
		antx = atual;
		atual = atual->prox;
	}
	antx->prox = no;
	no->prox = atual;
	no->ant = antx;
	return V;
}

// insercao memoria
int insere_fila_memo(fila_memoria *f, no_m *no){

	if (f->inicio == NULL) {    //fila vazia
            cria_fila_memoria(f);
	}

	no_m *atual;
	atual = f->inicio;

	while(atual->prox != NULL && atual->status == 'P' || (atual->status == 'H' && atual->tamanho < no->tamanho) )
        atual = atual->prox;

    if(atual == f->fim){
        // MATAR PROCESSO
        printf("1\n");
        retira_processo_aleatorio(f);
        printf("1\n");
        insere_fila_memo(f, no);
    }
    if(atual->status == 'H' && atual->tamanho >= no->tamanho){  // Se processo cabe nesse slot, insira
        // inserir no aqui
        atual->init += no->tamanho;
        atual->tamanho -= no->tamanho;
        atual->ant = no;
        f->count += 1;
        no->prox = f->inicio;
        no->init = f->inicio->init;

        atual = no;
        return V;
    }
}

int fila_vazia(fila_t *f) {
	if (f->inicio == NULL) return V;
	return F;
}

int fila_vazia_memo(fila_memoria *f) {
	if (f->inicio == NULL) return V;
	return F;
}

// retira_fila()
int retira_fila(fila_t *f, processo_t *proc) {
	if (fila_vazia(f)) return F;
	no_t *no = f->inicio;
	*proc = no->proc;
	f->inicio = f->inicio->prox;
	if (f->inicio == NULL) f->fim = NULL;
	else f->inicio->ant = NULL;
	free(no);
	return V;
}

int retira_processo_aleatorio(fila_memoria *f){
    int p = (rand()% (f->count) + 1);
    int i = 0;
    no_m *atual;
    atual = f->inicio;
    while(i <= p ){
        if(atual->prox->status == 'P'){
            i++;
            atual = atual->prox;
        }
        else
            atual = atual->prox;
    }
    return remove_no(atual);
}

int remove_no(no_m *no){
    if(no->ant == NULL){
        if(no->prox == NULL){
            no = cria_no_memoria_vazio(no->init, no->tamanho);
        }
        else if(no->prox->status == 'P'){
            no->status = 'H';
        }
        else if(no->prox->status == 'H'){
            no->prox->tamanho += no->tamanho;
            no->prox->ant = NULL;

            free(no);
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
                no->prox->prox->ant = no->ant;

                free(no);
                free(no->prox);
            }
            else if(no->ant->status == 'P'){
                no->tamanho += no->prox->tamanho;
                no->prox = no->prox->prox;
                no->prox->prox->ant = no;
                no->status = 'H';
                free(no->prox);
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
}

void imprime_processo(processo_t proc) {
	if (!imprime_header) {
		printf("pid prio stado cpu_bound io_bound ttotal_exec quantum texec total_preemp total_uso_cpu total_io total_tempo_ret tingresso\n");
		imprime_header = V;
	}
	printf("%d     %d    %d\t %.2f\t   %.2f\t    %d\t\t %d    %d\t %d\t\t %d\t  %d\t    %d\t\t   %d\n",
		proc.pid,
		proc.prio,
		proc.estado,
		proc.cpu_bound,
		proc.io_bound,
		proc.ttotal_exec,
		proc.quantum,
		proc.texec,
        	proc.total_preemp,
        	proc.total_uso_cpu,
        	proc.total_io,
		proc.total_tempo_ret,
		proc.tingresso );
}

void imprime_fila_memo(fila_memoria *f){
    int no_id = 0;
    no_m *atual;
    atual = f->inicio;
    if(atual == NULL){
        printf("Fila Vazia\n");
        return ;
    }
    do{
        printf("[ %c | init-%d | tam-%d ] --> ", atual->status, atual->init, atual->tamanho);
        no_id++;
    }
    while(atual->prox != NULL);

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
	return(rand()%(PSIZE_MAX));
}

// subtrai a-b, menor valor 0
int sub(int a, int b) {
	int r = a - b;
	if (r < 0) return 0;
	return r;
}


void executa_processo(processo_t *proc) {
	// preempcao
	if (prob(PROB_PREEMP)) {
		proc->texec = pega_tempo(proc->quantum);
		proc->estado = PRONTO;
		proc->total_preemp++;
	}
	else {
		if (prob(proc->cpu_bound)) { // executa cpu
			proc->texec = proc->quantum;
			proc->estado = PRONTO;
			proc->total_uso_cpu++;
		}
		else { // faz IO
			proc->texec = pega_tempo(proc->quantum);
			proc->estado = BLOQUEADO;
			proc->total_io++;
		}
	}
	total_tempo_cpu += MIN(proc->ttotal_exec, proc->texec);     // atualiza tempo da cpu
	proc->ttotal_exec = sub(proc->ttotal_exec, proc->texec);    // atuzliza tempo do processo
	proc->total_tempo_ret = total_tempo_cpu + proc->tingresso;  // marca tempo de retorno corrente do processo
	imprime_processo(*proc);
}

void escalonador(fila_t *f) {
	processo_t proc;
	while(!fila_vazia(f)) {
		bzero(&proc, sizeof(processo_t));
		retira_fila(f, &proc);
		switch (proc.estado) {
			case PRONTO:
				proc.estado = EXECUTANDO;
				executa_processo(&proc);
				if (proc.ttotal_exec > 0)
					insere_fila_prio(f, proc);
				break;
			case BLOQUEADO:
				if (prob(PROB_IO_DISPONIVEL)) {
					proc.estado = EXECUTANDO;
                                	executa_processo(&proc);
	                                if (proc.ttotal_exec > 0)
						insere_fila_prio(f, proc);
				}
				else {
					 insere_fila_prio(f, proc);
				}
				break;
		}
	}
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

void imprime_fila(fila_t *f){
	no_t *i;
	for(i=f->inicio; i!=NULL; i=i->prox) {
		imprime_processo(i->proc);
	}
}

void cria_todos_processos(fila_t *f, int np) {
	int i;
	processo_t proc;
	for (i=0; i<np; i++) {
		proc = cria_processo(i);
		insere_fila_prio(f, proc);
	}
}

void cria_todos_processos_m(fila_memoria *f, int np) {
	int i;
	processo_t proc;
	for (i=0; i<np; i++) {
		proc = cria_processo(i);
		no_m *no = cria_no_memoria(proc);
		insere_fila_memo(f, no);
		printf("4\n");
		sleep(10);
	}
}

int main(int argc, char *argv[]){
    int np = 2;
	fila_memoria f;
	cria_fila_memoria(&f);
	cria_todos_processos_m(&f, np);
	imprime_fila_memo(&f);
	   /*
	         Todos os np processos estao sendo criados no mesmo instante (tingresso = total_tempo_cpu = 0).
	      Portanto, a media do tempo de retorno sera sempre a soma de todos os ttotal_exec dos processos
	      dividida por np.
	        Obs.: para verificar diferentes tempos de retorno, os processos precisam chegar
	      na fila em diferentes momentos (ou seja, o tingresso nao pode ser o mesmo para todos).
	      Nesse caso, um nova funcao cria_todos_processo() deve inserir os processos na fila
              com diferentes tempos de tingresso.
	   */

	return 0;
}
