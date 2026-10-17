CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra
CPPFLAGS = -I/opt/homebrew/opt/nlohmann-json/include
LDLIBS = -lcurl

SOURCES = main.cpp HttpClient.cpp EspnClient.cpp
HEADERS = HttpClient.h EspnClient.h

sports: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o sports $(LDLIBS)