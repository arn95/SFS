//
// Created by Arnold Balliu on 4/17/17.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "filepath.h"


void filepath_init(struct filepath_s* filepath, char* path){

    if (filepath == NULL){
        perror("FILEPATH: Out of memory");
        return;
    }

    if (path == NULL){
        perror("FILEPATH: PATH IS NULL");
        return;
    }

    //make sure these are NULL to begin with
    int i = 0;
    for (; i<FILE_NUM; i++){
        filepath->names[i] = NULL;
    };


    //duplicate so we dont mess with users data
    char* path_cpy = strdup(path);

    char* token;

    filepath->len = 0;

    while ((token = strsep(&path_cpy, "/")))
        filepath->names[filepath->len++] = token; //get each file ex. /home/aballiu/test as [home,aballiu,test]

    filepath->curr_name = filepath->names[0];
    filepath->curr_index = 0;
}

void filepath_traverse(struct filepath_s* filepath){
    filepath->curr_name = filepath->names[filepath->curr_index]; // make sure this is set
}

int filepath_has_next(struct filepath_s* filepath){
    if (filepath->curr_index < filepath->len)
        return 1;
    else
        return 0;
}

char* filepath_next(struct filepath_s* filepath){

    filepath->curr_name = filepath->names[filepath->curr_index++]; //update curr_name pointer and curr_index
    return filepath->curr_name;
};



