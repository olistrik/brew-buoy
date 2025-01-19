default: compile

compile: compile-buoy

compile-buoy:
		arduino-cli compile --fqbn SiliconLabs:silabs:xiao_mg24 ./buoy

