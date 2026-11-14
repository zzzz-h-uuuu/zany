add_executable(zany_test
    main.cpp
)

target_link_libraries(zany_test PRIVATE zany::zany)

add_test(
    NAME zany_test
    COMMAND zany_test
)