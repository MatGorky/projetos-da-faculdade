#include <chrono>
#include <iostream>
#include <random>
#include <stdlib.h>

/*Autor : Matheus Moura Gorchinsky     DRE:119043032*/

using namespace std;
using namespace std::chrono;


/*primeiramente, algumas explicações sobre o algoritmo
Para não ficar alocando mais e mais espaço na heap com malloc a cada chamada recursiva, o algoritmo está reaproveitando matrizes já criadas para
representar outras e isso tem 2 desvantagens. A primeira é que as variáveis podem ficar menos intuitivas, pois estarão representando matrizes diferentes
em momentos diferentes. A segunda é que cada operação de soma e subtração e até algumas de atribuição foram mescladas, então elas são bem específicas, o que
acaba tirando a modularidade do algoritmo(não dá para criar funções de soma genéricas e ficar reutilando sempre)
enfim, para evitar a confusão, eu irei comentar os trechos do código e darei minhas explicações sobre algumas escolhas 
*/

int tamanhoIdeal = 32; /*tamanho máximo para o qual é melhor usar o algoritmo de multiplicação tradicional dentro do strassen.
Após realizar alguns testes com matrizes 2^10 x 2^10(o trecho de testagem está no main), tanto sem otimização quanto com otimização -O2, 
a maioria dos testes resultou no tamanho tamanho ideal para manter o algoritmo clássico 2^5, no entanto, as vezes 2^4 era melhor */


void Strassen(int **A,int **B, int **C, int tamanho)
{ 
    if(tamanho <= tamanhoIdeal){ //caso tamanho seja menor ou igual ao tamanho ideal, podemos utilizar o algoritmo clássico
        for(int i = 0;i < tamanho ; i++){
            for(int j = 0; j < tamanho ; j++){ 
                C[i][j] = 0; //como não inicialisamos com calloc, é melhor garantir que a posição esteja zerada, para não somarmos lixo na matriz
                for(int k = 0; k < tamanho ; k++){
                    C[i][j] += A[i][k] * B[k][j]; //o resultado será colocado em C, a matriz que é o terceiroa argumento
                }
            }
        }
        return;     //e sair desta chamada
    }
    //se o tamanho for maior que o tamanho ideal, prosseguiremos com o algoritmo de strassen

    int novoTamanho = tamanho/2; //para dividir as matrizes em 4 submatrizes, cada dimensão será reduzida pela metade
    //indexAtualizado será utilizado para definir o começo de uma nova submatriz, então é incrementando com o novo tamanho a cada instância de nível inferior de strassen(começa obrigatoriamente com)

    int **a22, **b22;  /*as matrizes que representam as subdivisões das originais, 
    temos 3 matrizes, se cada uma tem 4 subdiviões, éramos para ter 12 matrizes, no entanto, quase todas podem facilmente ser reaproveitadas das anteriores nas somas e subtrações do strassen
    apenas a subdivisão a22 da matriz A e a subdivisão b22 da matriz B que entram em uma multiplicação, então participarão diretamente de uma chamada recursiva, então é melhor serem matrizes próprias
    a11 e b11 também entram em multiplicação diretamente, no entanto, como elas tem o mesmo endereço inicial que a matriz anterior, apenas reduzir o tamanho já é suficiente para representar essa subdivisão*/

    int **resultado1, **resultado2; //neste contexto, resultado1 e 2 irão guardar algumas operações de soma e subtração que serão realizadas ao longo do algoritmo
    
    int **mat1, **mat2, **mat3, **mat4; //**m5, **m6, **m7; //matrizes que guardarão as multiplicações, eram para ser 7, de m1 até m7, no entanto, cada atribuição as Submatrizes de C, necessitam apenas de 4 matrizes

    a22 = (int**)malloc(novoTamanho * sizeof(int*));        //alocando espaço para todas as matrizes
    b22 = (int**)malloc(novoTamanho * sizeof(int*));
    resultado1 = (int**)malloc(novoTamanho * sizeof(int*));
    resultado2 = (int**)malloc(novoTamanho * sizeof(int*));
    mat1 = (int**)malloc(novoTamanho * sizeof(int*));
    mat2 = (int**)malloc(novoTamanho * sizeof(int*));
    mat3 = (int**)malloc(novoTamanho * sizeof(int*));
    mat4 = (int**)malloc(novoTamanho * sizeof(int*));
    

    for(int i = 0;i < novoTamanho; i++){
        a22[i] = (int*)malloc(novoTamanho * sizeof(int));
        b22[i] = (int*)malloc(novoTamanho * sizeof(int));
        for(int j = 0; j < novoTamanho; j++){ //para agilizar, já que é possível, melhor inicialisar a22 e b22 aqui também
            a22[i][j] = A[i + novoTamanho][j + novoTamanho];
            b22[i][j] = B[i + novoTamanho][j + novoTamanho];
            
        }
        resultado1[i] = (int*)malloc(novoTamanho * sizeof(int));
        resultado2[i] = (int*)malloc(novoTamanho * sizeof(int));
        mat1[i] = (int*)malloc(novoTamanho * sizeof(int));
        mat2[i] = (int*)malloc(novoTamanho * sizeof(int));
        mat3[i] = (int*)malloc(novoTamanho * sizeof(int));
        mat4[i] = (int*)malloc(novoTamanho * sizeof(int));
    }

    
    //Primeiramente, queremos calcular c11 = m1 + m4 - m5 + m7
    //agora começa o algoritmo de verdade, para calcular m1 =  (a11 + a22)(b11 + b22)
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            resultado1[i][j] = A[i][j] + a22[i][j]; //a11 + a22
            resultado2[i][j] = B[i][j] + b22[i][j]; //b11 + b22
        }
    }
    Strassen(resultado1, resultado2, mat1, novoTamanho); //chamada recursiva para calcular m1, mat1 irá guardar m1

    //para calcular m4 = a22 * (b21 - b11)
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            resultado1[i][j] = B[i + novoTamanho][j] - B[i][j]; //b21 - b11
        }
    }
    Strassen(a22, resultado1, mat2, novoTamanho); //chamada recursiva para calcular m4, mat2 irá guardar m4

    //para calcular m5 = (a11 + a12) * b22
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            resultado1[i][j] = A[i][j] + A[i][j + novoTamanho]; //a11 + a12
        }
    }

    Strassen(resultado1, b22, mat3, novoTamanho); //chamada recursiva para calcular m5, mat3 irá guardar m5

    //para calcular m7 = (a12 - a22)(b21 + b22)
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            resultado1[i][j] = A[i][j + novoTamanho] - a22[i][j] ; //a12 - a21
            resultado2[i][j] = B[i + novoTamanho][j] + b22[i][j]; //b21 + b22
        }
    }
    Strassen(resultado1, resultado2, mat4, novoTamanho); //chamada recursiva para calcular m7, mat4 irá guardar m7

    //agora temos todas as matrizes necessárias para calcular c11, e também, vemos que m7 não será utilizada mais em nenhuma operação, então podemos fazer mat4 guardar m3, para já prepararmos o cálculo de c12
    //c11 = m1(em mat1) + m4(em mat2) - m5(em mat3) + m7(em mat4)
    //para calcular m3 = a11 * (b12 - b22)
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            C[i][j] = mat1[i][j] + mat2[i][j] - mat3[i][j] + mat4[i][j]; //aqui estamos calculando c11
            resultado1[i][j] = B[i][j + novoTamanho] - b22[i][j]; //b12 - b22, preparando para o cálculo de m3
        }
    }
    Strassen(A, resultado1, mat4, novoTamanho); //chamada recursiva para calcular m3, mat4 irá guardar m3 agora

    //mas agora já temos m3(em mat4) e m5(em mat3), podemos calcular c12 = m3 + m5 agora, e também, após isso, m5 não será utilizada em nenhuma outra operação, podemos calcular m2 logo

    //para calcular m2 = (a21 + a22) * b11
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            C[i][j + novoTamanho] = mat4[i][j] + mat3[i][j]; //calculando c12
            resultado1[i][j] = A[i + novoTamanho][j] + a22[i][j]; //a21 + a22, preparando para o cálculo de m2
        }
    }
    Strassen(resultado1, B, mat3, novoTamanho); //chamada recursiva para calcular m2, mat3 irá guardar m2

    //já tendo m2(em mat3) e m4(em mat2), dá para calcular c21 = m2 + m4 e descartar m4 para termos m6

    //para calcular m6 = (a21 - a11) * (b11 + b12)
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            C[i + novoTamanho][j] = mat3[i][j] + mat2[i][j];
            resultado1[i][j] = A[i + novoTamanho][j] - A[i][j] ; //a21 - a11, preparando para o cálculo de m6
            resultado2[i][j] = B[i][j] + B[i][j + novoTamanho]; //b11 + b12
        }
    }
    Strassen(resultado1, resultado2, mat2, novoTamanho); //chamada recursiva para calcular m6, mat2 irá guardar m6

    //falta apenas calcular C22, estará em um loop só para isso
    //C22 = m1(em mat1) - m2(em mat3) + m3(em mat4) + m6(em mat2)
    
    for(int i = 0;i < novoTamanho; i++){
        for(int j = 0;j < novoTamanho; j++){
            C[i + novoTamanho][j + novoTamanho] = mat1[i][j] - mat3[i][j] + mat4[i][j] + mat2[i][j];
        }
    }
    //resta apenas liberar a memória alocada
    for(int i = 0; i < novoTamanho; i++){
        free(a22[i]);
        free(b22[i]);
        free(resultado1[i]);
        free(resultado2[i]);
        free(mat1[i]);
        free(mat2[i]);
        free(mat3[i]);
        free(mat4[i]);
    }
    free(a22);
    free(b22);
    free(resultado1);
    free(resultado2);
    free(mat1);
    free(mat2);
    free(mat3);
    free(mat4);

}


int main()
{
    std::ios_base::sync_with_stdio(false);

    int n;
    cout << "Entre com n para montar uma matriz 2^n x 2^n" << endl;
    cin >> n; //lendo n
    n = pow(2,n); //para manter a matriz 2^n x 2^n
    

    //alocando espaço para as matrizes
    int **matriz1 = (int**)malloc(n * sizeof(int*)); //matrizes que serão multiplicadas
    int **matriz2 = (int**)malloc(n * sizeof(int*));
    int **resultado1 = (int**)malloc(n * sizeof(int*)); //resultado para a multiplicação clássica
    int **resultado2 = (int**)malloc(n * sizeof(int*)); //e para o strassen
    for(int i = 0;i < n; i++){
        matriz1[i] = (int*)malloc(n * sizeof(int));
        matriz2[i] = (int*)malloc(n * sizeof(int));
        resultado1[i] = (int*)calloc(n , sizeof(int)); //calloc pois quero inicializar a matriz do resultado do algoritmo clássico com 0
        resultado2[i] = (int*)malloc(n * sizeof(int)); //como no strassen muitas matrizes serão criadas, o uso do calloc pode ser pior que colocar 0 nas posições certas no algoritmo de multiplicação dentro do strassen
    }
    
    time_t seed = system_clock::to_time_t(system_clock::now()); //gerando seed para o rand, má prática de randomização, mas simples
    srand(seed);

    //inicializando as matrizes
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n;j++){
            matriz1[i][j] = (rand() % 2001) - 1000; //rand gera qualquer inteiro aleatório, módulo com 2001 quer dizer que os resultados serão sempre entre 0 e 2000
            matriz2[i][j] = (rand() % 2001) - 1000; //subtraindo 1000, o resultado estará sempre entre -1000 e 1000            
        }
    }
    
    auto inic = high_resolution_clock::now(); //começa o contador de tempo

    /*agora para o algoritmo clássico de multiplicação de matrizes, basta fazer o passo a passo iterativamente
    cada posição da matriz resultante será a soma da multiplicação de cada elemento da linha respectiva da matriz 1 com cada elemento da coluna respectiva da matriz 2
    isto é, resultado i,j = (matriz1 i,1 * matriz2 1,j) + (matriz1 i,2 * matriz2 2,j) + ... + (matriz1 i,n * matriz2 n,j)
    então precisaremos de um loop extra para incrementar a variável deste somatório.*/

    for(int i = 0;i < n; i++){
        for(int j = 0; j < n; j++){
            for(int k = 0; k < n; k++){
                resultado1[i][j] += matriz1[i][k] * matriz2[k][j];
            }
        }
    }

    auto fim = high_resolution_clock::now(); //finaliza o contador de tempo

    auto duration = duration_cast<milliseconds>(fim - inic);
    cout << duration.count() << " millisegundos.\ttempo de execucao do algoritmo classico na matriz " << n << "x" << n << endl;


    //agora para o algoritmo de strassen

    inic = high_resolution_clock::now();

    Strassen(matriz1, matriz2, resultado2, n); //matriz 1 e 2 para serem multiplicadas e resultado2 será a matriz onde resultado será guardado. n é o tamanho da matriz

    fim = high_resolution_clock::now();
    
    duration = duration_cast<milliseconds>(fim - inic);
    cout << duration.count() << " millisegundos.\ttempo de execucao do algoritmo de strassen na matriz " << n << "x" << n << endl;

    //verificando se tudo deu certo
    bool correto = true;
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            if(resultado1[i][j] != resultado2[i][j]){
                correto = false;
            }
        }
    }

    if(correto){
        cout << "Resultado Correto" << endl;
    }

    //Realizando testes para descobrir o tamanho ideal até onde o algoritmo clássico é preferível ao strassen
    cout << "\nIniciando os testes de tempo" << endl;
    for(int i = 0;i < 11; i++){
        inic = high_resolution_clock::now();
        tamanhoIdeal = pow(2,i);
        Strassen(matriz1, matriz2, resultado2, n);
        fim = high_resolution_clock::now();
        duration = duration_cast<milliseconds>(fim - inic);
        cout << duration.count() << " millisegundos.\ttempo de execucao do algoritmo de strassen na matriz " << n << "x" << n << " com tamanho ideal: "<< tamanhoIdeal <<endl;
    }
    cout << "\nTestes finalizados" << endl;

    //liberando o espaço que aloquei nas matrizes
    for(int i = 0; i < n; i++){
        free(resultado1[i]);
        free(resultado2[i]);
        free(matriz1[i]);
        free(matriz2[i]);
    }
    
    free(resultado1);
    free(resultado2);
    free(matriz1);
    free(matriz2);
    
    return 0;
}