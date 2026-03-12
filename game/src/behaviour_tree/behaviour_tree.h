#ifndef BEHAVIOUR_TREE_H
#define BEHAVIOUR_TREE_H

#include <stddef.h>

#define BT_STACK_SIZE 100

typedef unsigned int BT_Index;

enum BT_Node_Type
{
    BT_EXECUTION,
    BT_SEQUENCE,
    BT_FALLBACK,
};

enum BT_Exec_State
{
    BT_RUNNING,
    BT_SUCCESS,
    BT_FAILURE,
};

typedef struct BT_BlackBoard
{
    int counter;
} BT_BlackBoard;

typedef enum BT_Exec_State (*BT_Action)(BT_BlackBoard *bb);

typedef struct BT_Node
{
    enum BT_Node_Type   type;

    BT_Action           exec_action;

    BT_Index            children[10];
    int                 child_count;
}BT_Node;

typedef struct BT_Stack_Frame
{
    BT_Index            node;
    int                 child_index;
} BT_Stack_Frame;

typedef struct BT_Runtime
{
    BT_Stack_Frame      stack[BT_STACK_SIZE];
    int                 stack_count;
    enum BT_Exec_State  last_return;
} BT_Runtime;

typedef struct BT_Tree
{
    BT_Node     nodes[100];
    int         node_count;
    BT_Index    first;
} BT_Tree;

BT_Tree BT_init();

void BT_tick(BT_Tree *tree, BT_Runtime *runtime, BT_BlackBoard *bb);

BT_Index BT_add_exec_node(BT_Tree *tree, BT_Action action);

BT_Index BT_add_sequ_node(BT_Tree *tree, BT_Index *indices, int indices_count);

BT_Index BT_add_fallback_node(BT_Tree *tree, BT_Index *indices, int indices_count);
 
#endif // BEHAVIOUR_TREE_H
