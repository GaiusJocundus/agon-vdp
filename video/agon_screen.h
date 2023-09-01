#ifndef AGON_SCREEN_H
#define AGON_SCREEN_H

#include <memory>
#include <fabgl.h>

std::shared_ptr<fabgl::Canvas>	canvas;			// The canvas class
std::shared_ptr<fabgl::VGABaseController>	_VGAController;		// Pointer to the current VGA controller class (one of the above)

uint8_t			_VGAColourDepth = -1;			// Number of colours per pixel (2, 4, 8, 16 or 64)
bool			doubleBuffered = false;			// Disable double buffering by default

// Get controller
// Parameters:
// - colours: Number of colours per pixel (2, 4, 8, 16 or 64)
// Returns:
// - A singleton instance of a VGAController class
//
std::shared_ptr<fabgl::VGABaseController> getVGAController(uint8_t colours = _VGAColourDepth) {
	if (_VGAController && (colours == _VGAColourDepth)) {	// Same number of colours as current controller?
		return _VGAController;
	} else {
		switch (colours) {
			case  2: return std::make_shared<fabgl::VGA2Controller>();
			case  4: return std::make_shared<fabgl::VGA4Controller>();
			case  8: return std::make_shared<fabgl::VGA8Controller>();
			case 16: return std::make_shared<fabgl::VGA16Controller>();
			case 64: return std::make_shared<fabgl::VGAController>();
		}
	}
	return nullptr;
}

// Update the internal FabGL LUT
//
void updateRGB2PaletteLUT() {
	// Use instance, as call not present on VGABaseController
	switch (_VGAColourDepth) {
		case 2: fabgl::VGA2Controller::instance()->updateRGB2PaletteLUT(); break;
		case 4: fabgl::VGA4Controller::instance()->updateRGB2PaletteLUT(); break;
		case 8: fabgl::VGA8Controller::instance()->updateRGB2PaletteLUT(); break;
		case 16: fabgl::VGA16Controller::instance()->updateRGB2PaletteLUT(); break;
	}
}

// Get current colour depth
//
inline uint8_t getVGAColourDepth() {
	return _VGAColourDepth;
}

// Set a palette item
// Parameters:
// - l: The logical colour to change
// - c: The new colour
// 
void setPaletteItem(uint8_t l, RGB888 c) {
	auto depth = getVGAColourDepth();
	if (l < depth) {
		// Use instance, as call not present on VGABaseController
		switch (depth) {
			case 2: fabgl::VGA2Controller::instance()->setPaletteItem(l, c); break;
			case 4: fabgl::VGA4Controller::instance()->setPaletteItem(l, c); break;
			case 8: fabgl::VGA8Controller::instance()->setPaletteItem(l, c); break;
			case 16: fabgl::VGA16Controller::instance()->setPaletteItem(l, c); break;
		}
	}
}

// Change video resolution
// Parameters:
// - colours: Number of colours per pixel (2, 4, 8, 16 or 64)
// - modeLine: A modeline string (see the FabGL documentation for more details)
// Returns:
// - 0: Successful
// - 1: Invalid # of colours
// - 2: Not enough memory for mode
//
int8_t change_resolution(uint8_t colours, char * modeLine) {
	auto controller = getVGAController(colours);

	if (!controller) {								// If controller is null, then an invalid # of colours was passed
		return 1;									// So return the error
	}
	canvas.reset();									// Delete the canvas

	_VGAColourDepth = colours;						// Set the number of colours per pixel
	if (_VGAController != controller) {				// Is it a different controller?
		if (_VGAController) {						// If there is an existing controller running then
			_VGAController->end();					// end it
		}
		_VGAController.reset();						// Delete the old controller
		_VGAController = controller;				// Switch to the new controller
		controller->begin();						// And spin it up
	}
	if (modeLine) {									// If modeLine is not a null pointer then
		if (doubleBuffered == true) {
			controller->setResolution(modeLine, -1, -1, 1);
		} else {
			controller->setResolution(modeLine);	// Set the resolution
		}
	} else {
		debug_log("change_resolution: modeLine is null\n\r");
	}
	controller->enableBackgroundPrimitiveExecution(true);
	controller->enableBackgroundPrimitiveTimeout(false);

	canvas = std::make_shared<fabgl::Canvas>(controller.get());		// Create the new canvas
	debug_log("after change of canvas...\n\r");
	debug_log("  free internal: %d\n\r  free 8bit: %d\n\r  free 32bit: %d\n\r",
		heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
		heap_caps_get_free_size(MALLOC_CAP_8BIT),
		heap_caps_get_free_size(MALLOC_CAP_32BIT)
	);
	//
	// Check whether the selected mode has enough memory for the vertical resolution
	//
	if (controller->getScreenHeight() != controller->getViewPortHeight()) {
		return 2;
	}
	return 0;										// Return with no errors
}

// Swap to other buffer if we're in a double-buffered mode
//
void switchBuffer() {
	if (doubleBuffered == true) {
		canvas->swapBuffers();
	}
}

// Wait for plot completion
//
inline void waitPlotCompletion(bool waitForVSync = false) {
	canvas->waitCompletion(waitForVSync);
}

#endif // AGON_SCREEN_H
