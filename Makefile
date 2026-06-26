# ── Ray Tracing Demo v2 ───────────────────────────────────────
CXX      = g++
CXXFLAGS = -std=c++17 -O2

TARGET   = raytracer
SRCS     = src/main.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lmingw32 -lSDL2main -lSDL2 -lm -mwindows \
	    -static-libgcc -static-libstdc++ \
	    -Wl,-Bstatic -lSDL2 -lSDL2main \
	    -Wl,-Bdynamic

clean:
	rm -f $(TARGET).exe

.PHONY: clean
