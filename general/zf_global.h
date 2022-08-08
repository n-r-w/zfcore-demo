#pragma once

#ifdef Z_JEMALLOC_ACTIVATED
#include <jemalloc/jemalloc.h>
#endif

#include <memory>
#include <QtGlobal>
#include <QString>
#include <QVariant>

#ifdef Z_VLD_ACTIVATED
#include <vld.h>
#endif

#if !defined(QT_STATIC) && !defined(ZFCORE_STATIC)
#if defined(ZFCORE_LIBRARY)
#define ZCORESHARED_EXPORT Q_DECL_EXPORT
#else
#define ZCORESHARED_EXPORT Q_DECL_IMPORT
#endif
#else
#define ZCORESHARED_EXPORT
#endif
