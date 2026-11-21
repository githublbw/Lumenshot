cmake_minimum_required(VERSION 3.16)
project(Lumenshot LANGUAGES CXX)

find_package(Qt6 6.2 REQUIRED COMPONENTS Gui Widgets DBus Concurrent)
add_library(lumenshot_crop src/crop.cpp)
target_include_directories(lumenshot_crop PUBLIC src)
target_compile_features(lumenshot_crop PUBLIC cxx_std_17)
target_link_libraries(lumenshot_crop PUBLIC Qt6::Gui)

find_library(XCB_LIBRARY xcb REQUIRED)
find_path(XCB_INCLUDE_DIR xcb/xcb.h REQUIRED)
add_library(lumenshot_masks src/masks.cpp)
target_include_directories(lumenshot_masks PUBLIC src)
target_link_libraries(lumenshot_masks PUBLIC Qt6::Gui)
add_executable(lumenshot src/panel.cpp src/windowframes.cpp src/hotkey.cpp src/preferences.cpp)
set_target_properties(lumenshot PROPERTIES AUTOMOC ON)
target_include_directories(lumenshot PRIVATE ${XCB_INCLUDE_DIR})
qt_add_resources(lumenshot icons PREFIX "/" BASE assets FILES
    assets/scan.png assets/monitor.png
    assets/copy.png assets/download.png assets/focus.png assets/circle.png
    assets/rectangle-horizontal.png assets/undo-2.png assets/redo-2.png
    assets/move-up-right.png assets/type.png assets/blend.png assets/grid-3x3.png
    assets/pin.png assets/pin-off.png assets/x.png assets/menu.png)
target_link_libraries(lumenshot PRIVATE lumenshot_crop lumenshot_masks Qt6::Widgets Qt6::DBus Qt6::Concurrent ${XCB_LIBRARY})
# OCR 为可选组件：third_party/onnxruntime 下放置预编译库时自动启用（RapidOCR/PP-OCRv4）。
find_library(ONNXRUNTIME_LIBRARY onnxruntime
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/lib NO_DEFAULT_PATH)
if(ONNXRUNTIME_LIBRARY)
    target_sources(lumenshot PRIVATE src/ocr.cpp)
    target_include_directories(lumenshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/include)
    target_link_libraries(lumenshot PRIVATE ${ONNXRUNTIME_LIBRARY})
    target_compile_definitions(lumenshot PRIVATE LUMENSHOT_OCR
        LUMENSHOT_OCR_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets/ocr")
endif()
install(TARGETS lumenshot RUNTIME DESTINATION bin)
install(FILES packaging/lumenshot.desktop DESTINATION share/applications)
install(FILES assets/lumenshot-icon.png DESTINATION share/icons/hicolor/256x256/apps RENAME lumenshot.png)
install(FILES assets/LUCIDE-LICENSE DESTINATION share/licenses/lumenshot)

include(CTest)
if(BUILD_TESTING)
    add_executable(check_hotkey tests/check_hotkey.cpp src/hotkey.cpp)
    set_target_properties(check_hotkey PROPERTIES AUTOMOC ON)
    target_include_directories(check_hotkey PRIVATE src ${XCB_INCLUDE_DIR})
    target_link_libraries(check_hotkey PRIVATE Qt6::Core ${XCB_LIBRARY})
    add_executable(test_crop tests/test_crop.cpp)
    target_link_libraries(test_crop PRIVATE lumenshot_crop)
    add_test(NAME crop COMMAND test_crop)
    add_executable(test_masks tests/test_masks.cpp)
    target_link_libraries(test_masks PRIVATE lumenshot_masks)
    add_test(NAME masks COMMAND test_masks)
    set_tests_properties(masks PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
