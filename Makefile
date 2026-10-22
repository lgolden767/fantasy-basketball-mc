CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra
CPPFLAGS = -I/opt/homebrew/opt/nlohmann-json/include
LDLIBS = -lcurl

SOURCES = main.cpp HttpClient.cpp EspnClient.cpp StatsCsv.cpp PlayerAnalysis.cpp simulation.cpp FantasyScoring.cpp HistoricalImport.cpp Backtest.cpp schedule.cpp
HEADERS = HttpClient.h EspnClient.h Game.h PlayerGameStats.h StatsCsv.h PlayerAnalysis.h simulation.h FantasyScoring.h HistoricalImport.h Backtest.h schedule.h

sports: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o sports $(LDLIBS)