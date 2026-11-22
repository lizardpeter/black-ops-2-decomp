// universal/com_workercmds.cpp (CoDMPServer_PC: com_workercmds.obj)
//
// The job queue's worker commands. So far the front end's check that its jobs have finished.
#define T6_SOURCE "universal/com_workercmds.cpp"

#include "universal/com_workercmds.h"

extern jqModule fx_spawnModule, fx_update_spotModule, fx_updateModule, fx_update_remainingModule,
	r_add_staticmodelModule, r_add_staticmodel_sunModule, r_add_bspModule, r_add_bsp_sunModule, r_dpvs_staticModule,
	r_dpvs_sceneentModule, r_dpvs_dynmodelModule, r_dpvs_dynbrushModule, dobj_skelModule, dobj_skinModule,
	fx_drawModule, fx_marks_drawModule, r_model_skinModule, nuge_physicsModule, r_add_draw_callsModule;
extern jqBatchGroup r_sceneentGroup;

// 0076FBA0  universal/com_workercmds.cpp:469 (inlined in the Store build: R_ToggleSmpFrameCmd 14071B880)
// Whether every front-end job group is idle, tested in the order the frame queues them.
int R_FinishedFrontendWorkerCmds()
{
	return !jqPoll(&fx_spawnModule.Group) && !jqPoll(&fx_update_spotModule.Group) && !jqPoll(&fx_updateModule.Group)
	    && !jqPoll(&fx_update_remainingModule.Group) && !jqPoll(&r_add_staticmodelModule.Group)
	    && !jqPoll(&r_add_staticmodel_sunModule.Group) && !jqPoll(&r_add_bspModule.Group)
	    && !jqPoll(&r_add_bsp_sunModule.Group) && !jqPoll(&r_dpvs_staticModule.Group)
	    && !jqPoll(&r_dpvs_sceneentModule.Group) && !jqPoll(&r_dpvs_dynmodelModule.Group)
	    && !jqPoll(&r_dpvs_dynbrushModule.Group) && !jqPoll(&r_sceneentGroup) && !jqPoll(&dobj_skelModule.Group)
	    && !jqPoll(&dobj_skinModule.Group) && !jqPoll(&fx_drawModule.Group) && !jqPoll(&fx_marks_drawModule.Group)
	    && !jqPoll(&r_model_skinModule.Group) && !jqPoll(&nuge_physicsModule.Group)
	    && !jqPoll(&r_add_draw_callsModule.Group);
}
