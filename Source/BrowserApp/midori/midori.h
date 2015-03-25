/*

*/

#ifndef __MIDORI_H__
#define __MIDORI_H__

#include "midori-app.h"
#include "midori-array.h"
#include "midori-bookmarks-db.h"
#include "midori-browser.h"
#include "midori-extension.h"
#include "midori-frontend.h"
#include "midori-locationaction.h"
#include "midori-panel.h"
#include "midori-preferences.h"
#include "midori-privatedata.h"
#include "midori-searchaction.h"
#include "midori-view.h"
#include "midori-websettings.h"
#include "midori-platform.h"
#include <midori/cdosbrowser-core.h> /* Vala API */
#include <function_control.h>

/* For convenience, include localization header */
#include <glib/gi18n-lib.h>

#define MIDORI_CHECK_VERSION(major, minor, micro) \
  (MIDORI_MAJOR_VERSION > (major) || \
  (MIDORI_MAJOR_VERSION == (major) && MIDORI_MINOR_VERSION > (minor)) || \
  (MIDORI_MAJOR_VERSION == (major) && MIDORI_MINOR_VERSION == (minor) && \
  MIDORI_MICRO_VERSION >= (micro)))

#endif /* __MIDORI_H__ */
