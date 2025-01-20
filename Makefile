default: compile

compile: compile-buoy

upload: upload-buoy

compile-buoy:
	arduino-cli compile buoy

upload-buoy: compile-buoy
	arduino-cli upload buoy
