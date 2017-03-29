#include "config.h"

#include "cinnamon-tile-hud.h"

#define GAP_PADDING 10
#define ANIMATION_TIME 150

/* edge zones for tiling/snapping identification
   copied from muffin/src/core/window-private.h

  ___________________________
  | 4          0          5 |
  |                         |
  |                         |
  |                         |
  |                         |
  |  2                   3  |
  |                         |
  |                         |
  |                         |
  |                         |
  | 7          1          6 |
  |_________________________|

*/

enum {
    ZONE_0 = 0,
    ZONE_1,
    ZONE_2,
    ZONE_3,
    ZONE_4,
    ZONE_5,
    ZONE_6,
    ZONE_7,
    ZONE_NONE
};

struct _CinnamonTileHud
{
    GObject parent_instance;

    GSettings *base_settings;
    GSettings *hud_settings;
    GSettings *interface_settings;

    ClutterActor *actor;

    gboolean hud_allowed;
    gboolean showing;
    gint hud_threshold;
    guint snap_queued;
    guint zone;

    gint x;
    gint y;
    gint width;
    gint height;
    gint anim_x;
    gint anim_y;
    gint anim_width;
    gint anim_height;
};

G_DEFINE_TYPE (CinnamonTileHud, cinnamon_tile_hud, G_TYPE_OBJECT)

static void
cinnamon_tile_hud_dispose (GObject *object)
{
    CinnamonTileHud *self = CINNAMON_TILE_HUD (object);

    g_clear_object (&self->base_settings);
    g_clear_object (&self->hud_settings);
    g_clear_object (&self->interface_settings);

    G_OBJECT_CLASS (cinnamon_tile_hud_parent_class)->dispose (object);
}

static void
on_show_hud_changed (GSettings       *settings,
                     const gchar     *key,
                     CinnamonTileHud *self)
{
    self->hud_allowed = g_settings_get_boolean (settings, key);
}

static void
update_hud_threshold (GSettings       *settings,
                      const gchar     *key,
                      CinnamonTileHud *self)
{
    gint size;
    guint scale;

    size = g_settings_get_int (self->hud_settings, "tile-hud-threshold");
    scale = g_settings_get_uint (self->interface_settings, "scaling-factor");

    if (scale > 1)
    {
        self->hud_threshold = size * scale;
    }
    else
    {
        self->hud_threshold = size;
    }
}

static void
cinnamon_tile_hud_init (CinnamonTileHud *self)
{
    self->base_settings = g_settings_new ("org.cinnamon");
    self->hud_settings = g_settings_new ("org.cinnamon.muffin");
    self->interface_settings = g_settings_new ("org.cinnamon.desktop.interface");

    self->hud_allowed = g_settings_get_boolean (self->base_settings, "show-tile-hud");

    update_hud_threshold (NULL, NULL, self);

    self->showing = FALSE;
    self->snap_queued = ZONE_NONE;

    g_signal_connect (self->base_settings, "changed::show-tile-hud",
                      G_CALLBACK (on_show_hud_changed), self);
    g_signal_connect (self->hud_settings, "changed::tile-hud-threshold",
                      G_CALLBACK (update_hud_threshold), self);
    g_signal_connect (self->interface_settings, "changed::scaling-factor",
                      G_CALLBACK (update_hud_threshold), self);
}

static void
cinnamon_tile_hud_class_init (CinnamonTileHudClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = cinnamon_tile_hud_dispose;
}

CinnamonTileHud *
cinnamon_tile_hud_new (void)
{
    return g_object_new (CINNAMON_TYPE_TILE_HUD, NULL);
}

static ClutterActor *
create_actor (void)
{
    StWidget *actor;

    actor = st_bin_new ();
    st_widget_add_style_class_name (actor, "tile-hud");
    st_widget_set_important (actor, TRUE);

    return CLUTTER_ACTOR (actor);
}

static void
update_snap_style (CinnamonTileHud *self)
{
    if (st_widget_has_style_class_name (ST_WIDGET (self->actor), "snap"))
    {
        st_widget_remove_style_class_name (ST_WIDGET (self->actor), "snap");
    }
    else
    {
        st_widget_add_style_class_name (ST_WIDGET (self->actor), "snap");
    }
}

static void
update_style (CinnamonTileHud *self,
              const gchar     *pseudo_class)
{
    const gchar *current_style;

    current_style = st_widget_get_style_pseudo_class (ST_WIDGET (self->actor));

    if (current_style)
    {
        st_widget_remove_style_pseudo_class (ST_WIDGET (self->actor), current_style);
    }

    if (pseudo_class)
    {
        st_widget_set_style_pseudo_class (ST_WIDGET (self->actor), pseudo_class);
    }
}

void
cinnamon_tile_hud_show (CinnamonTileHud *self,
                        MetaPlugin      *plugin,
                        guint            current_proximity_zone,
                        MetaRectangle   *work_area,
                        guint            snap_queued)
{
    MetaScreen *screen;
    ClutterActor *window_group;
    gboolean change_zone;

    screen = meta_plugin_get_screen (plugin);
    window_group = meta_get_window_group_for_screen (screen);

    if (!self->hud_allowed)
    {
        return;
    }

    if (!self->actor)
    {
        self->actor = create_actor ();
        clutter_actor_add_child (window_group, self->actor);
    }

    change_zone = (self->zone != current_proximity_zone);

    if (self->snap_queued != snap_queued)
    {
        update_snap_style (self);
        self->snap_queued = snap_queued;
    }

    if (!self->showing || change_zone)
    {
        gint tile_gap;
        const gchar *pseudo_class;

        self->zone = current_proximity_zone;
        tile_gap = self->hud_threshold + GAP_PADDING;

        switch (self->zone)
        {
            case ZONE_0:
                self->x = work_area->x + tile_gap;
                self->y = work_area->y;
                self->width = work_area->width - (tile_gap * 2);
                self->height = 0;
                self->anim_x = self->x;
                self->anim_y = self->y;
                self->anim_width = self->width;
                self->anim_height = self->height + self->hud_threshold;
                pseudo_class = "top";
                break;
            case ZONE_1:
                self->x = work_area->x + tile_gap;
                self->y = work_area->y + work_area->height;
                self->width = work_area->width - (tile_gap * 2);
                self->height = 0;
                self->anim_x = self->x;
                self->anim_y = self->y - self->hud_threshold;
                self->anim_width = self->width;
                self->anim_height = self->height + self->hud_threshold;
                pseudo_class = "bottom";
                break;
            case ZONE_2:
                self->x = work_area->x;
                self->y = work_area->y + tile_gap;
                self->width = 0;
                self->height = work_area->height - (tile_gap * 2);
                self->anim_x = self->x;
                self->anim_y = self->y;
                self->anim_width = self->width + self->hud_threshold;
                self->anim_height = self->height;
                pseudo_class = "left";
                break;
            case ZONE_3:
                self->x = work_area->x + work_area->width;
                self->y = work_area->y + tile_gap;
                self->width = 0;
                self->height = work_area->height - (tile_gap * 2);
                self->anim_x = self->x - self->hud_threshold;
                self->anim_y = self->y;
                self->anim_width = self->width + self->hud_threshold;
                self->anim_height = self->height;
                pseudo_class = "right";
                break;
            case ZONE_4:
                self->x = work_area->x;
                self->y = work_area->y;
                self->width = 0;
                self->height = 0;
                self->anim_x = self->x;
                self->anim_y = self->y;
                self->anim_width = self->width + self->hud_threshold;
                self->anim_height = self->height + self->hud_threshold;
                pseudo_class = "top-left";
                break;
            case ZONE_5:
                self->x = work_area->x + work_area->width;
                self->y = work_area->y;
                self->width = 0;
                self->height = 0;
                self->anim_x = self->x - self->hud_threshold;
                self->anim_y = self->y;
                self->anim_width = self->width + self->hud_threshold;
                self->anim_height = self->height + self->hud_threshold;
                pseudo_class = "top-right";
                break;
            case ZONE_6:
                self->x = work_area->x + work_area->width;
                self->y = work_area->y + work_area->height;
                self->width = 0;
                self->height = 0;
                self->anim_x = self->x - self->hud_threshold;
                self->anim_y = self->y - self->hud_threshold;
                self->anim_width = self->width + self->hud_threshold;
                self->anim_height = self->height + self->hud_threshold;
                pseudo_class = "bottom-right";
                break;
            case ZONE_7:
                self->x = work_area->x;
                self->y = work_area->y + work_area->height;
                self->width = 0;
                self->height = 0;
                self->anim_x = self->x;
                self->anim_y = self->y - self->hud_threshold;
                self->anim_width = self->width + self->hud_threshold;
                self->anim_height = self->height + self->hud_threshold;
                pseudo_class = "top-left";
                break;
            default:
                cinnamon_tile_hud_hide (self);
                return;
        }

        clutter_actor_set_size (self->actor, self->width, self->height);
        clutter_actor_set_position (self->actor, self->x, self->y);

        update_style (self, pseudo_class);

        self->showing = TRUE;
        clutter_actor_show (self->actor);
        clutter_actor_set_child_above_sibling (window_group, self->actor, NULL);
        clutter_actor_set_opacity (self->actor, 0);

        clutter_actor_save_easing_state (self->actor);
        clutter_actor_set_easing_mode (self->actor, CLUTTER_EASE_OUT_QUAD);
        clutter_actor_set_easing_duration (self->actor, ANIMATION_TIME);
        clutter_actor_set_position (self->actor, self->anim_x, self->anim_y);
        clutter_actor_set_size (self->actor, self->anim_width, self->anim_height);
        clutter_actor_set_opacity (self->actor, 255);
        clutter_actor_restore_easing_state (self->actor);
    }
}

static void
on_transition_stopped (ClutterActor *actor,
                       gchar        *name,
                       gboolean      is_finished,
                       gpointer      user_data)
{
    CinnamonTileHud *self = CINNAMON_TILE_HUD (user_data);

    clutter_actor_hide (self->actor);
    clutter_actor_destroy (self->actor);
    self->actor = NULL;
    self->zone = ZONE_NONE;
}

void
cinnamon_tile_hud_hide (CinnamonTileHud *self)
{
    if (!self->showing)
    {
        return;
    }

    self->showing = FALSE;

    clutter_actor_save_easing_state (self->actor);
    clutter_actor_set_easing_mode (self->actor, CLUTTER_EASE_OUT_QUAD);
    clutter_actor_set_easing_duration (self->actor, ANIMATION_TIME);
    clutter_actor_set_position (self->actor, self->x, self->y);
    clutter_actor_set_size (self->actor, self->width, self->height);
    clutter_actor_set_opacity (self->actor, 0);

    g_signal_connect (self->actor, "transition-stopped::opacity",
                      G_CALLBACK (on_transition_stopped), self);

    clutter_actor_restore_easing_state (self->actor);
}
