#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>          
#include <unistd.h>                 
#include <string.h>                 
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>   

#define MAXLINE 8192
#define MAXARGS 128
#define PATH_MAX 4096 //maior tamanho de path possível no linux

sigjmp_buf jmpbuf;
extern char **environ; //envp
const char *builtinList[] = {"quit - Sai da shell","jobs - lista os processos que estão sendo executados no background","cd [Caminho] - muda de diretório","fg [PID/%JID] - faz um programa rodar no foreground","bg [PID/%JID] - faz um programa rodar no background","pwd - mostra o caminho até o diretório atual"}; /*um array para a função help*/
char path[PATH_MAX];
char oldpath[PATH_MAX];
int shellpid; //pid da shell
int foregroundpid;//pid da job foreground
int fgrunning = 0; //se tiver com um processo foreground rodando, true, se não tiver, false, inicializado no false
int topo = -1; //apesar de não ser uma stack, pois não é necessariamente LIFO, a lista de jobs tem alguns comportamentos parecidos, então marcando um topo
//topo inicializado com - 1, pois começa vazia
//uma struct para jobs, irá montar o array que será mostrado pelo builtin "jobs"
typedef struct tJobs{ //o ideal seria fazer uma lista dinâmica, mas como o tempo é curto e enfrentar bugs de alocação dinâmica o reduziria, faremos por array mesmo
  pid_t pid; //pid = 0 irá representar que está vazio,
  char name[100]; //nome do comando
  char status[100]; //status do comando;
  //jid será o index do array + 1

}Jobs;

Jobs joblist[1000] = {0}; //inicializando com 0 e infelizmente, limitado em 1000

//funções essenciais para shell
void evaluate(char *commandline); //irá interpretar a input do usuário para um comando
int parseline(char *buffer, char **argv); //essa função pegará a input de entrada e separá cada palavra entre espaços, pois elas representam um comando e seus argumentos, e irá colocar cada um em uma posição do argv
int builtin_command(char **argv); //irá checar se o comando é uma função builtin e executar se for


//funções "wrapper" para alguns comandos builtin que tem um funcionamento um pouco mais complexo
void help(); //irá rodar o código de help caso o comando passado seja o builtin help
int chd(char **argv); //irá rodar o código de chd caso o comando passado seja o builtin cd
void listjobs();//irá rodar o código de listjobs caso o comando passado seja o builtin jobs
void bg(char **argv);//irá rodar o código de bg caso o comando passado seja o builtin bg
void fg(char **argv);//irá rodar o código de fg caso o comando passado seja o builtin fg

//handlers de sinais
void handler_interrupt(int sig); //handler de SIGINT, termina o processo foreground com ctrl+c, mas ignora a shell
void handler_stop(int sig); //handler de SIGTSTP, para o processo foreground com ctrl+z, e também ignora a shell
void handler_reaper(int sig); //handler de SIGCHLD, coleta os zumbis

//funções auxiliares
void input();//apenas uma wrapper para recebimento de input no main, para deixar o main um pouco mais limpo
void addjob(pid_t pid, char *name, char *status);//adiciona job ao array de jobs
int removejob(pid_t pid);//tira uma job do array de jobs e retorna o jid da job removida

void evaluate(char *commandline){ 
    char *argv[MAXARGS];    //o argument values do programa que será chamado pelo execve, onde argv[0] é o nome do programa e os seguintes são outros argumentos
    char buffer[MAXLINE];   //uma cópia da linha de comando, que pode ser modificada pelo parse
    int bgflag;     //determina se o processo vai ocorrer em bg(background, a shell não espera) ou fg(foreground, a shell espera)
    pid_t pid;      //no processo pai, vai guardar id do processo filho e no processo filho, guardar 0

    
    strcpy(buffer,commandline);
    bgflag = parseline(buffer,argv);
    if(argv[0] == NULL)
        return;     //sai da função se a lista estiver vazia, pois não há commando para ser executado

    if(!builtin_command(argv)){ //isso irá checar se é builtin ou não, se for, irá rodar.
        fgrunning = !bgflag; //se não for builtin, pode então agora saber se é background ou foreground       
        setpgid(0,0); //criando um grupo para shell, igual ao pid dela, fazendo isso em cada loop pois a documentação do gnu recomendou(para evitar problemas de corrida?)

        //se não for built-in, prosseguir com execve
        if((pid = fork()) == 0){ //realiza um fork e no filho gerado entra neste if
            //todo processo(job) deve estar em grupo diferente
            setpgid(0, 0); //irá criar um novo grupo em que o pgid será igual ao pid do processo



            if(execve(argv[0],argv,environ) < 0){ //chama execve pra executar o programa, se falhar, entra neste if
                printf("%s: Comando inexistente, digite 'help' para uma lista de comandos.\n", argv[0]);
                exit(0); //termina o processo filho que tentou executar o comando
            }
        }
        //essa parte só executa para a shell(Processo pai), pois entrando no execve, o filho não tem mais acesso a essa parte, e não entrando, ele termina.
        if(fgrunning){ //se estiver um foreground rodando
            tcsetpgrp(STDIN_FILENO, pid); //transfere o controle do terminal para a job foreground(processo filho sendo executado)
            int status;
            foregroundpid = pid; //já que a job está no foreground, vamos salvar seu pid
            if((waitpid(pid, &status, WUNTRACED)) < 0){ //espera até foreground job terminar ou parar
                printf("waitfg: waitpid error");
                exit(1);                          
            }
            else //se não teve erro
            {
                if(WIFSTOPPED(status)){//caso o processo tenha sido parado por um sigtstp
                    int jid = topo+2;//como topo + 1 será o index, jid será topo + 2
                    printf("\nstopped: [%d] e %d\n",jid,pid); //como o processo foi parado, já mostra pid e jid dele
                    addjob(pid,commandline,"stopped"); //coloca na lista de jobs
                }
                tcsetpgrp(STDIN_FILENO, shellpid);//retorna o controle do terminal para a shell
            }
        }else{ //caso seja background
            //processos background devem ir para a lista de jobs direto
            int jid = topo+2;//como topo + 1 será o index, jid será topo + 2
            printf("\nrunning: [%d] e %d\n",jid,pid); //como o processo foi parado, já mostra pid e jid dele
            addjob(pid,commandline,"running"); //coloca na lista de jobs
        }
        fgrunning = 0;
        
    }
    return;


}
int parseline(char *buffer, char **argv){
    int argc; //argument counter, número de argumentos
    int bgflag; //será retornado para saber se é background ou foreground pelo & no fim
    char *delim; //como o nome diz, irá apontar para a primeira ocorrência de espaço, e então terminar a string ali

    buffer[strlen(buffer) -1] = ' '; //como foi lido pela função gets, ele guarda o \n no final da string, então estamos substituindo ele pelo \0 que determina o final da leitura da string
    while(*buffer && (*buffer == ' ')){ //enquanto não estiver em byte nulo e o caracter atual for um espaço, incrementaremos o buffer
        buffer++; //pois, se houver espaços na frente do comando, iremos ignorá-los
    }

    argc = 0;
    while((delim = strchr(buffer,' '))) { 
        *delim = '\0'; //delim aponta para a primeira ocorrência de um espaço, substituindo pelo byte nulo, temos então uma string separada(delimitada)
        argv[argc] = buffer;
        argc++;
        buffer = delim + 1;//vai para depois da string delimitada
        while(*buffer && (*buffer == ' ')){ // o mesmo loop visto anterior, pois as vezes o usuário pode digitar o comando com mais de um espaço entre palavras
            buffer++; 
        }
    }
    argv[argc] = NULL; //no final do loop, por exemplo, se temos 5 argumentos, argc = 5, mas argv vai de 0 e 4, então a posição 5 será nula, para indicar que houve término.

    if(argc == 0){ //linha vazia, ignorar
        return 1;
    }

    if((bgflag = (*argv[argc-1] == '&')) != 0){ //basicamente, se a última entrada for '&', bgflag recebe 1, senão recebe 0
        argv[argc-1] = NULL; //e claro, se for, o '&' cumpriu sua função, então pode ser descartado, pois se algum momento quisermos listar argv, não precisaremos do &
    }

    return bgflag; //após isso, se bgflag receber 1, então é para o background
}   

int builtin_command(char **argv){//se for um comando builtin, executa ele
    if(!strcmp(argv[0],"quit")){
        exit(0);                    //se for quit, apenas termina o processo
    }
    if(!strcmp(argv[0],"help")){
        help();                   //se for help, lista os possíveis comandos e dará algumas instruções
        return 1;
    }
    if(!strcmp(argv[0],"pwd")){
        printf("%s\n",getcwd(path,PATH_MAX)); //para pegar o diretório atual, apenas precisamos de getcwd
        return 1;
    }
    if(!strcmp(argv[0],"jobs")){ //printa a lista de jobs
        listjobs();
        return 1;
    }
    if(!strcmp(argv[0],"fg")){ //leva uma job do background pro foreground
        fg(argv);
        return 1;
    }
    if(!strcmp(argv[0],"bg")){ //faz uma job que está parada começar a ser executada no background
        bg(argv);
        return 1;
    }
    if(!strcmp(argv[0],"cd")){ //para mudar o diretório atual, usaremos a função chdir, mas antes, passaremos pelo wrapper chd
        chd(argv);
        return 1;
    }
    return 0;               //se não for nenhum builtin, retorna 0 e prossegue
}
void help(){
    int builtinListSize = sizeof(builtinList)/sizeof(builtinList[0]); //tamanho do array de caracteres(matriz de strings, com os comandos)
    for(int i = 0;i<builtinListSize; i++)
    {
        printf("%s\n",builtinList[i]); //printa a lista de builtins
    }
}

int chd(char **argv){
    char home[100];
    char *lpath = path; //ponteiro local para variável global path
    char aux[PATH_MAX];
    strcpy(home,getenv("HOME"));
    
    if((argv[1]) == NULL){ //chdir não reconhece algumas coisas que cd reconhece, por exemplo, cd vazio retorna para home
        strcpy(lpath,home); //então, faremos isso manualmente
    }
    else{
        strcpy(lpath,argv[1]); //path agora tem o conteúdo de argv[1]
    }

    if((lpath[0]) == '~'){ //se o primeiro caracter de path for ~
        lpath++; //incrementando 1 no ponteiro, pois queremos ignorar esse primeiro caracter
        strcat(home,lpath); //concatenando path no fim de home
        strcpy(lpath,home); //mas queremos o conteúdo em path
    }
    if((lpath[0]) == '-'){//se o primeiro caracter de path for -, volta pro path anterior
        strcpy(lpath,oldpath); //path recebe copia de oldpath
    }
    strcpy(aux,oldpath);//aux guarda copia de oldpath para poder restaurar em caso de erro
    getcwd(oldpath,PATH_MAX); //oldpath recebe diretório atual, pois o atual será atualizado logo após
    if(chdir(lpath) < 0){ //se chdir falhar, retorna -1 e seta errno, entrando neste if
        char errorstring[PATH_MAX+4]; //string de erro, para error
        sprintf(errorstring,"cd: %s",lpath);//formatando a string de erro
        perror(errorstring);
        strcpy(oldpath,aux); //houve erro, oldpath precisa ser restaurado
        return 0;
    }
    
    return 1;
}

void listjobs(){
    int i = 0;
    while(i<=topo){//enquanto não chegou no fim do array de jobs ainda
        if(joblist[i].pid != 0){ //se não está numa posição vazia
            printf("[%d] %d  %s       %s",i+1, joblist[i].pid,joblist[i].status,joblist[i].name);
        }
        i++;
    }
}
void bg(char **argv){ 
    char id[12]; //será jid ou pid
    char *idp = id; //ponteiro para id
    int idinteger; //id integer, id é uma string e sera convertindo em inteiro
    if(argv[1]==NULL && topo > -1){//bg sem nada, então manda o mais recente(topo) começar a rodar em background
        kill(joblist[topo].pid,SIGCONT); //manda o processo rodar
        strcpy(joblist[topo].status,"running"); //muda o status para running
        return; //sai da função
    }else if (topo == -1){
        printf("Lista de jobs vazia\n");
    }   
        else //há argumento
        {
            strcpy(idp,argv[1]); //id agora guarda o argv
        }
    
    if(idp[0] == '%'){//verificamos se o usuário entrou com jid e se ele entrou, id é jid
        idp ++; //iremos avançar no buffer, para ignorar o '%'
        idinteger = atoi(idp); //transforma a string em inteiro
        if(idinteger < 1001 && joblist[idinteger-1].pid != 0){//se a job está na job list 
            kill(joblist[idinteger-1].pid,SIGCONT); //manda o processo rodar e lembrando que jid é index + 1
            strcpy(joblist[idinteger-1].status,"running"); //muda o status para running
        }
        else{
            printf("JID inexistente na lista de jobs\n");
        }
    }else //usuário entrou com pid
    {
        idinteger = atoi(idp);
        int j;
        while(j<=topo){//não chegou no fim do array de jobs ainda
            if(joblist[j].pid == idinteger){ //achou o pid da job na lista
                kill(joblist[j].pid,SIGCONT); //manda o processo rodar na posição desse pid
                strcpy(joblist[j].status,"running"); //muda o status para running
                return; //sai da função
            }
            j++;
        }
        //se saiu do while, foi porque não achou o pid na lista
        printf("PID inexistente na lista de jobs\n");
    }
}


void fg(char **argv){ ///a bg foi feita de forma meio redundante, a fg será feita de forma mais inteligente
    char id[12]; //será jid ou pid
    char *idp = id; //ponteiro para id
    int idinteger; //id integer, id é uma string e sera convertindo em inteiro
    int index; //index da job bg na joblist
    pid_t pid; //pid local
    if(argv[1]==NULL && topo > -1){//fg sem nada, então manda o mais recente(topo) para o foreground
        pid = joblist[topo].pid; //apenas atualiza o pid local
    }else if (topo == -1){
        printf("Lista de jobs vazia\n");
        return;
    }   
    else if(argv[1]!= NULL)//há argumento
    {
        strcpy(idp,argv[1]); //id agora guarda o argv
        if(idp[0] == '%'){//verificamos se o usuário entrou com jid e se ele entrou, id é jid
            idp ++; //iremos avançar no buffer, para ignorar o '%'
            idinteger = atoi(idp); //transforma a string em inteiro
            if(idinteger < 1001 && joblist[idinteger-1].pid != 0){//se a job está na job list 
                pid = joblist[idinteger-1].pid; //apenas atualiza o pid local
            }
            else{
                printf("JID inexistente na lista de jobs\n");
                return;
            }
        }else //usuário entrou com pid
        {
            idinteger = atoi(idp);
            int j;
            while(j<=topo){//não chegou no fim do array de jobs ainda
                if(joblist[j].pid == idinteger){ //achou o pid da job na lista
                    pid = idinteger; //apenas atualiza o pid local
                }
                j++;
            }
            if(j>topo && pid != idinteger){
                printf("PID inexistente na lista de jobs\n");
                return;
            }
        }
    }
    //pid foi atualizado em diversos casos, e se não foi, o return já tirou da função, então podemos fazer isso
    printf("%d\n",pid);
    kill(pid,SIGCONT); //o processo está parado, vamos continuar ele
    tcsetpgrp(STDIN_FILENO, pid); //transfere o controle do terminal para a job foreground
    printf("%d\n",pid);
    int status;
    foregroundpid = pid; //já que a job está no foreground, vamos salvar seu pid
    fgrunning = 1;
    index = removejob(pid); //pode remover a job da lista de jobs background, index do removido é retornado
    if((waitpid(pid, &status, WUNTRACED)) < 0){ //espera até foreground job terminar ou parar
        printf("waitfg: waitpid error");
        exit(1);                          
    }else //se não teve erro
    {

        if(WIFSTOPPED(status)){//caso o processo tenha sido parado por um sigtstp
            int jid = topo+2;//como topo + 1 será o index, jid será topo + 2
            printf("\nstopped: [%d] e %d\n",jid,pid); //como o processo foi parado, já mostra pid e jid dele
            addjob(pid,joblist[index].name,"stopped"); //coloca na lista de jobs
        }
        tcsetpgrp(STDIN_FILENO, shellpid);//retorna o controle do terminal para a shell
    }
    fgrunning = 0;
    tcsetpgrp(STDIN_FILENO, shellpid);//retorna o controle do terminal para a shell
}
    
    

void input(){
    char commandline[MAXLINE];
    fgets(commandline, MAXLINE, stdin);
    if (feof(stdin)){    //termina de executar em end of file(ctrl+d)
        exit(0);
    }
    evaluate(commandline);
}

void addjob(pid_t pid, char *name, char *status){//addjob não tem mistério, adiciona a nova job acima do topo e incrementa o topo
    joblist[topo+1].pid = pid;
    strcpy(joblist[topo+1].name,name);
    strcpy(joblist[topo+1].status,status);
    topo ++;
}

int removejob(pid_t pid){ //remove a job pelo pid
    int jidret;
    if(joblist[topo].pid == pid){ //queremos remover o topo
        jidret = topo+1;
        for(int i = topo;i>-1;i--){ //percorrendo do topo para baixo para achar a posição abaixo mais próxima que possa ser o novo topo
            if(joblist[i-1].pid!=0){  //assim que acharmos uma posição que não está vazia
                joblist[topo].pid=0; //removendo a posição topo;
                topo = i-1;   //novo topo
                return jidret;
            }
            if(i==0){ //lista vazia
                topo = -1;
                return jidret;
            }
        }
    }else{  //não queremos remover o topo
        for(int i = 0;i<topo;i++){
            if(joblist[i].pid==pid){
                joblist[i].pid = 0;
                jidret = i + 1;
                return jidret;
            }
        }
    }


}

void handler_interrupt(int sig){ //handler de SIGINT
    //na verdade eu criei 2 handlers, uma pra kill e outro pra stop apenas por "boas práticas, pois poderia ser apenas 1"

    //pois o que queremos é ignorar o sinal na shell(processo pai) e ter comportamento padrão no execve(processo filho)
    
    //se utilizássemos SIG_IGN, ele seria ignorado nos 2, mas se botarmos um handler vazio, ele não faz nada na shell(ok) e é substituído pelo default no execve
}

void handler_stop(int sig){ //handler de SIGTSTP

}
void handler_reaper(int sig) //handler de SIGCHLD
{
    int jid; //jid que será dada pelo remove
    int status; //status do waitpit
    pid_t pid; //pid local
    if(!fgrunning){ //apenas se fg não estiver rodando 
        while((pid = waitpid(-1,&status,WNOHANG | WUNTRACED)) > 0){ //apesar do uso do waitpid, o handler só será chamado quando um filho terminar, pois ele enviará o sinal de sigchld
        //nohang para não parar o programa
        //então na verdade, enquanto houver filhos que já terminaram, waitpid irá coletar eles, liberando recursos
        //o WUNTRACED, também dectará filhos que pararam, isso já será explicado
        if(WIFEXITED(status))//se o filho terminou
            jid = removejob(pid); //remove pid retorna a jid da job removida
            printf("\n[%d] %d  %s       %s",jid, pid , "terminated",joblist[jid].name);
        }
        if(WIFSTOPPED(status)){ //Aqui então, explicando o WUNTRACED se algum background tiver rodando e parar por algum motivo, a joblist será atualizado
            int j;
            while(j<=topo){//não chegou no fim do array de jobs ainda
                if(joblist[j].pid == pid){ //achou o pid da job na lista
                    strcpy(joblist[j].status,"stopped"); //muda o status para stopped
                    return; //sai da função
                }
                j++;
            }
        }
        
    }
    
}

int main(){
    if(signal(SIGINT, handler_interrupt) == SIG_ERR){ //se não conseguir instalar os handlers, não poderemeos executar
        exit(1);
    }
    if(signal(SIGCHLD, handler_reaper) == SIG_ERR){
        exit(1);
    }
    if(signal(SIGTSTP, handler_stop) == SIG_ERR){
        exit(1);
    }
    //queremos ignorar estes sinais para que a nossa shell não seja parada ao virar uma job background do linux quando executamos algum comando nosso que seja foreground
    signal (SIGTTIN, SIG_IGN); //quando tentar receber input
    signal (SIGTTOU, SIG_IGN); //quando tentar liberar output

    shellpid = getpid(); //salvando o pid da shell na variável global

    tcsetpgrp(STDIN_FILENO, shellpid);//leva controle do terminal para a shell
    printf ("Bem vindo ao lumashell\nDigite 'help' para listar comandos\n");
    while(1){ //loop infinito, para quantos comandos o usuário quiser
        printf("lumashell> ");
        input(); //função para receber a entrada do usuário, uma linha de comando
    }
}