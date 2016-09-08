UTF-8 Validator
===============

The UTF-8 validator reads chunks of bytes of arbitary length and outputs
chunks containing only complete UTF-8 sequences. Sequences overlapping the
chunk boundaries are joined. Invalid bytes and sequences are replaced with
the replacement glyph � (0xFFFD).

The validator uses the checks suggested by Markus G. Kuhn
<http://www.cl.cam.ac.uk/~mgk25/> using the test file
<http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt>.

The following is considered to be invalid:

- Invalid initial bytes and detached continuation bytes
- Incomplete sequences
- Overlong glyph representations
- Low and high surrogates
- Glyphs in the "internal use area"

## Example

```c
size_t dataSize;
uint8_t* data = readFile("UTF-8-test.txt", &dataSize);

uint8_t buffer[4096];
utf8_validator validator = {0};

uint8_t* dataPtr = data;
size_t inSize = dataSize;
size_t outSize;

while (inSize) {
    outSize = sizeof(buffer);
    utf8_validate(&validator, (uint8_t const**) &dataPtr, &inSize, buffer, &outSize);

    if (outSize) {
        handleChunk(buffer, outSize);
    }
}
```

The stream end is signalled by giving an empty chunk. This is to check if
the last sequence is truncated.

```c
outSize = sizeof(buffer);
utf8_validate(&validator, NULL, NULL, buffer, &outSize);

if (outSize) {
    handleChunk(buffer, outSize);
}
```

The output buffer size should be around 4096 bytes. The absolute minimum size
is 72 bytes, which is really ineffective
