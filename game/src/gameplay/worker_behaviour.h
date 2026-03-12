#ifndef WORKER_BEHAVIOUR_H
#define WORKER_BEHAVIOUR_H

#include "behaviour_tree/behaviour_tree.h"

enum BT_Exec_State do_zone_needs_boxes(BT_BlackBoard *bb);

enum BT_Exec_State retrieve_box(BT_BlackBoard *bb);

enum BT_Exec_State goto_assigned_zone(BT_BlackBoard *bb);
 
enum BT_Exec_State drop_box(BT_BlackBoard *bb);

enum BT_Exec_State do_zone_needs_worker(BT_BlackBoard *bb);
  
enum BT_Exec_State is_working(BT_BlackBoard *bb);

enum BT_Exec_State claim_work_in_zone(BT_BlackBoard *bb);

enum BT_Exec_State goto_work(BT_BlackBoard *bb);

enum BT_Exec_State work(BT_BlackBoard *bb);

enum BT_Exec_State wander(BT_BlackBoard *bb);

void create_tree(BT_Tree *tree);

#endif // WORKER_BEHAVIOUR_H
