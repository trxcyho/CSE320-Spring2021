#include <stdlib.h>
#include <stdio.h>

#include "bdd.h"
#include "debug.h"

/*
 * Macros that take a pointer to a BDD node and obtain pointers to its left
 * and right child nodes, taking into account the fact that a node N at level l
 * also implicitly represents nodes at levels l' > l whose left and right children
 * are equal (to N).
 *
 * You might find it useful to define macros to do other commonly occurring things;
 * such as converting between BDD node pointers and indices in the BDD node table.
 */
#define LEFT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->left)
#define RIGHT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->right)

int nodeEqual(BDD_NODE *node, char level, int left, int right);
int hashIndex(int level, int left, int right);
int bfr_recursion_helper(int wstart, int wend, int hstart, int hend, int w, int h, unsigned char *raster);
void bs_recursive(BDD_NODE *node, FILE *out, int *serial);
int bd_helper(int *serial, FILE *in);

/**
 * Look up, in the node table, a BDD node having the specified level and children,
 * inserting a new node if a matching node does not already exist.
 * The returned value is the index of the existing node or of the newly inserted node.
 *
 * The function aborts if the arguments passed are out-of-bounds.
 */
int bdd_lookup(int level, int left, int right) {
    //invalid args check
    if (left < 0 || left > BDD_NODES_MAX)
        return -1;
    if (level < 0 || level > BDD_NODES_MAX)
        return -1;
    if (right < 0 || right > BDD_NODES_MAX)
        return -1;
    if (left == right)
        return left;

    int index = hashIndex(level, left, right);

    while (*(bdd_hash_map + index) != NULL){
        if(nodeEqual(*(bdd_hash_map + index), level, left, right) == 1)
            return *(bdd_hash_map + index) - bdd_nodes;
        index++;
        if(index == BDD_HASH_SIZE) //looping around if reached the end
            index = 0;
    }

    //find free space
    BDD_NODE *new_node;
    for(int i = 256; i < BDD_NODES_MAX; i++){
        new_node = bdd_nodes + i;
        if(new_node -> level == 0)
            goto bd_create_node;
    }
    return -1;

    bd_create_node:
    //create node
    new_node -> level = level;
    new_node -> left = left;
    new_node -> right = right;

    *(bdd_hash_map + index) = new_node;
    return new_node - bdd_nodes;
}

int hashIndex(int level, int left, int right){
    int index = level & 0x7f;
    index = index << 7;
    index = index | (left & 0x7f);
    index = index <<7;
    index = index | (right & 0x7f);
    return index % BDD_HASH_SIZE;
}

int nodeEqual(BDD_NODE *node, char level, int left, int right){
    if (node -> level == level && node -> left == left && node -> right == right)
        return 1;
    return 0;
}

int bdd_min_level(int w, int h){
    if (w < 1 || h < 1)
        return -1; // invalid

    int max = w;
    if (h > w)
        max = h;
    //base cases
    if (max == 1)
        return 0;
    else if (max == 2)
        return 2;

    int level = 2;
    int size = 2;
    while (size < max){
        size *= 2;
        level += 2;
    }

    return level;
}

BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster) {
    if (w <= 0 || h <= 0)
        return NULL; // invalid
    //calculate square size of (w, h)
    int power = bdd_min_level(w, h);
    power /= 2;
    int square = 1;
    for (int i = 0; i < power; i++){
        square *= 2;
    }
    //call recursive func
    int index = bfr_recursion_helper(0, square, 0, square, w, h, raster);
    //recursive call fails
    if (index < 0)
        return NULL;
    return bdd_nodes + index;

}

int bfr_recursion_helper(int wstart, int wend, int hstart, int hend, int w, int h, unsigned char *raster){
    int width = wend - wstart;
    int height = hend - hstart;
    if(width * height == 1){
        if(wstart >= w || hstart >= h)
            return 0;
        return *(raster + (hstart * w) + wstart);
    }
    else{
        //continues to recursively call
        int w_midpoint = (wend + wstart)/2;
        int h_midpoint = (hend + hstart)/2;
        int level = bdd_min_level(width, height);

        int top_left = bfr_recursion_helper(wstart, w_midpoint, hstart, h_midpoint, w, h, raster);
        int top_right = bfr_recursion_helper(w_midpoint, wend, hstart, h_midpoint, w, h, raster);
        int top_half = bdd_lookup(level -1, top_left, top_right);

        int bottom_left = bfr_recursion_helper(wstart, w_midpoint, h_midpoint, hend, w, h, raster);
        int bottom_right = bfr_recursion_helper(w_midpoint, wend, h_midpoint, hend, w, h, raster);
        int bottom_half = bdd_lookup(level - 1, bottom_left, bottom_right);

        return bdd_lookup(level, top_half, bottom_half);
    }
}

void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster) {
    int arrayIndex = 0;
    for(int i = 0; i < h; i++){
        for (int j = 0; j < w; j++){
            *(raster + arrayIndex) = bdd_apply(node, i, j);
            // debug("%d\n", bdd_apply(node, i, j));
            arrayIndex++;
        }
    }
}

int bdd_serialize(BDD_NODE *node, FILE *out) {
    // invalid arg
    if (node == NULL)
        return -1;
    int serial = 1;
    bs_recursive(node, out, &serial);
    if(serial > BDD_NODES_MAX) //edge cases
        return -1;
    return 1;
}

void bs_recursive(BDD_NODE *node, FILE *out, int *serial){
    if (node - bdd_nodes > 255){
        int left = node -> left;
        int right = node -> right;
        char c = 0;

        if(left < 256){
            if(*(bdd_index_map + left) == 0){
                *(bdd_index_map + left) = *serial;  //puts serial number in index map
                fputc('@', out);
                c = left;
                fputc(c, out);//prints left leaf node
                (*serial)++; //is reference needed? is this valid?
            }
        }
        else
            bs_recursive((bdd_nodes+left), out, serial);
        if(right < 256){
            if(*(bdd_index_map + right) == 0){
                *(bdd_index_map + right) = *serial;
                fputc('@', out);
                c = right;
                fputc(c, out);//prints right leaf node
                (*serial)++;
            }
        }
        else
            bs_recursive((bdd_nodes + right), out, serial);

        int index = node - bdd_nodes;
        if(*(bdd_index_map + index) == 0){       //check if node already has serial number
            *(bdd_index_map + index) = *serial;
            (*serial)++;

            int level = node -> level;
            level += 64;
            left = *(bdd_index_map + left);
            right = *(bdd_index_map + right);

            fputc(level, out);
            int mask = 0xff;
            int rightmost = 0;
            for(int i = 0; i < 4; i++){
                rightmost = left & mask;
                fputc(rightmost, out);
                left = left >> 8;
            }
            for(int i = 0; i < 4; i++){
                rightmost = right & mask;
                fputc(rightmost, out);
                right = right >> 8;
            }
        }
    }
    else{
        if(*(bdd_index_map + (node - bdd_nodes)) == 0){
            *(bdd_index_map + (node - bdd_nodes)) = *serial;  //puts serial number in index map
            fputc('@', out);
            char c = (node - bdd_nodes);
            fputc(c, out);//prints left leaf node
            (*serial)++; //is reference needed? is this valid?
        }
    }
}

BDD_NODE *bdd_deserialize(FILE *in) {
    // return NULL when file format is incorrect
    int serial = 1;
    // debug("%d", serial);
    int index = bd_helper(&serial, in);
    // debug("index of bd helper: %d", index);
    if (index < 0)
        return NULL;
    return bdd_nodes + index;
}

int bd_helper(int *serial, FILE *in){
    int c= 0, left = 0, right = 0, index = 0;
    c = fgetc(in);
    while(c != EOF) {
        if(c == '@'){
            c = fgetc(in);
            // debug("%x", c);
            if(c > 255 || c < 0)
                return -1;
            *(bdd_index_map + (*serial)) = c;
            (*serial)++;
            if(*serial > BDD_NODES_MAX) // edge cases
                return -1;

        }
        else {
            c = c- '@';
            fread(&left, 4, 1, in);
            fread(&right, 4, 1, in);
            if(c < 0 || c > 32)
                return -1;
            if (left < 0 || right < 0)
                return -1;
            left = *(bdd_index_map + left);
            right = *(bdd_index_map + right);
            index = bdd_lookup(c, left, right);
            //put index into index map
            *(bdd_index_map + (*serial)) = index;
            (*serial)++;
        }
        c = fgetc(in);
    }
    return *(bdd_index_map + (*serial - 1));
}

unsigned char bdd_apply(BDD_NODE *node, int r, int c) {
    if (node == NULL)
        return -1;
    int level, path, child;
    // if (node - bdd_nodes < 256)

    while(node - bdd_nodes > 255){
        level = node -> level;
        if (level %2 == 0){
            level = (level -2)/2;
            path = (r >> level) & 1;
        }
        else{
            level = (level - 1)/2;
            path = (c >> level) & 1;
        }
        if (path == 0)
            child = node -> left;
        else
            child = node -> right;
        node = bdd_nodes+ child;
    }
    return node - bdd_nodes;
}

BDD_NODE *bdd_map(BDD_NODE *node, unsigned char (*func)(unsigned char)) {
    if(node == NULL)
        return NULL;
    if(node - bdd_nodes > 255){
        BDD_NODE *left = bdd_nodes + (node -> left);
        BDD_NODE *lchild = bdd_map(left, func);
        BDD_NODE *right = bdd_nodes + (node -> right);
        BDD_NODE *rchild = bdd_map(right, func);
        int newNodeIndex = bdd_lookup((node -> level), (lchild - bdd_nodes), (rchild -bdd_nodes));
        return bdd_nodes + newNodeIndex;
    }
    else{
        int index = func(node - bdd_nodes);
        return bdd_nodes + index;
    }
}

BDD_NODE *bdd_rotate(BDD_NODE *node, int level) {
    if(node == NULL)
        return NULL;
    if(node-bdd_nodes < 256)
        return node;

    BDD_NODE *left = bdd_nodes + (node -> left);
    BDD_NODE *right = bdd_nodes + (node -> right);
    BDD_NODE *temp_node;
    int correctlevel = node -> level;
    if(correctlevel % 2 != 0){
        temp_node = left;
        left = right;
        right = temp_node;

        node = bdd_nodes + bdd_lookup(correctlevel + 1, (left - bdd_nodes), (right - bdd_nodes));
    }
    else {
        BDD_NODE *top_left;
        BDD_NODE *top_right;
        if(left - bdd_nodes < 256){
            top_left = left;
            top_right = left;
        }
        else{
            top_left = bdd_nodes + (left -> left);
            top_right = bdd_nodes + (left -> right);
        }
        BDD_NODE *bottom_left;
        BDD_NODE *bottom_right;
        if(right - bdd_nodes < 256){
            bottom_left = right;
            bottom_right = right;
        } else {
            bottom_left = bdd_nodes + (right -> left);
            bottom_right = bdd_nodes + (right -> right);
        }

        if((left-> level) != correctlevel -1){
            top_left = left;
            top_right = left;
        }
        if((right -> level) != correctlevel -1){
            bottom_left = right;
            bottom_right = right;
        }

        //rotating nodes itself
        top_left = bdd_rotate(top_left, correctlevel -2);
        top_right = bdd_rotate(top_right, correctlevel -2);
        bottom_left = bdd_rotate(bottom_left, correctlevel -2);
        bottom_right = bdd_rotate(bottom_right, correctlevel -2);
        //rotating this node
        temp_node = top_left;
        top_left = top_right;
        top_right = bottom_right;
        bottom_right = bottom_left;
        bottom_left = temp_node;

        int topindex = bdd_lookup(correctlevel -1, (top_left - bdd_nodes), (top_right - bdd_nodes));
        int bottomindex = bdd_lookup(correctlevel -1, (bottom_left - bdd_nodes), (bottom_right - bdd_nodes));
        return bdd_lookup(correctlevel, topindex, bottomindex) + bdd_nodes;
    }
    return node;
}

BDD_NODE *bdd_zoom(BDD_NODE *node, int level, int factor) {
    if(node == NULL)
        return NULL;
    if(node - bdd_nodes < 256)
        return node;
    if(factor >= 0){
        if((node - bdd_nodes) < 256)
            return node;
        BDD_NODE *leftNode = bdd_nodes + (node -> left);
        BDD_NODE *rightNode = bdd_nodes + (node -> right);
        int node_level = node -> level;
        //increase level by 2k (k doubling of row and column dimension)
        node_level = node_level + (2 * factor);
        int leftIndex = (bdd_zoom(leftNode, level -1, factor)) - bdd_nodes;
        int rightIndex = (bdd_zoom(rightNode, level -1, factor)) - bdd_nodes;
        int newIndex = bdd_lookup(node_level, leftIndex, rightIndex);
        return bdd_nodes + newIndex;
    }
    else{
        //when reach a node with level 2k, decrease b 2k amount to replace with single leaf node
        int levelOfNode = node -> level;
        if(levelOfNode + (2*factor) <= 0){
            if(bdd_nodes - node == 0)
                return node;
            else
                return bdd_nodes + 255;
        }
        else{
            BDD_NODE *leftNode = bdd_nodes + (node -> left);
            BDD_NODE *rightNode = bdd_nodes + (node -> right);
            levelOfNode = levelOfNode + (2 * factor);
            int leftIndex = (bdd_zoom(leftNode, level -1, factor)) - bdd_nodes;
            int rightIndex = (bdd_zoom(rightNode, level -1, factor)) - bdd_nodes;
            int newIndex = bdd_lookup(levelOfNode, leftIndex, rightIndex);
            return bdd_nodes + newIndex;
        }
    }
    return NULL;
}
