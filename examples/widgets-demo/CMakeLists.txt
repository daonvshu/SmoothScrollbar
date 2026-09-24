add_executable(smoothscrollbar_demo main.cpp)
target_link_libraries(smoothscrollbar_demo
    PRIVATE
        SmoothScrollbar
        Qt${QT_VERSION_MAJOR}::Widgets
)
set_target_properties(smoothscrollbar_demo PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE ON
)
