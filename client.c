//Arnaud BONNET
//Antoine PROMIS

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#include "utils.h"
#include "myassert.h"

#include "client_master.h"


/************************************************************************
 * chaines possibles pour le premier paramètre de la ligne de commande
 ************************************************************************/
#define TK_STOP        "stop"             // arrêter le master
#define TK_HOW_MANY    "howmany"          // combien d'éléments dans l'ensemble
#define TK_MINIMUM     "min"              // valeur minimale de l'ensemble
#define TK_MAXIMUM     "max"              // valeur maximale de l'ensemble
#define TK_EXIST       "exist"            // test d'existence d'un élément, et nombre d'exemplaires
#define TK_SUM         "sum"              // somme de tous les éléments
#define TK_INSERT      "insert"           // insertion d'un élément
#define TK_INSERT_MANY "insertmany"       // insertions de plusieurs éléments aléatoires
#define TK_PRINT       "print"            // debug : demande aux master/workers d'afficher les éléments
#define TK_LOCAL       "local"            // lancer un calcul local (sans master) en multi-thread


/************************************************************************
 * structure stockant les paramètres du client
 * - les infos pour communiquer avec le master
 * - les infos pour effectuer le travail (cf. ligne de commande)
 *   (note : une union permettrait d'optimiser la place mémoire)
 ************************************************************************/
typedef struct {
    // communication avec le master
    //TODO
    
    //semaphores
    int sem_order;                    //permet de savoir si un client est déjà présent
    int sem_new_client;               //permet d'indiquer au master qu'il vient d'ouvrir le tube COM_FROM_CLIENT en ecriture
    
    //file descriptors des tubes
    int c_to_m;
    int m_to_c;
    
    // infos pour le travail à faire (récupérées sur la ligne de commande)
    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL
} Data;

//fonction init pour initialiser les semaphores
void init(Data *data)
{
    int cle1, cle2;

    cle1 = ftok("client_master.h", 0);
    myassert(cle1 != -1 , "erreur création de clé1");

    data->sem_new_client= semget(cle1, 1, 0);
    myassert(data->sem_new_client != -1, "client : erreur sem_new_client n'a pas été ouvert");

    cle2 = ftok("client_master.h", 1);
    myassert(cle2 != -1, "erreur création clé2");

    data->sem_order = semget(cle2, 1 , 0);
    myassert(data->sem_order != -1, "client : erreur sem_order n'a pas été ouvert");
}

/************************************************************************
 * Usage
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usages : %s <ordre> [[[<param1>] [<param2>] ...]]\n", exeName);
    fprintf(stderr, "   $ %s " TK_STOP "\n", exeName);
    fprintf(stderr, "          arrêt master\n");
    fprintf(stderr, "   $ %s " TK_HOW_MANY "\n", exeName);
    fprintf(stderr, "          combien d'éléments dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MINIMUM "\n", exeName);
    fprintf(stderr, "          plus petite valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MAXIMUM "\n", exeName);
    fprintf(stderr, "          plus grande valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_EXIST " <elt>\n", exeName);
    fprintf(stderr, "          l'élement <elt> est-il présent dans l'ensemble ?\n");
    fprintf(stderr, "   $ %s " TK_SUM "\n", exeName);
    fprintf(stderr, "           somme des éléments de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT " <elt>\n", exeName);
    fprintf(stderr, "          ajout de l'élement <elt> dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT_MANY " <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          ajout de <nb> élements (dans [<min>,<max>[) aléatoires dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_PRINT "\n", exeName);
    fprintf(stderr, "          affichage trié (dans la console du master)\n");
    fprintf(stderr, "   $ %s " TK_LOCAL " <nbThreads> <elt> <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          combien d'exemplaires de <elt> dans <nb> éléments (dans [<min>,<max>[)\n"
                    "          aléatoires avec <nbThreads> threads\n");

    if (message != NULL)
        fprintf(stderr, "message :\n    %s\n", message);

    exit(EXIT_FAILURE);
}


/************************************************************************
 * Analyse des arguments passés en ligne de commande
 ************************************************************************/
static void parseArgs(int argc, char * argv[], Data *data)
{
    data->order = CM_ORDER_NONE;

    if (argc == 1)
        usage(argv[0], "Il faut préciser une commande");

    // première vérification : la commande est-elle correcte ?
    if (strcmp(argv[1], TK_STOP) == 0)
        data->order = CM_ORDER_STOP;
    else if (strcmp(argv[1], TK_HOW_MANY) == 0)
        data->order = CM_ORDER_HOW_MANY;
    else if (strcmp(argv[1], TK_MINIMUM) == 0)
        data->order = CM_ORDER_MINIMUM;
    else if (strcmp(argv[1], TK_MAXIMUM) == 0)
        data->order = CM_ORDER_MAXIMUM;
    else if (strcmp(argv[1], TK_EXIST) == 0)
        data->order = CM_ORDER_EXIST;
    else if (strcmp(argv[1], TK_SUM) == 0)
        data->order = CM_ORDER_SUM;
    else if (strcmp(argv[1], TK_INSERT) == 0)
        data->order = CM_ORDER_INSERT;
    else if (strcmp(argv[1], TK_INSERT_MANY) == 0)
        data->order = CM_ORDER_INSERT_MANY;
    else if (strcmp(argv[1], TK_PRINT) == 0)
        data->order = CM_ORDER_PRINT;
    else if (strcmp(argv[1], TK_LOCAL) == 0)
        data->order = CM_ORDER_LOCAL;
    else
        usage(argv[0], "commande inconnue");

    // deuxième vérification : nombre de paramètres correct ?
    if ((data->order == CM_ORDER_STOP) && (argc != 2))
        usage(argv[0], TK_STOP " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_HOW_MANY) && (argc != 2))
        usage(argv[0], TK_HOW_MANY " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MINIMUM) && (argc != 2))
        usage(argv[0], TK_MINIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MAXIMUM) && (argc != 2))
        usage(argv[0], TK_MAXIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_EXIST) && (argc != 3))
        usage(argv[0], TK_EXIST " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_SUM) && (argc != 2))
        usage(argv[0], TK_SUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_INSERT) && (argc != 3))
        usage(argv[0], TK_INSERT " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_INSERT_MANY) && (argc != 5))
        usage(argv[0], TK_INSERT_MANY " : il faut 3 arguments après la commande");
    if ((data->order == CM_ORDER_PRINT) && (argc != 2))
        usage(argv[0], TK_PRINT " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_LOCAL) && (argc != 7))
        usage(argv[0], TK_LOCAL " : il faut 5 arguments après la commande");

    // extraction des arguments
    if (data->order == CM_ORDER_EXIST)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT_MANY)
    {
        data->nb = strtol(argv[2], NULL, 10);
        data->min = strtof(argv[3], NULL);
        data->max = strtof(argv[4], NULL);
        if (data->nb < 1)
            usage(argv[0], TK_INSERT_MANY " : nb doit être strictement positif");
        if (data->max < data->min)
            usage(argv[0], TK_INSERT_MANY " : max ne doit pas être inférieur à min");
    }
    else if (data->order == CM_ORDER_LOCAL)
    {
        data->nbThreads = strtol(argv[2], NULL, 10);
        data->elt = strtof(argv[3], NULL);
        data->nb = strtol(argv[4], NULL, 10);
        data->min = strtof(argv[5], NULL);
        data->max = strtof(argv[6], NULL);
        if (data->nbThreads < 1)
            usage(argv[0], TK_LOCAL " : nbThreads doit être strictement positif");
        if (data->nb < 1)
            usage(argv[0], TK_LOCAL " : nb doit être strictement positif");
        if (data->max <= data->min)
            usage(argv[0], TK_LOCAL " : max ne doit être strictement supérieur à min");
    }
}




/************************************************************************
 * Partie multi-thread
 ************************************************************************/
//TODO Une structure pour les arguments à passer à un thread (aucune variable globale autorisée)
typedef struct
{
    int *result;
    int nbtosearch;
    int scale;
    int start;
    float * tab;
    pthread_mutex_t *mutex;
} ThreadData;
//TODO
// Code commun à tous les threads
// Un thread s'occupe d'une portion du tableau et compte en interne le nombre de fois
// où l'élément recherché est présent dans cette portion. On ajoute alors,
// en section critique, ce nombre au compteur partagé par tous les threads.
// Le compteur partagé est la variable "result" de "lauchThreads".
// A vous de voir les paramètres nécessaires  (aucune variable globale autorisée)
//END TODO
void * codeThread(void * arg)
{
    ThreadData *dataThread = (ThreadData *) arg;
    int num = 0;
    int ret;

    for (int i = dataThread->start; i < dataThread->start + dataThread->scale; i++){
        if(dataThread->tab[i] == dataThread->nbtosearch ){
            num ++;
        }
    }

    ret = pthread_mutex_lock(dataThread->mutex);
    myassert(ret == 0, "l'entrée en section critique à échoué");
    (*(dataThread->result)) += num;
    ret = pthread_mutex_unlock(dataThread->mutex);
    myassert(ret == 0, "la sortie de la section critique a échoué");
    
    return NULL;
}

void lauchThreads(const Data *data)
{
    //TODO déclarations nécessaires : mutex, ...
    int result = 0;
    float * tabalea = ut_generateTab(data->nb, data->min, data->max, 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t tabId[data->nbThreads];
    ThreadData mydata[data->nbThreads];
    
    int lentab = data->nb;

    for(int i =0; i< data->nbThreads; i++){
        if(i < (data->nbThreads )-1){
            mydata[i].result = &result;
            mydata[i].nbtosearch = data->elt;
            mydata[i].scale =  lentab/data->nbThreads;
            mydata[i].start = i * (lentab/data->nbThreads);
            mydata[i].tab= tabalea;
            mydata[i].mutex = &mutex;
        } else {
            mydata[i].result = &result;
            mydata[i].nbtosearch = data->elt;
            mydata[i].start = i * (lentab/data->nbThreads);
            mydata[i].scale = lentab - (i * (lentab/data->nbThreads));
            mydata[i].tab= tabalea;
            mydata[i].mutex = &mutex;
        }
        
    }

    //TODO lancement des threads
    for(int i = 0; i < data->nbThreads; i++){
        int ret = pthread_create(&(tabId[i]), NULL, codeThread, &(mydata[i]));
        myassert(ret == 0, "erreur le thread ne s'est pas lancé correctement");
    }
    //TODO attente de la fin des threads
    for(int i =0; i <data->nbThreads; i++){
        int ret = pthread_join(tabId[i], NULL);
        myassert(ret == 0, "erreur, le thread n'a pas été attendu");
    }
    // résultat (result a été rempli par les threads)
    // affichage du tableau si pas trop gros
    if (data->nb <= 20)
    {
        printf("[");
        for (int i = 0; i < data->nb; i++)
        {
            if (i != 0)
                printf(" ");
            printf("%g", tabalea[i]);
        }
        printf("]\n");
    }
    // recherche linéaire pour vérifier
    int nbVerif = 0;
    for (int i = 0; i < data->nb; i++)
    {
        if (tabalea[i] == data->elt)
            nbVerif ++;
    }
    printf("Elément %g présent %d fois (%d attendu)\n", data->elt, result, nbVerif);
    if (result == nbVerif)
        printf("=> ok ! le résultat calculé par les threads est correct\n");
    else
        printf("=> PB ! le résultat calculé par les threads est incorrect\n");

    //TODO libération des ressources    
    int ret = pthread_mutex_destroy(&mutex);
    myassert(ret == 0, "erreur, le thread n'a pas été supprimé");
}



/************************************************************************
 * Partie communication avec le master
 ************************************************************************/
// envoi des données au master
void sendData(const Data *data)
{
    int ret;
    //TODO à enlever (présent pour éviter le warning)
    
    
    ret = write(data->c_to_m, &(data->order), sizeof(int));
    myassert(ret != 0 , "erreur write dans c_to_m aucune donnee ecrite");
    myassert(ret == sizeof(int), "erreur la valeur n'a pas été ecrite correctement");

    if(data->order == CM_ORDER_INSERT){
    	ret = write(data->c_to_m, &(data->elt), sizeof(float));
    	myassert(ret != 0 , "erreur write dans c_to_m aucune donnee ecrite");
    	myassert(ret == sizeof(float), "erreur la valeur n'a pas été ecrite correctement");
    } else if(data->order == CM_ORDER_EXIST){
        ret = write(data->c_to_m, &(data->elt), sizeof(float));
    	myassert(ret != 0 , "erreur write dans c_to_m aucune donnee ecrite");
    	myassert(ret == sizeof(float), "erreur la valeur n'a pas été ecrite correctement");
    } else if(data->order == CM_ORDER_INSERT_MANY){
        srand((unsigned int)time(NULL));

        //float *tab = malloc(sizeof(float) * data->nb);
        //for(int i = 0; i<data->nb; i++){
        //    tab[i] = ((float)rand()/RAND_MAX) * (data->max - data->min) + data->min;
        //}

        float *tab = ut_generateTab(data->nb, data->min, data->max, 0);

        ret = write(data->c_to_m, &(data->nb), sizeof(int));
    	myassert(ret != 0 , "erreur write dans c_to_m aucune donnee ecrite");
    	myassert(ret == sizeof(int), "erreur la valeur n'a pas été ecrite correctement");

        ret = write(data->c_to_m, tab, sizeof(float)*data->nb);
    	myassert(ret != 0 , "erreur write dans c_to_m aucune donnee ecrite");
    	myassert(ret == (int)sizeof(float) * data->nb, "erreur la valeur n'a pas été ecrite correctement");

        free(tab);
    }

    
    //TODO
    // - envoi de l'ordre au master (cf. CM_ORDER_* dans client_master.h)
    // - envoi des paramètres supplémentaires au master (pour CM_ORDER_EXIST,
    //   CM_ORDER_INSERT et CM_ORDER_INSERT_MANY)
    //END TODO
}

// attente de la réponse du master
void receiveAnswer(const Data *data)
{
    myassert(data != NULL, "pb !");   //TODO à enlever (présent pour éviter le warning)

    //TODO
    // - récupération de l'accusé de réception du master (cf. CM_ANSWER_* dans client_master.h)
    // - selon l'ordre et l'accusé de réception :
    //      . récupération de données supplémentaires du master si nécessaire
    // - affichage du résultat
    //END TODO
    int reponse;
    int ret = read(data->m_to_c, &reponse, sizeof(int));
    myassert(ret != 0 , "erreur read dans m_to_c, personne en écriture");
    myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");
    

    if(reponse == CM_ANSWER_SUM_OK){
        float elt;
        ret = read(data->m_to_c, &elt, sizeof(float));
        myassert(ret != 0 , "erreur read dans m_to_c, personne en écriture");
        myassert(ret == sizeof(float), "erreur la valeur lue n'est pas de la taille d'un float");
        printf("Somme calculée : %g \n", elt);

    } else if(reponse == CM_ANSWER_STOP_OK){
        printf("Le master s'arrête\n");

    } else if(reponse == CM_ANSWER_PRINT_OK){
        printf("L'affichage à été réalisé\n");

    } else if(reponse == CM_ANSWER_INSERT_OK){
        printf("Insertion réalisée\n");

    } else if(reponse == CM_ANSWER_EXIST_NO){
        printf("L'élément n'existe pas\n");

    } else if(reponse == CM_ANSWER_EXIST_YES){
        int number;
        ret = read(data->m_to_c, &number, sizeof(int));
        myassert(ret != 0 , "erreur read dans m_to_c, personne en écriture");
        myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un float");
        printf("L'élément {%g} existe %d fois\n", data->elt, number);

    } else if(reponse == CM_ANSWER_MINIMUM_EMPTY){
        printf("Ensemble vide : inserez un élément d'abord\n");

    } else if(reponse == CM_ANSWER_MINIMUM_OK){
        float elt;
        ret = read(data->m_to_c, &elt, sizeof(float));
        myassert(ret != 0 , "erreur read dans m_to_c, personne en écriture");
        myassert(ret == sizeof(float), "erreur la valeur lue n'est pas de la taille d'un float");
        printf("Minimum : %g \n", elt);

    } else if(reponse == CM_ANSWER_MAXIMUM_EMPTY){
        printf("Ensemble vide : inserez un élément d'abord\n");

    } else if(reponse == CM_ANSWER_MAXIMUM_OK){
        float elt;
        ret = read(data->m_to_c, &elt, sizeof(float));
        myassert(ret != 0 , "erreur read dans m_to_c, personne en écriture");
        myassert(ret == sizeof(float), "erreur la valeur lue n'est pas de la taille d'un float");
        printf("Maximum : %g \n", elt);
        
    } else if(reponse == CM_ANSWER_INSERT_MANY_OK){
        printf("Insertion multiples réalisées\n");

    } else if(reponse == CM_ANSWER_HOW_MANY_OK){
        int number;
        int number2;

        ret = read(data->m_to_c, &number, sizeof(int));
        myassert(ret != 0 , "erreur read dans m_to_c, personne en écriture");
        myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un float");

        ret = read(data->m_to_c, &number2, sizeof(int));
        myassert(ret != 0 , "erreur read dans m_to_c, personne en écriture");
        myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un float");

        printf("Nombre total d'éléments : %d\nNombre d'éléments distincts : %d \n", number, number2);
    }
}


/************************************************************************
 * Fonction principale
 ************************************************************************/
int main(int argc, char * argv[])
{
    Data data;
    
    int ret;    
    
    parseArgs(argc, argv, &data);

    if (data.order == CM_ORDER_LOCAL)
        lauchThreads(&data);
    else
    {
        //TODO
        // - entrer en section critique :
        //       . pour empêcher que 2 clients communiquent simultanément
        //       . le mutex est déjà créé par le master
        // - ouvrir les tubes nommés (ils sont déjà créés par le master)
        //       . les ouvertures sont bloquantes, il faut s'assurer que
        //         le master ouvre les tubes dans le même ordre
        //END TODO
        //création des sémaphores
    	init(&data);

        struct sembuf prendre = {0, -1, 0};
        ret = semop(data.sem_order, &prendre, 1);
        myassert(ret != -1, "erreur le client n'est pas entré en section critique");  

    	      
    	
    	//ouverture des tubes
    	data.c_to_m = open(COM_FROM_CLIENT, O_WRONLY);                                //ouverture en ecriture
    	myassert(data.c_to_m != -1, "le tube COM_FROM_CLIENT ne s'est pas ouvert correctement");
    
    	data.m_to_c = open(COM_TO_CLIENT, O_RDONLY);                                 //ouverture en lecture
    	myassert(data.m_to_c != -1, "le tube COM_TO_CLIENT ne s'est pas ouvert correctement");
    	
        

        sendData(&data);
        receiveAnswer(&data);

        //TODO
        // - sortir de la section critique
        // - libérer les ressources (fermeture des tubes, ...)
        // - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
        
        struct sembuf rendre = {0, +1, 0};
        ret = semop(data.sem_order, &rendre, 1);
    	myassert(ret != -1, "erreur le client n'est pas sorti de section critique");
          	
        //
        
        ret = close(data.c_to_m);
        myassert(ret == 0, "le tube c_to_m n'a pas été fermé");
        
        ret = close(data.m_to_c);
        myassert(ret == 0, "le tube m_to_c n'a pas été fermé");
        
        // Une fois que le master a envoyé la réponse au client, il se bloque
        // sur un sémaphore ; le dernier point permet donc au master de continuer
        //
        // N'hésitez pas à faire des fonctions annexes ; si la fonction main
        // ne dépassait pas une trentaine de lignes, ce serait bien.
        ret = semop(data.sem_new_client, &rendre, 1);
    	myassert(ret != -1, "erreur le client n'est pas sorti de section critique");
        
    }
    
    return EXIT_SUCCESS;
}
