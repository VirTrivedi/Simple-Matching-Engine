CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -g
INCLUDES = -I../TradeCoreExport
LIBS = -L../TradeCoreExport -ltradecore -lcurl -lpthread -lrt -lnuma

SRCDIR = .
BUILDDIR = build
BINDIR = $(BUILDDIR)/bin
TARGET = MatchingEngine
SOURCES = main.cpp matching_engine.cpp order_book.cpp
OBJECTS = $(SOURCES:%.cpp=$(BUILDDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CXX) $(OBJECTS) $(LIBS) -o $@
	@echo "Build complete: $@"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BUILDDIR)

-include $(DEPS)