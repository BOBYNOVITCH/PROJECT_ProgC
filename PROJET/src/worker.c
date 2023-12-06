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

#include "master_worker.h"


/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/
typedef struct
{
    // données internes (valeur de l'élément, cardinalité)
    float val;
    int nb;
    // communication avec le père (2 tubes) et avec le master (1 tube en écriture)
    int pere_to_fils;
    int fils_to_pere;
    int tube_to_master;
    bool fils_gauche;
    bool fils_droit;
    // communication avec le fils gauche s'il existe (2 tubes)
    int to_fils_gauche[2];
    int from_fils_gauche[2];
    // communication avec le fils droit s'il existe (2 tubes)
    int to_fils_droit[2];
    int from_fils_droit[2];
    //TODO
    //int dummy;  //TODO à enlever (présent pour éviter le warning)
} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <elt> <fdIn> <fdOut> <fdToMaster>\n", exeName);
    fprintf(stderr, "   <elt> : élément géré par le worker\n");
    fprintf(stderr, "   <fdIn> : canal d'entrée (en provenance du père)\n");
    fprintf(stderr, "   <fdOut> : canal de sortie (vers le père)\n");
    fprintf(stderr, "   <fdToMaster> : canal de sortie directement vers le master\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

static void parseArgs(int argc, char * argv[], Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");
    

    if (argc != 5)
        usage(argv[0], "Nombre d'arguments incorrect");

    //TODO initialisation data
    data->nb=1;
    data->fils_gauche = false;
    data->fils_droit = false;
    //TODO (à enlever) comment récupérer les arguments de la ligne de commande
    data->val = strtof(argv[1], NULL);
    data->pere_to_fils = strtol(argv[2], NULL, 10);
    data->fils_to_pere = strtol(argv[3], NULL, 10);
    data->tube_to_master = strtol(argv[4], NULL, 10);

    //END TODO
}


/************************************************************************
 * Stop 
 ************************************************************************/
void stopAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre stop\n", getpid(), getppid(), data->val /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int ret;

    //TODO
    // - traiter les cas où les fils n'existent pas
    int order = MW_ORDER_STOP;
    if(data->fils_gauche == true && data->fils_droit == true)
    {
      ret = write(data->to_fils_gauche[1], &order, sizeof(int));
      myassert(ret == sizeof(int), "erreur la valeur ecrite n'est pas de la taille d'un int");
      

      ret = write(data->to_fils_droit[1], &order, sizeof(int));
      myassert(ret == sizeof(int), "erreur la valeur ecrite n'est pas de la taille d'un int");

      wait(NULL); //attendre fin du fils gauche
      wait(NULL); //attendre fin du fils droit

    } else if(data->fils_gauche == true && data->fils_droit == false)
    {
    	ret = write(data->to_fils_gauche[1], &order, sizeof(int));
      myassert(ret == sizeof(int), "erreur la valeur ecrite n'est pas de la taille d'un int");

      wait(NULL); //attendre fin du fils gauche

    } else if(data->fils_gauche == false && data->fils_droit == true)
    {
      ret = write(data->to_fils_droit[1], &order, sizeof(int));
      myassert(ret == sizeof(int), "erreur la valeur ecrite n'est pas de la taille d'un int");

      wait(NULL); //attendre fin du fils droit

    }
    // - envoyer au worker gauche ordre de fin (cf. master_worker.h)
    // - envoyer au worker droit ordre de fin (cf. master_worker.h)
    // - attendre la fin des deux fils
    //END TODO
}


/************************************************************************
 * Combien d'éléments
 ************************************************************************/
static void howManyAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre how many\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre howmany (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir deux résultats (nb elts, nb elts distincts) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer les résultats (les cumuls des deux quantités + la valeur locale) au père
    //END TODO
}


/************************************************************************
 * Minimum
 ************************************************************************/
static void minimumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre minimum\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche n'existe pas (on est sur le minimum)
    //       . envoyer l'accusé de réception au master (cf. master_worker.h)
    //       . envoyer l'élément du worker courant au master
    // - sinon
    //       . envoyer au worker gauche ordre minimum (cf. master_worker.h)
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
}


/************************************************************************
 * Maximum
 ************************************************************************/
static void maximumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre maximum\n", getpid(), getppid(), data->val /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // cf. explications pour le minimum
    //END TODO
}


/************************************************************************
 * Existence
 ************************************************************************/
static void existAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre exist\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du père
    // - si élément courant == élément à tester
    //       . envoyer au master l'accusé de réception de réussite (cf. master_worker.h)
    //       . envoyer cardinalité de l'élément courant au master
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant)
    //       . envoyer au worker gauche ordre exist (cf. master_worker.h)
    //       . envoyer au worker gauche élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    // - sinon (donc elt à tester > elt courant)
    //       . envoyer au worker droit ordre exist (cf. master_worker.h)
    //       . envoyer au worker droit élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
}


/************************************************************************
 * Somme
 ************************************************************************/
static void sumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre sum\n", getpid(), getppid(), 3.14 /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre sum (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir résultat (somme du fils) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer le résultat (le cumul des deux quantités + la valeur locale) au père
    //END TODO
}


/************************************************************************
 * Insertion d'un nouvel élément
 ************************************************************************/
static void insertAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre insert\n", getpid(), getppid(), data->val /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du père
    // - si élément courant == élément à tester
    //       . incrémenter la cardinalité courante
    //       . envoyer au master l'accusé de réception (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . créer un worker à gauche avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . créer un worker à droite avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à insérer < elt courant)
    //       . envoyer au worker gauche ordre insert (cf. master_worker.h)
    //       . envoyer au worker gauche élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    // - sinon (donc elt à insérer > elt courant)
    //       . envoyer au worker droit ordre insert (cf. master_worker.h)
    //       . envoyer au worker droit élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    //END TODO

    float elt;
    int ret;
    int reponse;
    int retfork;
    ret = read(data->pere_to_fils, &elt, sizeof(float));
    myassert(ret != 0 , "erreur read dans pere_to_fils, personne en écriture");
    myassert(ret == sizeof(float), "erreur la valeur lue n'est pas de la taille d'un int");

    if(elt == data->val) //élément courant == élément à tester
    {
      data->nb = data->nb + 1; //on incrémente
      
      TRACE3("    [worker (%d, %d) {%g}] : nombre d'élément augmente \n", getpid(), getppid(), data->val);

      //on envoie la réponse
      reponse = MW_ANSWER_INSERT;
      ret = write(data->tube_to_master, &reponse, sizeof(int));
      myassert(ret != 0 , "erreur read dans COM_TO_CLIENT, personne en écriture");
      myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");

    } else if(elt < data->val && data->fils_gauche == false) //si (elt à tester < elt courant) et (pas de fils gauche)
    {
      
      ret = pipe(data->to_fils_gauche);
      myassert(ret == 0, "erreur le tube anonyme to_fils_gauche est vide");

      ret = pipe(data->from_fils_gauche);
      myassert(ret == 0, "erreur le tube anonyme from_fils_gauche est vide");
      
      data->fils_gauche=true;

      retfork = fork();
      myassert(retfork != -1, "erreur le fork ne s'est pas fait");
      

      if(retfork == 0)
      {
        char * argv[6];
    	  char newelt[20];
    	  char new_p_to_f[20];
    	  char new_f_to_p[20];
    	  char new_c_allw[20];

    	  sprintf(newelt, "%g", elt);
    	  sprintf(new_p_to_f, "%d", data->to_fils_gauche[0]);
    	  sprintf(new_f_to_p, "%d", data->from_fils_gauche[1]);
    	  sprintf(new_c_allw, "%d", data->tube_to_master);

    	  argv[0] = "worker";
    	  argv[1] = newelt;
    	  argv[2] = new_p_to_f;
    	  argv[3] = new_f_to_p;
    	  argv[4] = new_c_allw;
    	  argv[5] = NULL;

        ret = close(data->to_fils_ga
    if(data->exist_worker == true)
    {
      int order = MW_ORDER_PRINT;
    	ret = write(data->c_to_w[1], &order, sizeof(int));
    	myassert(ret == sizeof(int), "erreur la valeur envoyée n'est pas de la taille d'un int");

      int reponse;
      ret = read(data->c_from_w[0], &reponse, sizeof(int));
      myassert(ret != 0 , "erreur read dans c_from_w, personne en écriture");
      myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");
    }
    
    int reponse_pere = CM_ANSWER_PRINT_OK;
    ret = write(data->m_to_c, &reponse_pere, sizeof(int));
    myassert(ret != 0 , "erreur read dans COM_TO_CLIENT, personne en écriture");
    myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int")gauche[0]);
        myassert(ret == 0, "erreur le tube from_fils_gauche[0] n'est pas fermé");        
        
    	  execv(argv[0], argv);
      }
      if(retfork != 0){
        ret = close(data->to_fils_gauche[0]);
        myassert(ret == 0, "erreur le tube to_fils_gauche[1] n'est pas fermé");

        ret = close(data->from_fils_gauche[1]);
        myassert(ret == 0, "erreur le tube from_fils_gauche[0] n'est pas fermé");
      }
    } else if(elt > data->val && data->fils_droit == false) //si (elt à tester > elt courant) et (pas de fils droit)
    {
      ret = pipe(data->to_fils_droit);
      myassert(ret == 0, "erreur le tube anonyme to_fils_droit est vide");

      ret = pipe(data->from_fils_droit);
      myassert(ret == 0, "erreur le tube anonyme from_fils_droit est vide");

      data->fils_droit=true;

      retfork = fork();
      myassert(retfork != -1, "erreur le fork ne s'est pas fait");
      
      if(retfork == 0)
      {
        char * argv[6];
    	  char newelt[20];
    	  char new_p_to_f[20];
    	  char new_f_to_p[20];
    	  char new_c_allw[20];

    	  sprintf(newelt, "%g", elt);
    	  sprintf(new_p_to_f, "%d", data->to_fils_droit[0]);
    	  sprintf(new_f_to_p, "%d", data->from_fils_droit[1]);
    	  sprintf(new_c_allw, "%d", data->tube_to_master);

    	  argv[0] = "worker";
    	  argv[1] = newelt;
    	  argv[2] = new_p_to_f;
    	  argv[3] = new_f_to_p;
    	  argv[4] = new_c_allw;
    	  argv[5] = NULL;

        ret = close(data->to_fils_droit[1]);
        myassert(ret == 0, "erreur le tube to_fils_droit[1] n'est pas fermé");

        ret = close(data->from_fils_droit[0]);
        myassert(ret == 0, "erreur le tube from_fils_droit[0] n'est pas fermé");
        
    	  execv(argv[0], argv);
      }

      if(retfork != 0){
        ret = close(data->to_fils_droit[0]);
        myassert(ret == 0, "erreur le tube to_fils_droit[1] n'est pas fermé");

        ret = close(data->from_fils_droit[1]);
        myassert(ret == 0, "erreur le tube from_fils_droit[0] n'est pas fermé");
      }

    }
    if(elt < data->val) //si (elt à insérer < elt courant)
    {
      int order = MW_ORDER_INSERT;
      ret = write(data->to_fils_gauche[1], &order, sizeof(int));
    	myassert(ret == sizeof(int), "erreur la valeur envoyée n'est pas de la taille d'un int");

    	ret = write(data->to_fils_gauche[1], &elt, sizeof(float));
    	myassert(ret == sizeof(float), "erreur la valeur envoyée n'est pas de la taille d'un int");

    } else  //sinon (donc elt à insérer > elt courant)
    {
       int order = MW_ORDER_INSERT;
      ret = write(data->to_fils_droit[1], &order, sizeof(int));
    	myassert(ret == sizeof(int), "erreur la valeur envoyée n'est pas de la taille d'un int");

    	ret = write(data->to_fils_droit[1], &elt, sizeof(float));
    	myassert(ret == sizeof(float), "erreur la valeur envoyée n'est pas de la taille d'un int");
    }
    
}


/************************************************************************
 * Affichage
 ************************************************************************/
static void printAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre print\n", getpid(), getppid(), data->val);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - afficher l'élément courant avec sa cardinalité
    // - si le fils droit existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    //END TODO
    int ret;
    int order = MW_ORDER_PRINT;

    if(data->fils_gauche == true){
      
      ret = write(data->to_fils_gauche[1], &order, sizeof(int));
      myassert(ret == sizeof(int), "erreur la valeur ecrite n'est pas de la taille d'un int");

      int reponse;
      ret = read(data->from_fils_gauche[0], &reponse, sizeof(int));
      myassert(ret != 0 , "erreur read dans COM_FROM_CLIENT, personne en écriture");
      myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");
    }

    printf("    worker (%d, %d): ordre print, element = %g, cardinalite = %d\n", getpid(), getppid(), data->val, data->nb);

    if(data->fils_droit == true){
      ret = write(data->to_fils_droit[1], &order, sizeof(int));
      myassert(ret == sizeof(int), "erreur la valeur ecrite n'est pas de la taille d'un int");

      int reponse2;
      ret = read(data->from_fils_droit[0], &reponse2, sizeof(int));
      myassert(ret != 0 , "erreur read dans COM_FROM_CLIENT, personne en écriture");
      myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");
    }

    int reponse_pere = MW_ANSWER_PRINT;
    ret = write(data->fils_to_pere, &reponse_pere, sizeof(int));
    myassert(ret == sizeof(int), "erreur la valeur ecrite n'est pas de la taille d'un int");

}


/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;
    int ret;

    while (! end)
    {
        int order = MW_ORDER_STOP;   
        //TODO pour que ça ne boucle pas, mais recevoir l'ordre du père
        ret = read(data->pere_to_fils, &order, sizeof(int));
    	  myassert(ret != 0 , "erreur read dans pere_to_fils, personne en écriture");
    	  myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");
        switch(order)
        {
          case MW_ORDER_STOP:
            stopAction(data);
            end = true;
            break;
          case MW_ORDER_HOW_MANY:
            howManyAction(data);
            break;
          case MW_ORDER_MINIMUM:
            minimumAction(data);
            break;
          case MW_ORDER_MAXIMUM:
            maximumAction(data);
            break;
          case MW_ORDER_EXIST:
            existAction(data);
            break;
          case MW_ORDER_SUM:
            sumAction(data);
            break;
          case MW_ORDER_INSERT:
            insertAction(data);
            break;
          case MW_ORDER_PRINT:
            printAction(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        TRACE3("    [worker (%d, %d) {%g}] : fin ordre\n", getpid(), getppid(), data->val /*TODO élément*/);
        
    }
}


/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    Data data;
    int ret;
    int rep;
    
    //printf("%s\n", argv[2]);
    
    
    parseArgs(argc, argv, &data);
    TRACE3("    [worker (%d, %d) {%g}] : début worker\n", getpid(), getppid(), data.val /*TODO élément*/);
    

    //TODO envoyer au master l'accusé de réception d'insertion (cf. master_worker.h)
    TRACE3("    [worker (%d, %d) {%g}] : envoi de la reponse\n", getpid(), getppid(), data.val /*TODO élément*/);
    rep = MW_ANSWER_INSERT;
    ret = write(data.tube_to_master, &rep, sizeof(int));
    myassert(ret != 0 , "erreur read dans COM_TO_CLIENT, personne en écriture");
    myassert(ret == sizeof(int), "erreur la valeur lue n'est pas de la taille d'un int");
     
    //TODO note : en effet si je suis créé c'est qu'on vient d'insérer un élément : moi

    loop(&data);

    //TODO fermer les tubes
    close(data.pere_to_fils);
    close(data.fils_to_pere);
    close(data.tube_to_master);

    TRACE3("    [worker (%d, %d) {%g}] : fin worker\n", getpid(), getppid(), data.val /*TODO élément*/);
    return EXIT_SUCCESS;
}
