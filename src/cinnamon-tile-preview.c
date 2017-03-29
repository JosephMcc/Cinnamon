#include "config.h"

#include "cinnamon-tile-preview.h"

#define ANIMATION_TIME 175

struct _CinnamonTilePreview
{
    GObject parent_instance;

    ClutterActor *actor;
    MetaRectangle *rect;

    gboolean showing;
    gint monitor_index;
    guint snap_queued;
};

G_DEFINE_TYPE (CinnamonTilePreview, cinnamon_tile_preview, G_TYPE_OBJECT)

static ClutterActor *
create_actor (void)
{
    StWidget *actor;

    actor = st_bin_new ();
    st_widget_add_style_class_name (actor, "tile-preview");
    st_widget_set_important (actor, TRUE);

    return CLUTTER_ACTOR (actor);
}

static void
cinnamon_tile_preview_init (CinnamonTilePreview *self)
{
    self->showing = FALSE;
    self->monitor_index = -1;
    self->snap_queued = 0;
}

static void
cinnamon_tile_preview_class_init (CinnamonTilePreviewClass *klass)
{

}

CinnamonTilePreview *
cinnamon_tile_preview_new (void)
{
    return g_object_new (CINNAMON_TYPE_TILE_PREVIEW, NULL);
}

static void
update_style (CinnamonTilePreview *self)
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

void
cinnamon_tile_preview_show (CinnamonTilePreview *self,
                            MetaPlugin          *plugin,
                            MetaWindow          *window,
                            MetaRectangle       *tile_rect,
                            int                  tile_monitor,
                            guint                snap_queued)
{
    ClutterActor *window_actor;
    MetaScreen *screen;
    ClutterActor *window_group;
    gboolean change_monitor;

    screen = meta_plugin_get_screen (plugin);
    window_group = meta_get_window_group_for_screen (screen);

    if (!self->actor)
    {
        self->actor = create_actor ();
        clutter_actor_add_child (window_group, self->actor);
    }

    window_actor = CLUTTER_ACTOR (meta_window_get_compositor_private (window));

    if (!window_actor)
    {
        return;
    }

    if (self->snap_queued != snap_queued)
    {
        update_style (self);
        self->snap_queued = snap_queued;
    }

    if (self->rect && meta_rectangle_equal (self->rect, tile_rect))
    {
        return;
    }

    change_monitor = (self->monitor_index == -1 || self->monitor_index != tile_monitor);

    self->monitor_index = tile_monitor;
    self->rect = meta_rectangle_copy (tile_rect);

    clutter_actor_set_child_below_sibling (window_group, self->actor, window_actor);

    if (!self->showing || change_monitor)
    {
        MetaRectangle window_rect;
        MetaRectangle monitor_rect;
        MetaRectangle clipped_rect;

        meta_screen_get_monitor_geometry (screen, tile_monitor, &monitor_rect);
        meta_window_get_outer_rect (window, &window_rect);
        meta_rectangle_intersect (&window_rect, &monitor_rect, &clipped_rect);

        clutter_actor_set_opacity (self->actor, 0);
        clutter_actor_set_size (self->actor, window_rect.width, window_rect.height);
        clutter_actor_set_position (self->actor, window_rect.x, window_rect.y);
        clutter_actor_show (self->actor);

        self->showing = TRUE;
    }

    clutter_actor_save_easing_state (self->actor);
    clutter_actor_set_easing_mode (self->actor, CLUTTER_EASE_OUT_QUAD);
    clutter_actor_set_easing_duration (self->actor, ANIMATION_TIME);
    clutter_actor_set_position (self->actor, tile_rect->x, tile_rect->y);
    clutter_actor_set_size (self->actor, tile_rect->width, tile_rect->height);
    clutter_actor_set_opacity (self->actor, 255);
    clutter_actor_restore_easing_state (self->actor);
}

static void
on_transition_stopped (ClutterActor *actor,
                       gchar        *name,
                       gboolean      is_finished,
                       gpointer      user_data)
{
    CinnamonTilePreview *self = CINNAMON_TILE_PREVIEW (user_data);

    clutter_actor_hide (self->actor);
    meta_rectangle_free (self->rect);
    self->rect = NULL;
    clutter_actor_destroy (self->actor);
    self->actor = NULL;
    self->monitor_index = -1;
}

void
cinnamon_tile_preview_hide (CinnamonTilePreview *self)
{
    if (!self->showing)
    {
        return;
    }

    self->showing = FALSE;

    clutter_actor_save_easing_state (self->actor);
    clutter_actor_set_easing_mode (self->actor, CLUTTER_EASE_OUT_QUAD);
    clutter_actor_set_easing_duration (self->actor, ANIMATION_TIME);
    clutter_actor_set_opacity (self->actor, 0);

    g_signal_connect (self->actor, "transition-stopped::opacity",
                      G_CALLBACK (on_transition_stopped), self);

    clutter_actor_restore_easing_state (self->actor);
}
