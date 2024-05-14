build_and_launch_server:
	mkdir -p build
	cd build && \
	cmake .. && \
	cmake --build .

test: build_and_launch_server
	build/client_com & pytest test.py