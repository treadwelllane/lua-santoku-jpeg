# Santoku JPEG

JPEG image scaling module for Santoku with streaming support and memory-efficient processing.

## Module Reference

### `santoku.jpeg`

JPEG image scaling with streaming API for processing large images efficiently.

| Function | Arguments | Returns | Description |
|----------|-----------|---------|-------------|
| `scale` | `scale_num, scale_denom, [quality], [bufsize]` | `scaler` | Creates JPEG scaler with fractional scaling ratio |

#### Scaler Methods

| Method | Arguments | Returns | Description |
|--------|-----------|---------|-------------|
| `read` | `[data]` | `status, needed/output` | Reads input data or returns scaled output |
| `write` | `data` | `status, needed/output` | Writes input data for scaling |

#### Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `READ` | `1` | Status indicating more input data needed |
| `WRITE` | `0` | Status indicating output data available |
| `DONE` | `2` | Status indicating scaling complete |

## License

MIT License

Copyright 2025 Matthew Brooks

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.