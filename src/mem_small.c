/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void* emalloc_small(unsigned long size) {
  //cas ou arena.chuckpool n'est pas NULL : cad qu'il n'y a pas d'allocution
  if (arena.chunkpool != NULL) {
    /*
    on cree un pointeur qui sert a stocker l'adresse actuelle puis on va deplacer
    arena.chuckpool en lisant la valeur stocke dans les 8 premiers octet de arena.chuckpool
    Il y a deux cas possible : soit arena.chuckpool est a sa derniere valeur, on prepare donc
    le terrain pour le prochain appel, soit arena.chuckpool a une adresse en memoire et on a juste
    a le deplacer.
    */
    void* p = arena.chunkpool ; //pointeur de stockage qui rentrera dans memarea

    //on recupere maintenant la valeur stocke dans arena.chuckpool
    unsigned long* p2 = arena.chunkpool ; //en long pour pouvoir acceder aux 8 octets

    if (*p2 == 0 ) { // cas ou il n'y a rien ecrit, cad qu'on est a la fin de la zone memoire
      arena.chunkpool = NULL ; //il devient NULL pour etre realloue au prochain appel
      p = mark_memarea_and_get_user_ptr(p, CHUNKSIZE , SMALL_KIND ) ; //on fait rentrer p dans la fct
      return p ;
    }

    //on est dans le cas ou *p2 != 0 : on doit deplacer arena.chunkpool puis appeler memarea
    arena.chunkpool = (void*) *p2 ; /*on cast la valeur contenue dans p2 (l'adresse du next)
    pour que ce soit juste une adresse */
    p = mark_memarea_and_get_user_ptr(p, CHUNKSIZE , SMALL_KIND ) ; //on appelle la fct
    return p ;
  }

  /*
  on est dans le cas maintenant ou il faut reallouer arena.chuckpool. pour ca on ecrit tous les
  96 octets, l'adresse dans 96 octets suivants. Cette adresse doit etre ecrit dans les 8 premiers
  de chaque maillon de la chaine.
  Pour ecrire toutes ces adresses, on va se balader dans l'espace allloue (en utilisant un
  pointeur type long) puis tous les 96 octets on utilisera un cast pour declarer l'adresse comme un
  void**. on pourra ensuite ecrire a cette adresse, un void* qui sera l'adresse de 96 octets plus loin.
  */

  unsigned long taille = mem_realloc_small() / 96 ; //on a size/96 bloc
  unsigned long* p = arena.chunkpool ; //un long est sur 8 octets
  void** p2 ; //me servira a ecrire les adresses

  for (int i = 0 ; i < taille ; i++ ) {
    p2 = (void**) p ;
    p = p + 12 ; //on deplace p au debut des 96 octets suivants
    *p2 = (void*) p ; //l'adresse des 96 octets suivant est rentre dans *p2
  }

  //maintenant que arena.chunkpool est alloue, on peut d'occuper du deplacement :
  //on part du principe que *arena.chunckpool != 0 on reecrit le code plus haut
  p = arena.chunkpool ; //les long pour lire la valeur sur les 8 premiers bits
  void* p3 = arena.chunkpool ; //le void* qui sera appele par le fct
  arena.chunkpool = (void*) *p ; //on deplace arena.chunckpool
  p3 = mark_memarea_and_get_user_ptr(p3, CHUNKSIZE , SMALL_KIND ) ; //on appelle la fct
  return p3 ;
}

void efree_small(Alloc a) {
    void** p = (void**) a.ptr ; //on fait un cast pour pouvoir ecrire
    *p = arena.chunkpool ; //on donne a p la valeur suivante
    arena.chunkpool = a.ptr ;  //on insere bien le nouveau en tete de liste
}
