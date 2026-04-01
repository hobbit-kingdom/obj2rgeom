#include <stdlib.h>
#include <string.h>
#include <lwsdk\lwpanel.h>
#include <lwsdk\lwsurf.h>
#include <lwsdk\lwmodeler.h>

int run_rgeom_export_options(GlobalFunc *global, int *collision, char *coll_surface)
{
	LWPanelFuncs *panf = global(LWPANELFUNCS_GLOBAL, GFUSE_TRANSIENT);
	LWSurfaceFuncs *surff = global(LWSURFACEFUNCS_GLOBAL, GFUSE_TRANSIENT);
	LWStateQueryFuncs *statefunc = global(LWSTATEQUERYFUNCS_GLOBAL, GFUSE_TRANSIENT);
	
	static const char *choices[] = {
		"None",
		"By Render Surfaces",
		"By Surface:",
		NULL
	};
	
	LWPanelID panel;
	int result;
	
	LWControl *ctl_coll_choice;
	LWControl *ctl_coll_surface;
	
	int i,def=0;
	char **surface_list;
	LWSurfaceID *surfaces;
	
	/* variables needed by macros */
	LWPanControlDesc desc;
	LWValue ival = { LWT_INTEGER }, sval = { LWT_STRING };
	/**/
	
	/* make list of surfaces belonging to the object */
	surfaces = surff->byObject(statefunc->object());
	i = 0;
	while(surfaces && surfaces[i])
		i++;
		
	surface_list = malloc(sizeof(const char*) * (i+1));
	i = 0;
	while(surfaces && surfaces[i]) {
		surface_list[i] = strdup(surff->name(surfaces[i]));
		if(strcmp(surface_list[i], "Collision") == 0) def = i;
		i++;
	}
	surface_list[i] = NULL;
	
	/* create panel */
	panel = PAN_CREATE(panf, "Export RGeom");
	ctl_coll_choice = VCHOICE_CTL(panf, panel, "Collision:", choices);
	ctl_coll_surface = POPUP_CTL(panf, panel, "", surface_list); 
	
	SET_INT(ctl_coll_choice, 1);
	SET_INT(ctl_coll_surface, def);
	
	result = PAN_POST(panf, panel);
	
	if(result) {
		GET_INT(ctl_coll_choice, *collision);
		GET_INT(ctl_coll_surface, i);
		strcpy(coll_surface, surface_list[i]);
	}
	
	PAN_KILL(panf, panel);
	
	i = 0; 
	while(surface_list[i]) 
		free(surface_list[i++]);
	free(surface_list);
	
	return result;
}
