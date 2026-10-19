CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra
CPPFLAGS = -I/opt/homebrew/opt/nlohmann-json/include
LDLIBS = -lcurl

SOURCES = main.cpp HttpClient.cpp EspnClient.cpp StatsCsv.cpp PlayerAnalysis.cpp simulation.cpp FantasyScoring.cpp 
HEADERS = HttpClient.h EspnClient.h Game.h PlayerGameStats.h StatsCsv.h PlayerAnalysis.h simulation.h FantasyScoring.h

sports: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o sports $(LDLIBS)