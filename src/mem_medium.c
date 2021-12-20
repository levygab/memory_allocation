/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"


unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}

unsigned long pui(unsigned long x, unsigned int a) {
  //met x a la puissance a
  unsigned long retour = x ;
  for (int i = 0 ; i<a ; i++) {
    retour *= x  ;
  }
  return  retour ;
}

void* emalloc_medium(unsigned long size) {
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    void* retour ;

    //on commence par touver le bloc de bonne taille avec puiss2
    unsigned int nb = puiss2(size) ;

    //cas ou le pointeur n'est pas NULL, cad qu'on a un bloc
    if( arena.TZL[nb] != NULL) {

      //on regarde si il y a une adresse pour la case suivante
      unsigned long* p = (unsigned long*) (arena.TZL[nb]) ;
      void* p2 = arena.TZL[nb] ; //on stock egalement l'adresse en memoire


      if (*p == 0) { //si il n'y a rien, on dit que le arena.TZL est null
        arena.TZL[nb] =  NULL ;

      }
      else { //si il y a une adresse : on la donne comme valeur a arena.TZL
        printf("p = %p\n", (void*) *p);
        arena.TZL[nb] = (void*) *p ;
        unsigned long* test = (unsigned long*) arena.TZL[nb] ;
        printf("p suivant = %p\n", (void*) *test ) ;
      }

      //on va maintenant envoyer le pointeur dans la fct de marquage puis le renvoyer
      p2 = mark_memarea_and_get_user_ptr(p2 , size , MEDIUM_KIND) ; //appel de la fct

      return p2 ;
    }


    //cas ou il n'y a pas de bloc disponible de la bonne taille
    else {

      int i = 0 ;
      int presence = 0 ; //nb qui vaut 0 tant que les indices sont null et 1 des que un indice contient une pace en memoire
      while ( (nb + i < FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant) && presence == 0 ) {  //on cherche ici le premier bloc d'une taille supérieure disponible sans dépasser le dernier indice
        if (arena.TZL[nb+i] != NULL) presence = 1 ; //on a trouve un bloc non vide : on va l'utiliser
        i++ ;
      }

      if (presence == 0 ) { // dans le cas où il n'y a pas de grands blocs disponible
        mem_realloc_medium() ; // on appelle a fonction qui rajoute un grand bloc à l'indice FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant, elle se charge elle même d'augmenter arena.medium_next_exponant
        //une fois qu'on a appele la fct : on rappelle notre fonction
        retour = emalloc_medium(size) ; //l'argument reste le meme
      }

      else { // dans ce cas là on a trouve l'indice nb+i qui correspond au premier bloc de taille plus grande, on doit couper en deux un bloc qui se trouve à cet indice
        i-- ; //on commence par diminuer i qui a ete augmente apres avoir trouve le bloc
        unsigned long* p = (unsigned long*) (arena.TZL[nb+i]) ; //on stock l'adresse du bloc à couper en deux : on prend un long* dans le but de pouvoir ecrire dessus apres
        unsigned long* p3 = (unsigned long*) (arena.TZL[nb + i]) ;
        //on va egalemment aller stocker l'adresse de l'autre moitie du bloc
        printf("l'adresse servant a calculer le buddy est : %p\n", (arena.TZL[nb+i]));
        unsigned long calcul = pui(2,nb-1) ;
        void* p2 = (void*)  (((unsigned long) (arena.TZL[nb+i])) ^ (calcul )) ; //formule donne dans le sujet
        printf("le buddy calcule dans alloc vaut %p\n",p2 );
        //on doit maintenant changer les valeurs :
        if (*p3 != 0) { //si p3 contient une valeur (cad un next)
          arena.TZL[nb+i] = (void*) *p3 ; //on rentre cette valeur la ou elle doit etre
          printf("apres l'attribution dans le if, arena vaut %p\n",arena.TZL[nb+i] );
        }

        else { //si il n'y a pas de next
          printf("a la place je suis ici\n" );
          arena.TZL[nb+i] = NULL ; //ce sera un NULL alors
        }

        //on va rentrer les deux blocs dans les blocs de taille juste en dessous
        (arena.TZL) [nb+i-1] = p ; //le premier : on rentre juste sa valeur
        *p = (unsigned long) p2 ; //le deuxieme on rentre la valeur dans le premier
        void** ecriture = (void**) p2 ;
        *ecriture = 0 ;

        //on doit reappeler la fct
        printf("fin de l'alloc\n" );
        retour = emalloc_medium(size) ;


      }

      //return emalloc_medium(size) ; // on rappelle la fonction afin de trouver le bloc de la taille qu'on veut
    }



    return retour;
}


void efree_medium(Alloc a) {
  //int fin = 1 ; //vaut 1 si jamais on doit relancer un tour
  //on récupère l'adresse du bloc à libérer
  void* p = a.ptr ;

  // on calcule l'adresse du buddy
  unsigned long buddy =  *((unsigned long*) p)  ^ a.size ;  //pas sûr la
  void* buddyptr = (void*) buddy ;

  // on vérifie maintenant si le compagnon est présent dans la tzl
  unsigned int nb = puiss2(a.size) ;  //si il est présent dans la bonne liste de arena.tzl il est à l'indice nb car de taille a.size

  void* p2 = arena.TZL[nb] ;//il doit etre present dans cette liste
  int presence = 0 ; //variable indiquant s'il est present ou non
  unsigned long* p3 ;
  void* stock = (void*) arena.TZL[nb] ; //variable servant a stocker le precedent : servira si on trouve le buddy
  int i =0 ;
  printf("p2 = %p\n",p2 ); printf("arena.TZL[nb] de free = %p\n",arena.TZL[nb] );
  while (p2 != NULL && presence == 0) {
    printf("i = %d\n",i );
    //des qu'on est pas au premier tour, il faut stocker l'adresse actuelle de p3 dans stock
    if (i != 0) stock = (void*) p3 ;
    //printf("je suis 1\n" );
    printf("p2 = %p\n",p2 );
    p3 = (unsigned long*) p2 ; //on cast pour pouvoir utiliser cette valeur
    if (p3 == buddyptr) { //cas ou le buddy est present
      //printf("je suis 2, il y a le buddy\n" );
      presence = 1 ; //cas ou le buddy est present
    }

    else { //si le buddy n'est pas present pour l'instant
      //printf("je suis 3, pas encore de buddy\n" );
      printf("p3 = %p\n",p3 );
      printf("*p3 = %ld\n",*p3 );
      //on regarde si *p3 contient une adresse
      if (*p3 == 0) {// cas ou *p3 ne contient pas d'adresse, on est en bout de liste
        p2 = NULL ;
        //printf("%d\n",i );
        printf("je suis 4\n" );
      }

      else { //si *p3 contient une adresse, on doit l'affecter a p
      printf("je suis 5\n" );
        p2 = (void*) *p3 ;
      }


    }
    printf("en fin de tour, p2 = %p\n",p2 );
    printf(" i = %d\n",i );
    i++ ;

  }

  //cas ou le buddy est present
  if (presence == 1 ) {

    /*
    on doit supprimer le buddy de la liste d'ou on vient de le trouver
    p3 est l'@ du buddy, stock est l'@ de la variable avant le buddy
    */
    //on commence par regarder si on a une adresse supplementaire
    if (*p3 == 0) { //cas ou il n'y a rien apres le buddy
      void** ecriture = (void**) stock ; //pour pouvoir ecrire dans l'@ de stock
      *ecriture = NULL ; //on met cette variable a NULL, ne change rien qu'on ait fait 1 ou pldr tours de boucles
    }

    else { // cas ou il y a une @ apres le buddy
      void** ecriture = (void**) stock ; //pour pouvoir ecrire dans l'@ de stock
      *ecriture = (void*) *p3 ; //on met cette variable a NULL, ne change pas qu'on ait fait 1 ou plsr tours
    }


    //on doit calculer le bloc avec l'adresse la plus petite : cette adresse sera l'adresse de la
    //reunion des deux blocs

    if ((unsigned long ) a.ptr < (unsigned long) buddyptr ) { //cas ou le buddy doit etre devant
      unsigned long* p6 = (unsigned long*) arena.TZL[nb+1] ; //on va regarder la ou on doit placer la reunion
      if (p6 == NULL) arena.TZL[nb+1] = buddyptr ; //cas ou c'est vide, on a juste a ajouter la reunion

      //cas ou ce n'est pas vide
      else {
          unsigned long val = (unsigned long) *p6 ; //on stock l'adresse de p2 qu'on rentrera comme valeur dans le buddy
          arena.TZL[nb+1] =  buddyptr ; //on met buddy en tete
          void** p5 = (void**) buddyptr ; //on cast en void** pour pouvoir ecrire l'@ du suivant stockee dans val
          *p5 = (void*) val ;
      }
    }
    else { //cas ou le buddy doit etre derriere
      unsigned long* p6 = (unsigned long*) arena.TZL[nb+1] ; //on va regarder la ou on doit placer la reunion
      if (p6 == NULL) arena.TZL[nb+1] = a.ptr ; //cas ou c'est vide : on a juste a ajouter l'adresse en tete

      //cas ou ce n'est pas vide
      else {
        unsigned long val = (unsigned long) *p6 ; //on stock l'adresse de p2 qu'on rentrera comme valeur dans le buddy
        arena.TZL[nb+1] = a.ptr; //on met l'adresse en tete
        void** p5 = (void**) a.ptr ; //on cast en void** pour pouvoir ecrire l'@ suivante
        *p5 = (void*) val ; //on ecrit la valeur

      }
    }
  }

  //cas ou le buddy n'est pas present : on peut mettre le bloc a liberer dans la liste de sa taille
  else {
    printf("\nbuddy pas trouve\n\n" );
    p2 = arena.TZL[nb] ; //on stock l'@ de arena.TZL pour pouvoir la réécrire dans la tete de liste

    arena.TZL[nb] = a.ptr ; //on remplace la tete de liste
    void** ecrire = (void**) arena.TZL[nb] ;
    *ecrire = p2 ;
  }




}
