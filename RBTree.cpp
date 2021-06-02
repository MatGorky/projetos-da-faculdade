/*Autor: Matheus Moura Gorchinsky
  DRE: 119043032
*/

#include <iostream>
#include <stdlib.h>

typedef struct Node {
    int valor; //valor da chave
    bool cor; //só pode ser black ou red     red é true,   black é false(ou ponteiro nulo)

    Node *dir;   //ponteiro para o filho direito
    Node *esq; //ponteiro para o filho esquerdo

}node;

typedef struct StackNode {

    node *data; //irá guardar os nós da árvore(caminho da raíz até o novo elemento inserido)
    StackNode *prox; //o ponteiro para o próximo elemento da stack(que por ser LIFO, na verdade o próximo foi o elemento inserido anteriormente

}stack;


//funções da stack primeiro
void Push(stack **topo, node* ancestral){
    if(*topo == NULL){ //stack vazia
        *topo = (stack *)malloc(sizeof(stack));
        (*topo) -> data = ancestral; //nó da árvore colocado
        (*topo) -> prox = NULL; //nada foi inderido antes do primeiro elemento
        return;
    }

    //caso não esteja vazia
    stack *aux = (stack *)malloc(sizeof(stack)); //alocando espaço para o novo elemento em um ponteiro auxilar
    aux -> prox = *topo; //o antigo topo fica embaixo do novo elemento
    aux -> data = ancestral; //nó da árvore colocado
    *topo = aux; //colocando o novo elemento no topo
}

node* Pop(stack **topo){
    stack *aux = *topo; //ponteiro auxiliar para ajudar no retorno do resultado
    node *ancestral;
    if(aux != NULL){ //se a stack não estiver vazia
        *topo = (*topo) -> prox; //o topo é levado para o elemento abaixo
        ancestral = aux -> data; //preparando para o retorno
        free(aux);  //liberando o espaço alocado na heap
        return ancestral; //retornando o nó da árvore
    }
    
    //caso a stack esteja vazia
    return NULL;
}

void Libera(stack **topo){ //apenas para liberar a stack
    stack *aux;
    while(*topo != NULL){
        aux = *topo;
        *topo = (*topo) -> prox;
        free(aux);
    }
    free(topo);
}

//nas 2 rotações, **raiz é apenas para mudar a raíz da árvore caso necessário, *pt é nó principal que está sendo rotacionado e *pai é o ponteiro para o nó que vem antes de pt
void RotacaoEsquerda(node **raiz, node *pt, node* pai){
    node *aux = pt -> dir; //ponteiro auxilar para rotação, como é rotação para esquerda, um filho direito sempre existe
    pt -> dir = aux -> esq;  //subárvore da esquerda do filho, vira subárvore direita do pai
    
    if(pai == NULL){ //se o antigo pai(pt) não tem pai(pai), é porque era raíz
        *raiz = aux; //então o antigo filho agora é a raíz
    }
    //agora se tem pai, e ele for o filho esquerdo desse pai
    else if(pt == pai -> esq){
        pai -> esq = aux; //o antigo filho, novo pai, passa a ser o filho esquerdo do "avô"
    }
    else{ //se não, é filho direito desse pai
        pai -> dir = aux; //e passa ser o filho direito do seu antigo "avô"
    }
    aux -> esq = pt; //o antigo pai passa a ser o filho esquerdo de seu antigo filho
    pai = aux; //e consequentemente, seu pai passa a ser seu antigo filho
}

void RotacaoDireita(node **raiz, node *pt, node* pai){
    
    node * aux = pt -> esq; //ponteiro auxiliar para a rotação, seguindo a mesma lógica da rotação anterior, um filho esquerdo sempre existe
    
    pt -> esq = aux -> dir; //subárvore da direita do filho vira subárvore da esquerda do pai

    if(pai == NULL){ ////se o antigo pai(pt) não tem pai(pai), é porque era raíz
        *raiz = aux; //então o antigo filho agora é a raíz
    }
    //agora se tem pai, e ele for o filho esquerdo desse pai
    else if(pt == pai -> esq){
        pai -> esq = aux; //o antigo filho, novo pai, passa a ser o filho esquerdo do seu antigo "avô"
    }
    else{ //se não, é filho direito desse pai
        pai -> dir = aux; //e passa a ser o filho direito do seu antigo "avô"
    }
    aux -> dir = pt; //o antigo pai passa a ser o filho direito de seu antigo filho
    pai = aux; //e consequentemente, seu pai passa a ser seu antigo filho

}

void ConsertArvore(node **raiz, node *pt, stack** ancestrais){ //essa função vai lidar com as violações das propriedades da árvore rubro negra
    
    node* pai = Pop(ancestrais); //auxiliar para o pai do nó atual
    node* avo = Pop(ancestrais); //auxiliar para o avô do nó atual
    node* tio = NULL; //auxiliar para o tio do nó atual
    
    
    
    //enquanto não chegar na raíz, ainda devemos verificar se o pai é red, as propriedades foram violadas(não precisa verificar se o nó atual é red, porque ele sempre vai ser red(já que é o novo nó inserido))
    while((pt != *raiz) && (pai -> cor)){

        //esse if não precisava existir se a raíz começasse como black
        if(avo == NULL){ //o primeiro elemento a ser inserido depois da raíz, viola as propriedades da árvore
            pai -> cor = false;  //então, recolorimos a raíz, para ficar black
            return;   //não tem mais nada pra fazer, sair da função
        }
                
        if(pai == avo -> esq){ //se o pai for o filho esquerdo do avô
            tio = avo -> dir;  //o tio é o filho direito do avô
            
            if(tio != NULL && tio -> cor){ //se o tio é red, este caso é simples, basta recolorir
                avo -> cor = true; //o avô sempre é black(pois se não fosse, as propriedades já teriam sido violadas e consertada antes)
                pai -> cor = false; //pai e tio que eram red, ficam black
                tio -> cor = false;
                pt = avo; //o nó a ser analisado passa a ser o avô, para propagar as mudanças(consertar violações geradas pelas mudanças anteriores)
                pai = Pop(ancestrais); //próximo pai
                avo = Pop(ancestrais); //próximo avô
            }
            else{ //caso o tio seja black(null tbm é black), teremos que aplicar rotações
                //como já sabemos que o pai é o filho esquerdo do avo(no nó atual)
                
                if(pt == pai -> dir){ //se o nó atual é filho direito do pai, rotacionar pai para esquerda
                    
                    RotacaoEsquerda(raiz,pai,avo); //a rotação é no pai, mas como precisamos do nó pai, o avô é o pai do pai
                    pai = pt; //o pai trocou de lugar com o filho(será útil depois da segunda rotação, pois após ela, seja esse ponteiro o pai original ou o novo pai, ele irá ficar black de qlqr jeito)
                }

                //não importando agora se o nó é filho direito ou esquerdo do pai
                RotacaoDireita(raiz,avo,Pop(ancestrais)); //a rotação é no avô, mas como precisamos do nó pai, acima na stack está o pai do avô
                //trocando as cores
                avo -> cor = true;    //o avô, sempre será black e por isso sempre irá virar red
                pai -> cor = false;   //o pai(seja o pai apenas desse caso, ou o nó atual, vindo após as 2 rotações) será sempre red e por isso irá virar black

                
                //pt = pai;  //caso tenha sofrido as 2 rotações, pt continuará sendo o nó atual(que agora está como pai do antigo pai e do avô)
                           // caso tenha sofrido apenas 1 rotação, pt será realmente o pai do nó atual(sendo também pai do antigo avô)
                            //emfim, em ambos os casos, pt estará no topo da pequena subárvore analisada e irá ser o novo "nó atual" do próximo loop
                           //para facilitar a troca de cor
            }
        }
        else{//agora caso o pai seja o filho direito do avô
            tio = avo -> esq; //o tio é o filho esquerdo do avô
                                //e agora, só aplicar a lógica de forma simétrica

            if(tio != NULL && tio -> cor){ //se o tio é red, nada muda este caso continua simples, basta recolorir
                avo -> cor = true; //o avô sempre é black, vira red
                pai -> cor = false; //pai e tio que eram red, ficam black
                tio -> cor = false;
                pt = avo; //o nó a ser analisado passa a ser o avô, para propagar as mudanças(consertar violações geradas pelas mudanças anteriores)
                pai = Pop(ancestrais); //próximo pai
                avo = Pop(ancestrais); //próximo avô
            }
            else{// caso tio black
                
                if(pt == pai -> esq){ //se o nó atual é filho esquerdo do pai, rotacionar pai para direita
                    RotacaoDireita(raiz,pai,avo); //a rotação é no pai, mas como precisamos do nó pai, o avô é o pai do pai
                    pai = pt; //o pai trocou de lugar com o filho(será útil depois da segunda rotação, 
                }
                
                //e entrando de qualquer jeito aqui
                RotacaoEsquerda(raiz,avo,Pop(ancestrais)); //a rotação é no avô, mas como precisamos do nó pai, acima na stack está o pai do avô
                //trocando as cores
                avo -> cor = true;    //o avô, sempre será black e por isso sempre irá virar red
                pai -> cor = false;   //o pai(seja o pai apenas desse caso, ou o nó atual, vindo após as 2 rotações) será sempre red e por isso irá virar black
            }

        }
    }
}

//o código do insere está bem repetitivo, eu poderia modularizar melhor
void Insere(node  **raiz, int chave){ //redblack tree não deixa de ser uma árovore de busca e como a inserir em árvore binária de busca é bem simples, não farei por recursão
    //lembrando que raiz é o ponteiro para o ponteiro da raiz, *raiz é o ponteiro para a raíz correta

    //caso árvore vazia
    if(*raiz == NULL){//árvore vazia
        *raiz = (node *)malloc(sizeof(node));  //alocar espaço para o novo nó 
        (*raiz) -> valor = chave;
        (*raiz) -> dir = NULL; //não tem filhos ainda também
        (*raiz) -> esq = NULL;
        (*raiz) -> cor = true; //o primeiro nó(raíz) deveria ser black, mas como dá errado no run.codes, tive que botar ele como red.
        return;  //primeiro nó inserido, podemos retornar
    }

    stack **ancestrais = NULL; //stack para ter acesso aos nós anteriores ao elemento inserido(pai e avô), começa vazia
    ancestrais = (stack **)malloc(sizeof(stack*)); //inicializando a stack de ancestrais
    *ancestrais = NULL;

    node *aux; //ponteiro auxiliar para percorrer e inserir na árvore
    aux = *raiz; //auxiliar começa na raiz

    while(true){  //enquanto a função não retornar, fica nesse loop
        if(chave == aux -> valor){ //valor já existe ná árvore
            return; //podemos descartar
        }
        //como aqui já teremos garantia que um valor será inserido
        Push(ancestrais, aux); //cada vez que o loop chegar aqui, um nó anterior ao próximo será inserido, então o último nó a ser inserido será o pai do novo elemento

        if(chave < aux -> valor){         //se o valor for menor que o nó atual, vamos para esquerda
            if(aux -> esq != NULL){       //se o nó esquerdo não for nulo
                aux = aux -> esq;          //iremos avançar para o filho esquerdo da árvore
            }
            else{                            //agora, se o filho esquerdo for nulo, adicionamos o nó
                aux -> esq = (node*)malloc(sizeof(node));   //alocamos espaço
                aux -> esq -> valor = chave;                //chave em seu devido campo
                aux -> esq -> esq = NULL;                 //o mais novo nó não tem filhos
                aux -> esq -> dir = NULL;
                aux -> esq -> cor = true;                   //nove filho sempre é vermelho

                
                ConsertArvore(raiz, aux->esq, ancestrais); //iremos verificar e consertar a árvore caso alguma propriedade tenha sido violada
                Libera(ancestrais); //caso tenha sobrado algum elemento na stack, vamos liberar
                return;    //saímos da função
            }
        }
        else{      //se não entrar no if anterior, é porque a chave é maior que o nó atual, então iremos para a direita
            if(aux -> dir != NULL){   //se já existir um filho direito
                
                aux = aux -> dir;      //continuaremos a prosseguir por ele

            }
            else{                       //agora se não existir, adicionamos o novo nó
                aux -> dir = (node*)malloc(sizeof(node));   //alocamos espaço
                aux -> dir -> valor = chave;              //chave em seu devido campo
                aux -> dir -> esq = NULL;             //novo nó não tem filhos
                aux -> dir -> dir = NULL;              
                aux -> dir -> cor = true;               //novo filho sempre é vermelho
                ConsertArvore(raiz, aux->dir, ancestrais); //iremos verificar e consertar a árvore caso alguma propriedade tenha sido violada
                Libera(ancestrais); //caso tenha sobrado algum elemento na stack, vamos liberar
                return;    //saímos da função
            }
        }

        
    }
}

void PrintArvore(node *raiz){
    //como é para mostrar em pré ordem, o nó é visitado(no caso, um printf) antes de prosseguir pela árvore
    printf("%d", raiz -> valor);   //printamos primeiro
    if(raiz -> cor){   //apenas para printar o sufixo correto de cor
        printf("R ");
    }
    else{
        printf("N ");
    }
    if(raiz -> esq != NULL)  //se tiver filhos a esquerda
    {                            
        PrintArvore(raiz -> esq);  //prosseguir por eles recursivamente
    }
    if(raiz -> dir != NULL){   //se tiver filhos a direita
        PrintArvore(raiz -> dir);  //prosseguir por eles recursivamente
    }

    //já que fizemos o que queriámos com a árvore, resta liberar a memória alocada
    free(raiz);
}


int main(){

    int chave;
    node **redblacktree; //um ponteiro para o ponteiro para a raíz da árvore, irá facilitar as rotações (e também inserção em árvore vazia)
    redblacktree = (node **)malloc(sizeof(node*));//alocando espaço para ele

    *redblacktree = NULL; //o verdadeiro ponteiro para a raíz, começa vazia
    


    while(scanf("%d",&chave) != -1){ //lendo chaves até o fim do arquivo
        Insere(redblacktree, chave);  //inserindo na árvore
    }

    //para mostrar a árvore
    PrintArvore(*redblacktree);
    


    free(redblacktree);
    return 0;

}