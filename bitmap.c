//
// Created by Arnold Balliu on 4/19/17.
//

#include <printf.h>
#include "bitmap.h"

// Bit operations borrowed from here
// http://stackoverflow.com/a/47990/5028531

void bitmap_set_bit(unsigned char* c, int bit_val, int pos){
    *c ^= (-bit_val ^ *c) & (1 << pos); //set the bit in the specified position the bit_value given
}

int bitmap_check_bit(unsigned char* c, int pos){
    return ((*c >> pos) & 1); //Apply AND to check the bit in that position
}

int bitmap_set(struct bitmap_t* bitmap, int value, int index){

    if (index > MAX_BITS)
        return -1;

    bitmap_set_bit(&bitmap->array[GET_CHAR_INDEX(index)], value, GET_BIT_INDEX(index));

    return 0;
}

int bitmap_check(struct bitmap_t* bitmap, int index){
    if (index > MAX_BITS)
        return -1;

    return bitmap_check_bit(&bitmap->array[GET_CHAR_INDEX(index)],GET_BIT_INDEX(index));
}

void bitmap_init(struct bitmap_t* bitmap){
    int i;
    for (i = 0; i<MAX_BITS; i++){
        bitmap_set(bitmap,0,i);
    }
}