#ifndef __CINNAMON_TILE_PREVIEW_H__
#define __CINNAMON_TILE_PREVIEW_H__

#include "st.h"
#include "cinnamon-global.h"

G_BEGIN_DECLS

#define CINNAMON_TYPE_TILE_PREVIEW (cinnamon_tile_preview_get_type ())
G_DECLARE_FINAL_TYPE (CinnamonTilePreview, cinnamon_tile_preview, CINNAMON, TILE_PREVIEW, GObject)

CinnamonTilePreview *cinnamon_tile_preview_new (void);

void cinnamon_tile_preview_show (CinnamonTilePreview *self,
                                 MetaPlugin          *plugin,
                                 MetaWindow          *window,
                                 MetaRectangle       *tile_rect,
                                 int                  tile_monitor,
                                 guint                snap_queued);

void cinnamon_tile_preview_hide (CinnamonTilePreview *self);

G_END_DECLS

#endif /* __CINNAMON_TILE_PREVIEW_H__ */