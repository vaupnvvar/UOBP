# Dependencies for speech.$O:
speech.$O: $(SRC_DIR)/speech.c
speech.$O: $(BLD_TOP)config.h
speech.$O: $(BLD_TOP)forbuild.h
speech.$O: $(SRC_TOP)prologue.h
speech.$O: $(SRC_TOP)Programs/driver.h
speech.$O: $(SRC_TOP)Programs/spk.h
speech.$O: $(SRC_TOP)Programs/spk_driver.h
speech.$O: $(SRC_TOP)Programs/spkdefs.h
speech.$O: $(SRC_DIR)/speech.h
speech.$O: $(SRC_TOP)Drivers/Braille/BrailleLite/braille.h
speech.$O: $(SRC_TOP)Programs/io_serial.h
speech.$O: $(SRC_TOP)Programs/serialdefs.h

