find_package(Boost 1.55 REQUIRED COMPONENTS date_time regex system coroutine context atomic thread)
include_directories(${Boost_INCLUDE_DIRS})

option(INTERNALGLOOX "use bundled gloox" ON)

if(INTERNALGLOOX)
add_subdirectory(src/gloox)
endif()

add_library(xmpp STATIC src/xmpp_impl.hpp src/xmpp_impl.cpp include/libxmpp/xmpp.hpp src/xmpp.cpp)

target_include_directories(xmpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


if(INTERNALGLOOX)
target_include_directories(xmpp PRIVATE src/gloox)
endif()
target_link_libraries(xmpp ${Boost_LIBRARIES})

target_link_libraries(xmpp gloox)

if(ENABLE_TEST)
	add_executable(xmpptest test.cpp)
	target_link_libraries(xmpptest xmpp)
endif()
