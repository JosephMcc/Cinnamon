#include "config.h"

#include "cinnamon-info-osd.h"

struct _CinnamonInfoOsd
{
    StBoxLayout parent;

    StLabel *label;
};

G_DEFINE_TYPE (CinnamonInfoOsd, cinnamon_info_osd, ST_TYPE_BOX_LAYOUT)

enum
{
    PROP_0,
    PROP_TEXT,
    LAST_PROP
};

static GParamSpec *properties [LAST_PROP];

static void
cinnamon_info_osd_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    CinnamonInfoOsd *self = CINNAMON_INFO_OSD (object);

    switch (prop_id)
    {
        case PROP_TEXT:
            st_label_set_text (self->label, g_value_get_string (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
cinnamon_info_osd_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    CinnamonInfoOsd *self = CINNAMON_INFO_OSD (object);

    switch (prop_id)
    {
        case PROP_TEXT:
            g_value_set_string (value, clutter_text_get_text (CLUTTER_TEXT (self->label)));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
cinnamon_info_osd_dispose (GObject *object)
{
    CinnamonInfoOsd *self = CINNAMON_INFO_OSD (object);

    if (self->label)
    {
        clutter_actor_destroy (CLUTTER_ACTOR (self->label));
        self->label = NULL;
    }

    G_OBJECT_CLASS (cinnamon_info_osd_parent_class)->dispose (object);
}

static void
cinnamon_info_osd_init (CinnamonInfoOsd *self)
{
    st_widget_set_style_class_name (ST_WIDGET (self), "info-osd");
    st_widget_set_important (ST_WIDGET (self), TRUE);
    st_box_layout_set_vertical (ST_BOX_LAYOUT (self), TRUE);
}

static void
cinnamon_info_osd_class_init (CinnamonInfoOsdClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = cinnamon_info_osd_set_property;
    object_class->get_property = cinnamon_info_osd_get_property;
    object_class->dispose = cinnamon_info_osd_dispose;

    properties [PROP_TEXT] =
        g_param_spec_string ("text",
                             "Text",
                             "The text for the Osd",
                             NULL,
                             (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties (object_class, LAST_PROP, properties);
}

CinnamonInfoOsd *
cinnamon_info_osd_new (const gchar *text)
{
    if (text == NULL || *text == '\0')
    {
        return g_object_new (CINNAMON_TYPE_INFO_OSD, NULL);
    }
    else
    {
        return g_object_new (CINNAMON_TYPE_INFO_OSD,
                             "text", text,
                             NULL);
    }
}

void
cinnamon_info_osd_add_text (CinnamonInfoOsd *self,
                            const gchar     *text)
{
    StWidget *label;

    g_return_if_fail (text != NULL);

    label = st_label_new (text);
    clutter_actor_add_child (CLUTTER_ACTOR (self), CLUTTER_ACTOR (label));
}