#include "behaviour_tree.h"
#include "console/log.h"
#include <assert.h>

BT_Tree bt_init()
{
    return (BT_Tree){};
}

void stack_push(BT_Runtime *runtime, BT_Index node);
int node_tick(BT_Tree *tree, BT_Runtime *runtime, BT_BlackBoard *bb);
void stack_pop(BT_Runtime *runtime);

void bt_tick(BT_Tree *tree, BT_Runtime *runtime, BT_BlackBoard *bb)
{
    if(runtime->stack_count <= 0)
    {
        stack_push(runtime, tree->first);
    }

    do
    {
        if(!node_tick(tree, runtime, bb))
        {
            break;
        }
    }while(runtime->stack_count != 0);
}

int node_tick(BT_Tree *tree, BT_Runtime *runtime, BT_BlackBoard *bb)
{
    BT_Stack_Frame *frame = runtime->stack + runtime->stack_count - 1;
    BT_Node *node = tree->nodes + frame->node;

    if(node->type == BT_EXECUTION)
    {
        enum BT_Exec_State result = node->exec_action(bb);

        if(result == BT_RUNNING) return 0;

        runtime->last_return = result;
        stack_pop(runtime);
    }
    else if(node->type == BT_SEQUENCE)
    {
        if(frame->child_index >= 0)
        {
            if(runtime->last_return == BT_FAILURE)
            {
                stack_pop(runtime);
                return 1;
            }
        }

        frame->child_index++;

        if(frame->child_index >= node->child_count)
        {
            runtime->last_return = BT_SUCCESS;
            stack_pop(runtime);
            return 1;
        }

        stack_push(runtime, node->children[frame->child_index]);
    }
    else if(node->type == BT_FALLBACK)
    {
        if(frame->child_index >= 0)
        {
            if(runtime->last_return == BT_SUCCESS)
            {
                stack_pop(runtime);
                return 1;
            }
        }

        frame->child_index++;

        if(frame->child_index >= node->child_count)
        {
            runtime->last_return = BT_FAILURE;
            stack_pop(runtime);
            return 1;
        }

        stack_push(runtime, node->children[frame->child_index]);
    }
    return 1;
}

void stack_push(BT_Runtime *runtime, BT_Index node)
{
    assert(runtime->stack_count < BT_STACK_SIZE);
    runtime->stack[runtime->stack_count++] = (BT_Stack_Frame){
        .node = node,
        .child_index = -1,
    };
}

void stack_pop(BT_Runtime *runtime)
{
    --runtime->stack_count;
}

BT_Index BT_add_exec_node(BT_Tree *tree, BT_Action action)
{
    BT_Index index = tree->node_count++;
    BT_Node *node = tree->nodes + index; 
    *node = (BT_Node){
        .type = BT_EXECUTION,
        .exec_action = action,
    };
    return index;
}

BT_Index BT_add_sequ_node(BT_Tree *tree, BT_Index *indices, int indices_count)
{
    BT_Index index = tree->node_count++;
    BT_Node *node = tree->nodes + index; 
    *node = (BT_Node){
        .type = BT_SEQUENCE,
        .child_count = indices_count,
    };
    for(int i = 0; i < indices_count; ++i) 
    {
        node->children[i] = indices[i];
    }
    return index;
}

BT_Index bt_add_fallback_node(BT_Tree *tree, BT_Index *indices, int indices_count)
{
    BT_Index index = tree->node_count++;
    BT_Node *node = tree->nodes + index; 
    *node = (BT_Node){
        .type = BT_FALLBACK,
        .child_count = indices_count,
    };
    for(int i = 0; i < indices_count; ++i) 
    {
        node->children[i] = indices[i];
    }
    return index;
}
