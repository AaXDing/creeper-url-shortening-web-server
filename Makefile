docker:
	docker build -t creeper:base -f docker/base.Dockerfile .
	docker build -f docker/coverage.Dockerfile -t creeper-coverage .

build:
	mkdir -p build
	cd build && cmake .. && make

coverage:
	mkdir -p build_coverage
	cd build_coverage && cmake -DCMAKE_BUILD_TYPE=Coverage .. 
	make coverage

test:
	mkdir -p build
	cd build && cmake .. && make
	ctest --output-on-failure

clean:
	rm -rf build build_coverage

.PHONY: docker build coverage clean

format:
	find include \( -name '*.h' -o -name '*.hh' -o -name '*.hpp' \) -print0 | xargs -0 -n1 clang-format -i
	find src \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) -print0 | xargs -0 -n1 clang-format -i
	find tests \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) -print0 | xargs -0 -n1 clang-format -i
