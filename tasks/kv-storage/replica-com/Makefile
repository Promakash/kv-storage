.PHONY: build_app clear_processes test_communication test_replication test_fall_replica test_all

build_app:
	mkdir -p build
	cd build && \
	cmake .. && \
	cmake --build .

clear_processes:
	killall replica_com
	killall replica_com

test_communication: build_app
	pytest tests.py::TestServer::test_communication
	$(MAKE) clear_processes

test_replication: build_app
	pytest tests.py::TestServer::test_replication
	$(MAKE) clear_processes

test_fall_replica: build_app
	pytest tests.py::TestServer::test_fall_replica
	$(MAKE) clear_processes

test_all: build_app
	pytest tests.py::TestServer
	$(MAKE) clear_processes