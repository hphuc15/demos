# BH1750 Demo Collection

This repository contains multiple BH1750-related projects, each maintained as a separate folder on the `main` branch.

## Structure

```
BH1750_Demo/
├── BH1750_2SL_Entry/
├── BH1750_Arduino/
├── BH1750_ESP_IDF/
└── BH1750_STM32F401RCT6/
```

## Projects

### `BH1750_2SL_Entry`

The first project I built when starting my engineering studies. It was developed as the entry test project for Sensor Lab (HUST).

### `BH1750_Arduino`

A simple BH1750 implementation using the Arduino framework. Primarily created for experimentation and learning.

### `BH1750_ESP_IDF`

A BH1750 implementation based on ESP-IDF. This is currently the most well-structured version and serves as both an application example and a reference integration for:

* [`hphuc15/baredrv`](https://github.com/hphuc15/baredrv)
* [`hphuc15/WiFiPanel`](https://github.com/hphuc15/WiFiPanel)

### `BH1750_STM32F401RCT6`
A BH1750 implementation for the STM32F401RCT6, I created to learn STM32 programming. Built with CMake and STM32CubeCLT, following the `baredrv` driver pattern used across other sensor projects.

## License

MIT