add_executable(tst_smoothscrollbar
    tst_smoothscrollbar.cpp
)
set_target_properties(tst_smoothscrollbar PROPERTIES AUTOMOC ON)
target_link_libraries(tst_smoothscrollbar
    PRIVATE
        SmoothScrollbar
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Widgets
)
target_include_directories(tst_smoothscrollbar PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_test(NAME smoothscrollbar.unit COMMAND tst_smoothscrollbar)
set_tests_properties(smoothscrollbar.unit PROPERTIES TIMEOUT 30)

