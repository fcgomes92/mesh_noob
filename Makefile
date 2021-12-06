.PHONY: monitor upload build uploadFiles

build:
	@pio run

monitor:
	@pio device monitor

upload:
	@pio run --target upload

uploadFiles:
	@pio run --target uploadfs