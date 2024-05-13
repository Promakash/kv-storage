.PHONY: build_app clear_processes test_communication test_replication test_fall_replica test_all

build_app:
	mkdir -p build
	cd build && \
	cmake .. && \
	cmake --build .

clear_processes:
	-killall replica_com || true
	-killall replica_com || true

test_communication: build_app
	pytest test_communication.py
	$(MAKE) clear_processes

test_replication: build_app
	pytest test_replication.py
	$(MAKE) clear_processes

test_fall_replica: build_app
	pytest test_fall_replica.py
	$(MAKE) clear_processes

test_all: test_communication test_replication test_fall_replica