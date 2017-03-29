#ifndef __CINNAMON_INFO_OSD_H__
#define __CINNAMON_INFO_OSD_H__

#include "st.h"
#include "cinnamon-global.h"

G_BEGIN_DECLS

#define CINNAMON_TYPE_INFO_OSD              (cinnamon_info_osd_get_type ())
#define CINNAMON_INFO_OSD(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CINNAMON_TYPE_INFO_OSD, CinnamonInfoOsd))
#define CINNAMON_INFO_OSD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CINNAMON_TYPE_INFO_OSD, CinnamonInfoOsdClass))
#define CINNAMON_IS_INFO_OSD(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CINNAMON_TYPE_INFO_OSD))
#define CINNAMON_IS_INFO_OSD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CINNAMON_TYPE_INFO_OSD))
#define CINNAMON_INFO_OSD_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS (object), CINNAMON_TYPE_INFO_OSD, CinnamonInfoOsdClass);

typedef struct _CinnamonInfoOsd CinnamonInfoOsd;
typedef struct _CinnamonInfoOsdClass CinnamonInfoOsdClass;

struct _CinnamonInfoOsdClass
{
    StBoxLayoutClass parent_class;
};

GType cinnamon_info_osd_get_type (void) G_GNUC_CONST;

CinnamonInfoOsd *cinnamon_info_osd_new (const gchar *text);

void cinnamon_info_osd_add_text (CinnamonInfoOsd *self,
                                 const gchar     *text);

G_END_DECLS

#endif /* __CINNAMON_INFO_OSD_H__ */