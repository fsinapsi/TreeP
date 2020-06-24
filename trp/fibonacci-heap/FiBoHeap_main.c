#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define  TRUE 1
#define  FALSE 0
// type of the data
typedef int ElemType;

// the structure of each node of this heap
typedef struct FiboNode{
    // the key_value
    ElemType data;
    struct FiboNode *p;
    struct FiboNode *child;
    struct FiboNode *left;
    struct FiboNode *right;
    // the number of its children
    int degree;
    // indicates whether the node lost a child
    int marked;
} FiboNode;

// generate a node
FiboNode * generateFiboNode(ElemType data) {
    FiboNode * fn = NULL;
    fn = (FiboNode *) malloc(sizeof(FiboNode));
    if (fn != NULL) {
        fn->p = NULL;
        fn->child = NULL;
        fn->left = NULL;
        fn->right = NULL;
        fn->data = data;
        fn->degree = 0;
        fn->marked = FALSE;
    } else {
        printf("memory allocation of FiboNode failed");
    }
    return fn;
}

// represent the fibonacci heap
typedef struct FibonacciHeap{
    // pointer pointing to min key in the heap
    FiboNode *min;
    // the number of nodes the heap containing
    int n;
} *FibonacciHeap;

// create Fibonacci Heap, and return it
FibonacciHeap makeFiboHeap() {
    FibonacciHeap h = NULL;
    h = (struct FibonacciHeap *) malloc(sizeof(struct FibonacciHeap));
    if (h == NULL) {
        printf("memory allocation of FibonacciHeap failed");
    }
    h->min = NULL;
    h->n = 0;
    return h;
}

// insert a node into the fibonacci heap
void heapInsert(FibonacciHeap h, FiboNode *x) {
    FiboNode *min = h->min;
    // concatenate the root list containing x with root list h
    if (min == NULL) {
        x->left = x;
        x->right = x;
        h->min = x;
    } else {
        FiboNode * min_right = min->right;
        x->left = min;
        x->right = min_right;
        min->right = x;
        min_right->left = x;

        if (min->data > x->data) {
            h->min = x;
        }
    }

    h->n += 1;
}

// find the minimum node of a fibonacci by giving the pointer h.min
FiboNode * heapMinimum(FibonacciHeap h) {
    return h->min;
}

// union two heap
FibonacciHeap heapUnion(FibonacciHeap *h1, FibonacciHeap *h2) {
    // using a new heap
    FibonacciHeap h = makeFiboHeap();

    if (h != NULL) {
        // concatenate the root list of h with h1 and h2
        if ((*h1)->min == NULL) {
            h->min = (*h2)->min;
        } else if ((*h2)->min == NULL) {
            h->min = (*h1)->min;
        } else {
            FiboNode *min_h1 = (*h1)->min;
            FiboNode *min_right_h1 = min_h1->right;
            FiboNode *min_h2 = (*h2)->min;
            FiboNode *min_right_h2 = min_h2->right;

            min_h1->right = min_right_h2;
            min_right_h2->left = min_h1;

            min_h2->right = min_right_h1;
            min_right_h1->left = min_h2;

            if ((*h1)->min->data > (*h2)->min->data) {
                h->min = (*h2)->min;
            } else {
                h->min = (*h1)->min;
            }
        }

        // release the free memory
        free(*h1);
        *h1 = NULL;
        free(*h2);
        *h2 = NULL;

        // update the n
        h->n = (*h1)->n + (*h2)->n;
    }
    return h;
}

// make y a child of x
void heapLink(FibonacciHeap h, FiboNode *y, FiboNode *x) {
    // remove y from the root list of h
    y->left->right = y->right;
    y->right->left = y->left;

    // make y a child of x, incrementing x.degree
    FiboNode * child = x->child;
    if (child == NULL) {
        x->child = y;
        y->left = y;
        y->right = y;
    } else {
        y->right = child->right;
        child->right->left = y;
        y->left = child;
        child->right = y;
    }
    y->p = x;
    x->degree += 1;

    y->marked = FALSE;
}

void consolidate(FibonacciHeap h) {
    int dn = (int)(log(h->n) / log(2)) + 1;
    FiboNode * A[dn];
    int i;
    for (i = 0; i < dn; ++i) {
        A[i] = NULL;
    }

    // the first node we will consolidate
    FiboNode *w  = h->min;
    // the final node in this heap we will consolidate
    FiboNode *f = w->left;

    FiboNode *x = NULL;
    FiboNode *y = NULL;

    int d;
    // temp
    FiboNode *t = NULL;
    while (w != f) {
        d = w->degree;
        x = w;
        w = w->right;
        while (A[d] != NULL) {
            // another node with the same degree as x
            y = A[d];
            if(x->data > y->data) {
                t = x;
                x = y;
                y = t;
            }
            heapLink(h, y, x);
            A[d] = NULL;
            d += 1;
        }
        A[d] = x;
    }

    // the last node to consolidate (f == w)
    d = w->degree;
    x = w;
    while (A[d] != NULL) {
        // another node with the same degree as x
        y = A[d];
        if(x->data > y->data) {
            t = x;
            x = y;
            y = t;
        }
        heapLink(h, y, x);
        A[d] = NULL;
        d += 1;
    }
    A[d] = x;

    int min_key = 100000;
    h->min = NULL;
   // to get min in this heap
    for (i = 0; i < dn; ++i) {
        if(A[i] != NULL && A[i]->data < min_key) {
            h->min = A[i];
            min_key = A[i]->data;
        }
    }
}

// extract minimum node in this heap
FiboNode * extractMinimum(FibonacciHeap h) {
    // the minimum node
    FiboNode * z = h->min;
    if (z != NULL) {
        FiboNode * firstChid = z->child;
        // add the children of minimum node to the root list.
        if (firstChid != NULL) {
            FiboNode * sibling = firstChid->right;
            // min_right point the right node of minimum node
            FiboNode * min_right = z->right;

            // add the first child to the root list
            z->right = firstChid;
            firstChid->left = z;
            min_right->left = firstChid;
            firstChid->right = min_right;

            firstChid->p = NULL;
            min_right = firstChid;
            while (firstChid != sibling) {
                // record the right sibling of sibling
                FiboNode *sibling_right = sibling->right;

                z->right = sibling;
                sibling->left = z;
                sibling->right = min_right;
                min_right->left = sibling;

                min_right = sibling;
                sibling = sibling_right;

                // update the p
                sibling->p = NULL;

            }
        }
        // remove z from the root list
        z->left->right = z->right;
        z->right->left = z->left;

        // the root list has only one node
        if (z == z->right) {
            h->min = NULL;

            // the children of z shoud be the root list of the heap
            // and find the minimum in this heap
           if (z->child != NULL) {
               FiboNode *child = z->child;
               h->min = child;
               FiboNode *sibling = child->right;
               while (child != sibling) {
                   if (h->min->data > sibling->data) {
                       h->min = sibling;
                   }
                   sibling = sibling->right;
               }
           }
        } else {
            h->min = z->right;
            consolidate(h);
        }
        h->n -= 1;
    }
    return z;

}
void insert(FibonacciHeap h, ElemType e) {
    FiboNode * fb = generateFiboNode(e);
    heapInsert(h, fb);
}

// if we cut node x from y, it indicates root list of
// h not null
void cut(FibonacciHeap h, FiboNode *x, FiboNode *y) {
    // remove x from child list of y, decrementing degree[y]
    if(y->degree == 1) {
        y->child = NULL;
    } else {
        x->left->right = x->right;
        x->right->left = x->left;

        // update child[y]
        y->child = x->right;
    }

    // add x to the root list of h
    x->left = h->min;
    x->right = h->min->right;
    h->min->right = x;
    x->right->left = x;

    // updating p[x], marked[x] and decrementing degree[y]
    x->p = NULL;
    x->marked = FALSE;
    y->degree -= 1;
}



void cascadingCut(FibonacciHeap h, FiboNode *y) {
    FiboNode *z = y->p;
    if (z != NULL) {
        if (y->marked == FALSE) {
            y->marked = TRUE;
        } else {
            cut(h, y, z);
            cascadingCut(h, z);
        }
    }
}

// decrease the data of given node to the key
void heapDecreaseKey(FibonacciHeap h, FiboNode *x, ElemType key) {
    if (key >= x->data) {
        printf("new key is's smaller than original value");
        return;
    }

    x->data = key;
    FiboNode *y = x->p;
    // if x->data >= y->data, do nothing
    // else we need cut
    if (y != NULL && x->data < y->data) {
        cut(h, x, y);
        cascadingCut(h, y);
    }

    // get min node of this heap
    if (x->data < h->min->data) {
        h->min = x;
    }
}

void heapDelete(FibonacciHeap h, FiboNode *x) {
    heapDecreaseKey(h, x, -10000);
    extractMinimum(h);
}
int main() {
    printf("Hello, World!\n");

    FibonacciHeap h = makeFiboHeap();
    FiboNode * fb = generateFiboNode(5);
    heapInsert(h, fb);

    fb = generateFiboNode(1);
    heapInsert(h, fb);

    fb = generateFiboNode(2);
    heapInsert(h, fb);

    fb = generateFiboNode(35);
    heapInsert(h, fb);

    insert(h, 12);
    insert(h, 33);
    insert(h, 44);
    extractMinimum(h);
/*    extractMinimum(h);
    extractMinimum(h);
    extractMinimum(h);
    extractMinimum(h);
    extractMinimum(h);
 */
    heapDelete(h, fb);
    return 0;

}