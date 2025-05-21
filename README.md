# 🪁 Zephyr Terminal Display

A Zephyr Driver for a 256-color terminal-based display. This code is mostly
intended as an example for how Kconfig and Devicetree are used to create
hardware-independent application code in Zephyr.


![example](https://github.com/xv-engineering/zephyr-terminal-display/blob/main/doc/example.gif)

It is used as part of a presentation for the Zephyr Developer Meetup in Boston,
summer 2025.

## Usage

This repository can be used as a starting point, or as a module in your own project.

### As a starting point

use `west` to clone this into a workspace, along with the zephyr tree. See
more information in the [Zephyr documentation](https://docs.zephyrproject.org/latest/develop/west/built-in.html#west-init)


### As a module

In your `west.yml`, add the following:

```yaml
manifest:

  remotes:
    - name: xv-engineering
      url-base: https://github.com/xv-engineering
  
  projects:
    - name: terminal-display
      remote: xv-engineering
      repo-path: zephyr-terminal-display
      revision: v0.0.3 # or whatever tag is latest if I forgot to update this readme
      import: false # assuming you have your own zephyr module already
```


## Samples

There are two samples/tests in the `tests` directory:

- `direct-draw`: a simple test that will draw a circle to the terminal
- `lvgl`: a sample using the LVGL library to draw text to the terminal

These samples include overlays for the native_sim_64 platform, as well
as the nrf52840dk_nrf52840 platform. It should be trivial to add support
for other platforms via devicetree overlays.

Run the samples as you would any other Zephyr application.

On the native_sim_64 platform, the display will be written to `uart_1`. The ptty
that this will ultimately correspond to will be printed at application startup. To
view the output, you may find something like `screen` helpful...

```bash
screen /dev/pts/2
```

On the nrf52840dk platform, the display will be written out over the USB tty
UART port. The built in terminal viewer in the nRF Connect vscode extension
is helpful for viewing the output.

## Contributing

Open a pull request or create an issue on GitHub.





