#include <iostream>
#include <stdlib.h>
#include <cstring>

/*Autor: Matheus Moura Gorchinsky
  DRE: 119043032
*/

//para ficar mais simples, não guardarei referência a caudas
typedef struct Node {
    int valor; //valor do vértice
    Node *prox;   //ponteiro o próximo elemento da lista encadeade de adjacências
}node;

typedef struct SortedNode {
    int valor; //valor do vértice na lista
    SortedNode *prox;   //ponteiro para o próximo elemento da lista
    SortedNode *ant;    //pronteiro para o elemento anterior da lista
}sortNode;

/*função para construir o grafo, *vertice se refere ao vértice atual
  valor se refere ao valor do vértice  que será adicionado a lista de adjacências.
*/
void Construtora(node *vertices, int valor){
    node *aux = vertices; //ponteiro auxiliar para percorrer a lista encadeada de adjacências até chegar no último elemento(pois não temos cauda), começa apontando para a cabeça
    while(aux -> prox != NULL){ //enquanto não chega no fim da lista
        aux = aux -> prox;     //apontará para o próximo
    }                       
    node *novaAdjacencia; //ao chegar no fim da lista, alocamos espaço para o novo vertice adjacente
    novaAdjacencia = (node*)malloc(sizeof(node));
    aux -> prox = novaAdjacencia;       //colocamos a nova adjacencia após o antigo fim da lista
    novaAdjacencia -> prox = NULL;        //então a nova adjacencia é o novo fim, aponta para NULL
    novaAdjacencia -> valor = valor;      //colocamos o valor no seu devido campo          
}

//Essa função adiciona elementos na lista duplamente encadeada de vértices organizados
void Adicionar(sortNode *lista, int valor){
    if(lista -> valor < 0){ //a lista só foi inicializada
        lista -> valor = valor; //valor em seu devido campo
        return; //e saímos da função
    }

    sortNode *aux = lista; //ponteiro auxiliar para percorrer a lista duplamente encadeada até chegar no último elemento(pois não temos cauda), começa apontando para a cabeça
    while(aux -> prox != NULL){ //enquanto não chega no fim da lista
        aux = aux -> prox;     //apontará para o próximo
    }                       
    sortNode *novoElemento; //ao chegar no fim da lista, alocamos espaço para o novo elemento
    novoElemento = (sortNode*)malloc(sizeof(sortNode));
    aux -> prox = novoElemento;       //o colocamos após o antigo fim da lista
    novoElemento -> prox = NULL;        //Este elemento é o novo fim, aponta para NULL
    novoElemento -> ant = aux;        //o anterior do novo elemento é o antigo último
    novoElemento -> valor = valor;      //colocamos o valor no seu devido campo    
}

//função recursiva de topological sort utilizando DFS
void TopologicalSort(int i,node *vertices,bool *visitados, sortNode *listaOrganizada){

    visitados[i] = true; //ao ser chamada, indica que o vértice foi visitado, então marcamos o vértice como visitado
    node *aux = &vertices[i]; //ponteiro auxiliar para percorrer as listas encadeadas de ajdacências
    while(aux -> prox != NULL){ //prosseguindo na lista de adjacências, pois queremos chegar em um vértice sem adjacências(sem uma aresta saindo dele para outro vértice)
        aux = aux -> prox;      //pois este vértice terá a "menor prioridade" no topological sort
        if(!visitados[(aux -> valor) - 1]){ //claro, checar se esse o vértice não foi checado ainda.
            TopologicalSort((aux -> valor) - 1, vertices, visitados, listaOrganizada); //se não foi checado, chamaremos a função de novo recursivamente
        }
    }
    //por ser uma recursão, o primeiro vértice a ser adicionado a lista será um vértice que não "envia arestas" para nenhum outro vértice
    //e o último a ser colocado será um vértice que não "recebe arestas" de nenhum outro vértice, logo, essa lista ficará ao contrário
    //mas ainda organizada
    Adicionar(listaOrganizada,vertices[i].valor);
}

//função para liberar alguns espaços de memória que foram alocacos durante a execução do programa
//essa função não irá liberar a lista organizada, isso foi feito durante o printf dela
void Liberadora(node *vertices, bool *visitados,int n){
    node *aux, *aux2;
    //primeiramente, liberar as listas de adjacências e por consequência, também o array de vértices
    for(int i = 0; i<n ;i++){
        aux = &vertices[i];  
        if(aux -> prox == NULL){ //se o vértice não tem lista de adjacências, deixamos sua liberação para depois(ao liberar o array de vértices no caso)
            break;
        }
        //caso possua lista de adjacências
        aux = aux -> prox;
        while(aux -> prox != NULL){
            aux2 = aux; //é necessário uma segunda auxiliar aqui para liberar a memória alocada sem perder a referência de percorrer a lista
            aux = aux -> prox;
            free(aux2);
        }
        free(aux); //liberação do últimi elemento da lista que fica fora do loop
    }
    //agora liberar os arrays de vértices e de visitados
    free(vertices);
    free(visitados);

}


int main(){
    int n,m;
    int auxNumero;
    char caracter;
    char numero[20]; //array de caracteres de 20 posições, pois nenhum int chegá perto de 19 dígitos

    scanf("%d %d \n", &n, &m);

    //como o número de vértices é definido nesta leitura, podemos usar listas sequenciais(arrays de alocação dinâmica) para 
    node *vertices; //criar a lista sequencial para representar os vértices do grafo
    vertices = (node*)malloc(n * sizeof(node));
    //e também para um indicador de vértices visitados(importante para a DFS do topological sort)
    bool *visitados;
    visitados = (bool*)malloc(n * sizeof(bool));

    sortNode *listaOrganizada;//lista duplamente encadeada para os vértices organizados
    listaOrganizada = (sortNode *)malloc(sizeof(sortNode)); //criamos o primeiro elemento para ela(e também a cabeça)
    listaOrganizada -> prox = NULL;  //não terá próximo elemento
    listaOrganizada -> ant = NULL;   //nem anterior
    listaOrganizada -> valor = -1; //valor placeholder para identificar que o primeiro elemento não existe e a lista só foi inicializada


    for(int i = 0; i < n; i++){
        int j = 0;

        //inicializando os vértices
        vertices[i].valor = i+1; //cada vértice sendo inicializado para que seu valor seja em relação a i-ésima linha 
        vertices[i].prox = NULL; //como estamos inicizaliando os vértices, as listas de adjacência estarão vazias
        //também é importante inicializar o array de bools
        visitados[i]=false;//nenhum vértice foi visitado pela DFS do sort ainda

        while(1){//o parsing da input é feito neste while (lendo dígitos de números até terminar um número(achar input que não é número(um espaço ou fim de linha no caso)))
            if(scanf("%c", &caracter) == -1){ //Lendo e avançando no buffer até chegar no fim do arquivo(este if é a última parte a ser executada)
                if(j){// se a última linha não for uma linha vazia
                    numero[j] = '\0'; //terminamos o número que faltava, última aresta do último vértice
                    auxNumero = atoi(numero); //e transformamos ele em inteiro
                    Construtora(&vertices[i], auxNumero); //e agora podemos adicionar este vértice na lista de adjacências e terminar de montar o grafo
                }
                break; //saímos do último loop de leitura
            }
            if(caracter >= '0' && caracter <= '9'){ //se for dígito
                numero[j] = caracter;   // coloca na string de numero
            }else{
                if(!j){ //se o primeiro caracter já não for um número, estamos em linha vazia
                    break; //pulamos a linha
                }
                numero[j] = '\0'; //se não for um número, mas não for o primeiro caracter, acabamos de terminar um número
                auxNumero = atoi(numero); //agora temos o número em inteiro que indica com qual vértice o vértice i tem aresta
                Construtora(&vertices[i], auxNumero); //e podemos adicionar este vértice na lista de adjacências e ir montando o grafo
                if(caracter == '\n'){ //se for caracter de fim de linha, pulamos a linha.
                    break;
                }
                j = 0;   //se não for, é um espaço, então continuamos a ler
                continue;
            }
            j++;
        }
    }

    /* esse trecho testava o grafo
    for(int i = 0;i<n;i++){
        node *auxiliar = &vertices[i];
        printf("teste de compare: vertice[%d] = %d\t",i+1,vertices[i].valor);
        while(auxiliar->prox != NULL){
            printf("valor do vertice = %d, e seu proximo = %d",auxiliar->valor,auxiliar->prox->valor);
            auxiliar = auxiliar -> prox;
        }
        printf("\n");
    }*/

    //agora, falta apenas realizar o topological sort dos grafos
    for(int i = 0;i < n; i++){
        if(!visitados[i]){
            TopologicalSort(i,vertices,visitados,listaOrganizada);
        }
    }

    //Com o sort realizado, printaremos o resultado
    //como a lista está ao contrário, percorremos até o fim primeiro
    while(listaOrganizada -> prox != NULL){
        listaOrganizada = listaOrganizada -> prox;
    }
    //e então voltaremos printando(também podemos aproveitar e ir liberando a memória)
    while(listaOrganizada -> ant != NULL){
        printf("%d ",listaOrganizada -> valor);
        listaOrganizada = listaOrganizada -> ant;
        free(listaOrganizada -> prox);
    }
    //o último valor não é printado quando o while acaba
    //nem o último espaço é liberado
    printf("%d",listaOrganizada -> valor); //então fazemos isso fora do loop
    free(listaOrganizada);

    //liberando o que falta liberar
    Liberadora(vertices,visitados,n);

    return 0;
}
