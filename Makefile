docker:
	docker build -t creeper:base -f docker/base.Dockerfile .
	docker build -f docker/coverage.Dockerfile -t creeper-coverage .

build:
	mkdir -p build
	cd build && cmake .. && make

coverage:
	mkdir -p build_coverage
	cd build_coverage && cmake -DCMAKE_BUILD_TYPE=Coverage .. && make coverage

clean:
	rm -rf build build_coverage

.PHONY: docker build coverage clean

