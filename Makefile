# SOURCES := client.cpp server.cpp
# OBJECTS := client.o server.o
TARGETS := client.exe server.exe

all: $(TARGETS)

$(TARGETS): %.exe: %.cpp
	$(CXX) $^ -o $@ -lws2_32 

# $(OBJECTS): %.o: %.cpp
# 	$(CXX) -c $^ -o $@

# Activates server
.PHONY: serve
# Joins server on Localhost (don't use if server is on another device)
.PHONY: join
# Cleans directory (including clearing user list)
.PHONY: clean
# Clears user list
.PHONY: clear

serve: server.exe
	.\server

join: client.exe
	.\client 127.0.0.1

clean:
	del $(TARGETS) .\users.txt

clear:
	del users.txt