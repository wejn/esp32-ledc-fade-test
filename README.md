# LEDC quick test

## What's this?

A quick test suite that I originally wrote to test behavior of `LEDC` non-blocking fading (on ESP32-C6).

For background info see [FIXME: link blogpost].

## Compilation

To compile:

``` sh
./in-docker.sh idf.py set-target esp32-c6 build
```

To flash:

``` sh
# pipx install esptool
cd build
esptool.py --chip esp32c6 -b 460800 \
  --before default_reset --after hard_reset \
  write_flash "@flash_args"
```

To clean up:

``` sh
./in-docker.sh rm -rf build dependencies.lock managed_components/ sdkconfig
```

## Results

That won't make much sense unless you take a look at the `main/main.c`. ;)

``` text
I (297) LEDC_TEST: ==== LEDC Concurrent Fades Test ====
I (307) LEDC_TEST: Boot button config...
I (307) gpio: GPIO[9]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (317) LEDC_TEST: Trigger button (& GND+SIGNAL pins) config...
I (327) gpio: GPIO[3]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 1| Intr:0 
I (337) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 1| Intr:0 
I (347) gpio: GPIO[7]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 1| Intr:0 
W (357) LEDC_TEST: Punch the BOOT button to start test1 (sequential interrupted fades)...
I (6667) LEDC_TEST: Test 1: Sequential fades on one channel
I (6667) LEDC_TEST: ... took: 27μs
I (6667) LEDC_TEST: Initial duty: 0
I (6717) LEDC_TEST: Starting long fade to max duty
I (6717) LEDC_TEST: ... took: 169μs
I (6767) LEDC_TEST: Duty before interrupt: 1235
I (6767) LEDC_TEST: Interrupting with fade to 0
I (7037) LEDC_TEST: ... took: 278274μs
I (7337) LEDC_TEST: Final duty: 0 (expected to be close to 0)
W (7337) LEDC_TEST: Punch the BOOT button to start test2 (multiple rapid fade changes)...
I (15637) LEDC_TEST: Test 2: Rapid successive fades on one channel
I (15637) LEDC_TEST: ... took: 22μs
I (15687) LEDC_TEST: Starting fade to 25% duty
I (15687) LEDC_TEST: ... took: 31μs
I (15737) LEDC_TEST: Quickly overriding with fade to 50% duty
I (16097) LEDC_TEST: ... took: 359966μs
I (16147) LEDC_TEST: Quickly overriding with fade to 75% duty
I (16297) LEDC_TEST: ... took: 155153μs
I (16347) LEDC_TEST: Finally overriding with fade to 100% duty
I (16397) LEDC_TEST: ... took: 57768μs
I (16697) LEDC_TEST: Final duty: 8191 (expected to be close to max: 8191)
W (16697) LEDC_TEST: Punch the BOOT button to start test3 (concurrent fades on two channels)...
I (18097) LEDC_TEST: Test 3: Concurrent fades on different channels
I (18097) LEDC_TEST: ... took: 38μs
I (18147) LEDC_TEST: Initial duties - Channel 1: 0 (expected near 0), Channel 2: 8191 (expected near max)
I (18147) LEDC_TEST: Starting concurrent fades in opposite directions
I (18147) LEDC_TEST: ... took: 79μs
I (18307) LEDC_TEST: Mid-fade duties - Channel 1: 3825 (increasing), Channel 2: 4366 (decreasing)
I (18507) LEDC_TEST: Final duties - Channel 1: 8191 (expected near max), Channel 2: 0 (expected near 0)
W (18507) LEDC_TEST: Punch the BOOT button to start test1 (sequential interrupted fades)...
```
