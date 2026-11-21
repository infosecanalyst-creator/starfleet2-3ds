# Screen regression test (runs on the PC / in CI, not on the 3DS).
#   make check    compare every dump in screens/ with expected/
#   make update   rewrite expected/ after an intended change (review the git diff!)
#   make png      also render both screens of every dump into out/ (PPM)
CXX ?= g++
CXXFLAGS ?= -std=c++14 -O1 -Wall -Wno-unused-function -Wno-unused-variable

screentest: screentest.cpp ../sf2ui/sf2ui.cpp ../sf2ui/sf2ui.h ../sf2ui/draw.cpp ../sf2ui/draw.h ../sf2ui/fonts.h ../sf2ui/library_data.h ../sf2ui/refusals.h
	$(CXX) $(CXXFLAGS) -I../sf2ui -o $@ screentest.cpp ../sf2ui/draw.cpp

check: screentest
	./screentest screens expected

update: screentest
	./screentest screens expected --update

png: screentest
	mkdir -p out && ./screentest screens expected --png out

.PHONY: check update png
