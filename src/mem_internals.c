/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in) {
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k) {

  //creation d'un pointeur de taille 8 octets
  unsigned long* p = ptr ;

  //on peut donc ecrire sur les 8 octets suivants
  *p = size ;

  //on ecrit maintenant le magic : on commence par deplacer de 8 octets pour etre au debut du magic
  p = p+1 ;



  //on garde les 62 bits de poids fort (ils disent 61  dans le sujet.....) avec &~ puis on fait un ou avec k qui correpond bien aux valeurs de la fin
  unsigned long magic = (knuth_mmix_one_round( (unsigned long) ptr) & ~(0b11UL)) | ((unsigned long) k) ;
  *p = magic ;
  p = p+1 ; //on avance encore de 8 octets pour arriver sur la zone utilisable par l'utilisateur

  //on est a l'adresse utilisable par l'utilisateur, il faut donc garder cette adresse en memoire : on va utiliser ptr pour la stocker
  ptr = p ;

  //on va faire un cast pour pouvoir se deplacer sur un octet
  unsigned char* p2 = (unsigned char*) p ;
  p2 = p2 + (size - 32) ; //32 correspond a la taille en octet du marquage de debut et de fin

  //on va refaire un cast pour repasse sur un long
  p = (unsigned long*) p2 ;

  *p = magic ; //on reecrit le magic
  p = p + 1 ; //on l'avance au debut de la taille

  *p = size ; //on écrit la taille dans les 8 derniers octets

  return ptr;
}

Alloc mark_check_and_get_alloc(void *ptr) {
  //pointeur sachant que long est sur 8 octets. on recule de 2 pour arriver au debut du marquage
  unsigned long* p = ptr ;
  p = p - 2 ;

  //creation variable retour
  Alloc retour ;


  //on est au debut de la zone, on remplit la varible, on peut aussi remplir size facilement
  retour.ptr = (void*) p ;
  retour.size = *p  ; //plus 4 pour les deux marquages de 2 octets chacun
  retour.kind = *(p+1) & 0b11UL ;




    /* ecrire votre code ici */

    return retour ;
}


unsigned long mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1 << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;

    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
