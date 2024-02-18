find_package(cmocka CONFIG REQUIRED)

add_library(uri uri.c)

add_executable(uri_tester tester.c uri.c)

target_link_libraries(uri_tester cmocka)

add_test(UriTester uri_tester)
