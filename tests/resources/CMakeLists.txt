project(${PROJECT_NAME}_tests_resources)

add_executable(posixClient posixClient.c)
add_executable(posixEchoServer posixEchoServer.c)
add_executable(cxxClient client.cpp)
add_executable(cxxEchoServer echoServer.cpp)
