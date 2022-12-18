# boilerd
boilerd is a boiler controller for the tuxpresso project.
It manipulates a GPIO, and reads temperature from an IIO sensor.

## Build Dependencies
[pidc](https://github.com/ahepp/pidc)

## Quickstart
    # Build 
    cd boilerd && make

    # Initialize the IIO temperature sensor
    echo mcp9600 0x60 > /sys/class/i2c-adapter/i2c-1/new_device

    # Initialize the boiler GPIO
    echo $GPIO > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio$GPIO/direction

    boilerd -g $GPIO -i $IIO -sp 1632 -kp 64 -ki 1 -kd 0 -z tcp://lo:5555
