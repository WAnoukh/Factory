#ifndef WORKER_BEHAVIOUR_H
#define WORKER_BEHAVIOUR_H

#include "behaviour_tree/behaviour_tree.h"

enum BT_Exec_State is_zone_need_boxes(BT_BlackBoard *bb);

enum BT_Exec_State go_get_box(BT_BlackBoard *bb);

enum BT_Exec_State goto_assigned_zone(BT_BlackBoard *bb);
 
enum BT_Exec_State drop_box(BT_BlackBoard *bb);

enum BT_Exec_State is_zone_need_worker(BT_BlackBoard *bb);
  
enum BT_Exec_State is_working(BT_BlackBoard *bb);

enum BT_Exec_State take_job_in_zone(BT_BlackBoard *bb);

enum BT_Exec_State goto_work(BT_BlackBoard *bb);

enum BT_Exec_State work(BT_BlackBoard *bb);

enum BT_Exec_State wander(BT_BlackBoard *bb);

void create_tree(BT_Tree *tree);

#endif // WORKER_BEHAVIOUR_H
