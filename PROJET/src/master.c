#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#include "utils.h"
#include "myassert.h"

#include "client_master.h"
#include "master_worker.h"

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/
typedef struct
{
    // communication avec le client
    
    //sémaphores
    int sem_order;                    //permet de savoir si un client est déjà présent
    int sem_new_client;               //permet au master de savoir si il peut ouvrir le tube en lecture

    // données internes
    bool exist_worker;                //savoir si le premier worker existe

    // communication avec le premier worker (double tubes)
    int c_to_w[2];                    //tube anonyme pour communiquer du master vers le premier worker
    int c_from_w[2];                  //tube anonyme pour communiquer du premier worker vers le master

    // communication en provenance de tous les workers (un seul tube en lecture)
    int com_from_allworker[2];        //tube anonyme pour communiquer de n'importe quel worker vers le master

} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/************************************************************************
 * initialisation complète
 ************************************************************************/
void init(Data *data)
{
    
    //TODO initialisation data   
    
    data->exist_worker = false;
    

    myassert(data != NULL, "il faut l'environnement d'exécution");
    
    
    int ret;
    key_t cle1;
    key_t cle2; 
    
    //création des sémaphores
    cle1 = ftok("client_master.h", 0);
    myassert(cle1 != -1 , "erreur création de clé1");
    
    data->sem_new_client= semget(cle1, 1, IPC_CREAT|IPC_EXCL|0641);
    myassert(data->sem_new_client != -1, "erreur sem_new_client n'a pas été créé");
    
    cle2 = ftok("client_master.h", 1);
    myassert(cle2 != -1, "erreur création clé2");
    
    data->sem_order = semget(cle2, 1 , IPC_CREAT|IPC_EXCL|0641);
    myassert(data->sem_order != -1, "erreur sem_order n'a pas été créé");

    //initilaistation des sémaphores
    ret = semctl(data->sem_new_client, 0, SETVAL, 1);
    myassert(ret != -1, "sem_new_client n'a pas été initialisé");

    ret = semctl(data->sem_order, 0, SETVAL, 1);
    myassert(ret != -1, "sem_order n'a pas été initialisé");
    
    //création des tubes nommés
    ret = mkfifo(COM_TO_CLIENT, 0644); 
    myassert(ret == 0, "erreur le tube master_to_client n'a pas été créé");
    
    ret = mkfifo(COM_FROM_CLIENT, 0644);
    myassert(ret == 0, "erreur le tube client_to_mster n'a pas été créé");
    
    /*
    //ouverture des tubes en ecriture pour l'un et en lecture pour l'autre
    ret = open(COM_TO_CLIENT, O_WRONLY);                                            //ouverture en ecriture
    myassert(ret != -1, "le tube COM_TO_CLIENT ne s'est pas ouvert correctement");
    
    ret = open(COM_FROM_CLIENT, O_RDONLY);                                            //ouverture en lecture
    myassert(ret != -1, "le tube COM_FROM_CLIENT ne s'est pas ouvert correctement");*/

    /*************************************************************************
     * creer et gérer les tubes anonymes pour les workers
    *************************************************************************/
    
}


/************************************************************************
 * fin du master
 ************************************************************************/
void orderStop(Data *data)
{
    TRACE0("[master] ordre stop\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre de fin (cf. master_worker.h)
    // - attendre sa fin
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
    printf("ordre stop demandé par le client");
    
}


/************************************************************************
 * quel est la cardinalité de l'ensemble
 ************************************************************************/
void orderHowMany(Data *data)
{
    TRACE0("[master] ordre how many\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre howmany (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultats (deux quantités) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer les résultats au client
    //END TODO
}


/************************************************************************
 * quel est la minimum de l'ensemble
 ************************************************************************/
void orderMinimum(Data *data)
{
    TRACE0("[master] ordre minimum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre minimum (cf. master_worker.h)
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . recevoir résultat (la valeur) venant du worker concerné
    //       . envoyer l'accusé de réception au client (cf. client_master.h)
    //       . envoyer le résultat au client
    //END TODO
}


/************************************************************************
 * quel est la maximum de l'ensemble
 ************************************************************************/
void orderMaximum(Data *data)
{
    TRACE0("[master] ordre maximum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // cf. explications pour le minimum
    //END TODO
}


/************************************************************************
 * test d'existence
 ************************************************************************/
void orderExist(Data *data)
{
    TRACE0("[master] ordre existence\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre existence (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à tester
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . si élément non présent
    //             . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    //       . sinon
    //             . recevoir résultat (une quantité) venant du worker concerné
    //             . envoyer l'accusé de réception au client (cf. client_master.h)
    //             . envoyer le résultat au client
    //END TODO
}

/************************************************************************
 * somme
 ************************************************************************/
void orderSum(Data *data)
{
    TRACE0("[master] ordre somme\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker) : la somme est alors 0
    // - envoyer au premier worker ordre sum (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultat (la somme) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer le résultat au client
    //END TODO
}

/************************************************************************
 * insertion d'un élément
 ************************************************************************/

//TODO voir si une fonction annexe commune à orderInsert et orderInsertMany est justifiée

void orderInsert(Data *data)
{
    TRACE0("[master] ordre insertion\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . créer le premier worker avec l'élément reçu du client
    // - sinon
    //       . envoyer au premier worker ordre insertion (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à insérer
    // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * insertion d'un tableau d'éléments
 ************************************************************************/
void orderInsertMany(Data *data)
{
    TRACE0("[master] ordre insertion tableau\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir le tableau d'éléments à insérer en provenance du client
    // - pour chaque élément du tableau
    //       . l'insérer selon l'algo vu dans orderInsert (penser à factoriser le code)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * affichage ordonné
 ************************************************************************/
void orderPrint(Data *data)
{
    TRACE0("[master] ordre affichage\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre print (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    //   note : ce sont les workers qui font les affichages
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;
    int ret;
    int fd_from_client;
    int fd_to_client;
    
    init(data);

    while (! end)
    {
        //TODO ouverture des tubes avec le client (cf. explications dans client.c)
        
        //ouverture du tube en attente de communication du client
        fd_from_client = open(COM_FROM_CLIENT, O_RDONLY);
        myassert(fd_from_client != -1, "l'ouverture du tube COM_FROM_CLIENT en lecture a échoué");
        
        //ouverture du tube en ecriture pour envoyer des information au client
        fd_to_client = open(COM_TO_CLIENT, O_WRONLY);
        myassert(fd_to_client != -1, "l'ouverture du tube COM_TO_CLIENT en écriture a échoué");
        
        

        int order = CM_ORDER_STOP;   //TODO pour que ça ne boucle pas, mais recevoir l'ordre du client
        ret = read(fd_from_client, &order, sizeof(int));
        myassert(ret != 0 , "erreur read dans fd_from_client, personne en écriture");
        myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");
        
        switch(order)
        {
          case CM_ORDER_STOP:
            orderStop(data);
            end = true;
            break;
          case CM_ORDER_HOW_MANY:
            orderHowMany(data);
            break;
          case CM_ORDER_MINIMUM:
            orderMinimum(data);
            break;
          case CM_ORDER_MAXIMUM:
            orderMaximum(data);
            break;
          case CM_ORDER_EXIST:
            orderExist(data);
            break;
          case CM_ORDER_SUM:
            orderSum(data);
            break;
          case CM_ORDER_INSERT:
            orderInsert(data);
            break;
          case CM_ORDER_INSERT_MANY:
            orderInsertMany(data);
            break;
          case CM_ORDER_PRINT:
            orderPrint(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        //TODO fermer les tubes nommés
        //     il est important d'ouvrir et fermer les tubes nommés à chaque itération
        //     voyez-vous pourquoi ?
        ret = close(fd_from_client);
        myassert(ret == 0, "le tube fd_from_client n'a pas été fermé");
        
        ret = close(fd_to_client);
        myassert(ret == 0, "le tube fd_to_client n'a pas été fermé");
        
        //TODO attendre ordre du client avant de continuer (sémaphore pour une précédence)
        //struct sembuf operation = {0; 0; 0};
        //ret = semop(sem_new_client, &operation, 1);
        //myassert(ret != -1, "erreur l'attente du passage du semaphore sem_new_client a échoué");

        TRACE0("[master] fin ordre\n");
    }
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

//TODO N'hésitez pas à faire des fonctions annexes ; si les fonctions main
//TODO et loop pouvaient être "courtes", ce serait bien

int main(int argc, char * argv[])
{
    if (argc != 1)
        usage(argv[0], NULL);

    TRACE0("[master] début\n");

    Data data;
    int ret;
    
    
    //TODO
    // - création des sémaphores
    // - création des tubes nommés
    
    //fait dans la fonction init
    
    //END TODO

    loop(&data);

    //TODO destruction des tubes nommés, des sémaphores, ...
    //destruction des tubes
    ret = unlink(COM_TO_CLIENT);
    myassert(ret == 0, "le tube COM_TO_CLIENT n'a pas été détruit");
    
    ret = unlink(COM_FROM_CLIENT);
    myassert(ret == 0, "le tube COM_FROM_CLIENT n'a pas été détruit");
    
    //destruction des sémaphores
    //printf("valeur de sem_new_client : %d ",data.sem_new_client);
    ret = semctl(data.sem_new_client, -1, IPC_RMID);
    //printf("valeur de ret : %d", ret);
    myassert(ret != -1, "la sémaphore sem_new_client n'a pas été détruite");
    
    ret = semctl(data.sem_order, -1, IPC_RMID);
    myassert(ret != -1, "la sémaphore sem_order n'a pas été détruite");
    
    //char *semarg[2];
    //semarg[0]="./rmsempipe.sh";
    //semarg[1]=NULL;
    //execv("./rmsempipe.sh", semarg);

    TRACE0("[master] terminaison\n");
    return EXIT_SUCCESS;
}
