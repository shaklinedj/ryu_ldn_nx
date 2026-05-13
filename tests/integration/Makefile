#---------------------------------------------------------------------------------
# ryu_ldn_nx - Integration Test Makefile
#---------------------------------------------------------------------------------

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -O0
CXXFLAGS += -I../../sysmodule/source
CXXFLAGS += -DTEST_BUILD

# Coverage support
CXXFLAGS_COV := $(CXXFLAGS) --coverage -fprofile-arcs -ftest-coverage -fno-inline-small-functions -fno-inline-functions
LDFLAGS_COV := --coverage

TARGET := run_integration_tests

IMPL_SOURCES := \
	../../sysmodule/source/config/config.cpp \
	../../sysmodule/source/config/config_manager.cpp \
	../../sysmodule/source/debug/log.cpp \
	../../sysmodule/source/network/socket.cpp \
	../../sysmodule/source/network/tcp_client.cpp \
	../../sysmodule/source/network/connection_state.cpp \
	../../sysmodule/source/network/reconnect.cpp \
	../../sysmodule/source/network/client.cpp

TEST_SOURCE := integration_tests.cpp

.PHONY: all clean test test-local test-docker docker-compose wait-for-server

all: $(TARGET)

COV ?= 0
ifeq ($(COV),1)
CXXFLAGS_ACT := $(CXXFLAGS_COV)
LDFLAGS_ACT := $(LDFLAGS_COV)
else
CXXFLAGS_ACT := $(CXXFLAGS)
LDFLAGS_ACT :=
endif

$(TARGET): integration_tests.o config.o config_manager.o log.o socket.o tcp_client.o connection_state.o reconnect.o client.o
	$(CXX) $(LDFLAGS_ACT) -o $@ $^ -lpthread

# Compile test source
integration_tests.o: integration_tests.cpp \
	../../sysmodule/source/network/client.hpp \
	../../sysmodule/source/network/tcp_client.hpp \
	../../sysmodule/source/network/socket.hpp \
	../../sysmodule/source/network/connection_state.hpp \
	../../sysmodule/source/network/reconnect.hpp \
	../../sysmodule/source/config/config.hpp \
	../../sysmodule/source/protocol/types.hpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

# Compile implementation sources
config.o: ../../sysmodule/source/config/config.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

config_manager.o: ../../sysmodule/source/config/config_manager.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

log.o: ../../sysmodule/source/debug/log.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

socket.o: ../../sysmodule/source/network/socket.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

tcp_client.o: ../../sysmodule/source/network/tcp_client.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

connection_state.o: ../../sysmodule/source/network/connection_state.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

reconnect.o: ../../sysmodule/source/network/reconnect.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

client.o: ../../sysmodule/source/network/client.cpp
	$(CXX) $(CXXFLAGS_ACT) -c -o $@ $<

# Generate docker-compose.yml from submodule (replaces image owner ryubing→ethiquema)
docker-compose.yml: prepare-compose.sh ../ldn-server/docker-compose.yml
	bash prepare-compose.sh

docker-compose: docker-compose.yml

# Run against a server already running on localhost
test-local: $(TARGET)
	@echo "=== Running integration tests against localhost:30456 ==="
	./$(TARGET) localhost 30456

# Run with Docker-managed server (generates compose from submodule)
test-docker: $(TARGET) docker-compose
	@echo "=== Starting LdnServer via docker-compose ==="
	docker compose -f docker-compose.yml up -d
	bash wait-for-server.sh localhost 30456 60
	@echo "=== Giving server time to fully initialize ==="
	sleep 2
	@echo "=== Running integration tests ==="
	INTEGRATION_COMPOSE_FILE="$(shell pwd)/docker-compose.yml" ./$(TARGET) localhost 30456 ; exit_code=$$? ; \
	echo "=== Stopping LdnServer ===" ; \
	docker compose -f docker-compose.yml down -v ; \
	exit $$exit_code

clean:
	rm -f *.o *.gcda *.gcno $(TARGET) docker-compose.yml

# Coverage: build with --coverage, run tests against Docker server, generate report
COV_SOURCES := \
	../../sysmodule/source/network/client.cpp \
	../../sysmodule/source/network/tcp_client.cpp \
	../../sysmodule/source/network/connection_state.cpp \
	../../sysmodule/source/network/reconnect.cpp \
	../../sysmodule/source/network/socket.cpp

coverage: clean
	$(MAKE) COV=1 all
	@echo "=== Starting LdnServer via docker-compose (coverage build) ==="
	bash prepare-compose.sh
	docker compose -f docker-compose.yml up -d
	bash wait-for-server.sh localhost 30456 60
	sleep 2
	@echo "=== Running integration tests with coverage ==="
	INTEGRATION_COMPOSE_FILE="$$(pwd)/docker-compose.yml" ./$(TARGET) localhost 30456 ; exit_code=$$? ; \
	echo "=== Generating coverage report ===" ; \
	for src in $(COV_SOURCES); do \
		echo "--- $$src ---" ; \
		gcov --branch-counts --branch-probabilities -o . "$$src" 2>/dev/null \
			| grep -E "Lines executed|Branches executed" | head -2 ; \
	done ; \
	echo "=== Stopping LdnServer ===" ; \
	docker compose -f docker-compose.yml down -v ; \
	exit $$exit_code