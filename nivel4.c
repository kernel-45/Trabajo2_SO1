#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define PROMPT '$'
#define MAX_COMAND_SIZE 1024  //llargada MAX de cada comanda
#define ARGS_SIZE 64
#define N_JOBS 10   //Numero de trabajos que almacena la tabla
static char mi_shell[MAX_COMAND_SIZE]; //variable global para guardar el nombre del minishell
char line[MAX_COMAND_SIZE]; //Array on guardem la comanda
//Declaración struct jobs
struct info_job {
   pid_t pid;
   char estado; // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: Ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado) 
   char cmd[MAX_COMAND_SIZE]; // línea de comando asociada
};

//Declaración tabla jobs
static struct info_job jobs_list [N_JOBS]; 

//declaració funcions
char *read_line(char *line);
void imprimir_prompt();
int execute_line(char *line);
int parse_args(char **args, char *line);
int check_internal(char **args);
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs(char **args);
int internal_fg(char **args);
int internal_bg(char **args);
void resetear_job(int idx); 
void actualizar_job(int numTabla, pid_t pid, char estado, char cmd[]); 
void reaper(int signum);
void ctrlc(int signum);

//Definició colors
#define RESET "\033[0m"     //Posar els colors per defecte
#define NEGRO_T "\x1b[30m"  //Text Negre
#define NEGRO_F "\x1b[40m"  //Fons negre
#define GRIS_T "\x1b[90m"   //La resta canvia el color del text per l'especificat per el nom
#define ROJO_T "\x1b[31m"
#define VERDE_T "\x1b[32m"
#define AMARILLO_T "\x1b[33m"
#define AZUL_T "\x1b[34m"
#define MAGENTA_T "\x1b[35m"
#define CYAN_T "\x1b[36m"
#define BLANCO_T "\x1b[97m"
#define NEGRITA "\x1b[1m"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////Si tarda más de 1 segundo, puede hacer ctrl+C sin problemas, si pasa ese segundo, aborta el programa :D por algún motivo

//Bucle infinit principal
int main(int argc, char *argv[]){
    //Llamada al enterrador de zombies cuando un hijo acaba (señal SIGCHLD)
    signal(SIGCHLD, reaper);
    //SIGINT es la señal de interrupción que produce Ctrl+C
    signal(SIGINT, ctrlc);
    
    //Inicialización del primer job
    resetear_job(0);
    //Guardamos el nombre del programa
    if (argc > 0) {
        strcpy(mi_shell, argv[0]); // Copia el contenido de argv[0] a mi_shell
    } else {
        printf(NEGRITA ROJO_T "Ha habido un error almacenando el nombre del programa.\n" RESET);
    }

    while(1){
        if(read_line(line)){ //Llegim i verifiquem que estigui correcte
            execute_line(line); //executem 
        }
    }
}

// Funció que ens imprimeix el directori on estem actualment del prompt
void imprimir_prompt(){
    char *user = getenv("USER"); //Obtenim el USER
    char *home = getenv("HOME"); //Obtenim el HOME
    char *pwd = getcwd(NULL, 0); //Obtenim el directori de treball actual

    printf(CYAN_T "%s", user);
    printf(RESET ":");
    printf(AMARILLO_T "%s%s%s", home, strstr(pwd, home) + strlen(home), strstr(pwd, home) + strlen(home) == pwd ? "" : "/");
    printf(RESET "$ ");

    free(pwd);  // Alliberar memoria assignada per getcwd
    fflush(stdout); //Ntetejem el buffer de sortida
    sleep(1);  // Espera de 1 segon per poder imprimir altres missatges 
}

char *read_line(char *line) {
    imprimir_prompt();
    if (fgets(line, MAX_COMAND_SIZE, stdin) != NULL) { //Llegir linea i comprovar que la lectura estigui correcta
        size_t length = strlen(line); // Calcular llargada del String
        //Comprova si l'ultim caracter es un salt de linea, si ho és el canvia per un \0
        if (line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }
        return line;
        //Comprovar si l'usuari ha apretat Ctrl+D
    } else if (feof(stdin)) {
        printf("\r\nAdiós!\n");
        exit(0);
        //Comprovar si hi ha hagut error al llegir la linea
    } else {
        perror("Error al leer la línea");
        exit(EXIT_FAILURE);
    }
}

int execute_line(char *line) {
    char *args[ARGS_SIZE];//Array on guardarem els tokens
    char lineaBuffLoc[MAX_COMAND_SIZE];
    strcpy(lineaBuffLoc, line);//Se copia la línea de comandes original al buffer local

    if (parse_args(args, line) > 0) {//Comprovem que haguem trobat almenys 1 token
        //comprovar si es una comanda interna
        if (check_internal(args)) {
            return 1;
        }

        //Si llega aquí es que no es un comando interno, creamos un hijo para ejecutar el comando
        pid_t pid = fork();
        int status;

        if (pid == 0){ // Proceso hijo
            // Establece el manejo de señales para el hijo
            signal(SIGCHLD, SIG_DFL);  // Restaura el manejo por defecto de SIGCHLD
            signal(SIGINT, SIG_IGN);    // Ignora la señal SIGINT para el hijo

            // Ejecutamos el comando en el proceso hijo usando execvp
            if (execvp(args[0], args) == -1) {
                perror("Error ejecutando execvp"); // Imprime el mensaje de error según errno
                printf("Código de error (errno): %d\n", errno); // Imprime el valor numérico de errno
            }
        } else if (pid > 0){ // Proceso padre
            fprintf(stderr, GRIS_T "[execute_line()→ PID padre: %d (%s)]\n" RESET, pid, mi_shell);
            // Actualizamos la información del trabajo en la estructura jobs
            actualizar_job(0, pid, 'E', lineaBuffLoc);
            
            // Esperamos a que el proceso hijo termine y actualizamos la información del trabajo
            pid_t hijo_terminado = waitpid(pid, &status, 0);

            if (hijo_terminado == -1) {
                perror("Error con waitpid()");
            }

            // Comprobamos si el proceso hijo terminó normalmente
            if (WIFEXITED(status)) {
                printf("Proceso hijo terminado, código de salida %d\n", WEXITSTATUS(status));
            }

            // Reseteamos la información del trabajo en la estructura trabajos
            resetear_job(0);
        }
    }

    return 0;
}

int parse_args(char **args, char *line) {
    int token_count = 0;

    // Extreure el primer token
    args[token_count] = strtok(line, " \t\n\r");
    //fprintf(stderr, GRIS_T "[parse_args()-> token %i: %s]\n" RESET, token_count, args[token_count]);
    //Bucle que extreu tots els tokens fins que arriba a null o a #
    while (args[token_count] && (args[token_count][0] != '#')) {
        token_count++;
        args[token_count] = strtok(NULL, " \t\n\r");
        //fprintf(stderr, GRIS_T "[parse_args()-> token %i: %s]\n" RESET, token_count, args[token_count]);
    }

    if (args[token_count]){
        //Si es troba un # deixa de trosejar pero no deixa l'ultim token com a null
        //Aqui el posem a null si l'ultim no ho es, si ja ho es no entra al if
        args[token_count] = NULL;
        //fprintf(stderr, GRIS_T "[parse_args()-> token %i corretgit: %s]\n" RESET, token_count, args[token_count]);
    }
    return token_count;
}

int check_internal(char **args) {
    if (!strcmp(args[0], "cd")) {
        return internal_cd(args);
    }
    if (!strcmp(args[0], "export")) {
        return internal_export(args);
    }
    if (!strcmp(args[0], "source")) {
        return internal_source(args);
    }
    if (!strcmp(args[0], "jobs")) {
        return internal_jobs(args);
    }
    if (!strcmp(args[0], "fg")) {
        return internal_fg(args);
    }
    if (!strcmp(args[0], "bg")) {
        return internal_bg(args);
    }
    if (!strcmp(args[0], "exit")) {
        printf("\rAdiós!\n");
        exit(0);
    }
    return 0;
}

//FUNCIONES PROVISIONALES
int internal_cd(char **args) {
    
    char *path[PATH_MAX];

    //Posem args[1], ja que a args[0] tenim el cd
    if (args[1]==NULL){ //Si posen cd sense cap argument més, va al home
        chdir(getenv("HOME"));
    } else if ((args[1]!=NULL)&&(args[2]==NULL)){ //Si tiene un argumento en args[1] y no tiene argumentos en args[2]
        if (strcmp(args[1], "..") == 0) { // Si escribe cd ..
            //Aquí entra

        }
        if (chdir(args[1]) == -1) {
            perror("chdir() error");
        } else{
            chdir(args[1]);
        }
    } else if (args[2]!=NULL) {//Si tiene más de un argumento (cd avanzado)
        char buffer[1024];

        buffer[0] = '\0';  // Inicializar buffer como una cadena vacía

        int i = 1;
        while (args[i] != NULL) {
            if ((strchr(args[i], '\'') == NULL) && (strchr(args[i], '\"') == NULL) && (strchr(args[i], '\\') == NULL)) {
                // Si no tiene caracteres especiales copiamos el token en buffer
                strcat(buffer, args[i]);
            } else {
                // Si tiene caracteres especiales copiamos el token en buffer sin el caracter especial
                for (size_t k = 0; k < strlen(args[i]); k++) {
                    if ((args[i][k] != '\'') && (args[i][k] != '\"') && (args[i][k] != '\\')) {
                        strncat(buffer, &args[i][k], 1);
                    }
                }
            }

            //Añadimos un char espacio en buffer si hay otro token
            if (args[i + 1] != NULL) {
                strcat(buffer, " ");
            }
            i++;
        }

        // Eliminar el espacio extra al final del buffer
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == ' ') {
            buffer[len - 1] = '\0';
        }

        if (chdir(buffer) == -1) {
            perror(ROJO_T "chdir() error" RESET);
        } else{
            chdir(buffer);
        }
    }
    
    return 1;
}

int internal_export(char **args) {    
    char *nom = strtok(args[1], "=");
    char *valor = strtok(NULL, "=");

    //Comprovar sintaxis
    if(nom == NULL || valor == NULL){
        fprintf(stderr, ROJO_T  "Sintaxis incorrecta. Us: export NOM=VALOR\n" RESET);
        return 1;
    }

    fprintf(stderr, GRIS_T "[internal_export()-> nom: %s]\n" RESET, nom);
    fprintf(stderr, GRIS_T "[internal_export()-> valor: %s]\n" RESET, valor);

    //Mostrem el valor inicial de la variable d'entorn
    char *inicial = getenv(nom);
    fprintf(stderr, GRIS_T "Valor inicial de %s: %s\n", nom, inicial ? inicial : "(no definido)" RESET);

    //Assignar canvi
    if (setenv(nom, valor, 1) != 0){
        perror("Error al canviar la variable d'entorn");
        return 1;
    }

    //Mostrar nou valor
    char *nou = getenv(nom);
    fprintf(stderr, GRIS_T "Nou valor de %s: %s\n", nom, nou ? nou : "(no definido)" RESET);
   
    return 1;
}

int internal_source(char **args) {
    fprintf(stderr, GRIS_T"[internal_source()-> Executara comandes fitxer]\n");
    FILE *file;

    if (!args[1]){
        fprintf(stderr, ROJO_T "Error de sintaxis. Uso: source <nombre_fichero>\n" RESET);
        return 0;
    }
    if (!(file = fopen(args[1], "r"))){
        fprintf(stderr, ROJO_T "Error al abrir el fichero\n" RESET);
        return 0;
    }
    fflush(file);
    while (fgets(line, MAX_COMAND_SIZE, file)){
        line[strlen(line) - 1] = '\0';
        fflush(file);
        execute_line(line);
    }
    fclose(file);
    return 1;
}

int internal_jobs(char **args) {

        fprintf(stderr, GRIS_T "[internal_jobs()-> Mostrará PID dels processos que no estiguin al foreground]\n" RESET);

    return 1;
}

int internal_fg(char **args) {
    
        fprintf(stderr, GRIS_T "[internal_fg()-> Envia de background a foreground o viceversa]\n" RESET);
    
    return 1;
}

int internal_bg(char **args) {
    
        fprintf(stderr, GRIS_T "[internal_bg()-> Reactivara proces detingut en segón pla]\n" RESET);
    
    return 1;
}

void resetear_job(int idx){
    jobs_list[idx].pid = 0; 
    jobs_list[idx].estado = 'N'; 
    memset(jobs_list[idx].cmd, '\0', MAX_COMAND_SIZE); 
}

void actualizar_job(int numTabla, pid_t pid, char estado, char cmd[]){
    jobs_list[numTabla].pid = pid; 
    jobs_list[numTabla].estado = estado; 
    strcpy(jobs_list[numTabla].cmd, cmd); 
}

void reaper(int signum) {
    // Variables locales para almacenar el estado de terminación y el PID del proceso hijo que ha terminado
    pid_t ended;
    int status;

    // Establece reaper como el manejador de la señal SIGCHLD
    signal(SIGCHLD, reaper);

    // Bucle para esperar y recolectar información sobre procesos hijos que han terminado
    while ((ended = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Proceso hijo terminado: PID %d", ended);

        if (WIFEXITED(status)) {
            printf(" con código de salida %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf(" terminado por la señal %d\n", WTERMSIG(status));
        }

        // Comprueba si el proceso hijo que ha terminado es el mismo que el proceso en primer plano
        if (ended == jobs_list[0].pid) {
            // Restablece el trabajo en primer plano (foreground) indicando que no hay trabajo en primer plano
            resetear_job(0);
        }
    }
}

void ctrlc(int signum) {
    // Establece ctrlc como el manejador de la señal SIGINT
    signal(SIGINT, ctrlc);
    // Manejador para la señal SIGINT (Ctrl+C) en el padre
    if (jobs_list[0].pid > 0) { // Hay un proceso en foreground
        // Verifica si el comando del proceso en primer plano no es el shell actual
        if (strcmp(jobs_list[0].cmd, mi_shell) != 0) {
            printf("\nCtrl+C detectado. Abortando proceso en primer plano...\n");

            // Envia la señal SIGTERM al proceso en primer plano
            kill(jobs_list[0].pid, SIGTERM);

            // Notificar provisionalmente por pantalla
            printf("Señal SIGTERM enviada a PID %d\n", jobs_list[0].pid);
        } else {
            printf("\nCtrl+C detectado. Abortar el mini shell o procesos en background con 'exit' o Ctrl+D.\n");
        }
    } else {
        printf("\nCtrl+C detectado. No hay proceso en foreground.\n");
    }
    fflush(stdout);////ÚLTIMO AÑADIDO, ALOMEJOR LO ROMPE, NO OLVIDAR
}
