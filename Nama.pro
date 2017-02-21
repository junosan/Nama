HEADERS       = \
    code/constants.h \
    code/glwidget.h \
    code/textdisp.h \
    code/textedit.h \
    code/window.h \
    code/gamethread.h \
    code/player.h \
    code/gameinfo.h \
    code/haibutton.h \
    code/tcpserver.h \
    code/tcpclient.h \
    code/namaai_global.h
SOURCES       = \
    code/glwidget.cpp \
    code/main.cpp \
    code/textdisp.cpp \
    code/textedit.cpp \
    code/window.cpp \
    code/gamethread.cpp \
    code/mt19937ar.cpp \
    code/player.cpp \
    code/gameinfo.cpp \
    code/haibutton.cpp \
    code/tcpserver.cpp \
    code/tcpclient.cpp
RESOURCES     = \
    code/resources.qrc

QT  += opengl widgets multimedia

win32 {
    RC_ICONS = images/icon/nama.ico
}


# install
# target.path = $$[QT_INSTALL_EXAMPLES]/opengl/textures
# INSTALLS += target

OTHER_FILES += \
    code/documentation.txt
