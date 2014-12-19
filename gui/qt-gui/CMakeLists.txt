find_package(Qt4 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
QT4_WRAP_UI(ui_dialog dialog.ui)
QT4_WRAP_CPP(moc_dialog dialog.h)
add_library(avbotgui STATIC dialog.h dialog.cpp ${ui_dialog} ${moc_dialog})
target_link_libraries(avbotgui ${QT_LIBRARIES})

