default: all
all: upload monitor

.PHONY: monitor upload build uploadFiles

build:
	@pio run

monitor:
	@pio device monitor

upload:
	@pio run --target upload

_uploadFiles:
	@pio run --target uploadfs

uploadFiles: _uploadFiles monitor