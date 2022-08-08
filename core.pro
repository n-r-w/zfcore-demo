TARGET = zfcore

include($$TOP_SRC_DIR/share/pri/general_allocation.pri)

QT += core gui printsupport xml uitools concurrent network svg webenginewidgets charts pdf pdfwidgets
QT += core-private widgets-private gui-private
win32 {
    QT += winextras
    DEFINES += UNICODE _UNICODE
}

TEMPLATE = lib
CONFIG += c++17

CONFIG += precompile_header
PRECOMPILED_HEADER = core_pch.h

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += ZFCORE_LIBRARY QT_USE_QSTRINGBUILDER SPDLOG_COMPILED_LIB

win32 {
    # борьба с загадочным багом, когда игнорируется CONFIG += c++17
    QMAKE_CXXFLAGS += /std:c++17

    DEFINES += RP_DLL # генерация отчетов
}

SOURCES += $$files("data_abstraction/*.cpp", true)
HEADERS += $$files("data_abstraction/*.h", true)

SOURCES += $$files("highlight/*.cpp", true)
HEADERS += $$files("highlight/*.h", true)
FORMS += $$files("highlight/*.ui", true)

SOURCES += $$files("database/*.cpp", false)
HEADERS += $$files("database/*.h", false)

SOURCES += $$files("framework/*.cpp", false)
HEADERS += $$files("framework/*.h", false)

SOURCES += $$files("general/*.cpp", false)
HEADERS += $$files("general/*.h", false)

SOURCES += $$files("general/utils/*.cpp", false)

SOURCES += $$files("gui/*.cpp", false)
HEADERS += $$files("gui/*.h", false)
FORMS += $$files("gui/*.ui", false)

HEADERS += $$files("gui/private/*.h", false)
SOURCES += $$files("gui/private/*.cpp", false)

SOURCES += $$files("gui/kde-widgets/*.cpp", false)
HEADERS += $$files("gui/kde-widgets/*.h", false)

SOURCES += $$files("gui/nc_frameless/*.cpp", false)
HEADERS += $$files("gui/nc_frameless/*.h", false)

SOURCES += $$files("gui/uni-view/*.cpp", false)
HEADERS += $$files("gui/uni-view/*.h", false)
FORMS += $$files("gui/uni-view/*.ui", false)

SOURCES += $$files("gui/uni-view/pdf_view/*.cpp", false)
HEADERS += $$files("gui/uni-view/pdf_view/*.h", false)

SOURCES += $$files("item_model/*.cpp", false)
HEADERS += $$files("item_model/*.h", false)
SOURCES += $$files("item_model/private/*.cpp", false)
HEADERS += $$files("item_model/private/*.h", false)

SOURCES += $$files("message_dispatcher/*.cpp", false)
HEADERS += $$files("message_dispatcher/*.h", false)

SOURCES += $$files("messages/*.cpp", false)
HEADERS += $$files("messages/*.h", false)

SOURCES += $$files("model/*.cpp", false)
HEADERS += $$files("model/*.h", false)

SOURCES += $$files("operation/*.cpp", false)
HEADERS += $$files("operation/*.h", false)

SOURCES += $$files("plugin/*.cpp", false)
HEADERS += $$files("plugin/*.h", false)

SOURCES += $$files("reporting/*.cpp", false)
HEADERS += $$files("reporting/*.h", false)

SOURCES += $$files("shell/*.cpp", true)
HEADERS += $$files("shell/*.h", true)
FORMS += $$files("shell/*.ui", true)

SOURCES += $$files("translation/*.cpp", false)
HEADERS += $$files("translation/*.h", false)

SOURCES += $$files("uid/*.cpp", false)
HEADERS += $$files("uid/*.h", false)

SOURCES += $$files("view/*.cpp", false)
HEADERS += $$files("view/*.h", false)

SOURCES += $$files("thread_controller/*.cpp", false)
HEADERS += $$files("thread_controller/*.h", false)

SOURCES += $$files("rest_server/*.cpp", false)
HEADERS += $$files("rest_server/*.h", false)

SOURCES += $$files("driver/*.cpp", false)
HEADERS += $$files("driver/*.h", false)

SOURCES += $$files("script_player/*.cpp", false)
HEADERS += $$files("script_player/*.h", false)
FORMS += $$files("script_player/*.ui", true)
SOURCES += $$files("script_player/private/*.cpp", false)
HEADERS += $$files("script_player/private/*.h", false)

SOURCES += $$files("fuzzy_search/*.cpp", false)
HEADERS += $$files("fuzzy_search/*.h", false)

SOURCES += $$files("single_app/*.cpp", false)
HEADERS += $$files("single_app/*.h", false)

SOURCES += $$files("simple_crypt/*.cpp", false)
HEADERS += $$files("simple_crypt/*.h", false)

SOURCES += $$files("services/*.cpp", true)
HEADERS += $$files("services/*.h", true)

HEADERS += $$files($$TOP_SRC_DIR/3rdparty/spdlog/include/spdlog/*.h, true)

HEADERS += $$files($$TOP_SRC_DIR/3rdparty/base64/*.h, false)
SOURCES += $$files($$TOP_SRC_DIR/3rdparty/base64/*.cpp, false)

HEADERS += $$files($$TOP_SRC_DIR/3rdparty/http-parser/*.h, false)
SOURCES += $$files($$TOP_SRC_DIR/3rdparty/http-parser/*.c, false)

OTHER_FILES += $$TOP_SRC_DIR/share/entity_codes.h
OTHER_FILES += $$files("$$TOP_SRC_DIR/share/catalogs/*.h", false)
OTHER_FILES += $$files("$$TOP_SRC_DIR/share/features/*.h", false)

SOURCES += $$TOP_SRC_DIR/client_version.h

win32 {
    SOURCES += $$files("gui/uni-view/mso/*.cpp", false)
    HEADERS += $$files("gui/uni-view/mso/*.h", false)
}

INCLUDEPATH += \
    $$TOP_SRC_DIR/3rdparty \
    $$TOP_SRC_DIR/3rdparty/spdlog/include \
    general \
    uid \
    framework \
    translation \
    model_manager \
    database \
    model \
    view \
    plugin \
    data_abstraction \
    highlight \
    message_dispatcher \
    item_model \
    operation \
    gui \
    gui/kde-widgets \
    gui/nc_frameless \
    gui/uni-view \
    gui/uni-view/mso \
    gui/uni-view/private \
    reporting \
    messages \
    shell \
    shell/mdi_window \
    shell/modal_window \
    thread_controller \
    rest_server \
    driver \
    script_player \
    fuzzy_search \
    single_app \
    gui/uni-view/pdf_view


OTHER_FILES += ../.qmake.conf
OTHER_FILES += $$TOP_SRC_DIR/3rdparty/.qmake.conf

RESOURCES += resources.qrc
RESOURCES += $$TOP_SRC_DIR/share/icons/share_icons.qrc

INCLUDEPATH += $$TOP_SRC_DIR/3rdparty/hyphenator
INCLUDEPATH += $$TOP_SRC_DIR/3rdparty/quazip

INCLUDEPATH += $$TOP_SRC_DIR/3rdparty/libxml2-win/libxml2/source/include
INCLUDEPATH += $$TOP_SRC_DIR/3rdparty/libxml2-win/zlib/source
INCLUDEPATH += $$TOP_SRC_DIR/3rdparty/libxml2-win/iconv/source/include

INCLUDEPATH += $$TOP_SRC_DIR/3rdparty/qxlsx/src


LIBS += $$LINK_DIRS -lquazip -lhyphenator -lz-qxlsx
LIBS_PRIVATE += -lspdlog
win32 {
    LIBS += $$LINK_DIRS -lz-iconv -lz-libxml2
    LIBS += -luser32 -lshell32 -lgdi32 -lole32 -ladvapi32 -lWinspool -lComdlg32 -lComctl32 -lwsock32
}
unix {
    LIBS += -lxml2
}

# Копирование файла перевода ядра
#defineTranslation($$PWD/translation/zf_translation.h)

unix {
    pvs_studio.target = $$TARGET
    pvs_studio.output = true
    pvs_studio.cxxflags = -std=c++17
    pvs_studio.sources = $${SOURCES}" "$${HEADERS}
    pvs_studio.cfg = $$TOP_SRC_DIR/pvs/pvs.cfg
    include($$TOP_SRC_DIR/pvs/PVS-Studio.pri)
}

