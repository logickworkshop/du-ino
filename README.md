# DU-INO Arduino Library

## Overview

This is the official Arduino library for the DU MDLR [DU-INO](http://logick.ca/du-mdlr/du-ino/) Eurorack module.

## Getting Started

1. Download the [Arduino IDE](https://www.arduino.cc/en/Main/Software) for your system.
2. From the Arduino IDE Library Manager (**Tools** - **Manage Libraries**), install the [TimerOne](https://www.arduinolibraries.info/libraries/timer-one) library.
3. Download the [latest release](https://github.com/logickworkshop/du-ino/releases) of the DU-INO library, and install it via **Sketch** - **Include Library** - **Add .ZIP Library**.
4. Start hacking!

A growing collection of Detroit Underground functions are being added as examples, which can be accessed from **File** - **Examples** - **DU-INO** in the IDE once the library is installed.

Program the DU-INO by connecting a USB cable to the Arduino Uno while it is seated in the module. Note that the module has a small switch on the lower left (from the front) of the PCB that must be switched to the *up* position for programming and the *down* position to run. Also be sure to set the 8 switches on the rear of the module to the positions specified by the switch configuration in the function. See our [YouTube video](https://www.youtube.com/watch?v=DT4nvzAziTM) demonstrating this process.

## Modules

### Function Module

`#include <du-ino_function.h>`

The core function module provides a base for any kind of function. The idea is to subclass `DUINO_Function`, and (usually) do the following things:

* Call the `DUINO_Function` base constructor passing an explicit switch configuration parameter (in binary `0b` notation).
* Override the `function_setup()` and `function_loop()` methods.
* Create a global pointer to an instance of your subclass, and instantiate it with `new` in `setup()`.
* Call the `begin()` and `function_loop()` methods of the global instance in `setup()` and `loop()`, respectively.

A function subclass generally contains data members relating to the parameters and state of the function itself, and the function initialization and run logic in the `function_setup()` and `function_loop()` methods, respectively. This logic can use methods from the base class to read CV and gate voltages from input jacks, write CV and gate/trigger voltages to output jacks, and attach interrupts to GT3 and GT4.

The function subclass is also responsible for creating and driving the UI. When widgets are used (see **Widget Module** below for details), the widget hierarchy is constructed followed by a call to `widget_setup()` in the `function_setup()` method, and `widget_loop()` is called somewhere in the `function_loop()` method to process the encoder interactions.

### Display Driver Module

`#include <du-ino_sh1106.h>`

The display driver module provides a global `Display` object that allows interaction with the DU-INO's SH1106 OLED display. It provides a variety of functions for drawing shapes, text, and bitmaps to the display buffer, and a `display()` method for flushing part or all of the buffer to the actual display.

Since flushing the display buffer uses SPI transfer in an atomic (uninterruptible) block of code, it is advisable to only call `display()` when and where needed; keep track of changes to the display buffer and call it conditionally, and provide the column and page limit parameters.

### Widget Module

`#include <du-ino_widgets.h>`

The widget module is a convenient way to build a hierarchical, event-driven graphical UI from individual elements. It provides a family of classes derived from the base `DUINO_Widget` class, including displayable widgets that can look after their own area of the display, and container widgets that can logically "own" and order other widgets.

There are two basic concepts to a widget. First, a widget can be *inverted* (selected) or not; usually, if a widget is inverted, the widget that contains it is also inverted, and inverting a displayable widget usually has some effect on its appearance (e.g. a box appears around it). Second, a widget has definable responses to each of the three actions of the click encoder (click, double click, and scroll); user callbacks can always be attached to these actions, but some widget subclasses define intrinsic responses as well.

The `DUINO_WidgetContainer` class allows a set (by template parameter) number of widgets to be attached to it, in a specific order. On instantiation, one of the three encoder actions is selected for its "select" response, and when inverted, the widget container will cycle through inverting its contained widgets in response to that encoder action. For example,

```
DUINO_WidgetContainer<4> * c = new DUINO_WidgetContainer<4>(DUINO_Widget::Scroll);
```

produces a container `c` to which 4 sub-widgets can be attached, and when container `c` is inverted, scrolling the encoder wheel will invert the next or previous sub-widget accordingly (and, of course, un-invert the one that was previously inverted).

The idea is to create a "root" widget, specified to `widget_setup()` in the function and thereby initially and always inverted, and then attach a hierarchy of other containers or widgets to it. Because there are only three encoder actions, and usually at least one has a callback attached to it in a "leaf" widget to actually do something, you will generally want to limit the depth of this hierarchy to a depth of three; that is, either a single widget, a container of widgets, or a container of containers of widgets.

The `DUINO_DisplayWidget` class is the other basic widget. It has a defined position and size on the display, and is capable of redrawing its portion of the display. You can choose one of several different invert styles that defines how the widget's appearance will change when it is inverted. Other than the invert, the widget does not actually draw anything to the display; the idea is to draw directly to the `Display` buffer relative to the `x()`, `y()`, `width()`, and `height()` returns from the display widget object, and then call the object's `display()` method to flush the buffer to the display.

The `DUINO_MultiDisplayWidget` class is more convenient and memory-efficient than a container and individual display widgets for cases involving a set of similar widgets evenly spaced horizontally or vertically on the display with the same callback logic. Both it and the `DUINO_WidgetContainer` are widget arrays, using the same sub-widget selection logic, and allowing the specification of callback arrays (so that repetitive callbacks don't need to be created for each individual sub-widget).

### Indicator Module

`#include <du-ino_indicators.h>`

The indicator module provides a set of standard graphical indicators for use in a function UI. Like display widgets, indicators are display objects with a position and size, but they are not in the widget hierarchy and are not selectable.

### Save Widget (EEPROM) Module

`#include <du-ino_save.h>`

The `DUINO_SaveWidget` is a special widget that can save parameter values to the Arduino's internal EEPROM memory and load them back again, so that a function can come back to life as it was configured when the module is powered off and back on. The class is templated on a fixed-size `struct` of parameters that you define, and internally handles the byte serialization and EEPROM access.

### Clock Module

`#include <du-ino_clock.h>`

The clock module provides a non-blocking timer loop whose behaviour can be configured in musical ways, including swing and clock divisions.

### DSP Module

`#include <du-ino_dsp.h>`

The DSP module provides digital signal processing functionality. Currently, this consists of a discrete-time filter that can be configured as either low-pass or high-pass.

### Musical Scale Module

`#include <du-ino_scales.h>`

The musical scale module defines a set of common musical scales by semitone, and provides some convenience functions for using them. The scales and their three-letter identifiers are based on those in the [Korg Kaossilator](https://en.wikipedia.org/wiki/Korg_Kaossilator).

## Software CV Calibration

In case the hardware calibration of the CV inputs and/or outputs is insufficient for your needs, it is possible to fine-tune it for each jack in software with scale and offset parameters. Simply copy `du-ino_calibration.h.sample` within the `src` subdirectory where you installed the DU-INO library (normally, under the `libraries` directory of your Arduino IDE) to `du-ino_calibration.h`, and adjust the parameters in the latter file. You can determine the appropriate values by loading the `test` example program, and then sending precise voltage signals to each input and/or precisely measuring the voltages from each output.

Note that the precise values of the CV input parameters will depend on both the individual DU-INO and the individual Arduino, and the precise values of the CV output parameters will depend on the individual DU-INO.

## Supported Arduino Boards

The following boards are currently supported:

- Arduino Uno R3

Compatible boards and other Arduino boards may work; however, some hardware-specific registers and settings may need to be modified for different microcontroller models, and some functions may exceed program memory, RAM, or EEPROM limitations on other devices.

We are hoping to expand this list. Pull requests with compile-time checks adding support for different devices are very welcome.
