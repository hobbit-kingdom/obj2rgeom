#ifndef __RGEOM_OPTIONS_H__
#define __RGEOM_OPTIONS_H__

#define COLL_NONE 0
#define COLL_BYRENDERSURFACES 1
#define COLL_BYSURFACE 2

int run_rgeom_export_options(GlobalFunc *global, int *collision, char *coll_surface);

#endif

