# rider : le .nwa du jeu (prebuilt/), que STI2D-Play integre tel quel (voir les gros jeux de NumPlay).
BUILD_DIR = output
.PHONY: build module clean
build: $(BUILD_DIR)/rider.nwa
$(BUILD_DIR)/rider.nwa: prebuilt/rider.nwa
	@mkdir -p $(BUILD_DIR)
	cp $< $@
module: build
clean:
	rm -rf $(BUILD_DIR)
