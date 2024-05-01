build_and_launch_server:
	mkdir -p build
	cd build && \
	cmake .. && \
	cmake --build .
	build/client_com

test: build_and_launch_server
	pytest
