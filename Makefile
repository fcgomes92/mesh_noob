default: all
all: upload monitor

.PHONY: monitor upload build uploadFiles

build:
	@pio run

monitor:
	@pio device monitor

upload:
	@pio run --target upload -e $(PIOENV)

_uploadFiles:
	@pio run --target uploadfs -e $(PIOENV)

uploadFiles: _uploadFiles monitor