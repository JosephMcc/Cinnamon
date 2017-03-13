#ifndef __CINNAMON_TILE_HUD_H__
#define __CINNAMON_TILE_HUD_H__

#include "st.h"
#include "cinnamon-global.h"

G_BEGIN_DECLS

#define CINNAMON_TYPE_TILE_HUD (cinnamon_tile_hud_get_type ())
G_DECLARE_FINAL_TYPE (CinnamonTileHud, cinnamon_tile_hud, CINNAMON, TILE_HUD, GObject)

CinnamonTileHud *cinnamon_tile_hud_new (void);

void cinnamon_tile_hud_show (CinnamonTileHud *self,
                             MetaPlugin      *plugin,
                             guint            current_proximity_zone,
                             MetaRectangle   *work_area,
                             guint            snap_queued);

void cinnamon_tile_hud_hide (CinnamonTileHud *self);

G_END_DECLS

#endif /* __CINNAMON_TILE_HUD_H__ */